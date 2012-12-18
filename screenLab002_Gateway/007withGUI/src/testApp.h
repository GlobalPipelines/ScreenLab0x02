#pragma once

#include "ofMain.h"
#include "ofxOpenNI.h"
#include "ofxControlPanel.h"

#define MAX_VELOCITY 30.0f //for now...
#define MAX_BEAMS 10

struct Beam {
    ofVec3f position; //in terms of pixel values
    ofVec3f velocity; //in terms of pixel values per time since last frame
    ofVec3f size; //pixels again
    ofColor colour;
    bool vertical; // is it vertical
};

class testApp : public ofBaseApp{
	public:
		void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    bool joelInside(ofRectangle r, ofPoint p);
    void addRandomBeam();
    void addRandomBeamFromLeft();
    void addRandomBeamFromRight();
    void addMouseSpeedBeam(float mouseHorizontalSpeed);
    void checkForMouseGestureAndCreateNewBeams();
    void userEvent(ofxOpenNIUserEvent & event);
    void exit();
    void checkForLeftHandGestureAndCreateNewBeams();
    void addSpeedBeam(float speed);
    void checkForAnyHandGesturesAndCreateNewBeams();    
    
    // objects
    vector<Beam> beams;
    
    ofRectangle guiScreen;
    ofRectangle presentationScreen;
    
    float timeOfPreviousFrame;
    
    ofVec2f previousMousePosition;
    
	ofxOpenNI openNIDevice;
    ofTrueTypeFont verdana;
    
    ofVec3f previousLeftHandPosition;
    ofVec3f previousRightHandPosition;
    
    bool  bFullscreen;
	bool  bShowControlPanel;
    
	ofxControlPanel gui;
	void setupControlPanel();
    
    float leftHandMagnitude;
    float rightHandMagnitude;
};
