#define _CRT_SECURE_NO_WARNINGS

#include "CrowdAnnotator.h"
#include <vector>
#include <direct.h>
#include <string.h>
#include <filesystem>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <list>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define TIME_FACTOR 12.0
#define FPS 30
#define DISPLAY_FLAG FALSE
#define WANDERING_RADIUS 10.0
#define MAX_PED_TO_CAM_DISTANCE 100.0
#define DEMO FALSE

void dummy_wait(BOOL demo) {
	if (demo) {
		WAIT(50);
	}
	else {
		WAIT(2000);
	}
}

std::string statusText;
DWORD statusTextDrawTicksMax;
bool statusTextGxtEntry; // should indicate the wheather there is a message
bool camLock = true;
Cam activeCam;

void set_status_text(std::string str, DWORD time = 2000, bool isGxtEntry = false) {
	/*
	this method is to set the text, 类似于正常程序中的print函数
	*/
	statusText = str;
	statusTextDrawTicksMax = GetTickCount() + time; // set the time how long the text is showing
	statusTextGxtEntry = isGxtEntry;
}

std::vector<const char*> weathers = {
		"WIND",       // 0
		"EXTRASUNNY", // 1
		"CLEAR",      //  2
		"CLOUDS",     //   3
		"SMOG",        //  4
		"FOGGY",      //   5
		"OVERCAST",   //   6
		"RAIN",       //   7
		"THUNDER",    //   8
		"CLEARING",   //   9
		"NEUTRAL",    //   10
		"SNOW",       //   11
		"BLIZZARD",   //   12
		"SNOWLIGHT",  //   13
};


static char scenarioTypes[14][40]{
	"NEAREST",
	"RANDOM",
	"WORLD_HUMAN_MUSICIAN",
	"WORLD_HUMAN_SMOKING",
	"WORLD_HUMAN_BINOCULARS",
	"WORLD_HUMAN_CHEERING",
	"WORLD_HUMAN_DRINKING",
	"WORLD_HUMAN_PARTYING",
	"WORLD_HUMAN_PICNIC",
	"WORLD_HUMAN_STUPOR",
	"WORLD_HUMAN_PUSH_UPS",
	"WORLD_HUMAN_LEANING",
	"WORLD_HUMAN_MUSCLE_FLEX",
	"WORLD_HUMAN_YOGA"
};

float random_float(float min, float max)  // make a float random value
{
	return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max-min)));
}

float random_int(int min, int max)
{
	return min + rand() % (max - min + 1);
}

Vector3 coordsToVector(float x, float y, float z)
{
	Vector3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

void log_info(char *message)
{
	/*
	Use to record the information to debug
	*/
	time_t raw_time;
	struct tm* ptminfo;
	std::ofstream log_file;
	char time_string[200];
	// get the local time
	time(&raw_time);
	ptminfo = localtime(&raw_time);
	sprintf(time_string, "[%02d-%02d-%02d %02d:%02d:%02d]:",
		ptminfo->tm_year + 1900, ptminfo->tm_mon + 1, ptminfo->tm_mday,
		ptminfo->tm_hour, ptminfo->tm_min, ptminfo->tm_sec);
	strcat(time_string, message);
	log_file.open("log_info_annotator.txt", std::ios::app);
	log_file << time_string << std::endl;
}

void camLockChange() {

	if (camLock) {
		// if camLock is TRUE
		CAM::RENDER_SCRIPT_CAMS(0, 0, 3000, 1, 0); // Go back to the gameplayer's camera
	}
	else {
		Vector3 oldCamCoords = CAM::GET_GAMEPLAY_CAM_COORD(); // should get the game player's camera coords
		Vector3 oldCamRots = CAM::GET_GAMEPLAY_CAM_ROT(2); // should get the rotation of game player's camera
		float fov = CAM::GET_GAMEPLAY_CAM_FOV();//should get the field of view of the camera
		CAM::DESTROY_ALL_CAMS(true);
		activeCam = CAM::CREATE_CAM_WITH_PARAMS((char*)"DEFAULT_SCRIPTED_CAMERA", oldCamCoords.x, oldCamCoords.y, oldCamCoords.z, oldCamRots.x, oldCamRots.y, oldCamRots.z, fov, true, 2);
		CAM::SET_CAM_ACTIVE(activeCam, true);
		CAM::RENDER_SCRIPT_CAMS(1, 0, 3000, 1, 0);

	}
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num = 0; // number of image encoders
	UINT size = 0; // size, in bytes, of the image encoder array

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);

	if (size == 0){
	
		return -1;  // Failure, indicate that don't have encoders
	}

	// Create a buffer large enough to hold the array of ImageCodecInfo
    // objects that will be returned by GetImageEncoders.
	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));

	if (pImageCodecInfo == NULL)
		return -1;  // Failure, indicate we can't malloc the memory

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j<num;j++)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format)) {
			//if the coder's format is same as the required
			//it will give the class id to the pClsid
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j; // Sucess
		}
	}

	free(pImageCodecInfo);
	return -1; //Failure, we can't find the required format
}

int StringToWString(std::wstring &ws, const std::string &s) {
	std::wstring wsTmp(s.begin(), s.end());
	ws = wsTmp;
	return 0;
}

CrowdAnnotator::CrowdAnnotator(std::string _output_path, const char* _file_scenario, int _max_samples, int _is_night) 
{
	// the construct method of the CrowdAnnotator
	PLAYER::SET_EVERYONE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);
	PLAYER::SET_POLICE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);
	PLAYER::CLEAR_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_PED_ID());

	ENTITY::SET_ENTITY_COLLISION(PLAYER::PLAYER_PED_ID(), TRUE, TRUE);
	ENTITY::SET_ENTITY_VISIBLE(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
	ENTITY::SET_ENTITY_ALPHA(PLAYER::PLAYER_PED_ID(), 255, FALSE);
	ENTITY::SET_ENTITY_CAN_BE_DAMAGED(PLAYER::PLAYER_PED_ID(), FALSE);

	GAMEPLAY::SET_TIME_SCALE(1);

	// Set the attribute of the class
	this->output_path = _output_path;
	this->file_scenario = _file_scenario;
	this->max_samples = _max_samples;
	this->is_night = _is_night;

	// Should Set some scenarios, we don't need
	this->bad_scenarios = {
		"WORLD_BOAR_GRAZING",
		"WORLD_CAT_SLEEPING_GROUND",
		"WORLD_CAT_SLEEPING_LEDGE",
		"WORLD_COW_GRAZING",
		"WORLD_COYOTE_HOWL",
		"WORLD_COYOTE_REST",
		"WORLD_COYOTE_WANDER",
		"WORLD_CHICKENHAWK_FEEDING",
		"WORLD_CHICKENHAWK_STANDING",
		"WORLD_CORMORANT_STANDING",
		"WORLD_CROW_FEEDING",
		"WORLD_CROW_STANDING",
		"WORLD_DEER_GRAZING",
		"WORLD_DOG_BARKING_ROTTWEILER",
		"WORLD_DOG_BARKING_RETRIEVER",
		"WORLD_DOG_BARKING_SHEPHERD",
		"WORLD_DOG_SITTING_ROTTWEILER",
		"WORLD_DOG_SITTING_RETRIEVER",
		"WORLD_DOG_SITTING_SHEPHERD",
		"WORLD_DOG_BARKING_SMALL",
		"WORLD_DOG_SITTING_SMALL",
		"WORLD_FISH_IDLE",
		"WORLD_GULL_FEEDING",
		"WORLD_GULL_STANDING",
		"WORLD_HEN_PECKING",
		"WORLD_HEN_STANDING",
		"WORLD_MOUNTAIN_LION_REST",
		"WORLD_MOUNTAIN_LION_WANDER",
		"WORLD_PIG_GRAZING",
		"WORLD_PIGEON_FEEDING",
		"WORLD_PIGEON_STANDING",
		"WORLD_RABBIT_EATING",
		"WORLD_RATS_EATING",
		"WORLD_SHARK_SWIM",
		"PROP_BIRD_IN_TREE",
		"PROP_BIRD_TELEGRAPH_POLE"
	};

	// Find the joint's code, 我们可以从哪里找到这些code是如何定义的呢?（已解决：https://pastebin.com/3pz17QGd）
	joint_int_codes[0] = m.find("SKEL_Head")->second; 
	joint_int_codes[1] = m.find("SKEL_Neck_1")->second;
	joint_int_codes[2] = m.find("SKEL_R_Clavicle")->second;
	joint_int_codes[3] = m.find("SKEL_R_UpperArm")->second;
	joint_int_codes[4] = m.find("SKEL_R_Forearm")->second;
	joint_int_codes[5] = m.find("SKEL_R_Hand")->second;
	joint_int_codes[6] = m.find("SKEL_L_Clavicle")->second;
	joint_int_codes[7] = m.find("SKEL_L_UpperArm")->second;
	joint_int_codes[8] = m.find("SKEL_L_Forearm")->second;
	joint_int_codes[9] = m.find("SKEL_L_Hand")->second;
	joint_int_codes[10] = m.find("SKEL_Spine3")->second;
	joint_int_codes[11] = m.find("SKEL_Spine2")->second;
	joint_int_codes[12] = m.find("SKEL_Spine1")->second;
	joint_int_codes[13] = m.find("SKEL_Spine0")->second;
	joint_int_codes[14] = m.find("SKEL_Spine_Root")->second;
	joint_int_codes[15] = m.find("SKEL_R_Thigh")->second;
	joint_int_codes[16] = m.find("SKEL_R_Calf")->second;
	joint_int_codes[17] = m.find("SKEL_R_Foot")->second;
	joint_int_codes[18] = m.find("SKEL_L_Thigh")->second; // 大腿，股
	joint_int_codes[19] = m.find("SKEL_L_Calf")->second; // 小腿
	joint_int_codes[20] = m.find("SKEL_L_Foot")->second;


	// inizialize the coords_file used to storage coords data
	log_file.open(output_path + "\\log.txt"); // define in the .h file, std::ofstream
	coords_file.open(output_path + "\\coords.csv"); // define in the .h file, std::ofstream
	action_file.open(output_path + "\\action.csv"); // define in the .h file, std::ofstream
	coords_file << "frame,pedestrian_id,joint_type,2D_x,2D_y,3D_x,3D_y,3D_z,occluded,self_occluded,";
	coords_file << "cam_3D_x,cam_3D_y,cam_3D_z,cam_rot_x,cam_rot_y,cam_rot_z,fov\n";
	action_file << "ped_id,actionType,actionSn\n";
	log_file << "Start!!\n";

	// the return values of these following two methods are same
	// because in the story mode
	this->player = PLAYER::PLAYER_ID();
	this->playerPed = PLAYER::PLAYER_PED_ID();

	this->line = "";
	this->log = "";

	this->captureFreq = (int)(FPS / TIME_FACTOR); // the captureFreq != FPS?
	this->SHOW_JOINT_RECT = DISPLAY_FLAG;

	this->fov = 50;

	std::vector<const char*> weathers = {
		"WIND",       // 0
		"EXTRASUNNY", // 1
		"CLEAR",      //  2
		"CLOUDS",     //   3
		"SMOG",        //  4
		"FOGGY",      //   5
		"OVERCAST",   //   6
		"RAIN",       //   7
		"THUNDER",    //   8
		"CLEARING",   //   9
		"NEUTRAL",    //   10
		"SNOW",       //   11
		"BLIZZARD",   //   12
		"SNOWLIGHT",  //   13
	};


	//**************************************
	loadScenario(file_scenario); // Load the scenario form the log file, and generate the peds
	//*****************************************
	
	// Screen capture buffer, 获得了一个DC的句柄
	GRAPHICS::_GET_SCREEN_ACTIVE_RESOLUTION(&windowWidth, &windowHeight); // get the resolution fo the screen
	hWnd = ::FindWindow(NULL, "Compatitibility Theft Auto V");
	hWindowDC = GetDC(hWnd);
	hCaptureDC = CreateCompatibleDC(hWindowDC);
	hCaptureBitmap = CreateCompatibleBitmap(hWindowDC, SCREEN_WIDTH, SCREEN_HEIGHT);
	SelectObject(hCaptureDC, hCaptureBitmap);// 把一个对象(位图、画笔、画刷等)选入指定的设备描述表
	//SetStretchBltMode(hCaptureDC, COLORONCOLOR);
	SetStretchBltMode(hCaptureDC, HALFTONE); // in order to get the better pic, but slow than the previous one

	// used to decide how often save the sample
	recordingPeriod = 1.0f / captureFreq;

	// initialize recording stuff
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	GetEncoderClsid(L"image/jpeg", &pngClsid); // obtain the encoder's id of jpeg

	// inizialize the int used to count the saved frame
	nsample = 0;
	/*
	*/
	//Avoid bad things such as getting killed by the police, robbed, dying in car accidents or other horrible stuff
	PLAYER::SET_EVERYONE_IGNORE_PLAYER(player, TRUE);
	PLAYER::SET_POLICE_IGNORE_PLAYER(player, TRUE);
	PLAYER::CLEAR_PLAYER_WANTED_LEVEL(player);

	PLAYER::SET_PLAYER_INVINCIBLE(player, TRUE);
	PLAYER::SPECIAL_ABILITY_FILL_METER(player, 1);
	PLAYER::SET_PLAYER_NOISE_MULTIPLIER(player, 0.0);
	PLAYER::SET_SWIM_MULTIPLIER_FOR_PLAYER(player, 1.49f);
	PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(player, 1.49f);
	PLAYER::DISABLE_PLAYER_FIRING(player, TRUE);
	PLAYER::SET_DISABLE_AMBIENT_MELEE_MOVE(player, TRUE);

	/*
	// will cause bugs, not use
	//invisible and intangible player
	if (moving)
		ENTITY::SET_ENTITY_COLLISION(PLAYER::PLAYER_PED_ID(), TRUE, TRUE);
	else
		ENTITY::SET_ENTITY_COLLISION(PLAYER::PLAYER_PED_ID(), FALSE, TRUE);
	*/

	//ENTITY::SET_ENTITY_ALPHA(PLAYER::PLAYER_PED_ID(), 0, FALSE);
	//ENTITY::SET_ENTITY_CAN_BE_DAMAGED(PLAYER::PLAYER_PED_ID(), FALSE);

	ENTITY::SET_ENTITY_VISIBLE(PLAYER::PLAYER_PED_ID(), FALSE, FALSE);
	//seconds are proportional to number of peds
	if (DEMO) {
		this->secondsBeforeSaveImages = 10;
	}
	else {
		this->secondsBeforeSaveImages = max_waiting_time / 1000 + 10 + 10; // ? max_waiting_time 从哪儿来的？
	}

	// Because std::clock() returns ms, and we use seconds, we need plus the seconds with CLOCKS_PER_SEC(1000)
	// clock_t is a long int type
	lastRecordingTime = std::clock() + (clock_t)((float)secondsBeforeSaveImages * CLOCKS_PER_SEC);
	
}


Cam CrowdAnnotator::lockCam(Vector3 pos, Vector3 rot, float fov) 
{
	//fov = 50;
	CAM::DESTROY_ALL_CAMS(true);
	//args: "DEFAULT_SCRIPTED_CAMERA", pos.x, pos.y, pos.z, rot.z, rot.y, rot.z, fov, true, 2
	//create the cam
	Cam lockedCam = CAM::CREATE_CAM_WITH_PARAMS((char *)"DEFAULT_SCRIPTED_CAMERA", pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, fov, true, 2);
	//active the cam
	CAM::SET_CAM_ACTIVE(lockedCam, true);
	CAM::RENDER_SCRIPT_CAMS(1, 0, 3000, 1, 0);

	return lockedCam; // hw, we don't use this return cam
}


int CrowdAnnotator::myreadLine(FILE *f, Vector3 *pos, int *nPeds, int *ngroup, int *currentBehaviour, float *speed, Vector3 *goFrom, Vector3 *goTo, int *task_time, int *type,
	int *radius, int *min_lenght, int *time_between_walks, int *spawning_radius, int *actionType, int *actionSn)
{
	//read the lines of creating the peds
	int result = fscanf_s(f, "%d %f %f %f %d %d %f %f %f %f %f %f %f %d %d %d %d %d %d %d %d\n", nPeds, &(*pos).x, &(*pos).y, &(*pos).z,
		ngroup, currentBehaviour, speed,
		&(*goFrom).x, &(*goFrom).y, &(*goFrom).z, &(*goTo).x, &(*goTo).y, &(*goTo).z,
		task_time, type, radius, min_lenght, time_between_walks, spawning_radius, actionType, actionSn);

	return result; // in order to indicate wheather we come the end of the file
}

void CrowdAnnotator::setCameraFixed(Vector3 coords, Vector3 rot, float cam_z, int fov)
{
	// Create the camera based on the coords and rot
	// And get the value to the global variable

	// cam_z the offset of the camera in z-axis
	CAM::DESTROY_ALL_CAMS(TRUE);
	this->camera = CAM::CREATE_CAM((char *)"DEFAULT_SCRIPTED_CAMERA", TRUE);
	//CAM::SET_CAM_COORD(camera, coords.x, coords.y, coords.z + cam_z);
	CAM::SET_CAM_COORD(camera, coords.x, coords.y, coords.z);
	CAM::SET_CAM_ROT(camera, rot.x, rot.y, rot.z, 2);
	CAM::SET_CAM_FOV(camera, (float)fov);
	CAM::SET_CAM_ACTIVE(camera, TRUE);
	//CAM::RENDER_SCRIPT_CAMS(TRUE, FALSE, 0, TRUE, TRUE);
	CAM::RENDER_SCRIPT_CAMS(1, 0, 3000, 1, 0);


	// set the cam_coords used on update() function
	this->cam_coords = CAM::GET_CAM_COORD(camera);
	this->cam_rot = CAM::GET_CAM_ROT(camera, 2);
	this->fov = (int)CAM::GET_CAM_FOV(camera);
	
	char log_message[100];
	sprintf_s(log_message, "The camera X:%.3f, Y:%.3f, Z:%.3f, FOV:%d", coords.x, coords.y, coords.z, fov);
	log_info(log_message);
}

void CrowdAnnotator::setCameraMoving(Vector3 A, Vector3 B, Vector3 C, int fov)
{
	// create the A,B,C 
}


void CrowdAnnotator::loadScenario(const char* fname) 
{
	/*
	load from the file and generate the scenarion
	TODO:
		-[x] Add the function to load the weather 
	*/
	FILE *f = fopen(fname, "r");
	Vector3 cCoords, cRot;
	Vector3 vTP1, vTP2, vTP1_rot, vTP2_rot;
	float fov;

	int weather_code;
	int record_hour;
	int record_minute;

	int stop;
	fscanf_s(f, "%d", &moving);

	if (moving == 0)
		fscanf_s(f, "%f %f %f %d %f %f %f %f %d %d %d\n", &cCoords.x, &cCoords.y, &cCoords.z, &stop, &cRot.x, &cRot.y, &cRot.z, &fov, &weather_code, &record_hour, &record_minute);
	else
		fscanf_s(f, "%f %f %f %d %f %f %f %f %f %f %d %d %d\n", &A.x, &A.y, &A.z, &stop, &B.x, &B.y, &B.z, &C.x, &C.y, &C.z, &weather_code, &record_hour, &record_minute);
	
	log_info((char *)weathers[weather_code]);
	fscanf_s(f, "%f %f %f %f %f %f\n", &vTP1.x, &vTP1.y, &vTP1.z, &vTP1_rot.x, &vTP1_rot.y, &vTP1_rot.z);
	fscanf_s(f, "%f %f %f %f %f %f\n", &vTP2.x, &vTP2.y, &vTP2.z, &vTP2_rot.x, &vTP2_rot.y, &vTP2_rot.z);

	Entity e = PLAYER::PLAYER_PED_ID();
	//ENTITY::SET_ENTITY_VISIBLE(e, FALSE, FALSE); // invisible

	// set the time and freeze the time
	TIME::SET_CLOCK_TIME(record_hour, record_minute, 0);
	//TIME::PAUSE_CLOCK(true);
	//WAIT(3000);
	
	// set the  weather
	GAMEPLAY::CLEAR_WEATHER_TYPE_PERSIST();
	GAMEPLAY::SET_WEATHER_TYPE_NOW_PERSIST((char *)weathers[weather_code]);

	char log_message[100];
	sprintf_s(log_message, "Create the weather:%s,time:%d:%d", weathers[weather_code], record_hour, record_minute);
	log_info(log_message);

	// teleport twice in order to give the enough time to generate the sceneario
	// teleport far away in order to load game scenarios
	
	ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, vTP1.x, vTP1.y, vTP1.z, 0, 0, 1);
	lockCam(vTP1, vTP1_rot,50);
	WAIT(5000);

	// teleport far away in order to load game scenarios
	ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, vTP2.x, vTP2.y, vTP2.z, 0, 0, 1);
	lockCam(vTP2, vTP2_rot,50);
	WAIT(5000);
	
	// e is the player, and only transport the player to the place
	if (moving == 0)
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, cCoords.x, cCoords.y, cCoords.z, 0, 0, 1);
	else
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, B.x, B.y, B.z, 0, 0, 1);
	/*
	camLock = false;
	camLockChange();
	CAM::SET_CAM_ROT(activeCam, cRot.x, cRot.y, cRot.z, 2);
	CAM::SET_CAM_COORD(activeCam, cCoords.x, cCoords.y, cCoords.z);
	CAM::SET_CAM_FOV(activeCam, fov);
	*/
	// the params in the log file
	Vector3 pos, goFrom, goTo;
	int npeds, ngroup, currentBehaviour, task_time, type, radius, min_lenght, time_between_walks, spawning_radius;
	float speed;
	int actionType;
	int actionSn;

	while (myreadLine(f, &pos, &npeds, &ngroup, &currentBehaviour, &speed,
		&goFrom, &goTo, &task_time, &type, &radius, &min_lenght,
		&time_between_walks, &spawning_radius, &actionType, &actionSn)>= 0) 
	{
		if (currentBehaviour == 8) {
			// using spawn_peds_flow generate the MOVE function
			spawn_peds_flow(pos, goFrom, goTo, npeds, ngroup,
				currentBehaviour, task_time, type, radius,
				min_lenght, time_between_walks, spawning_radius, speed, actionType, actionSn);
		}
		else {
			// using spawn_peds generate the other function
			spawn_peds(pos, goFrom, goTo, npeds, ngroup,
				currentBehaviour, task_time, type, radius,
				min_lenght, time_between_walks, spawning_radius, speed, actionType, actionSn);
		}
	}
	fclose(f);

	ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, cCoords.x, cCoords.y, cCoords.z, 0, 0, 1);

	
	if (moving == 0) {
		CrowdAnnotator::setCameraFixed(cCoords, cRot, 0, fov);
	}else {
		CrowdAnnotator::setCameraMoving(A, B, C, fov);
	}
	

	//walking_peds();

}


/*
addwPed() & addwPed_scenario() have the same function, need to combine them into one function
each one has the max value of the peds, 299
TODO: mergen them
*/
void CrowdAnnotator::addwPed(Ped p, Vector3 from, Vector3 to, int stop, float spd) 
{
	// add the walking ped
	/*
	if (nwPeds > 1000) {
		return;
	}
	*/
	wPeds[nwPeds].ped = p;
	wPeds[nwPeds].from = from;
	wPeds[nwPeds].to = to;
	wPeds[nwPeds].stopTime = stop;
	wPeds[nwPeds].speed = spd;

	nwPeds++;
}


void CrowdAnnotator::spawn_peds_flow(Vector3 pos, Vector3 goFrom, Vector3 goTo, int npeds, int ngroup, int currentBehaviour,
	int task_time, int type, int radius, int min_lenght, int time_between_walks, int spawning_radius, float speed, int actionType, int actionSn) 
{
	// generate the walking flow of the peds

	Ped peds[500];
	//Ped ped_speculars[100];
	Vector3 current;
	//int i = 0; // ? use? 

	float rnX, rnY;
	
	//if (currentBehaviour == 8) {
	// when currentBehaviour = 8, the task is MOVE
	for (int i = 0; i < npeds; i++) {
		peds[i] = PED::CREATE_RANDOM_PED(goFrom.x, goFrom.y, goFrom.z); // create the random ped
		char log_message[200];
		sprintf_s(log_message, "In spawning flow(cb:8) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], goFrom.x, goFrom.y, goFrom.z);
		log_info(log_message);
		WAIT(100);
	}
	//dummy_wait(DEMO);

	for (int i = 0; i < npeds;i++) {
		ENTITY::SET_ENTITY_HEALTH(peds[i], 0); // kill all peds, in order to avoid the bugs
		WAIT(50);
	}

	//dummy_wait(DEMO);

	for (int i = 0; i < npeds; i++) {
		// set the basic attributes of the peds
		AI::CLEAR_PED_TASKS_IMMEDIATELY(peds[i]);
		PED::RESURRECT_PED(peds[i]);
		PED::REVIVE_INJURED_PED(peds[i]);

		ENTITY::SET_ENTITY_COLLISION(peds[i], TRUE, TRUE);

		PED::SET_PED_CAN_RAGDOLL(peds[i], TRUE); // 让ped像布偶一样
		PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(peds[i], TRUE);
		PED::SET_PED_COMBAT_ATTRIBUTES(peds[i], 1, FALSE);
	}

		//dummy_wait(DEMO);
	//}

	// resurrecting all pedestrains and assigning them a task
	for (int i = 0; i < npeds; i++) {
		WAIT(50);

		current = ENTITY::GET_ENTITY_COORDS(peds[i],TRUE);

		// Not use in this function
		Ped target = peds[0];
		if (npeds > 1) {
			target = peds[1];
		}
		/*
		Use in the future to generate mult-target
		if (currentBehaviour == 6){
		int target_number = npeds / 3;
		Ped target_peds[target_number];
		int temp_index;
		for (int i =0; i<target_number;i++){
			temp_index = random_int(0, npeds);
			target_peds[i] = peds[temp_index];
		}
		}
		*/
		addPeds(peds[i]);
		//Vector3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_ID(), TRUE); // the position of the current player

		if (spawning_radius == -1) {
			rnX = (float)(((rand() % 81) - 40) / 10.0); // may be the experience value
			rnY = (float)(((rand() % 81) - 40) / 10.0);
		}
		else {
			rnX = (float)((rand() % (spawning_radius * 2)) - spawning_radius);
			rnY = (float)((rand() % (spawning_radius * 2)) - spawning_radius);
		}
		//float speed_rnd = (float)(10 + rand() % 4) / 10;
		WAIT(50);
		// add the walking ped
		addwPed(peds[i], coordsToVector(goFrom.x + rnX, goFrom.y + rnY, goFrom.z), coordsToVector(goTo.x + rnX, goTo.y + rnY, goTo.z), time_between_walks, speed);
		Object seq; // actually it is a int
		
		// waiting time proportional to distance
		float distance = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(goFrom.x, goFrom.y, goFrom.z, goTo.x, goTo.y, goTo.z, 1);
		int max_time = (int)((distance / 2.5) * 1000);

		if (max_time > max_waiting_time)
			max_waiting_time = max_time;

		AI::OPEN_SEQUENCE_TASK(&seq);
		//AI::TASK_USE_MOBILE_PHONE_TIMED(0, rand() % max_time);

		AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed, 0, 0, 786603, 0xbf800000);
		AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed, 0, 0, 786603, 0xbf800000);

		AI::SET_SEQUENCE_TO_REPEAT(seq, TRUE);
		/*
		for (int round = 0; round < walking_round; round++) {
			// same two line code
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed, 0, 0, 786603, 0xbf800000);
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed, 0, 0, 786603, 0xbf800000);
		}
		*/
		/*
		AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
		AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);

		AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
		AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);

		AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
		AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
		*/
		
		AI::CLOSE_SEQUENCE_TASK(seq);
		AI::TASK_PERFORM_SEQUENCE(peds[i], seq);
		AI::CLEAR_SEQUENCE_TASK(&seq);
	}

	for (int i = 0; i < npeds; i++) {
		action_file << peds[i]; // the ped_id of the pedestrain
		action_file << "," << actionType; // the action type of the pedestrain 
		action_file << "," << actionSn; // the sequence number of the crowd which the person is in.
		action_file << "\n";
	}

}

void CrowdAnnotator::spawn_peds(Vector3 pos, Vector3 goFrom, Vector3 goTo, int npeds, int ngroup, int currentBehaviour,
	int task_time, int type, int radius, int min_lenght, int time_between_walks, int spawning_radius, float speed, int actionType, int actionSn) 
{
	// generate the standard peds
	Ped peds[500]; // a maximun number of each line is 100
	Vector3 current;
	int i = 0;

	char action_message[100];
	float heading_rn = 0.0; // make the pedestrains have different heading

	int rn;

	for (int i = 0; i < npeds; i++) {
		peds[i] = PED::CREATE_RANDOM_PED(pos.x, pos.y, pos.z);
		WAIT(50);
	}
	for (int i = 0; i < npeds; i++) {
		ENTITY::SET_ENTITY_HEALTH(peds[i], 0);
		WAIT(50);
	}
	WAIT(500);
	for (int i = 0; i < npeds; i++) {
		AI::CLEAR_PED_TASKS_IMMEDIATELY(peds[i]);
		PED::RESURRECT_PED(peds[i]);
		PED::REVIVE_INJURED_PED(peds[i]);
		ENTITY::SET_ENTITY_COLLISION(peds[i], TRUE, TRUE);
		PED::SET_PED_CAN_RAGDOLL(peds[i], TRUE);
		PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(peds[i], TRUE);
		PED::SET_PED_COMBAT_ATTRIBUTES(peds[i], 1, FALSE);
	}

	// assigning the task to each peds 
	for (int i = 0; i < npeds; i++) {
		WAIT(50);
		current = ENTITY::GET_ENTITY_COORDS(peds[i], TRUE);

		Ped target = peds[0];
		if (npeds > 1) {
			target = peds[1];
		}
		/*
		Use in the future to generate mult-target
		if (currentBehaviour == 6){
		int target_number = npeds / 3;
		Ped target_peds[target_number];
		int temp_index;
		for (int i =0; i<target_number;i++){
			temp_index = random_int(0, npeds);
			target_peds[i] = peds[temp_index];
		}
		}
		*/
		addPeds(peds[i]);

		Vector3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_ID(), TRUE);
		char log_message[200];

		switch (currentBehaviour) 
		{
		case 0:
			rn = rand() % 12 + 2;
			if (type == 0) {
				heading_rn = rand();
				AI::TASK_USE_NEAREST_SCENARIO_TO_COORD(peds[i], current.x, current.y, current.z, 100.0, task_time);
				AI::TASK_ACHIEVE_HEADING(peds[i], heading_rn,1000);
				//char log_message[200];
				sprintf_s(log_message, "In spawning (cb:0,type:nearst) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
				log_info(log_message);
			}
			else if (type == 1)
			{	
				heading_rn = rand();
				AI::TASK_START_SCENARIO_IN_PLACE(peds[i], scenarioTypes[rn], 0, true);
				//char log_message[200];
				AI::TASK_ACHIEVE_HEADING(peds[i], heading_rn, 1000);
				sprintf_s(log_message, "In spawning (cb:0,type:random) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
				log_info(log_message);
			}
			else {
				heading_rn = rand();
				AI::TASK_START_SCENARIO_IN_PLACE(peds[i], scenarioTypes[type], 0, true);
				AI::TASK_ACHIEVE_HEADING(peds[i], heading_rn, 1000);
				//char log_message[200];
				sprintf_s(log_message, "In spawning (cb:0,type:%s) the pedid:%d, the location:(%.3f, %.3f,%.3f)", scenarioTypes[type], peds[i], current.x, current.y, current.z);
				log_info(log_message);
			}
			
			break;
		case 1:
			AI::TASK_STAND_STILL(peds[i], task_time);
			//char log_message[200];
			heading_rn = rand();
			AI::TASK_ACHIEVE_HEADING(peds[i], heading_rn, 1000);
			sprintf_s(log_message, "In spawning (cb:1) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
			log_info(log_message);
			break;
		case 2:
			AI::TASK_USE_MOBILE_PHONE_TIMED(peds[i], task_time);
			//char log_message[200];
			heading_rn = rand();
			AI::TASK_ACHIEVE_HEADING(peds[i], heading_rn, 1000);
			sprintf_s(log_message, "In spawning (cb:2) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
			log_info(log_message);
			break;
		case 3:
			AI::TASK_COWER(peds[i], task_time);
			//char log_message[200];
			sprintf_s(log_message, "In spawning (cb:3) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
			log_info(log_message);
			break;
		case 4:
			AI::TASK_WANDER_IN_AREA(peds[i], current.x, current.y, current.z, (float)radius, (float)min_lenght, (float)time_between_walks);
			//char log_message[200];
			sprintf_s(log_message, "In spawning (cb:4) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
			log_info(log_message);
			break;
		case 5:
			if (i > 0)
				AI::TASK_CHAT_TO_PED(peds[i], peds[0], 16, 0.0, 0.0, 0.0, 0.0, 0.0);
			else
				AI::TASK_CHAT_TO_PED(peds[i], target, 16, 0.0, 0.0, 0.0, 0.0, 0.0);
			//char log_message[200];
			sprintf_s(log_message, "In spawning (cb:5) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
			log_info(log_message);
			break;
		case 6:
			/*
			TODO: the different peds can fight with each other
			*/
			if (i > 0)
				AI::TASK_COMBAT_PED(peds[i], peds[0], 0, 16); // need to imporve!!!
			//char log_message[200];
			sprintf_s(log_message, "In spawning (cb:6) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
			log_info(log_message);
			break;
		case 7:
			AI::TASK_STAY_IN_COVER(peds[i]);
			//char log_message[200];
			sprintf_s(log_message, "In spawning (cb:7) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
			log_info(log_message);
			break;
		default:
			break;
		}

	}

	for (int i = 0; i < npeds; i++) {
		action_file << peds[i]; // the ped_id of the pedestrain
		action_file << "," << actionType; // the action type of the pedestrain 
		action_file << "," << actionSn; // the sequence number of the crowd which the person is in.
		action_file << "\n";
	}

}

void CrowdAnnotator::spawn_combat()
{
	// need to do
}

void CrowdAnnotator::walking_peds() 
{
	// NOT USE in this class!!!!!!!!!!!
	// TODO: delete it
	for (int i = 0; i < nwPeds; i++) {
		//if (PED::IS_PED_STOPPED(wPeds[i].ped && !AI::GET_IS_TASK_ACTIVE(wPeds[i].ped, 426))) {
		if (PED::IS_PED_STOPPED(wPeds[i].ped)) {
			int currentTime = (TIME::GET_CLOCK_HOURS()) * 60 + TIME::GET_CLOCK_MINUTES();
			if (wPeds[i].timeFix == -1) {
				wPeds[i].timeFix = currentTime;
			}
			if (wPeds[i].timeFix + wPeds[i].stopTime < currentTime) 
			{
				wPeds[i].goingTo = !wPeds[i].goingTo;
				wPeds[i].timeFix = -1;
				if (wPeds[i].goingTo) {
					AI::TASK_GO_TO_COORD_ANY_MEANS(wPeds[i].ped, wPeds[i].to.x, wPeds[i].to.y, wPeds[i].to.z, wPeds[i].speed, 0, 0, 786603, 0xbf800000);
				}
				else {
					AI::TASK_GO_TO_COORD_ANY_MEANS(wPeds[i].ped, wPeds[i].from.x, wPeds[i].from.y, wPeds[i].from.z, wPeds[i].speed, 0, 0, 786603, 0xbf800000);// stand
				}
			}
		}
	}
}

void CrowdAnnotator::get_desnity_map() 
{
	/*Get the density map of the frame
	maybe used in the update()
	refer the GCC
	TODO:
		1. get the density map of the frame
		2. save the density map
	*/
}

void CrowdAnnotator::get_2D_from_3D(Vector3 v, float *x2d, float *y2d)
{
	/*
	Not understand the theory, just use
	*/
	// translation
	float x = v.x - cam_coords.x;
	float y = v.y - cam_coords.y;
	float z = v.z - cam_coords.z;

	// rotation
	float cam_x_rad = cam_rot.x * (float)M_PI / 180.0f;
	float cam_y_rad = cam_rot.y * (float)M_PI / 180.0f;
	float cam_z_rad = cam_rot.z * (float)M_PI / 180.0f;

	// cos
	float cx = cos(cam_x_rad);
	float cy = cos(cam_y_rad);
	float cz = cos(cam_z_rad);

	// sin
	float sx = sin(cam_x_rad);
	float sy = sin(cam_y_rad);
	float sz = sin(cam_z_rad);

	Vector3 d;
	d.x = cy * (sz*y + cz * x) - sy * z;
	d.y = sx * (cy*z + sy * (sz*y + cz * x)) + cx * (cz*y - sz * x);
	d.z = cx * (cy*z + sy * (sz*y + cz * x)) - sx * (cz*y - sz * x);

	float fov_rad = fov * (float)M_PI / 180;
	float f = (SCREEN_HEIGHT / 2.0f) * cos(fov_rad / 2.0f) / sin(fov_rad / 2.0f);

	*x2d = ((d.x * (f / d.y)) / SCREEN_WIDTH + 0.5f);
	*y2d = (0.5f - (d.z * (f / d.y)) / SCREEN_HEIGHT);
}

void CrowdAnnotator::save_frame()
{
	// save the frames
	StretchBlt(hCaptureDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hWindowDC, 0, 0, windowWidth, windowHeight, SRCCOPY | CAPTUREBLT);
	Gdiplus::Bitmap image(hCaptureBitmap, (HPALETTE)0);
	std::wstring ws;
	StringToWString(ws, output_path);

	image.Save((ws + L"\\" + std::to_wstring(nsample) + L".jpeg").c_str(), &pngClsid, NULL);
}

int CrowdAnnotator::update() 
{
	/*
	Based on the Log file to re-create the scene!
	Need to imporve !!!!!!!
	*/
	float delay = ((float)(std::clock() - lastRecordingTime)) / CLOCKS_PER_SEC; // calculate the period between the strat and now
	
	// if delay is smaller than the period, we will return the nsample and wait
	// Useful, not accurate, need to improve to truely same as the FPS 
	if (delay >= recordingPeriod)
		lastRecordingTime = std::clock();
	else
		return nsample;

	//PED::SET_PED_DENSITY_MULTIPLIER_THIS_FRAME(1.0); // Normal setting
	walking_peds();
	Ped peds[max_number_of_peds];   // array of pedestrains
	int number_of_peds = worldGetAllPeds(peds, max_number_of_peds); // number of pedestrain taken
	float C;              // coefficent used to adjust the size of rectangles drawn around the joints

	// setting the basic attributes based on the category of each ped
	for (int i = 0; i < number_of_peds; i++) 
	{
		if (!PED::IS_PED_A_PLAYER(peds[i]) && peds[i] != ped_with_cam) {
			ENTITY::SET_ENTITY_COLLISION(peds[i], TRUE, TRUE); // can collision
			ENTITY::SET_ENTITY_VISIBLE(peds[i],TRUE, FALSE); // visible
			ENTITY::SET_ENTITY_ALPHA(peds[i], 255, FALSE); // not transparent
			ENTITY::SET_ENTITY_CAN_BE_DAMAGED(peds[i], FALSE);// can't be damaged
		}
		else if (PED::IS_PED_A_PLAYER(peds[i])) {
			if (moving) {
				ENTITY::SET_ENTITY_COLLISION(peds[i], TRUE, TRUE); // can collision
				ENTITY::SET_ENTITY_VISIBLE(peds[i], FALSE, FALSE); // invisible
				ENTITY::SET_ENTITY_ALPHA(peds[i], 0, FALSE); // transparent
				ENTITY::SET_ENTITY_CAN_BE_DAMAGED(peds[i], FALSE); // can't be damaged
			}
			else {
				ENTITY::SET_ENTITY_COLLISION(peds[i], FALSE, TRUE); // can't collision
				ENTITY::SET_ENTITY_VISIBLE(peds[i], FALSE, FALSE); // invisible
				ENTITY::SET_ENTITY_ALPHA(peds[i], 0, FALSE); // transparent
				ENTITY::SET_ENTITY_CAN_BE_DAMAGED(peds[i], FALSE); // can't be damaged
			}
		}
		else if (peds[i] == ped_with_cam) {
			ENTITY::SET_ENTITY_COLLISION(ped_with_cam, TRUE, TRUE); // can collision
			ENTITY::SET_ENTITY_VISIBLE(ped_with_cam, FALSE, FALSE); // invisible
			ENTITY::SET_ENTITY_ALPHA(ped_with_cam, 0, FALSE); // transparent
			ENTITY::SET_ENTITY_CAN_BE_DAMAGED(ped_with_cam, FALSE); // can't be damaged
		}
	}

	// 如果摄像头会移动的话
	/*
	if (moving) {
		this->cam_coords = CAM::GET_CAM_COORD(camera);
		Vector3 ped_with_cam_rot = ENTITY::GET_ENTITY_ROTATION(this->ped_with_cam, 2);
		CAM::SET_CAM_ROT(camera, ped_with_cam_rot.x, ped_with_cam_rot.y, ped_with_cam_rot.z,2);
		this->cam_rot = CAM::GET_CAM_ROT(this->camera,2);
	}
	*/

	// scan all the pedestrains taken, because number_of_peds are the total number of peds in frame
	for (int i = 0; i < number_of_peds; i++) 
	{
		// ignore pedestrains in vehicles or dead pedestrains
		if (PED::IS_PED_IN_ANY_VEHICLE(peds[i], TRUE) || PED::IS_PED_DEAD_OR_DYING(peds[i], TRUE)) {
			log_file << "IN car or dead:"<< peds[i] << "\n";
			continue;
		}
		// ignore the player
		else if (PED::IS_PED_A_PLAYER(peds[i])) {
			log_file << "It is the player:" << peds[i] << "\n";
			continue;
		}
		// ignore the ped is not in the screen
		else if (!ENTITY::IS_ENTITY_ON_SCREEN(peds[i])) {
			log_file << "It is not in the screen:" << peds[i] << "\n";
			continue;
		}
		// ignore the non-human
		else if (!PED::IS_PED_HUMAN(peds[i])) {
			log_file << "It is not a human:" << peds[i] << "\n";
			continue;
		}
		// ignore the invisible ped
		else if (!ENTITY::IS_ENTITY_VISIBLE(peds[i])) {
			log_file << "It is not a human:" << peds[i] << "\n";
			continue;
		}

		Vector3 ped_coords = ENTITY::GET_ENTITY_COORDS(peds[i],TRUE);
		char log_message[200];
		sprintf_s(log_message, "In updating (cb:?) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], ped_coords.x, ped_coords.y, ped_coords.z);
		log_info(log_message);
		// cam_coords is set by setCameraFixed(), if not moving
		float ped2cam_distance = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(
			cam_coords.x, cam_coords.y, cam_coords.z,
			ped_coords.x, ped_coords.y, ped_coords.z, 1
			);
		
		// if the ped2cam distance is smaller than the max value
		if (ped2cam_distance < MAX_PED_TO_CAM_DISTANCE) {

			// for each pedestrians scan all the joint_ID we choose on the subset
			// TODO: get more joit_ID
			for (int n = -1; n < number_of_joints; n++) 
			{
				Vector3 joint_coords;
				if (n == -1) {
					// head_top 是计算出来的，所有才会有-1 这个index， -1 对应的就是head_top
					Vector3 head_coords = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(peds[i],PED::GET_PED_BONE_INDEX(peds[i], joint_int_codes[0]));
					Vector3 neck_coords = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(peds[i],PED::GET_PED_BONE_INDEX(peds[i], joint_int_codes[1]));
					float head_neck_norm = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(neck_coords.x, neck_coords.y, neck_coords.z, head_coords.x, head_coords.y, head_coords.z, 1);
					
					// the origin of the xyz is ?
					float dx = abs((head_coords.x - neck_coords.x)) / head_neck_norm;
					float dy = abs((head_coords.y - neck_coords.y)) / head_neck_norm;
					float dz = abs((head_coords.z - neck_coords.z)) / head_neck_norm;

					joint_coords.x = head_coords.x + head_neck_norm * dx;
					joint_coords.y = head_coords.y + head_neck_norm * dy;
					joint_coords.z = head_coords.z + head_neck_norm * dz;
				}
				else {
					joint_coords = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(peds[i], PED::GET_PED_BONE_INDEX(peds[i], joint_int_codes[n]));
				}

				// find the vector (dx,dy,dz) pointing the point to the camera
				float joint2cam_distance = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(
					joint_coords.x, joint_coords.y, joint_coords.z,
					cam_coords.x, cam_coords.y, cam_coords.z, 1
				);
				float dx = abs((cam_coords.x - joint_coords.x)) / joint2cam_distance;
				float dy = abs((cam_coords.y - joint_coords.y)) / joint2cam_distance;
				float dz = abs((cam_coords.z - joint_coords.z)) / joint2cam_distance;
				// ray #1: from joint to cam_coords (ignoring the pedestrian to whom the joint belongs and intersecting only pedestrian (8))
				// ==> useful for detecting occlusions of pedestrian
				Vector3 end_coords1, surface_norm1;
				BOOL occlusion_ped;
				Entity entityHit1 = 0;

				int ray_ped_occlusion = WORLDPROBE::_CAST_RAY_POINT_TO_POINT(
					joint_coords.x, joint_coords.y, joint_coords.z,
					cam_coords.x, cam_coords.y, cam_coords.z,
					8, peds[i], 7
				);
				WORLDPROBE::_GET_RAYCAST_RESULT(ray_ped_occlusion, &occlusion_ped, &end_coords1, &surface_norm1, &entityHit1);

				if (entityHit1 == ped_with_cam)
					occlusion_ped = FALSE;


				// ray #2: from joint to camera (without ignoring the pedestrian to whom the joint belongs and intersecting only pedestrian (8))
				// ==> useful for detecting self-occlusions
				Vector3 endCoords2, surfaceNormal2;
				BOOL occlusion_self;
				Entity entityHit2 = 0;
				int ray_joint2cam = WORLDPROBE::_CAST_RAY_POINT_TO_POINT(
					joint_coords.x + 0.1f*dx, joint_coords.y + 0.1f*dy, joint_coords.z + 0.1f*dz,
					cam_coords.x, cam_coords.y, cam_coords.z,
					8, 0, 7
				);
				WORLDPROBE::_GET_RAYCAST_RESULT(ray_joint2cam, &occlusion_self, &endCoords2, &surfaceNormal2, &entityHit2);

				if (entityHit2 == ped_with_cam)
					occlusion_self = FALSE;


				// ray #3: from camera to joint (ignoring the pedestrian to whom the joint belongs and intersecting everything but peds (4 and 8))
				// ==> useful for detecting occlusions with objects
				Vector3 endCoords3, surfaceNormal3;
				BOOL occlusion_object;
				Entity entityHit3 = 0;
				int ray_joint2cam_obj = WORLDPROBE::_CAST_RAY_POINT_TO_POINT(
					cam_coords.x, cam_coords.y, cam_coords.z,
					joint_coords.x, joint_coords.y, joint_coords.z,
					(~0 ^ (8 | 4)), peds[i], 7
				);
				WORLDPROBE::_GET_RAYCAST_RESULT(ray_joint2cam_obj, &occlusion_object, &endCoords3, &surfaceNormal3, &entityHit3);


				BOOL occluded = occlusion_ped || occlusion_object;
				/*
				Not use the Display flag
				*/
				float x, y;
				get_2D_from_3D(joint_coords, &x, &y);
				x = x * SCREEN_WIDTH;
				y = y * SCREEN_HEIGHT;
				coords_file << nsample;					  // frame number
				coords_file << "," << peds[i];			  // pedestrian ID
				coords_file << "," << n + 1;				  // joint type
				coords_file << "," << x;				  // camera 2D x [px]
				coords_file << "," << y;	              // camera 2D y [px]
				coords_file << "," << joint_coords.x;	  // joint 3D x [m]
				coords_file << "," << joint_coords.y;	  // joint 3D y [m]
				coords_file << "," << joint_coords.z;	  // joint 3D z [m]
				coords_file << "," << occluded;			  // is joint occluded?
				coords_file << "," << occlusion_self;	  // is joint self-occluded?
				coords_file << "," << cam_coords.x;		  // camera 3D x [m]
				coords_file << "," << cam_coords.y;	      // camera 3D y [m]
				coords_file << "," << cam_coords.z;	      // camera 3D z [m]
				coords_file << "," << cam_rot.x;		  // camera 3D rotation x [degrees]
				coords_file << "," << cam_rot.y;	      // camera 3D rotation y [degrees]
				coords_file << "," << cam_rot.z;	      // camera 3D rotation z [degrees]
				coords_file << "," << fov;				  // camera FOV  [degrees]
				coords_file << "\n";
			}

		}

	}
	save_frame();
	nsample++;
	if (nsample == max_samples) {
		/*for (int i = 0; i < nwPeds; i++) {
			PED::DELETE_PED(&wPeds[i].ped);
		}
		for (int i = 0; i < nwPeds_scenario; i++) {
			PED::DELETE_PED(&wPeds_scenario[i].ped);
		}*/
		for (int i = 0; i < nPeds; i++) {
			PED::DELETE_PED(&new_ped_spawned[i].ped);
		}
	}

	return nsample;
}

CrowdAnnotator::~CrowdAnnotator() {

	ReleaseDC(hWnd, hWindowDC);
	DeleteDC(hCaptureDC);
	DeleteObject(hCaptureBitmap);
	coords_file.close();
	log_file.close();
	action_file.close();
}

void CrowdAnnotator::addPeds(Ped p) {
	new_ped_spawned[nPeds].ped = p;
	nPeds++;
}