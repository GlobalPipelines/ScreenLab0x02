#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    ofSetFrameRate(60); //smooth
    ofSetVerticalSync(true);

    guiScreen = ofRectangle(0.f,0.f,1024.f, 768.f); //laptop screen
    presentationScreen = ofRectangle(guiScreen.getWidth(),0.f, 1920.f, 1080.f); // 1080p output projector screen
    
    ofBackground(0,0,0);
    
    previousMousePosition = ofVec2f(mouseX, mouseY);
    
    //openNI
    
    openNIDevice.setup();
    openNIDevice.addImageGenerator();
    openNIDevice.addDepthGenerator();
    openNIDevice.setRegister(true);
    openNIDevice.setMirror(true);
    openNIDevice.addUserGenerator();
    openNIDevice.setMaxNumUsers(1); //there can be only one
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
    
    previousLeftHandPosition = ofVec3f();
    previousRightHandPosition = ofVec3f();
    
    leftHandMagnitude = 0.f;
    rightHandMagnitude = 0.f;
    
    setupControlPanel();
}

// Set up the controls in the ofxControlPanel
void testApp::setupControlPanel(){
    
	gui.setup("Gateway", 10, 10, 350, 640);
    gui.addPanel("Hand", 1);
    gui.setWhichPanel("Hand");
	gui.addLabel("'f' to toggle fullscreen \n'h' to toggle this panel \n");
	gui.addSlider("Min Beam Magnitude",		"MIN_BEAM_MAGNITUDE",		100.0,	10.0, 200.0,	false);
    //gui.addChartPlotter(string name, guiStatVarPointer varPtr, float width, float height, int maxNum, float minYVal, float maxYVal)
    //guiStatVarPointer(string displayNameIn, void *varPtr, guiVarType theDataType, bool autoUpdateGraph)
    gui.addChartPlotter("L Hand Chart", guiStatVarPointer("Left Hand Magnitude", &leftHandMagnitude, GUI_VAR_FLOAT, true), 200, 100, 200, 0.f, 200.f);
    gui.addChartPlotter("R Hand Chart", guiStatVarPointer("Right Hand Magnitude", &rightHandMagnitude, GUI_VAR_FLOAT, true), 200, 100, 200, 0.f, 200.f);
    gui.addSlider("Velocity scale factor", "VELOCITY_SCALE", 1.f, 0.f, 20.f, false);
    gui.addSlider("Width scale factor", "WIDTH_SCALE", 1.f, 0.f, 4.f, false);
    gui.addSlider("Min Width", "MIN_WIDTH", 2, 0, 40, true);
    gui.addToggle("Clear Beams", "CLEAR_BEAMS", false);

	gui.loadSettings("controlPanelSettings.xml");
}

//--------------------------------------------------------------
void testApp::update(){
	// Calculate time past per frame
	float timeNow = ofGetElapsedTimef();
    float timeSinceLastFrame = timeNow - timeOfPreviousFrame;
    
    //gui
    gui.update();
    //openni
    openNIDevice.update();
    
	// Update positions, using velocities....
    
    for (vector<Beam>::iterator myBeamIterator = beams.begin(); myBeamIterator != beams.end(); /*note emptyness of increment command*/)
    {
		myBeamIterator->position += myBeamIterator->velocity * timeSinceLastFrame;
        
        ofPoint positionOfBeam = ofPoint(myBeamIterator->position.x, myBeamIterator->position.y);
        
		// check to see if it's off the screen;
        //if(!(theScreen.inside(positionOfBeam))){ // bollocks!
        if(!(joelInside(presentationScreen, positionOfBeam))){
            //delete it if it is, returning a new iterator, safely http://www.cplusplus.com/reference/vector/vector/erase/
            myBeamIterator = beams.erase(myBeamIterator);
        } else {
            myBeamIterator++;
        }
    }
    
    bool clearBeams = gui.getValueB("CLEAR_BEAMS");
    
    if(clearBeams){
        beams.clear();
        gui.setValueB("CLEAR_BEAMS", false);
    }
    
//    while(beams.size() < MAX_BEAMS){
//        //keep adding beams
//        addRandomBeam();
//    }
    
    timeOfPreviousFrame = timeNow;

    //no more mouse gestures....
//    checkForMouseGestureAndCreateNewBeams();
    
    previousMousePosition = ofVec2f(mouseX, mouseY);
    
    int numUsers = openNIDevice.getNumTrackedUsers(); //should be only one
    
    if(numUsers > 0)
        checkForAnyHandGesturesAndCreateNewBeams();
}

//--------------------------------------------------------------
void testApp::draw(){
    ofEnableAlphaBlending();
    
    ofPushMatrix();
    ofSetColor(ofColor::white);
    // draw debug (ie., image, depth, skeleton)
    openNIDevice.drawDebug();
    ofPopMatrix();
    
    //fill black, safely
    ofPushStyle();
    ofFill();
    ofSetColor(ofColor::black);
    ofRect(presentationScreen);
    
    int i = 0;
    
    for (vector<Beam>::iterator myBeamIterator = beams.begin(); myBeamIterator != beams.end(); myBeamIterator++)
    {
        ofSetColor(myBeamIterator->colour);
        
        if(myBeamIterator->vertical){
            float widthOfBeam = myBeamIterator->size.x;
            float halfWidthOfBeam = widthOfBeam/2.f;
            float heightOfBeam = presentationScreen.getHeight();
            
            ofRect(myBeamIterator->position.x - halfWidthOfBeam, 0, widthOfBeam, heightOfBeam);
        }
    }
    
    ofPopStyle();
    
    // draw some info regarding frame counts etc
	ofSetColor(0, 255, 0);
	string msg = " MILLIS: " + ofToString(ofGetElapsedTimeMillis()) + " FPS: " + ofToString(ofGetFrameRate()) + " Device FPS: " + ofToString(openNIDevice.getFrameRate());
    
	verdana.drawString(msg, 20, openNIDevice.getNumDevices() * 480 - 20);
    
    gui.draw();
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    switch (key) {
        case 'h':
            gui.toggleView();
            break;
        case 'f':
            ofToggleFullscreen();
            break;
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

bool testApp::joelInside(ofRectangle r, ofPoint p){
    return p.x >= r.getMinX() && p.y >= r.getMinY() && p.x <= r.getMaxX() && p.y <= r.getMaxY();
}

void testApp::addRandomBeam(){
    Beam randomBeam;
    
    randomBeam.position = ofVec3f(presentationScreen.getX(),0.f,0.f); //on the top left
    randomBeam.velocity = ofVec3f(ofRandom(10.f, 100.f),ofRandom(10.f, 100.f),0.f); //move a little to the right every frame
    randomBeam.size = ofVec3f(ofRandom(10.f, 100.f),ofRandom(10.f, 100.f),0.f); //between 10 pixels wide and high
    randomBeam.colour = ofColor::white; //any colour as long as it's white...

    randomBeam.vertical = true; //yes it is
    
    beams.push_back(randomBeam); //add it to the vector
}

void testApp::addRandomBeamFromLeft(){
    Beam randomBeam;
    
    randomBeam.position = ofVec3f(presentationScreen.getX(),0.f,0.f); //on the top left
    randomBeam.velocity = ofVec3f(ofRandom(10.f, 100.f),0.f,0.f); //move a little to the right every frame
    randomBeam.size = ofVec3f(ofRandom(10.f, 100.f),0.f,0.f); //between 10 and 100 pixels wide
    randomBeam.colour = ofColor::white; //any colour as long as it's white...
    
    randomBeam.vertical = true; //yes it is
    
    beams.push_back(randomBeam); //add it to the vector
}

void testApp::addRandomBeamFromRight(){
    Beam randomBeam;
    
    randomBeam.position = ofVec3f(presentationScreen.getX() + presentationScreen.getWidth(),0.f,0.f); //on the top right
    randomBeam.velocity = ofVec3f(-ofRandom(10.f, 100.f),0.f,0.f); //move a little to the left every frame
    randomBeam.size = ofVec3f(ofRandom(10.f, 100.f),0.f ,0.f); //between 10 and 100 pixels wide
    randomBeam.colour = ofColor::white; //any colour as long as it's white...
    
    randomBeam.vertical = true; //yes it is
    
    beams.push_back(randomBeam); //add it to the vector
}

void testApp::addMouseSpeedBeam(float mouseHorizontalSpeed){
    Beam randomBeam;
    
    if(mouseHorizontalSpeed < 0){
        //then the beam should come from the left
        randomBeam.position = ofVec3f(presentationScreen.getX(),0.f,0.f); //on the top left
    }else{
        //then the beam should come from the right
        randomBeam.position = ofVec3f(presentationScreen.getX() + presentationScreen.getWidth(), 0.f, 0.f);
    }
    
    randomBeam.velocity = ofVec3f(-mouseHorizontalSpeed,0.f,0.f); //other way around to the way you think...
    randomBeam.size = ofVec3f(abs(mouseHorizontalSpeed),0.f,0.f); //size on speed too? hmm
    randomBeam.colour = ofColor::white; //any colour as long as it's white...
    
    randomBeam.vertical = true; //yes it is
    
    beams.push_back(randomBeam); //add it to the vector
}

void testApp::checkForMouseGestureAndCreateNewBeams(){
    ofVec2f currentMousePosition = ofVec2f(mouseX, mouseY);
    
    float halfWidth = guiScreen.getWidth()/2.f;
    
    float mouseHorizontalSpeed = previousMousePosition.x - currentMousePosition.x;
    
    if( previousMousePosition.x <= halfWidth && currentMousePosition.x >= halfWidth){
        addMouseSpeedBeam(mouseHorizontalSpeed);
    }
    
    if( previousMousePosition.x >= halfWidth && currentMousePosition.x <= halfWidth){
        addMouseSpeedBeam(mouseHorizontalSpeed);
    }
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

void testApp::checkForLeftHandGestureAndCreateNewBeams(){
    // get number of current users
    int numUsers = openNIDevice.getNumTrackedUsers(); //should be only one
    
    ofVec3f currentLeftHandPosition = ofVec3f();
    
    // iterate through users
//    for (int i = 0; i < numUsers; i++){
    
        // get a reference to this user
        ofxOpenNIUser & user = openNIDevice.getTrackedUser(0); //safe as we know there is at least one, otherwise this function wouldn't have been called
        
        ofxOpenNIJoint & leftHand =  user.getJoint(JOINT_LEFT_HAND);
        
        currentLeftHandPosition = leftHand.getWorldPosition();
//    }
    
    ofVec3f handVelocity = previousLeftHandPosition - currentLeftHandPosition;
    
    float handMagnitude = handVelocity.length();
    bool handConfidence = leftHand.getPositionConfidence();
    
    cout << "Hand magnitude is " << handMagnitude << endl;
    cout << "Hand confidence is" << handConfidence << endl;
    
    if(handMagnitude > 100.f && handConfidence){
        addSpeedBeam(handVelocity.x);
    }
    
    previousLeftHandPosition = currentLeftHandPosition;
}

void testApp::addSpeedBeam(float speed){
    Beam randomBeam;
    
    int minWidth = gui.getValueI("MIN_WIDTH");
    
    if(speed < 0){
        //then the beam should come from the left
        randomBeam.position = ofVec3f(presentationScreen.getX(),0.f,0.f); //on the top left
    }else{
        //then the beam should come from the right
        randomBeam.position = ofVec3f(presentationScreen.getX() + presentationScreen.getWidth(), 0.f, 0.f);
    }
    
    float velocityScale = gui.getValueF("VELOCITY_SCALE");
    float widthScale = gui.getValueF("WIDTH_SCALE");
    
    randomBeam.velocity = ofVec3f(-speed*velocityScale,0.f,0.f); //other way around to the way you think...
    randomBeam.size = ofVec3f(abs(speed*widthScale),0.f,0.f); //size on speed too? abs otherwise width is negative!
    randomBeam.colour = ofColor::white; //any colour as long as it's white...
    
    randomBeam.vertical = true; //yes it is
    
    if(randomBeam.size.x > (float)minWidth) //nothing smaller than two pixels
        beams.push_back(randomBeam); //add it to the vector
}

void testApp::checkForAnyHandGesturesAndCreateNewBeams(){
    // get number of current users
    int numUsers = openNIDevice.getNumTrackedUsers();
    
    ofVec3f currentLeftHandPosition = ofVec3f();
    ofVec3f currentRightHandPosition = ofVec3f();
    
    bool leftHandConfidence = false;
    bool rightHandConfidence = false;
    
    // iterate through users
    for (int i = 0; i < numUsers; i++){
        
        // get a reference to this user
        ofxOpenNIUser & user = openNIDevice.getTrackedUser(i);
    
        ofxOpenNIJoint & leftHand =  user.getJoint(JOINT_LEFT_HAND);
        ofxOpenNIJoint & rightHand = user.getJoint(JOINT_RIGHT_HAND);
    
    //    currentLeftHandPosition = leftHand.getWorldPosition();
    //    currentRightHandPosition = rightHand.getWorldPosition();
        currentLeftHandPosition = leftHand.getProjectivePosition();
        currentRightHandPosition = rightHand.getProjectivePosition();
        
        leftHandConfidence = (bool)leftHand.getPositionConfidence();
        rightHandConfidence = (bool)rightHand.getPositionConfidence();
    }
    
    ofVec3f leftHandVelocity = previousLeftHandPosition - currentLeftHandPosition;
    ofVec3f rightHandVelocity = previousRightHandPosition - currentRightHandPosition;
    
    leftHandMagnitude = leftHandVelocity.length();
    rightHandMagnitude = rightHandVelocity.length();
    
    float minimumMagniudeToSpawnBeam = gui.getValueF("MIN_BEAM_MAGNITUDE");
    
    if(leftHandMagnitude > minimumMagniudeToSpawnBeam && leftHandConfidence){
        addSpeedBeam(leftHandVelocity.x);
    }
    
    if(rightHandMagnitude > minimumMagniudeToSpawnBeam && rightHandConfidence){
        addSpeedBeam(rightHandVelocity.x);
    }
    
    previousLeftHandPosition = currentLeftHandPosition;
    previousRightHandPosition = currentRightHandPosition;
}



