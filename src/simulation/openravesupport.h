#ifndef _OPENRAVESUPPORT_H_
#define _OPENRAVESUPPORT_H_

#include <openrave/openrave.h>
#include <btBulletDynamicsCommon.h>
#include <vector>
#include "environment.h"
#include "basicobjects.h"
#include "util.h"
#include "simulation_fwd.h"
#include "config_bullet.h"

using namespace std;
using namespace OpenRAVE;

class RaveLinkObject;
class RaveObject;

struct RaveInstance {
  typedef boost::shared_ptr<RaveInstance> Ptr;

  bool isRoot;
  EnvironmentBasePtr env;
  std::map<KinBodyPtr, RaveObject*> rave2bulletsim;
  std::map<RaveObject*, KinBodyPtr> bulletsim2rave;

  std::map<KinBody::LinkPtr, btRigidBody*> rave2bulletsim_links;
  std::map<btRigidBody*, KinBody::LinkPtr> bulletsim2rave_links;

  RaveInstance();
  RaveInstance(OpenRAVE::EnvironmentBasePtr);
  RaveInstance(const RaveInstance &o, int cloneOpts);
  ~RaveInstance();
};

// Corresponds to an OpenRAVE link
class RaveLinkObject : public BulletObject {
public:
  typedef boost::shared_ptr<RaveLinkObject> Ptr;

  RaveInstance::Ptr rave;
  KinBody::LinkPtr link;

  RaveLinkObject(RaveInstance::Ptr rave_, KinBody::LinkPtr link_, btScalar mass, btCollisionShape *cs, const btTransform &initTrans, bool isKinematic_=false);
  RaveLinkObject(RaveInstance::Ptr rave_, KinBody::LinkPtr link_, btScalar mass, boost::shared_ptr<btCollisionShape> cs, const btTransform &initTrans, bool isKinematic_=false);
  void init();
  void destroy();
  // copying doesn't work!
};

void LoadFromRave(Environment::Ptr env, RaveInstance::Ptr rave);
void LoadFromRaveSingle(Environment::Ptr env, RaveInstance::Ptr rave, OpenRAVE::KinBodyPtr body, bool isKinematic, bool checkLoaded=true);
void LoadFromRaveExplicit(Environment::Ptr env, RaveInstance::Ptr rave, const vector<string> &dynamicNames);
void Load(Environment::Ptr env, RaveInstance::Ptr rave, const string& name);
void GetLoadedBodies(Environment::Ptr env, std::set<string> &bodiesAlreadyLoaded);

enum TrimeshMode {
  CONVEX_HULL, // use btShapeHull
  RAW, // use btBvhTriangleMeshShape (not recommended, makes simulation very slow)
};

typedef CompoundObject<RaveLinkObject> CompoundRaveLinkObject;
// Corresponds to an OpenRAVE KinBody
class RaveObject : public CompoundRaveLinkObject {
public:
  typedef boost::shared_ptr<RaveObject> Ptr;
  RaveInstance::Ptr rave;
  KinBodyPtr body;

  // constructor that takes pre-created bullet objects for the links and arbitrary constraints
  RaveObject(RaveInstance::Ptr rave_, KinBodyPtr body_, const vector<RaveLinkObject::Ptr> &bulletLinks, const vector<BulletConstraint::Ptr> &constraints_, bool isKinematic_=true);
  // constructor that creates bullet objects for the links
  RaveObject(RaveInstance::Ptr rave_, KinBodyPtr body, TrimeshMode trimeshMode = CONVEX_HULL, bool isKinematic=true);
  // This constructor assumes the robot is already in openrave. Use this if you're loading a bunch of stuff from an
  // xml file, and you want to put everything in bullet

  void init();
  void destroy();
  void prePhysics();

  // forking
  EnvironmentObject::Ptr copy(Fork &f) const;
  void postCopy(EnvironmentObject::Ptr copy, Fork &f) const;

  // Gets equivalent rigid bodies in OpenRAVE and in Bullet
  RaveLinkObject::Ptr associatedObj(KinBody::LinkPtr link) const {
    std::map<KinBody::LinkPtr, RaveLinkObject::Ptr>::const_iterator i = linkMap.find(link);
    return i == linkMap.end() ? RaveLinkObject::Ptr() : i->second;
  }
  KinBody::LinkPtr associatedObj(btCollisionObject *obj) const {
    std::map<btCollisionObject *, KinBody::LinkPtr>::const_iterator i = collisionObjMap.find(obj);
    return i == collisionObjMap.end() ? KinBody::LinkPtr() : i->second;
  }

  btTransform toRaveFrame(const btTransform &t) const { return util::scaleTransform(t, 1./GeneralConfig::scale); }
  btTransform toWorldFrame(const btTransform &t) const { return util::scaleTransform(t, GeneralConfig::scale); }
  // When getting transforms of links, you must remember to scale!
  // or just get the transforms directly from the equivalent Bullet rigid bodies
  btTransform getLinkTransform(KinBody::LinkPtr link) const {
    return toWorldFrame(util::toBtTransform(link->GetTransform()));
  }

  void ignoreCollisionWith(const btCollisionObject *obj) { ignoreCollisionObjs.insert(obj); }
  // Returns true if the robot's current pose collides with anything in the environment
  // (this will call updateAabbs() on the dynamicsWorld)
  bool detectCollisions();

  // Positions the robot according to DOF values in the OpenRAVE model
  // and copy link positions to the Bullet rigid bodies.
  void updateBullet();
  // update's openrave stuff based on bullet
  void updateRave();

  bool getIsKinematic() const { return isKinematic; }

protected:
  // these two containers just keep track of the smart pointers
  // so that the objects get deallocated on destruction
  std::vector<boost::shared_ptr<btStridingMeshInterface> > meshes;
  std::vector<boost::shared_ptr<btCollisionShape> > subshapes;

  // for looking up the associated Bullet object for an OpenRAVE link
  std::map<KinBody::LinkPtr, RaveLinkObject::Ptr> linkMap;
  std::vector<BulletConstraint::Ptr> constraints;
  std::map<btCollisionObject *, KinBody::LinkPtr> collisionObjMap;

  // maps a child to a position in the children array. used for copying
  std::map<RaveLinkObject::Ptr, int> childPosMap;

  // maps from child index to link index. only used in updateBullet
  std::vector<int> linkIndsWithGeometry;

  // vector of objects to ignore collision with
  BulletInstance::CollisionObjectSet ignoreCollisionObjs;

  // initializes the children vector with pre-created BulletObjects (bulletLinks.size() == body_->GetLinks().size()) and arbitrary constraints
  void initRaveObject(RaveInstance::Ptr rave_, KinBodyPtr body_, const vector<RaveLinkObject::Ptr> &bulletLinks, const vector<BulletConstraint::Ptr> &constraints_, bool isKinematic_);
  // for the loaded robot, this will create BulletObjects
  // and place them into the children vector
  void initRaveObject(RaveInstance::Ptr rave_, KinBodyPtr body_, TrimeshMode trimeshMode, bool isKinematic);
  RaveObject() {} // for manual copying
  void internalCopy(RaveObject::Ptr o, Fork &f) const;
  bool isKinematic;
};

class RaveRobotObject : public RaveObject {
public:
  typedef boost::shared_ptr<RaveRobotObject> Ptr;
	typedef RobotManipulator Manipulator;
	
  RobotBasePtr robot;

  map<RaveObject::Ptr, KinBody::LinkPtr> m_targ2grabber;
  map<KinBody::LinkPtr, RaveObject::Ptr> m_grabber2targ;


  RaveRobotObject(RaveInstance::Ptr rave_, RobotBasePtr robot, TrimeshMode trimeshMode = CONVEX_HULL, bool isStatic=true);
  RaveRobotObject(RaveInstance::Ptr rave_, const std::string &uri, TrimeshMode trimeshMode = CONVEX_HULL, bool isStatic=true);

  EnvironmentObject::Ptr copy(Fork &f) const;

  void setDOFValues(const vector<int> &indices, const vector<dReal> &vals);
  vector<double> getDOFValues(const vector<int> &indices);
  vector<double> getDOFValues();
  void setTransform(const btTransform& trans) {
    robot->SetTransform(util::toRaveTransform(util::scaleTransform(trans,1/METERS)));
    updateBullet();
  }
  void setTransform(float x, float y, float a) {
    setTransform(btTransform(btQuaternion(0,0,a), btVector3(x,y,0)));
  }
  btTransform getTransform() {
    return util::toBtTransform(robot->GetTransform(), METERS);
  }
	void grab(RaveObject::Ptr target, KinBody::LinkPtr link);
  void release(RaveObject::Ptr target);
	KinBody::LinkPtr getGrabberLink(RaveObject::Ptr target);

  RobotManipulatorPtr createManipulator(const std::string &manipName);
  void destroyManipulator(RobotManipulatorPtr m); // not necessary to call this on destruction
  RobotManipulatorPtr getManipByIndex(int i) const { return createdManips[i]; }
  RobotManipulatorPtr getManipByName(const std::string& name);
  int numCreatedManips() const { return createdManips.size(); }
protected:
  std::vector<RobotManipulatorPtr> createdManips;
  RaveRobotObject() {}
};

struct RobotManipulator {
  typedef boost::shared_ptr<RobotManipulator> Ptr;

  RaveRobotObject *robot;
  ModuleBasePtr ikmodule;
  RobotBase::ManipulatorPtr manip, origManip;
  int index; // id for this manipulator in this robot instance

  RobotManipulator(RaveRobotObject *robot_) : robot(robot_) { }

  btTransform getTransform() const {
    return robot->toWorldFrame(util::toBtTransform(manip->GetTransform()));
  }
  // Gets one IK solution closest to the current position in joint space
  bool solveIKUnscaled(const OpenRAVE::Transform &targetTrans,
     vector<dReal> &vsolution);
  bool solveIK(const btTransform &targetTrans, vector<dReal> &vsolution) {
    return solveIKUnscaled(
            util::toRaveTransform(robot->toRaveFrame(targetTrans)),
            vsolution);
  }

  // Gets all IK solutions
  bool solveAllIKUnscaled(const OpenRAVE::Transform &targetTrans,
        vector<vector<dReal> > &vsolutions);
  bool solveAllIK(const btTransform &targetTrans,
      vector<vector<dReal> > &vsolutions) {
    return solveAllIKUnscaled(
            util::toRaveTransform(robot->toRaveFrame(targetTrans)),
            vsolutions);
  }
  vector<double> getDOFValues();
  void setDOFValues(const vector<double>& vals);

  // Moves the manipulator with IK to targetTrans in unscaled coordinates
  // Returns false if IK cannot find a solution
  // If checkCollisions is true, then this will return false if the new
  // robot pose collides with anything in the environment (true otherwise).
  // The robot will revert is position to the pre-collision state if
  // revertOnCollision is set to true.
  bool moveByIKUnscaled(const OpenRAVE::Transform &targetTrans, bool checkCollisions = false, bool revertOnCollision=true);
  // Moves the manipulator in scaled coordinates
  bool moveByIK(const btTransform &targetTrans, bool checkCollisions = false, bool revertOnCollision=true) {
    return moveByIKUnscaled(util::toRaveTransform(robot->toRaveFrame(targetTrans)), checkCollisions, revertOnCollision);
  }

  float getGripperAngle();
  void setGripperAngle(float);

  RobotManipulatorPtr copy(RaveRobotObjectPtr newRobot, Fork &f);
};

std::vector<RaveRobotObject::Ptr> getRobots(Environment::Ptr env, RaveInstance::Ptr rave);
RaveObject::Ptr getObjectByName(Environment::Ptr env, RaveInstance::Ptr rave, const string& name);
RaveRobotObject::Ptr getRobotByName(Environment::Ptr env, RaveInstance::Ptr rave, const string& name);

class ScopedRobotSave {
  std::vector<double> m_dofvals;
  OpenRAVE::Transform m_tf;
  OpenRAVE::RobotBasePtr m_robot;
public:
  ScopedRobotSave(OpenRAVE::RobotBasePtr robot) : m_robot(robot) {
    robot->GetDOFValues(m_dofvals);
    m_tf = robot->GetTransform();
  }
  ~ScopedRobotSave() {
    m_robot->SetDOFValues(m_dofvals);
    m_robot->SetTransform(m_tf);
  }
};


#endif // _OPENRAVESUPPORT_H_
