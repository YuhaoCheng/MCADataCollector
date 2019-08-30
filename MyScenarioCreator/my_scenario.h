#ifndef MY_SCENARIO_H
#define MY_SCENARIO_H

#include "script.h"
#include <string>
#include "dictionary.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <Windows.h>
#include <gdiplus.h>
#include <iostream>
#include <cmath>
#include <fstream>
#include "keyboard.h"

typedef struct wPed {
	Ped ped;
	bool goingTo = true;
	Vector3 from, to;
	int stopTime;
	int timeFix = -1;
	float speed;
} wPed;

class MyScenarioCreator {
public:
	MyScenarioCreator();
	void update();

private:
	Player player;
	Ped playerPed;
	std::string line;  // string use the safe the frame data-line
	std::string log;
 	Cam camera;         // camera
	//Vector3 cam_coords; // coordinates of the camera, not use in the class. in the class file, we use the camCoords
	//Vector3 cam_rot;    // not use in th e class. In the class file, we use the camRot
	float cam_fov; // the fov of the camera
	bool SHOW_JOINT_RECT; // used in JTA, used to switch the rectangle drawing around the joint

	int window_width;
	int window_height;
	int image_width;
	int image_height;
	int seconds_before_save_images;  // should to set the how many seconds before recording the video
	int capture_freq; // should be the FPS
	int joint_int_codes[22]; // the code of each joit type

	int walking_round = 20; // indicate how many walking round of the peds

	/******New Parameters**********/
	//int weather_code = 2;  // indicate the weather, default is 2, meaning the weather is CLEAR 放到CPP文件中
	//float speed; // indicate the speed of peds
	wPed present_peds[1024]; // record the present peds
	int nPresent_peds = 0; // indicate the present peds
	
	HWND hwnd; // Handel A Window
	HDC hWindowDC; // Handel the device context
	HDC hCaptureDC; // Handel the device context
	HBITMAP hCaptureBitmap; // Handel the bitmap

	float recording_period; // should be the time period of the recording
	std::clock_t last_recording_time; // should be the last timestamp of the recording 
	int nsmaple;
	std::ofstream coords_file; // file used to save the joint coordinates data
	std::ofstream ped_spawn_log; //file used to save the spwaning of peds
	std::ofstream log_file;     // need to record the type of action in the video and the total number of the people 

	CLSID png_clsid;

	void cameraCoords(); // fucntion is used to show the camera coordinates
	void spawn_peds_o(Vector3 spwanAreaCenter, int numPed); // function used to spwan the pedestrains at the bedgining of the scenario
	void listen_for_keystrokes(); //function for keyboard input

	void resetMenuCommands(); // reset the commands state
	void main_menu(); // function used to load the meanu
	void weather_menu(); // function used to load the weather menu
	void time_menu(); // function to load the time menu
	void place_menu(); //function to load the place menu
	void camera_menu(); // function to load the camera menu
	void peds_menu(); // fucntion to load the peds menu
	void tasks_sub_menu(); // to change the behavour settings
	void stopControl(); //to fix player coords
	void walking_peds(); // to make peds walking
	void file_menu(); // function used to load or save files

	void cancelLastLog();
	void loadFile(char *fname);
	int myreadLine(FILE *f, Vector3 *pos, int *nPeds, int *ngroup, int *currentBehaviour, float *speed, Vector3 *goFrom, Vector3 *goTo, int *task_time,
		int *type, int *radius, int *min_lenght, int *time_between_walks, int *spawning_radius, int *actionType, int *actionSn);

	void spawn_peds_flow(Vector3 pos, Vector3 goFrom, Vector3 goTo, int npeds, int ngroup, int currentBehaviour,
		int task_time, int type, int radius, int min_lenght, int time_between_walks, int spawning_radius, float speed, int actionType, int actionSn);

	void spawn_peds(Vector3 pos, Vector3 goFrom, Vector3 goTo, int npeds, int ngroup, int currentBehaviour,
		int task_time, int type, int radius, int min_lenght, int time_between_walks, int spawning_radius, float speed, int actionType, int actionSn);

	void draw_text(char *text, float x, float y, float scale);
	
	bool keyboard_flag;

	// New add feature
	void addPed(Ped p);
	void clear_method();
	void copy_with_cam();
};

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

#endif
