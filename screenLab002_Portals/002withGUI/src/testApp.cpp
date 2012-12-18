#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    ofSetFrameRate(60); //smooth
    ofSetVerticalSync(true);

    guiScreen = ofRectangle(0.f,0.f,1024.f, 768.f); //laptop screen
    presentationScreen = ofRectangle(guiScreen.getWidth(),0.f, 1920.f, 1080.f); // 1080p output projector screen
    
    ofBackground(0,0,0); //any colour as long as its black
    
    //openNI
    
    openNIDevice.setup();
    openNIDevice.addImageGenerator();
    openNIDevice.addDepthGenerator();
    openNIDevice.addInfraGenerator();
    openNIDevice.setRegister(true);
    openNIDevice.setMirror(true);
    openNIDevice.addUserGenerator();
    openNIDevice.setMaxNumUsers(1); //there can be only one!
    openNIDevice.start();
    
    // set properties for all user masks and point clouds
    //openNIDevice.setUseMaskPixelsAllUsers(true); // if you just want pixels, use this set to true
    openNIDevice.setUseMaskTextureAllUsers(true); // this turns on mask pixels internally AND creates mask textures efficiently
    openNIDevice.setUsePointCloudsAllUsers(true);
    openNIDevice.setPointCloudDrawSizeAllUsers(2); // size of each 'point' in the point cloud
    openNIDevice.setPointCloudResolutionAllUsers(2); // resolution of the mesh created for the point cloud eg., this will use every second depth pixel
    
    verdana.loadFont(ofToDataPath("verdana.ttf"), 24);
    
    //last setup time....
    
    timeOfPreviousFrame = ofGetElapsedTimef();
    
    eyes.vertical = false;
    breasts.vertical = false;
    crotch.vertical = false;
    
    eyes.velocity = ofVec3f();
    breasts.velocity = ofVec3f();
    crotch.velocity = ofVec3f();
    
    eyes.size = ofVec3f(0.f, 40.f ,0.f);
    breasts.size = ofVec3f(0.f, 40.f ,0.f);
    crotch.size = ofVec3f(0.f, 40.f ,0.f);
    
    setupControlPanel();
    
//    renderManager.allocateForNScreens(1, presentationScreen.getWidth(), presentationScreen.getHeight());
//    renderManager.loadFromXml("RenderManager.xml");
    
    presentationScreenFBO.allocate(presentationScreen.getWidth(), presentationScreen.getHeight());
    
    plane.setSource(presentationScreenFBO);
	plane.setCalibrateMode(true);
	plane.load("warp.bin");
}

// Set up the controls in the ofxControlPanel
void testApp::setupControlPanel(){
	gui.setup("Portals", 10, 10, 350, 640);
    gui.addPanel("Portal Control", 1);
    gui.setWhichPanel("Portal Control");
	gui.addLabel("'f' to toggle fullscreen \n'h' to toggle this panel \n");
	gui.addSlider("Eye Offset (top down)", "EYE_RATIO",	0.2f, -2.f, 2.f,	false);
	gui.addSlider("Breast Offset (top down)", "BREAST_RATIO", 0.5f, -2.f, 2.f, false);
    gui.addSlider("Crotch Offset (top down)", "CROTCH_RATIO", 0.f, -1.f, 1.f, false);
	gui.loadSettings("controlPanelSettings.xml");
}

//--------------------------------------------------------------
void testApp::update(){
	// Calculate time past per frame
	float timeNow = ofGetElapsedTimef();
    float timeSinceLastFrame = timeNow - timeOfPreviousFrame;
    
    gui.update();
    
    ofVec3f dimensionsOfKinectScreen = ofVec3f(openNIDevice.getWidth(), openNIDevice.getHeight(), 1.f); //don't care about Z
    
    //openni
    openNIDevice.update();
        
    int numUsers = openNIDevice.getNumTrackedUsers(); //should be only one
    
    if(numUsers > 0){
        ofVec3f eyePosition = ofVec3f();
        ofVec3f breastPosition = ofVec3f();
        ofVec3f crotchPosition = ofVec3f();
        
        // get a reference to this user
        ofxOpenNIUser & user = openNIDevice.getTrackedUser(0); //safe as we know there is at least one
        
        ofxOpenNIJoint & head = user.getJoint(JOINT_HEAD);
        ofxOpenNIJoint & neck = user.getJoint(JOINT_NECK);
        ofxOpenNIJoint & torso = user.getJoint(JOINT_TORSO);
        
        ofxOpenNILimb & pelvis = user.getLimb(LIMB_PELVIS);
        
        ofVec3f headProjectivePosition = head.getProjectivePosition();
        ofVec3f neckProjectivePosition = neck.getProjectivePosition();
        ofVec3f torsoProjectivePosition = torso.getProjectivePosition();
        
        float eyeRatio = gui.getValueF("EYE_RATIO");
        float breastRatio = gui.getValueF("BREAST_RATIO");
        float crotchRatio = gui.getValueF("CROTCH_RATIO");
        
        ofVec3f interpolatedEyePosition = headProjectivePosition.getInterpolated(neckProjectivePosition, eyeRatio); //does this go 20% of the way along the line from head to neck?
        ofVec3f interpolatedBreastPosition = neckProjectivePosition.getInterpolated(torsoProjectivePosition, breastRatio); //does this go 50% of the way along line from neck to torso?
        ofVec3f interpolatedCrotchPosition = (pelvis.getEndJoint().getProjectivePosition()+pelvis.getStartJoint().getProjectivePosition())/2.f;
        
        //scale back to 0..1 from 640,480 co-ordinates
        ofVec3f unitEyePosition = interpolatedEyePosition/dimensionsOfKinectScreen;
        ofVec3f unitBreastPosition = interpolatedBreastPosition/dimensionsOfKinectScreen;
        ofVec3f unitCrotchPosition = interpolatedCrotchPosition/dimensionsOfKinectScreen;
        
        unitCrotchPosition.y += crotchRatio;
        
        eyes.position = unitEyePosition;
        breasts.position = unitBreastPosition;
        crotch.position = unitCrotchPosition;
        
        // should not only do positions, but sizes, from scale proportions based on:
        // http://en.wikipedia.org/wiki/Body_proportions
        // http://en.wikipedia.org/wiki/File:Neoteny_body_proportion_heterochrony_human.png
        // http://en.wikipedia.org/wiki/Female_body_shape etc.
        // sex guess too?
    }
    
    timeOfPreviousFrame = timeNow;
}

//--------------------------------------------------------------
void testApp::draw(){
    ofEnableAlphaBlending();
    
    ofPushMatrix();
    if (plane.getCalibrateMode()){
        ofSetColor(ofColor::red);
    }else{
        ofSetColor(ofColor::black);
    }
    //draw a rectangle to make calibration easier
    ofRect(presentationScreen);
    ofSetColor(ofColor::white);
    // draw debug (ie., image, depth, skeleton)
    openNIDevice.drawDebug();
    ofPopMatrix();
    
    ofPushStyle();
    
    ofVec3f presentationScreenVector = ofVec3f(presentationScreen.getWidth(), presentationScreen.getHeight(), 1.f); //z doesn't matter
    
    ofPushMatrix();
    
    presentationScreenFBO.begin();
    ofSetColor(ofColor::black);
    ofRect(0,0,presentationScreen.getWidth(),presentationScreen.getHeight());
    
    // get number of current users
    int numUsers = openNIDevice.getNumTrackedUsers();
    
    // just do the first one
    if(numUsers > 0){        
        // get a reference to this user
        ofxOpenNIUser & user = openNIDevice.getTrackedUser(0);
        
        //now do all the portals...
        //should do this properly with scaling of size as you lean over and the like...
        
        //eye block
        ofSetColor(ofColor::white);
        float widthOfBeam = presentationScreen.getWidth();
        float heightOfBeam = eyes.size.y;
        float yPositionOfBeamScaledToPresentationScreen = eyes.position.y * presentationScreen.getHeight();
        float yPositionOfBeamScaledToPresentationScreenWithOffset = yPositionOfBeamScaledToPresentationScreen - (heightOfBeam/2.f);

        //ofRect(presentationScreen.getX(), yPositionOfBeamScaledToPresentationScreenWithOffset, widthOfBeam, heightOfBeam);
        ofRect(0, yPositionOfBeamScaledToPresentationScreenWithOffset, widthOfBeam, heightOfBeam);
        
        //breasts block
        heightOfBeam = breasts.size.y;
        yPositionOfBeamScaledToPresentationScreen = breasts.position.y * presentationScreen.getHeight();
        yPositionOfBeamScaledToPresentationScreenWithOffset = yPositionOfBeamScaledToPresentationScreen - (heightOfBeam/2.f);
        
        //ofRect(presentationScreen.getX(), yPositionOfBeamScaledToPresentationScreenWithOffset, widthOfBeam, heightOfBeam);
        ofRect(0, yPositionOfBeamScaledToPresentationScreenWithOffset, widthOfBeam, heightOfBeam);
        
        //crotch block
        heightOfBeam = crotch.size.y;
        yPositionOfBeamScaledToPresentationScreen = crotch.position.y * presentationScreen.getHeight();
        yPositionOfBeamScaledToPresentationScreenWithOffset = yPositionOfBeamScaledToPresentationScreen - (heightOfBeam/2.f);
        
        //ofRect(presentationScreen.getX(), yPositionOfBeamScaledToPresentationScreenWithOffset, widthOfBeam, heightOfBeam);
        ofRect(0, yPositionOfBeamScaledToPresentationScreenWithOffset, widthOfBeam, heightOfBeam);
        
        ofTexture& tex = user.getMaskTextureReference();
        
        //ofSetColor(0,0,0,255); //mask like this?
        ofSetColor(ofColor::black); //does setting it to black instead get rid of the outline?
        //tex.draw(guiScreen.getWidth()+presentationScreen.getWidth(), 0, -presentationScreen.getWidth(), presentationScreen.getHeight());
        tex.draw(0, 0, presentationScreen.getWidth(), presentationScreen.getHeight());
        //flip it so it feels right
    }
    
    presentationScreenFBO.end();
    
    ofPopMatrix();
    ofPopStyle();
    
    // draw some info regarding frame counts etc
	ofSetColor(0, 255, 0);
	string msg = " MILLIS: " + ofToString(ofGetElapsedTimeMillis()) + " FPS: " + ofToString(ofGetFrameRate()) + " Device FPS: " + ofToString(openNIDevice.getFrameRate());
    
	verdana.drawString(msg, 20, openNIDevice.getNumDevices() * 480 - 20);
    
    gui.draw();
    
    ofSetColor(ofColor::white);
    
    plane.draw();
	
	if (plane.getCalibrateMode()) {
		int y = 500;
		ofDrawBitmapString("Use the handles to warp and move the video", 20, y+=15);
		ofDrawBitmapString("[SPACE] = toggle calibration mode", 20, y+=10);
		ofDrawBitmapString("Use mouse to select points and warp the video", 20, y+=15);
		ofDrawBitmapString("[TAB] = select points", 20, y+=15);
		ofDrawBitmapString("[UP]/[DOWN]/[LEFT]/RIGHT = move the selected point", 20, y+=15);
		ofDrawBitmapString("[SHIFT] + arrow key = move the selected point faster", 20, y+=15);
		ofDrawBitmapString("[r] = reset calibration", 20, y+=15);
        ofDrawBitmapString("[s] = save calibration", 20, y+=15);
        ofDrawBitmapString("[w] = load calibration", 20, y+=15);
	}
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    switch (key) {
        case ' ':
            plane.toggleCalibrateMode();
            if (plane.getCalibrateMode()){
                plane.setSource(openNIDevice.getDepthTextureReference());
                ofBackground(50);
            }else{
                plane.setSource(presentationScreenFBO);
                ofBackground(0);
            }
            break;
        case 's':
            plane.save("warp.bin");
            break;
        case 'w':
            plane.load("warp.bin");
            break;
        case 'h':
            gui.toggleView();
        case 'f':
            ofToggleFullscreen();
        case 'x':
            break;
        case OF_KEY_UP:
            break;
        case OF_KEY_DOWN:
            break;
        case OF_KEY_RIGHT:
            break;
        case OF_KEY_LEFT:
            break;
		default:
			break;
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	gui.mouseDragged(x, y, button);
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	gui.mousePressed(x, y, button);
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
	gui.mouseReleased();
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------
void testApp::userEvent(ofxOpenNIUserEvent & event){
    // show user event messages in the console
//    ofLogNotice() << getUserStatusAsString(event.userStatus) << "for user" << event.id << "from device" << event.deviceID;
}

//--------------------------------------------------------------
void testApp::exit(){
    openNIDevice.stop();
}

bool testApp::joelInside(ofRectangle r, ofPoint p){
    return p.x >= r.getMinX() && p.y >= r.getMinY() && p.x <= r.getMaxX() && p.y <= r.getMaxY();
}



