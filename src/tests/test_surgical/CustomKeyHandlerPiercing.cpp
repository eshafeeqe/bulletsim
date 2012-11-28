#include "CustomKeyHandlerPiercing.h"


bool CustomKeyHandler::handle(const osgGA::GUIEventAdapter &ea,
		                      osgGA::GUIActionAdapter &) {
    switch (ea.getEventType()) {
    case osgGA::GUIEventAdapter::KEYDOWN:
        switch (ea.getKey()) {
        case 'a':
            scene.leftAction->reset();
            scene.leftAction->toggleAction();
            scene.runAction(scene.leftAction, BulletConfig::dt);
            break;
        case 's':
            scene.rightAction->reset();
            scene.rightAction->toggleAction();
            scene.runAction(scene.rightAction, BulletConfig::dt);
            break;
        case 'v':
        	if (!scene.isRaveViewer) {
        		scene.rave_viewer = OpenRAVE::RaveCreateViewer(scene.rave->env, "qtcoin");
        		scene.rave->env->AddViewer(scene.rave_viewer);
        		scene.isRaveViewer = true;
        	}
        	scene.rave_viewer->main(true);
    		break;
        case 'm':
        	scene.sNeedle->togglePiercing();
        	std::cout<<"Is needle piercing? "<<scene.sNeedle->s_piercing<<std::endl;
        	break;
        case 'M':
        	scene.holes[0]->togglePiercing();
        	break;
        case 'c':
        	scene.cutCloth();
        	break;
        case 'f':
        	scene.plotNeedle();
        	break;
        case 'F':
            scene.plotNeedle(true);
            break;
        case 'o':
        	scene.plotHoles();
        	break;
        case 'O':
        	scene.plotHoles(true);
        	break;
        case 't': // generates a kinematic from the cloth and adds to the openrave environment
        	createKinBodyFromBulletSoftObject(scene.sCloth->cloth, scene.rave);
        	createKinBodyFromBulletBoxObject(scene.table, scene.rave);
        	break;
        }
        break;
    }
    return false;
}
