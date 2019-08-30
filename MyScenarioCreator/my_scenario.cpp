#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_TR2_SYS_NAMESPACE_DEPRECATION_WARNING
#define DEMO FALSE

#include "my_scenario.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <direct.h>

namespace fs = std::experimental::filesystem;

using namespace std::tr2::sys;

int log_lenght = 150;   // should be the length of each log?
char *log_string = (char*)malloc(log_lenght * sizeof(char)); // allocate the memory of each log string

FILE *f;
const char *files_path = "..\\Multi-Crowd-Action\\";
char file_name[20] = "None";
int nFiles = 0, currentFile = 1; 


std::string statusText;
DWORD statusTextDrawTicksMax;
bool statusTextGxtEntry; // should indicate the wheather there is a message

bool firstTime = true;
bool menuActive = false;
bool fly = false;

// menu command
bool bUp = false,
bDown = false,
bLeft = false,
bRight = false,
bEnter = false,
bBack = false,
bQuit = false;
bool stopCoords = false; // should indicate whether lock the coordinates, if false not, if true yes.

Vector3 playerCoords; // the coordinate of the player
Vector3 fixCoords; // should be the camera coordinate 
Vector3 goFrom, goTo; //should be the route of the wander
Vector3 A, B, C; // should be the loacation of moving cameras
Vector3 TP1, TP2, TP1_rot, TP2_rot; // 为了将任务送到某一个地方，然后再其他地方生成环境，之后再将其传回来。一般感觉设置成场景的坐标就可以了

int weather_code = 2;
int record_hour = 9;  // the time of we set, the default value is 9
int record_minute = 30; // the time of we set, the default value is 30
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

// Camera varibales
Cam activeCam;
Vector3 camCoords, camRot;
float camFov;
bool camLock = true;
bool camMoving = false;

// Preserve varibles
Vector3 perserve_cam_coords, perserve_cam_rot;
float perserve_cam_fov;
bool perserve_cam_lock;
bool perserve_cam_moving;
int perserve_weather;
int perserve_hour;
int perserve_minute;

//menu parameters, control the shape of the menu
float mHeight = 9, mTopFlat = 60, mTop = 36, mLeft = 3, mTitle = 5;
int activeLineIndexMain = 0; // indicate which in the main menu is active
bool visible = false;

//menu peds parameters
int activeLineIndexPeds = 0; // don't know
int nPeds = 1;  // should be the number of peds
int currentBehaviour = 0; // decide the behviour of the peds
bool group = false;
int actiontype = 0; // indiacte which action the person is included. It includes the meta-action and the complex action
int actionsn = 0; // indicate the sequence number of the action in this video

const int nMaxPeds = 100; //it is the number of how many peds we produce each time
const int numberTaks = 9; // is the number of the action of each person
const int numberActionTypes = 20; // is the number of the action type of each crowd, including the meta and complex actions
const int numberCrowd = 20;  // is the max number of crowd in one video, each crowd with a action is counted one.

const int number_walking_peds = 1024;
const int number_other_peds = 1024;

LPCSTR tasks[numberTaks] = {
	"SCENARIO",  // 0
	"STAND",     // 1
	"PHONE",     // 2
	"COWER",     // 3
	"WANDER",    // 4
	"CHAT",      // 5
	"COMBAT",    // 6
	"COVER",     // 7
	"MOVE"       // 8
};

bool subMenuActive = false;

//behaviour params
int maxNumber = 1000000; // ? what's meaning of this parameters.....the max time of each action
const int nScenario = 14; // scenario's number, 
float speed = 1;

struct {
	LPCSTR text;
	int param;
} paramsLines[10] = {
	{"Task Time", 1000000}, //0
	{"Type", 0},            //1
	{"Radius", 20},         //2
	{"Minimal Length", 1},  //3
	{"Time between Walks", 1}, //4
	{"Spawning Radius", 1}     //5
};

static char scenarioTypes[14][40]{
	"NEAREST",                          //0
	"RANDOM",                           //1
	"WORLD_HUMAN_MUSICIAN",             //2
	"WORLD_HUMAN_SMOKING",              //3
	"WORLD_HUMAN_BINOCULARS",           //4 望远镜
	"WORLD_HUMAN_CHEERING",             //5
	"WORLD_HUMAN_DRINKING",             //6
	"WORLD_HUMAN_PARTYING",             //7
	"WORLD_HUMAN_PICNIC",               //8
	"WORLD_HUMAN_STUPOR",               //9 恍惚
	"WORLD_HUMAN_PUSH_UPS",             //10
	"WORLD_HUMAN_LEANING",              //11
	"WORLD_HUMAN_MUSCLE_FLEX",          //12
	"WORLD_HUMAN_YOGA"                  //13
};



wPed wPeds[number_walking_peds];  // define an array of the walking pedestrains
int nwPeds = 0;   // number of walking pedestrain

//wPed present_peds[1024]; // record the present peds
//int nPresent_peds = 0; // indicate the present peds


// active parameters, to indicate the selected ones
int activeLineIndexCamera = 0;
int activeLineIndexPlace = 0;
int activeLineIndexTime = 0;
int activeLineIndexWeather = 0;
int activeLineIndexSubmenu = 0;
int activeLineIndexFile = 0;

bool wind = false;

void dummy_wait(BOOL demo) {
	if (demo) {
		WAIT(50);
	}
	else {
		WAIT(2000);
	}
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
	log_file.open("log_info_creator.txt", std::ios::app);
	log_file << time_string << std::endl;
}

void update_status_text() {
	/*
	this method is used to update the text showing in the game
	*/
	if (GetTickCount() < statusTextDrawTicksMax) {
		/*
		Set some parameters of showing the text
		*/
		UI::SET_TEXT_FONT(0);
		UI::SET_TEXT_SCALE(0.55f, 0.55f);
		UI::SET_TEXT_COLOUR(255, 255, 255, 255);
		UI::SET_TEXT_WRAP(0.0, 1.0); // It sets the text in a specified box and wraps the text if it exceeds the boundries.
		UI::SET_TEXT_CENTRE(1);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
		if (statusTextGxtEntry) {
			UI::_SET_TEXT_ENTRY((char*)statusText.c_str()); // can't find in NativeDB, find UI::_SET_NOTIFICATION_TEXT_ENTRY()
		}
		else {
			UI::_SET_TEXT_ENTRY((char*)"STRING");
			UI::_ADD_TEXT_COMPONENT_STRING((char *)statusText.c_str()); // should update the statusText
		}
		UI::_DRAW_TEXT(0.5f, 0.1f);
	}
}
// ? Gxt = Game Text entry
void set_status_text(std::string str, DWORD time = 2000, bool isGxtEntry = false) {
	/*
	this method is to set the text, 类似于正常程序中的print函数
	*/
	statusText = str;
	statusTextDrawTicksMax = GetTickCount() + time; // set the time how long the text is showing
	statusTextGxtEntry = isGxtEntry;
}

void firstOpen() {
	/*
	Setting the paramters when we first open the menu
	*/
	Player mainPlayer = PLAYER::PLAYER_ID();  // obtain the YOU in the story mode, alawys 0
	//protect the main player
	PLAYER::SET_PLAYER_INVINCIBLE(mainPlayer, TRUE);
	PLAYER::SET_POLICE_IGNORE_PLAYER(mainPlayer, TRUE);
	PLAYER::SET_EVERYONE_IGNORE_PLAYER(mainPlayer, TRUE);

	PLAYER::SPECIAL_ABILITY_FILL_METER(mainPlayer, 1);
	PLAYER::SET_PLAYER_NOISE_MULTIPLIER(mainPlayer, 0.0); // slience the main player
	PLAYER::SET_SWIM_MULTIPLIER_FOR_PLAYER(mainPlayer, 1.49f);
	PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(mainPlayer, 1.49f);
	PLAYER::DISABLE_PLAYER_FIRING(mainPlayer, TRUE); // should 防止着火
	PLAYER::SET_DISABLE_AMBIENT_MELEE_MOVE(mainPlayer, TRUE);// should 防止周围人乱走

	goFrom = playerCoords;
	goTo = playerCoords;

	A = playerCoords;
	B = playerCoords;
	C = playerCoords;

	/*TP1 = playerCoords;
	TP2 = playerCoords;*/
	// setting the start and end point
	TP1.x = 423.777;
	TP1.y = 5614.188;
	TP1.z = 766.722;
	TP1_rot.x = -15.0;
	TP1_rot.y = 0.1;
	TP1_rot.z = 166.6;

	TP2.x = 12.1;
	TP2.y = 551.7;
	TP2.z = 176.9;
	TP2_rot.x = 6.2;
	TP2_rot.y = 0.1;
	TP2_rot.z = -19.4;

	goTo.x += 2; goTo.y += 2;

	srand((unsigned int)time(NULL));

	strcpy(log_string, ""); // set the log message
	_mkdir(files_path); // create the directory of the log files
	firstTime = false;
}

void draw_rect(float x, float y, float width, float height, int r, int g, int b, int a) {
	/*
	x:The relative X point of the center of the rectangle. 
		(0.0-1.0, 0.0 is the left edge of the screen, 1.0 is the right edge of the screen)
	y:The relative Y point of the center of the rectangle. 
		(0.0-1.0, 0.0 is the top edge of the screen, 1.0 is the bottom edge of the screen)
	width: The relative width of the rectangle. (0.0-1.0, 1.0 means the whole screen width)
	height: The relative height of the rectangle. (0.0-1.0, 1.0 means the whole screen width)
	r: red part of the color (0-255)
	g: green part of the color (0-255)
	b: blue part of the color (0-255)
	a: Alpha part of the color. (0-255, 0 means totally transparent, 255 means totally opaque)

	The total number of rectangles to be drawn in one frame is apparently limited to 399.
	*/

	GRAPHICS::DRAW_RECT((x + (width * 0.5f)), (y + (height * 0.5f)),width,height,r,g,b,a);
}

void draw_menu_line(std::string caption, float lineWidth, float lineHeight, float lineTop, float lineLeft, float textLeft, bool active, bool title, bool rescaleText = true) {

	// default values
	int text_col[4] = { 255, 255, 255, 255 },
		rect_col[4] = { 0, 0, 0, 190 };
	float text_scale = 0.35f;
	int font = 0;

	// correcting values for active line
	if (active) {

		// make the active words have a higher brightness
		text_col[0] = 0;
		text_col[1] = 0;
		text_col[2] = 0;

		// make the active recetangle have a different color and higher brightness
		rect_col[0] = 0;
		rect_col[1] = 180;
		rect_col[2] = 205;
		rect_col[3] = 220;

	}

	if (title)
	{
		rect_col[0] = 0;
		rect_col[1] = 0;
		rect_col[2] = 0;

		if (rescaleText) text_scale = 0.70f;
		font = 1;
	}

	int screen_w, screen_h;
	GRAPHICS::GET_SCREEN_RESOLUTION(&screen_w, &screen_h); // get the resolution of the screen

	textLeft += lineLeft;

	float lineWidthScaled = lineWidth / (float)screen_w; // line width
	float lineTopScaled = lineTop / (float)screen_h; // line top offset
	float textLeftScaled = textLeft / (float)screen_w; // text left offset
	float lineHeightScaled = lineHeight / (float)screen_h; // line height

	float lineLeftScaled = lineLeft / (float)screen_w;

	// this is how it's done in original scripts

	// text upper part
	UI::SET_TEXT_FONT(font);
	UI::SET_TEXT_SCALE(0.0, text_scale);
	UI::SET_TEXT_COLOUR(text_col[0], text_col[1], text_col[2], text_col[3]);
	UI::SET_TEXT_CENTRE(0);
	UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
	UI::SET_TEXT_EDGE(0, 0, 0, 0, 0);
	UI::_SET_TEXT_ENTRY((char*)"STRING");
	UI::_ADD_TEXT_COMPONENT_STRING((LPSTR)caption.c_str());
	UI::_DRAW_TEXT(textLeftScaled, (((lineTopScaled + 0.00278f) + lineHeightScaled) - 0.005f));

	// text lower part
	UI::SET_TEXT_FONT(font);
	UI::SET_TEXT_SCALE(0.0, text_scale);
	UI::SET_TEXT_COLOUR(text_col[0], text_col[1], text_col[2], text_col[3]);
	UI::SET_TEXT_CENTRE(0);
	UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
	UI::SET_TEXT_EDGE(0, 0, 0, 0, 0);
	UI::_SET_TEXT_GXT_ENTRY((char*)"STRING");
	UI::_ADD_TEXT_COMPONENT_STRING((LPSTR)caption.c_str());
	int num25 = UI::_0x9040DFB09BE75706(textLeftScaled, (((lineTopScaled + 0.00278f) + lineHeightScaled) - 0.005f));

	// rect
	draw_rect(lineLeftScaled, lineTopScaled + (0.00278f),
		lineWidthScaled, ((((float)(num25)* UI::_0xDB88A37483346780(text_scale, 0)) + (lineHeightScaled * 2.0f)) + 0.005f),
		rect_col[0], rect_col[1], rect_col[2], rect_col[3]);
}

Cam lockCam(Vector3 pos, Vector3 rot) {
	/*
	The method is used to lock a camera
	pos：the location of the camera
	rot: the rotation of the camere
	message: the name of the camera
	*/
	CAM::DESTROY_ALL_CAMS(true); //destore all the camera, in order to make the new locked camera
	Cam lockedCam = CAM::CREATE_CAM_WITH_PARAMS((char*)"DEFAULT_SCRIPTED_CAMERA", pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, 50, true, 2);
	CAM::SET_CAM_ACTIVE(lockedCam, true);
	CAM::RENDER_SCRIPT_CAMS(1, 0, 3000, 1, 0); // p3 & p4 should be 0
	return lockedCam;
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

void saveFile() {
	std::string fname = "";
	int stop = 0;
	if (stopCoords)
		stop = 1;

	fname = fname + std::string(files_path) + std::string(file_name);
	f = fopen(fname.c_str(), "w");

	if (camMoving) {
		fprintf_s(f, "%d %f %f %f %d %f %f %f %f %f %f %d %d %d\n", (int)camMoving, A.x, A.y, A.z, stop, B.x, B.y, B.z, C.x, C.y, C.z, weather_code, record_hour, record_minute);
	}
	else {
		fprintf_s(f, "%d %f %f %f %d %f %f %f %f %d %d %d\n", (int)camMoving, camCoords.x, camCoords.y, camCoords.z, stop, camRot.x, camRot.y, camRot.z, camFov,weather_code, record_hour, record_minute);
	}

	fprintf_s(f, "%f %f %f %f %f %f\n", TP1.x, TP1.y, TP1.z, TP1_rot.x, TP1_rot.y, TP1_rot.z);
	fprintf_s(f, "%f %f %f %f %f %f\n", TP2.x, TP2.y, TP2.z, TP2_rot.x, TP2_rot.y, TP2_rot.z);

	fprintf_s(f, "%s", log_string);
	fclose(f);

	fname = fname + "  SAVED!";
	set_status_text(fname);
}

int readLine(FILE *f, Vector3 *pos) {
	/*
	read peds' information in the log file
	*/
	int ngroup = 0;
	int result = fscanf_s(f, "%d %f %f %f %d %d %f %f %f %f %f %f %f %d %d %d %d %d %d %d %d\n", &nPeds, &(*pos).x, &(*pos).y, &(*pos).z, &ngroup, &currentBehaviour, &speed, &goFrom.x, &goFrom.y, &goFrom.z, &goTo.x, &goTo.y, &goTo.z, &paramsLines[0].param, &paramsLines[1].param, &paramsLines[2].param, &paramsLines[3].param, &paramsLines[4].param, &paramsLines[5].param, &actiontype, &actionsn);
	if (ngroup == 0) {
		group = false;
	}
	if (ngroup == 1) {
		group = 1;
	}
	char message[5];
	sprintf_s(message, "%d", result);
	set_status_text(message);
	return result;

}

bool file_exist(std::string file_name) {
	/*
	judge wheather the file exists
	*/
	std::ifstream infile(file_name);
	return infile.good();
}

void readDirr() {
	/*
	Get the number of files in the directory
	files_path is constant, 'Multi-Crowd-Action'
	*/

	int i = 0;
	std::string s;
	char file_name_t[20];
	do {
		i++;
		sprintf_s(file_name_t, "log_%03d.txt", i);
		//s = std::string(files_path) + "log_" + std::to_string(i) + ".txt";
		s = std::string(files_path) + std::string(file_name_t);

	} while (file_exist(s));
	nFiles = i - 1; // because we will add the i first and then ti judge wheather it exists
}

void addwPed(Ped p, Vector3 from, Vector3 to, int stop, float speed) {
	/*
	add the walking pedestrain, and increase the count of walking pedestrain
	*/
	/*
	if (nwPeds > 1000) {
		// if the number of waking pedestrain, we will stop making the walking pedestrain
		return;
	}
	*/

	wPeds[nwPeds].ped = p;
	wPeds[nwPeds].from = from;
	wPeds[nwPeds].to = to;
	wPeds[nwPeds].stopTime = stop;
	wPeds[nwPeds].speed = speed;

	nwPeds++;
}

Vector3 coordsToVector(float x, float y, float z) 
{
	Vector3 v;
	v.x = x;
	v.y = y;
	v.z = z;

	return v;

}

void writeLogLine(float x, float y, float z) {

	//line format: "nPeds X Y Z group currentBehaviour speed goFrom[3] goTo[3] paramLines.param[5] \n"
	char line[200];
	int size;
	int ngroup = 0;
	if (group) ngroup = 1;
	sprintf_s(line, "%d %f %f %f %d %d %f %f %f %f %f %f %f %d %d %d %d %d %d %d %d\n", nPeds, x, y, z, ngroup, currentBehaviour, speed, goFrom.x, goFrom.y, goFrom.z, goTo.x, goTo.y, goTo.z, paramsLines[0].param, paramsLines[1].param, paramsLines[2].param, paramsLines[3].param, paramsLines[4].param, paramsLines[5].param, actiontype, actionsn);

	size = (int)(strlen(line) + strlen(log_string));

	if (size >= log_lenght)
		log_string = (char*)realloc(log_string, (150 + size) * sizeof(char));

	strcat(log_string, line);  // add the new line of log 
}

MyScenarioCreator::MyScenarioCreator() {
	PLAYER::SET_EVERYONE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);
	PLAYER::SET_POLICE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);
	PLAYER::CLEAR_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_PED_ID());

	ENTITY::SET_ENTITY_COLLISION(PLAYER::PLAYER_PED_ID(), TRUE, TRUE);
	ENTITY::SET_ENTITY_VISIBLE(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
	ENTITY::SET_ENTITY_ALPHA(PLAYER::PLAYER_PED_ID(), 255, FALSE);
	ENTITY::SET_ENTITY_CAN_BE_DAMAGED(PLAYER::PLAYER_PED_ID(), FALSE);

	GAMEPLAY::SET_TIME_SCALE(1.0);
}

void MyScenarioCreator::draw_text(char *text, float x, float y, float scale)
{
	UI::SET_TEXT_FONT(0);
	UI::SET_TEXT_SCALE(scale, scale);
	UI::SET_TEXT_COLOUR(255, 255, 255, 245);
	UI::SET_TEXT_WRAP(0.0, 1.0);
	UI::SET_TEXT_CENTRE(0);
	UI::SET_TEXT_DROPSHADOW(2, 2, 0, 0, 0);
	UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
	UI::_SET_TEXT_ENTRY((char*)"Attention:");
	UI::_ADD_TEXT_COMPONENT_STRING(text);
	UI::_DRAW_TEXT(y, x);
}

void MyScenarioCreator::update() {

	PLAYER::SET_POLICE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);
	PLAYER::SET_EVERYONE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);

	listen_for_keystrokes();

	cameraCoords(); // Can get the location of palyer, and give the value to playercoords

	stopControl();

	walking_peds();

	update_status_text();
}

void MyScenarioCreator::stopControl() {
	if (stopCoords) {
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(PLAYER::PLAYER_PED_ID(), fixCoords.x, fixCoords.y, fixCoords.z, 0, 0, 0); // should make the camera to the player's
	}
}

void MyScenarioCreator::listen_for_keystrokes() {
	/*
	Define the different action based on the keys
	*/

	//cancel last inseriment
	/*
	IsKeyJustUp() is used to judege which key is pushed
	*/
	if (IsKeyJustUp(VK_F9)) {
		cancelLastLog(); // cancel method, is used to delete the log of last insertment
		set_status_text("Last Log: Cancelled!");
	}

	// clear the log string
	if (IsKeyJustUp(VK_F11)) {
		log_string[0] = '\0';
	}

	// show MainMenu, the truely function to stat creating the scenario
	if (IsKeyJustUp(VK_F5)) {
		Player mainPlayer = PLAYER::PLAYER_ID();
		if (firstTime) {
			firstOpen(); // perapare for the making process
		}
		PLAYER::CLEAR_PLAYER_WANTED_LEVEL(mainPlayer);

		// becasue the menuActive is False in default
		if (!menuActive) {
			main_menu();
		}
		else {
			bQuit = true;
		}
	}

	if (menuActive) {
		if (IsKeyJustUp(VK_NUMPAD5))							bEnter = true;
		if (IsKeyJustUp(VK_NUMPAD0) || IsKeyJustUp(VK_BACK))	bBack = true;
		if (IsKeyJustUp(VK_NUMPAD8))							bUp = true;
		if (IsKeyJustUp(VK_NUMPAD2))							bDown = true;
		if (IsKeyJustUp(VK_NUMPAD6))							bRight = true;
		if (IsKeyJustUp(VK_NUMPAD4))							bLeft = true;
	}

	//fly
	if (fly) {
		if (IsKeyJustUp(0x45)) {
			//button E to go up
			ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(PLAYER::PLAYER_PED_ID(), 1, 0, 0, 2, true, true, true, true);
		}
		if (IsKeyJustUp(0x51)) {
			//button Q to go up
			ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(PLAYER::PLAYER_PED_ID(), 1, 0, 0, -2, true, true, true, true);
		}
	}

	if (IsKeyJustUp(VK_SPACE)) //button SPACE to stop 
	{
		if (fly) {
			stopCoords = !stopCoords;
			char message[] = "UNLOCK";
			if (stopCoords)
				strcpy(message, "LOCK");
			fixCoords = playerCoords;
			ENTITY::SET_ENTITY_VELOCITY(PLAYER::PLAYER_PED_ID(), 0, 0, 0); // set the v of the object to 0
			set_status_text(message);
		}
		else if (stopCoords) {		//unlock if not flying
			stopCoords = !stopCoords;
			char message[] = "UNLOCK";
			set_status_text(message);
		}
	}

}

void MyScenarioCreator::cameraCoords() {
	/*
	Get the information about the camera
	*/

	char text[100];

	if (camLock) {
		// if the camera is locked, we get the lock camera's coords in other words, the gameplayer's coords
		camCoords = CAM::GET_GAMEPLAY_CAM_COORD();
		camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
		camFov = CAM::GET_GAMEPLAY_CAM_FOV();
	}
	else {
		// if the camera is not locked, we get the active camera's coords
		camCoords = CAM::GET_CAM_COORD(activeCam);
		camRot = CAM::GET_CAM_ROT(activeCam, 2);
		camFov = CAM::GET_CAM_FOV(activeCam);
	}

	Vector3 v = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true); // get the coords of the ped player
	//sprintf_s(text, "player_coords: (%.3f, %.3f, %.3f)", v.x, v.y, v.z); // write the player's coords into the string
	//log_info((char*)text);
	playerCoords = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true);
	
	//text will be the last value of entering in it, so the value will be th cam rot, so how does this function get all the infotrmation of
	// all the camera.
	v = camCoords;
	//sprintf_s(text, "cam_coords: (%.3f, %.3f, %.3f)", v.x, v.y, v.z);
	//log_info((char*)text);

	v = camRot;
	//sprintf_s(text, "cam_rot: (%.3f, %.3f, %.3f)", v.x, v.y, v.z);
	//log_info((char*)text);

	//sprintf_s(text, "cam_fov: (%.3f)", camFov);
	//log_info((char*)text);
}

void MyScenarioCreator::resetMenuCommands()
{
	// set all the actions to False
	bEnter = false;
	bBack = false;
	bUp = false;
	bLeft = false;
	bRight = false;
	bDown = false;
	bQuit = false;
}

void MyScenarioCreator::cancelLastLog() {
	// Make all the char in one line is '\0'
	// I'll improve the function to another alghrothim

	int nchar = (int)strlen(log_string);
	int n = 0;
	for (int i = nchar; i >= 0; i--) {
		if (log_string[i] == '\n') {
			n++;
		}
		if (n == 2) {
			break;
		}
		log_string[i] = '\0';
	}
}

void MyScenarioCreator::main_menu() {
	/*
	Function to construct the main menu
	*/
	
	const float lineWidth = 250.0;
	const int lineCount = 10;
	menuActive = true;

	std::string caption = "Crowd Action Simulator"; // the titile of the menu

	static LPCSTR lineCaption[lineCount] = {
		"PEDS",           // control the pedestrain: 0
		"CAMERA",         // control the camera:     1
		"PLACE",          // control the place:      2
		"TIME",           // control the time:       3
		"WEATHER",        // control the weather:    4
		"INVISIBLE",      // make the main player invisible:  5
		"FLY",            // make the main player fly:        6
		"FILE",           // save & load the file:            7
		"Clear",           // clear the peds which we have add
		"0827_2157"
	};

	DWORD waitTime = 150; 
	while (true) {
		DWORD maxTickCount = GetTickCount() + waitTime; // base the current time to set the actual wait time

		if (visible)
			lineCaption[5] = "INVISIBLE		~g~ON"; // ~g~ means the color of the words  is green, this unqiue feature of this program
		else
			lineCaption[5] = "INVISIBLE		~r~OFF"; // ~r~ means the color of the words is red

		if (fly)
			lineCaption[6] = "FLY		~g~ON";
		else
			lineCaption[6] = "FLY		~r~OFF";

		do {
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++) {
				if (i != activeLineIndexMain) {
					draw_menu_line(lineCaption[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
				}
			}
			draw_menu_line(lineCaption[activeLineIndexMain], lineWidth, mHeight, mTopFlat + activeLineIndexMain * mTop, mLeft, mHeight, true, false);

			update(); // update the status
			WAIT(0);
		} while (GetTickCount() < maxTickCount);

		waitTime = 0; // initalize the wait time

		update();

		//process buttons;
		if (bEnter) {
			resetMenuCommands();
			switch (activeLineIndexMain) {
			case 0:
				// process ped menu
				peds_menu();
				break;
			case 1:
				// process camera menu
				camera_menu();
				break;
			case 2:
				// process place menu, will improve it's function
				place_menu();
				break;
			case 3:
				// process time menu, will impove it's function
				// add the function to freeze the time
				time_menu();
				break;
			case 4:
				// process weather menu;
				weather_menu();
				break;
			case 5:
				// change visibility
				visible = !visible;
				if (visible)
					ENTITY::SET_ENTITY_VISIBLE(PLAYER::PLAYER_PED_ID(), FALSE, true);
				else
					ENTITY::SET_ENTITY_VISIBLE(PLAYER::PLAYER_PED_ID(), TRUE, true);
				break;
			case 7:
				file_menu();
				break;
			case 8:
				clear_method();
				break;
			case 6:
				// change the fly status
				// 有entity的一定要放到最后，PLAYer会跳过case
				fly = !fly;
				Entity me = PLAYER::PLAYER_PED_ID();
				if (fly) {
					ENTITY::SET_ENTITY_HAS_GRAVITY(me, false);
					ENTITY::SET_ENTITY_COLLISION(me, FALSE, FALSE);
				}
				else {
					ENTITY::SET_ENTITY_HAS_GRAVITY(me, true);
					ENTITY::SET_ENTITY_COLLISION(me, TRUE, TRUE);
				}
				break;
			
			}
			waitTime = 200;
		}

		if (bBack || bQuit) {
			menuActive = false;
			resetMenuCommands();
			break;
		}
		else if (bUp) {
			// up means the (line count-1)
			if (activeLineIndexMain == 0)
				activeLineIndexMain = lineCount;
			activeLineIndexMain--;
			waitTime = 150;
		}
		else if (bDown) {
			// down means the (line count+1)
			activeLineIndexMain++;
			if (activeLineIndexMain == lineCount) {
				activeLineIndexMain = 0;
			}
			waitTime = 150;
		}

		resetMenuCommands();
	}

}

void MyScenarioCreator::peds_menu() {
	/*
	the pedestrain menu
	Most import menu!!!!!
	*/

	const float lineWidth = 250.0;
	const int lineCount = 6; // means that we have 5 sub-menu in this menu
	menuActive = true;

	std::string caption = "Pedestrain";

	char lines[lineCount][50] = {
		"Number",  // 0
		"Behaviour", //1
		"Group",    //2
		"Action",   //3
		"ActionSN",  //4
		"Spawn Peds" //5
	};

	DWORD waitTime = 150;

	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;
		sprintf_s(lines[0], "NUMBER			 ~y~[ %d ]", nPeds);
		sprintf_s(lines[1], "BEHAVIOUR		~y~[ %s ]", tasks[currentBehaviour]);
		sprintf_s(lines[3], "Action        ~y~[ %d ]", actiontype);
		sprintf_s(lines[4], "Action No.    ~y~[ %d ]", actionsn);
		if (group)
			sprintf_s(lines[2], "GROUP			~y~[ ON ]");
		else
			sprintf_s(lines[2], "GROUP			~y~[ OFF ]");

		do {
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++) {
				if (i != activeLineIndexPeds) {
					draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
				}
			}
			draw_menu_line(lines[activeLineIndexPeds], lineWidth, mHeight, mTopFlat + activeLineIndexPeds * mTop, mLeft, mHeight, true, false);

			if (!subMenuActive)
				update(); // major to listen for the keyboards
			else
				return;
			WAIT(0);

		} while (GetTickCount() < maxTickCount);
		waitTime = 0; // initalize the wait time

		update();
		if (bEnter) {
			switch (activeLineIndexPeds)
			{
			case 0:
				if (nPeds == nMaxPeds) {
					nPeds = 0;
				}
				else {
					nPeds++;
				}
				break;
			case 1:
				resetMenuCommands(); // Before going to the next menu, we should reset all the actions of the keyboards
				tasks_sub_menu(); // the pedestrain tasks
				break;
			case 2:
				group = !group;
				break;
			case 5:
				if (currentBehaviour == 8) {
					spawn_peds_o(goFrom, nPeds);
				}
				else {
					spawn_peds_o(playerCoords, nPeds);
				}
				break;
			
			}
			waitTime = 200;
		}
		if (bBack) {
			resetMenuCommands();
			break;
		}
		else if (bQuit) {
			resetMenuCommands();
			bQuit = true;
			break;
		}
		else if (bUp) {
			// similiar to previous
			if (activeLineIndexPeds == 0)
				activeLineIndexPeds = lineCount;
			activeLineIndexPeds--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexPeds++;
			if (activeLineIndexPeds == lineCount)
				activeLineIndexPeds = 0;
			waitTime = 150;
		}

		// Using the left and right to change the number of the pedestrain
		if (activeLineIndexPeds == 0)
		{
			if (bLeft) {
				if (nPeds == 0)
					nPeds = nMaxPeds;
				else
					nPeds--;
			}
			else if (bRight) {
				if (nPeds == nMaxPeds)
					nPeds = 0;
				else
					nPeds++;
			}
		}

		//Using the left and right to choose the behaviour of the pedestrain
		if (activeLineIndexPeds == 1)
		{
			if (bLeft) {
				if (currentBehaviour == 0)
					currentBehaviour = numberTaks - 1;
				else
					currentBehaviour--;
			}
			else if (bRight) {
				if (currentBehaviour == numberTaks - 1)
					currentBehaviour = 0;
				else
					currentBehaviour++;
			}
		}

		//Using the left and right to define the action type of the crowd
		if (activeLineIndexPeds == 3) {
			if (bLeft) {
				if (actiontype == 0) {
					actiontype = numberActionTypes - 1;
				}
				else {
					actiontype--;
				}
			}
			else if(bRight){
				if (actiontype == numberActionTypes - 1){
					actiontype = 0;
				}
				else {
					actiontype++;
				}

			}
		}
		//Using the left and right to choose the behaviour of the pedestrain
		if (activeLineIndexPeds == 4) {
			if (bLeft) {
				if (actionsn == 0) {
					actionsn = numberCrowd - 1;
				}
				else {
					actionsn--;
				}
			}
			else if (bRight) {
				if (actionsn == numberCrowd - 1) {
					actionsn = 0;
				}
				else {
					actionsn++;
				}

			}
		}
		resetMenuCommands();
	}
}

void MyScenarioCreator::tasks_sub_menu() {
	/*
	The sub meanu of the pedestrtain menu
	*/
	const float lineWidth = 320.0;
	int lineCount = 0;
	activeLineIndexSubmenu = 0; // refer to the sub menu item, such as task time, number and so on
	subMenuActive = true;

	switch (currentBehaviour) {
	case 0: // Scenario
		lineCount = 2;
		break;
	//case 1: //Stand  ? we still have the task time of this entry, we also need the number of this function
	//case 2: //Phone
	case 3: //Cower
		lineCount = 1;
		break;
	case 4: //Wander
		lineCount = 3;
		break;
	//case 5: //Chat
	//case 6: //Combat
	//case 7: //Cover
	case 8: //Move
		lineCount = 5;
		break;
	default:
		subMenuActive = false;
		return;
		break;
	}

	char lines[5][40] = {"","","","",""};
	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		switch (currentBehaviour)
		{
		case 0:
			sprintf_s(lines[0], "%s			 ~y~[ %d ]", paramsLines[0].text, paramsLines[0].param);
			sprintf_s(lines[1], "%s		~y~[ %s ]", paramsLines[1].text, scenarioTypes[paramsLines[1].param]);
			break;
		case 3:
			sprintf_s(lines[0], "%s			 ~y~[ %d ]", paramsLines[0].text, paramsLines[0].param);
			break;
		case 4:
			sprintf_s(lines[0], "%s			 ~y~[ %d ]", paramsLines[2].text, paramsLines[2].param);
			sprintf_s(lines[1], "%s			 ~y~[ %d ]", paramsLines[3].text, paramsLines[3].param);
			sprintf_s(lines[2], "%s			 ~y~[ %d ]", paramsLines[4].text, paramsLines[4].param);
			break;
		case 8:
			sprintf_s(lines[0], "From	~y~[ x=%0.1f y=%0.1f ]", goFrom.x, goFrom.y);
			sprintf_s(lines[1], "To		~y~[ x=%0.1f y=%0.1f ]", goTo.x, goTo.y);
			sprintf_s(lines[2], "Speed			 ~y~[ %0.1f ]", speed);
			sprintf_s(lines[3], "%s			 ~y~[ %d ]", paramsLines[4].text, paramsLines[4].param);
			sprintf_s(lines[4], "%s			 ~y~[ %d ]", paramsLines[5].text, paramsLines[5].param);
			break;

		//default:
		//	break;
		}

		do
		{
			// draw menu
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexSubmenu)
					draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft + 260, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexSubmenu], lineWidth, mHeight, mTopFlat + activeLineIndexSubmenu * mTop, mLeft + 260, mHeight, true, false);

			update();
			peds_menu();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		peds_menu();

		//process buttons
		if (bEnter) {
			if (currentBehaviour == 8) {
				if (activeLineIndexSubmenu == 0) {
					// if the behaviour is move, the start point of crowds is where the main player is
					goFrom = playerCoords;
				}
				if (activeLineIndexSubmenu == 1) {
					// if the behaviour is stand, the start and end point of crowd is where the main player is
					goTo = playerCoords;
				}
			}
			waitTime = 200;
		}
		else if(bBack){
			resetMenuCommands();
			subMenuActive = false;
			break;
		}
		else if (bQuit) {
			resetMenuCommands();
			subMenuActive = false;
			bQuit = true;
			break;
		}
		else if (bUp) {
			if (activeLineIndexSubmenu == 0) {
				activeLineIndexSubmenu = lineCount;
			}
			activeLineIndexSubmenu--;
			//set_status_text(std::to_string(activeLineIndexSubmenu));
			waitTime = 150;
		}
		else if (bDown) {
			activeLineIndexSubmenu++;
			if (activeLineIndexSubmenu == lineCount)
				activeLineIndexSubmenu = 0;
			waitTime = 150;
		}
		else if (bLeft) {
			switch (currentBehaviour) {
			case 0: // Scenario
				switch (activeLineIndexSubmenu)
				{
				case 0: // Number
					if (paramsLines[0].param == 0) {
						paramsLines[0].param = maxNumber;
					}
					else {
						paramsLines[0].param--;
					}
					break;
				case 1: // type
					if (paramsLines[1].param == 0) {
						paramsLines[1].param = nScenario - 1;
					}
					else {
						paramsLines[1].param--;
					}
					break;
				}
				break;
			case 3://Cower
				if (paramsLines[0].param == 0)
					paramsLines[0].param = maxNumber;
				else
					paramsLines[0].param--;
				break;
			case 4://Wander
				if (paramsLines[activeLineIndexSubmenu + 2].param == 0)
					paramsLines[activeLineIndexSubmenu + 2].param = maxNumber;
				else
					paramsLines[activeLineIndexSubmenu + 2].param--;
				break;
			case 8: // move
				if (activeLineIndexSubmenu == 2) {
					//control the speed
					if (speed < 1.01) {
						speed = 2;
					}
					else {
						speed -= 0.1f;
					}
				}

				if (activeLineIndexSubmenu == 3) {
					// control the time between the walk
					if (paramsLines[4].param == 0)
						paramsLines[4].param = maxNumber;
					else
						paramsLines[4].param--;
				}

				if (activeLineIndexSubmenu == 4) {
					if (paramsLines[5].param == 0)
						paramsLines[5].param = maxNumber;
					else
						paramsLines[5].param--;
				}
				break;
			}
		}
		else if (bRight) {
			switch (currentBehaviour)
			{
			case 0:// scenario
				switch (activeLineIndexSubmenu)
				{
				case 0:
					if (paramsLines[0].param == maxNumber)
						paramsLines[0].param = 0;
					else
						paramsLines[0].param++;
					break;
				case 1:
					if (paramsLines[1].param == nScenario - 1)
						paramsLines[1].param = 0;
					else
						paramsLines[1].param++;
					break;
				}
				break;
			case 3://cower
				if (paramsLines[0].param == maxNumber)
					paramsLines[0].param = 0;
				else
					paramsLines[0].param++;
				break;
			case 4://wander
				if (paramsLines[activeLineIndexSubmenu + 2].param == maxNumber)
					paramsLines[activeLineIndexSubmenu + 2].param = 0;
				else
					paramsLines[activeLineIndexSubmenu + 2].param++;
				break;
			case 8://move
				if (activeLineIndexSubmenu == 2)
					if (speed > 1.99)
						speed = 1;
					else
						speed += 0.1f;
				if (activeLineIndexSubmenu == 3)
					if (paramsLines[4].param == maxNumber)
						paramsLines[4].param = 0;
					else
						paramsLines[4].param++;
				if (activeLineIndexSubmenu == 4)
					if (paramsLines[5].param == maxNumber)
						paramsLines[5].param = 0;
					else
						paramsLines[5].param++;
				break;
			}

		}
		resetMenuCommands();

	}
}

void MyScenarioCreator::spawn_peds_o(Vector3 pos, int num_ped) {
	/*
	this function is to generate the pedestrain
	*/
	Ped ped[1024];
	Vector3 current;
	int i = 0;

	float rnX, rnY; // random X and random Y
	int rn; // random number
	float heading_rn; // the test value of the persons a heading

	writeLogLine(pos.x, pos.y, pos.z);

	// spawn 'num_ped' pedestrains
	for (int i = 0; i < num_ped; i++) {
		ped[i] = PED::CREATE_RANDOM_PED(pos.x, pos.y, pos.z); // create the random pedestrains, we can gnerate the specific ped using PED::CREATE_PED()
		WAIT(50);
		// add the ped into the list in order to use the clear method
		addPed(ped[i]);
	}

	//kill all pedestrain in order to prevent a bug
	for (int i = 0; i < num_ped;i++) {
		ENTITY::SET_ENTITY_HEALTH(ped[i], 0);
		WAIT(50);
	}

	WAIT(500);

	int groupId = 0;
	if (group) {
		groupId = PED::CREATE_GROUP(1); //Creates a new ped group. Groups can contain up to 8 peds.
	}

	// resurrecting all pedestrians and assigning them a task
	for (int i = 0; i < num_ped; i++)
	{
		WAIT(50);

		AI::CLEAR_PED_TASKS_IMMEDIATELY(ped[i]); // clear the ped[i]'s task
		PED::RESURRECT_PED(ped[i]); // recurrect ped[i]
		PED::REVIVE_INJURED_PED(ped[i]); // It will revive/cure the injured ped. The condition is ped must not be dead. if health falls below 5, the ped will lay on the ground in pain(Maximum default health is 100).

		// in order to prevent them from falling in hell
		ENTITY::SET_ENTITY_COLLISION(ped[i], TRUE, TRUE);
		PED::SET_PED_CAN_RAGDOLL(ped[i], TRUE);

		current = ENTITY::GET_ENTITY_COORDS(ped[i], TRUE);// get the current ped

		// Will change the target ped, we don't want to only have one taget.!!!!!!!!!
		Ped targetPed = ped[0];
		if (num_ped > 1)
			targetPed = ped[1];

		// Attention!! Each group only has 8 people.
		if (group) {
			PED::SET_PED_AS_GROUP_MEMBER(ped[i], groupId);
			PED::SET_PED_NEVER_LEAVES_GROUP(ped[i], true);
		}

		PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped[i], TRUE); // make the ped[i] oblivious
		PED::SET_PED_COMBAT_ATTRIBUTES(ped[i], 1, FALSE); // stop combat, hw, this function can control the ped's behviour in combat

		Vector3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_ID(), TRUE); // get the player's coordates

		// currentBehaviour is the task, such as sceneari, stand and so on...
		switch (currentBehaviour) 
		{
		case 0: // sceneario
			rn = rand() % 12 + 2; // because the 0 is 'nearst', and 1 is 'random', so we add 2 to make it >=2.

			if (paramsLines[1].param == 0) {
				// Nearest ? what the meanning of this action?
				AI::TASK_USE_NEAREST_SCENARIO_TO_COORD(ped[i], current.x, current.y, current.z, 100.0, paramsLines[0].param);
			}
			else if (paramsLines[1].param == 1) {
				AI::TASK_START_SCENARIO_IN_PLACE(ped[i], scenarioTypes[rn], 0, true);
			}
			else {
				AI::TASK_START_SCENARIO_IN_PLACE(ped[i], scenarioTypes[paramsLines[1].param], 0, true);
			}
			break;
		case 1: // stand
			heading_rn = rand();
			AI::TASK_STAND_STILL(ped[i], paramsLines[0].param);
			AI::TASK_ACHIEVE_HEADING(ped[i], heading_rn, 1000);
			break;
		case 2: // phone
			heading_rn = rand();
			AI::TASK_ACHIEVE_HEADING(ped[i], heading_rn, 1000);
			AI::TASK_USE_MOBILE_PHONE_TIMED(ped[i], paramsLines[0].param); // Maybe we can change it to TASK_USE_MOBILE_PHONE
			break;
		case 3: // cower
			AI::TASK_COWER(ped[i], paramsLines[0].param);
			break;
		case 4: // wander
			AI::TASK_WANDER_IN_AREA(ped[i], current.x, current.y, current.z, (float)paramsLines[2].param, (float)paramsLines[3].param, (float)paramsLines[4].param);
			break;
		case 5: // chat
			// we can make peds to chat with each other, by imporving the function. 
			if (i > 0) {
				AI::TASK_CHAT_TO_PED(ped[i], ped[0], 16, 0.0, 0.0, 0.0, 0.0, 0.0);
			}
			else {
				AI::TASK_CHAT_TO_PED(ped[i], targetPed, 16, 0.0, 0.0, 0.0, 0.0, 0.0);
			}
			break;
		case 6: // combat
			// similar , we can make them fight with each other by improving the function.
			if (i > 0)
				AI::TASK_COMBAT_PED(ped[i], ped[0], 0, 16);
			break;
		case 7: // cover
			AI::TASK_STAY_IN_COVER(ped[i]);
			break;
		case 8: 
		{  //move
			if (paramsLines[5].param == -1) {
				rnX = (float)(((rand() % 81) - 40) / 10.0);
				rnY = (float)(((rand() % 81) - 40) / 10.0);
			}
			else {
				rnX = (float)((rand() % (paramsLines[5].param * 2)) - paramsLines[5].param);
				rnY = (float)((rand() % (paramsLines[5].param * 2)) - paramsLines[5].param);
			}

			//float speed_rnd = (float)(10 + rand() % 4) / 10; // the speed of the pedestrains
			// add the walking ped into the array
			//addwPed(ped[i], coordsToVector(goFrom.x + rnX, goFrom.y + rnY, goFrom.z), coordsToVector(goTo.x + rnX, goTo.y + rnY, goTo.z), paramsLines[4].param, speed_rnd);
			addwPed(ped[i], coordsToVector(goFrom.x + rnX, goFrom.y + rnY, goFrom.z), coordsToVector(goTo.x + rnX, goTo.y + rnY, goTo.z), paramsLines[4].param, speed);
			Object seq; // it's actual a int

			// waiting time proportional to distance
			float atob = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(goFrom.x, goFrom.y, goFrom.z, goTo.x, goTo.y, goTo.z, 1);
			//int max_time = (int)((atob / 2.5) * 1000);
			//max_time = 15000;

			// Based on the present understanding, the following is to make a series of action into a address, and then give these actions to the ped
			// Hw, there are some stranges in this process
			AI::OPEN_SEQUENCE_TASK(&seq);
			//AI::TASK_USE_MOBILE_PHONE_TIMED(0, rand() % max_time);
			
			for (int round = 0; round < walking_round ; round++) {
				// same two line code
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed, 0, 0, 786603, 0xbf800000);
			}
			
			//AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
			//AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
			//AI::SET_SEQUENCE_TO_REPEAT(seq, TRUE);

			AI::CLOSE_SEQUENCE_TASK(seq);
			AI::TASK_PERFORM_SEQUENCE(ped[i], seq);
			AI::CLEAR_SEQUENCE_TASK(&seq);
			/*
			if (paramsLines[5].param != -1) {
				// ? 之前已经有了相同的判断，为何又写一次？
				rnX = (float)((rand() % (paramsLines[5].param * 2)) - paramsLines[5].param);
				rnY = (float)((rand() % (paramsLines[5].param * 2)) - paramsLines[5].param);

				speed_rnd = (float)(10 + rand() % 4) / 10;

				int ped_specular = PED::CREATE_RANDOM_PED(goTo.x, goTo.y, goTo.z);
				WAIT(100);

				ENTITY::SET_ENTITY_HEALTH(ped_specular, 0);
				WAIT(100);
				AI::CLEAR_PED_TASKS_IMMEDIATELY(ped_specular);
				PED::RESURRECT_PED(ped_specular);
				PED::REVIVE_INJURED_PED(ped_specular);

				// in order to prevent them from falling in hell
				ENTITY::SET_ENTITY_COLLISION(ped_specular, TRUE, TRUE);
				PED::SET_PED_CAN_RAGDOLL(ped_specular, TRUE);

				PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped_specular, TRUE);
				PED::SET_PED_COMBAT_ATTRIBUTES(ped_specular, 1, FALSE);
				addwPed(ped_specular, coordsToVector(goTo.x + rnX, goTo.y + rnY, goTo.z), coordsToVector(goFrom.x + rnX, goFrom.y + rnY, goFrom.z), paramsLines[4].param, speed_rnd);

				Object seq2;
				AI::OPEN_SEQUENCE_TASK(&seq2);
				AI::TASK_USE_MOBILE_PHONE_TIMED(0, rand() % max_time);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::CLOSE_SEQUENCE_TASK(seq2);
				AI::TASK_PERFORM_SEQUENCE(ped_specular, seq2);
				AI::CLEAR_SEQUENCE_TASK(&seq2);
			}
			*/
			break;

		}

		default:
			break;

		}
	}

}

void MyScenarioCreator::walking_peds() 
{
	// Make the peds walk
	for (int i = 0; i < nwPeds; i++) 
	{	
		// why we need to judge the task about the action is mobile phone?
		if(PED::IS_PED_STOPPED(wPeds[i].ped) && !AI::GET_IS_TASK_ACTIVE(wPeds[i].ped, 426)) 
		{
			int currentTime = (TIME::GET_CLOCK_HOURS()) * 60 + TIME::GET_CLOCK_MINUTES(); // max value is 12 * 60 + 60 = 780
			// if timeFix is -1, the peds are not walking, because the timeFix + stopTime is always larger than currentTime
			if (wPeds[i].timeFix == -1) {
				wPeds[i].timeFix = currentTime;
			}
			// if timeFix is not -1, it will walk
			if (wPeds[i].timeFix + wPeds[i].stopTime < currentTime)
			{
				wPeds[i].goingTo = !wPeds[i].goingTo; // set it to false
				wPeds[i].timeFix = -1;
				if (wPeds[i].goingTo) {
					AI::TASK_GO_TO_COORD_ANY_MEANS(wPeds[i].ped, wPeds[i].to.x, wPeds[i].to.y, wPeds[i].to.z, wPeds[i].speed, 0, 0, 786603, 0xbf800000);
				}
				else {
					AI::TASK_GO_TO_COORD_ANY_MEANS(wPeds[i].ped, wPeds[i].from.x, wPeds[i].from.y, wPeds[i].from.z, wPeds[i].speed, 0, 0, 786603, 0xbf800000);
				}

			}

		}
	}

}

// Until there, we code the most important part of the script, to create the different peds

void MyScenarioCreator::camera_menu()
{
	// the menu to adjust the camera
	const float lineWidth = 250.0;
	const int lineCount = 14;
	menuActive = true;

	std::string caption = "Camera Coords";
	char lines[lineCount][50] = { "", "", "", "", "", "", "", "", "", "" };
	DWORD waitTime = 150;
	while(true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;
		if (camLock)
			sprintf_s(lines[0], "LOCK		~g~ON");
		else
			sprintf_s(lines[0], "LOCK		~r~OFF");

		sprintf_s(lines[1], "X			~y~[ %0.1f ]", camCoords.x);
		sprintf_s(lines[2], "Y			~y~[ %0.1f ]", camCoords.y);
		sprintf_s(lines[3], "Z			~y~[ %0.1f ]", camCoords.z);
		sprintf_s(lines[4], "FOV        ~y~[ %0.1f ]", camFov);


		if (camMoving)
			sprintf_s(lines[5], "MOVING		~g~ON");
		else
			sprintf_s(lines[5], "MOVING		~r~OFF");

		sprintf_s(lines[6], "A	~y~[ x=%0.1f y=%0.1f z=%0.1f]", A.x, A.y, A.z);
		sprintf_s(lines[7], "B	~y~[ x=%0.1f y=%0.1f z=%0.1f]", B.x, B.y, B.z);
		sprintf_s(lines[8], "C	~y~[ x=%0.1f y=%0.1f z=%0.1f]", C.x, C.y, C.z);

		//sprintf_s(lines[9], "Teleport 1	~y~[ x=%0.1f y=%0.1f ]", TP1.x, TP1.y); // 传送？是的，设置成定值标志着开始和结束，不用每次都设置成不一样的点
		//sprintf_s(lines[10], "Teleport 2	~y~[ x=%0.1f y=%0.1f ]", TP2.x, TP2.y);
		/*sprintf_s(lines[9], "Teleport 1	rot ~y~[ x=%0.1f y=%0.1f z=%0.1f]", TP1_rot.x, TP1_rot.y, TP1_rot.z);
		sprintf_s(lines[10], "Teleport 2 rot	~y~[ x=%0.1f y=%0.1f z=%0.1f]", TP2_rot.x, TP2_rot.y, TP2_rot.z);
		*/
		sprintf_s(lines[9], "Start Point:~y~[ x=%0.1f y=%0.1f z=%0.1f]", TP1.x, TP1.y,TP1.z);
		sprintf_s(lines[10], "End Point:~y~[ x=%0.1f y=%0.1f z=%0.1f]", TP2.x, TP2.y,TP2.z);

		sprintf_s(lines[11], "Rot.X			~y~[ %0.1f ]",camRot.x);
		sprintf_s(lines[12], "Rot.Y			~y~[ %0.1f ]",camRot.y);
		sprintf_s(lines[13], "Rot.Z         ~y~[ %0.1f ]",camRot.z);
		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexCamera)
					draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexCamera], lineWidth, mHeight, mTopFlat + activeLineIndexCamera * mTop, mLeft, mHeight, true, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();

		//process buttons
		if (bBack) {
			resetMenuCommands();
			break;
		}
		else if (bQuit) {
			resetMenuCommands();
			break;
		}
		else if (bUp) {
			if (activeLineIndexCamera == 0)
				activeLineIndexCamera = lineCount;
			activeLineIndexCamera--;
			waitTime = 150;
		}
		else if (bDown) {
			activeLineIndexCamera++;
			if (activeLineIndexCamera == lineCount)
				activeLineIndexCamera = 0;
			waitTime = 150;
		}
		else if (bEnter) {
			// 执行命令
			if (activeLineIndexCamera == 0) {
				camLock = !camLock;
				camLockChange();
			} 
			else if (activeLineIndexCamera == 5) {
				camMoving = !camMoving;
			}
			else if (activeLineIndexCamera == 6) {
				// need to push the enter button, when we select the 
				A = playerCoords;
			}
			else if (activeLineIndexCamera == 7) {
				B = playerCoords;
			}
			else if (activeLineIndexCamera == 8) {
				C = playerCoords;
			}
			// change to use the fix location
			/*else if (activeLineIndexCamera == 9) {
				//TP1 = playerCoords;
				TP1_rot = CAM::GET_GAMEPLAY_CAM_ROT(2);
			}
			else if (activeLineIndexCamera == 10) {
				//TP2 = playerCoords;
				TP2_rot = CAM::GET_GAMEPLAY_CAM_ROT(2);
			}*/
			waitTime = 150;
		}
		else if (bLeft) {
			switch (activeLineIndexCamera) {
			case 1:
				camCoords.x -= 1;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 2:
				camCoords.y -= 1;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 3:
				camCoords.z -= 1;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 4:
				camFov -= 1.0;
				CAM::SET_CAM_FOV(activeCam, camFov);
				break;
			case 11:
				camRot.x -= 1;
				CAM::SET_CAM_ROT(activeCam, camRot.x, camRot.y, camRot.z, 2);
				break;
			case 12:
				camRot.y -= 1;
				CAM::SET_CAM_ROT(activeCam, camRot.x, camRot.y, camRot.z, 2);
				break;
			case 13:
				camRot.z -= 1;
				CAM::SET_CAM_ROT(activeCam, camRot.x, camRot.y, camRot.z, 2);
				break;
			}
		}
		else if (bRight) {
			switch (activeLineIndexCamera)
			{
			case 1:
				camCoords.x++;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 2:
				camCoords.y++;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 3:
				camCoords.z++;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 4:
				camFov += 1.0;
				CAM::SET_CAM_FOV(activeCam,camFov);
				break;
			case 11:
				camRot.x += 1;
				CAM::SET_CAM_ROT(activeCam, camRot.x, camRot.y, camRot.z, 2);
				break;
			case 12:
				camRot.y += 1;
				CAM::SET_CAM_ROT(activeCam, camRot.x, camRot.y, camRot.z, 2);
				break;
			case 13:
				camRot.z += 1;
				CAM::SET_CAM_ROT(activeCam, camRot.x, camRot.y, camRot.z, 2);
				break;
			}
		}

		resetMenuCommands();
	}

}

void MyScenarioCreator::place_menu()
{
	// the place menu to control the palyer to different location
	const float lineWidth = 320.0;
	const int lineCount = 12;
	menuActive = true;

	std::string caption = "Place";

	static struct {
		LPCSTR  text;
		float x;
		float y;
		float z;
	} lines[lineCount] = {
		//{ "MICHAEL'S HOUSE", -852.4f, 160.0, 65.6f },
		//{ "FRANKLIN'S HOUSE", 7.9f, 548.1f, 175.5f },
		//{ "TREVOR'S TRAILER", 1985.7f, 3812.2f, 32.2f },
		//{ "AIRPORT ENTRANCE", -1034.6f, -2733.6f, 13.8f },
		//{ "AIRPORT FIELD", -1336.0, -3044.0f, 13.9f },
		//{ "ELYSIAN ISLAND", 338.2f, -2715.9f, 38.5f },
		//{ "JETSAM", 760.4f, -2943.2f, 5.8f },
		//{ "STRIPCLUB", 127.4f, -1307.7f, 29.2f },
		//{ "ELBURRO HEIGHTS", 1384.0f, -2057.1f, 52.0f },
		//{ "FERRIS WHEEL", -1670.7f, -1125.0f, 13.0 },
		//{ "CHUMASH", -3192.6f, 1100.0f, 20.2f },
		//{ "WINDFARM", 2354.0f, 1830.3f, 101.1f },
		//{ "MILITARY BASE", -2047.4f, 3132.1f, 32.8f },
		//{ "MCKENZIE AIRFIELD", 2121.7f, 4796.3f, 41.1f },
		//{ "DESERT AIRFIELD", 1747.0f, 3273.7f, 41.1f },
		//{ "CHILLIAD", 425.4f, 5614.3f, 766.5f }, // 16
		//{ "Cross#1", -788.25f, -44.575f, -37.747},
		//{ "Railway Station", -790.594f,-126.746f, 12.950f},
		{ "Opening#1", 760.4f, -2943.2f,5.8f},
		{ "Cross#2", 117.344f, -1348.525f, 29.26f},
		{ "Opening#2",1384.000f, -2057.100f, 51.999f},
		{ "Playland", -1670.700f, -1125.0f, 13.025f},
		{ "Park#1", -523.048f, -254.715f, 35.673f},
		{ "Opening#3", -467.561f, -167.192f, 38.080f},
		{ "OPening#4", -964.994f, 316.310f, 70.603f},
		{ "Lane", -368.490f, 498.611f, 117.241f},
		{ "Cross#3", -561.840f, 28.532f, 44.173f},
		{ "Square", -589.607f, -94.020f, 42.335f}, //28
		{ "Stadium", -311.871f, -1846.498f, 24.791f},
		{ "Airport", -1024.665f, -2729.343f, 12.629f} // 30
	};

	DWORD waitTime = 150;
	while (true) {
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;
		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexPlace)
					draw_menu_line(lines[i].text, lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexPlace].text, lineWidth, mHeight, mTopFlat + activeLineIndexPlace * mTop, mLeft, mHeight, true, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount); // 
		waitTime = 0;

		update();
		//process buttons
		if (bEnter) {
			Entity e = PLAYER::PLAYER_PED_ID();
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, lines[activeLineIndexPlace].x, lines[activeLineIndexPlace].y, lines[activeLineIndexPlace].z, 0, 0, 1);

		}
		else if (bBack) {
			resetMenuCommands();
			break;
		}
		else if (bQuit) {
			resetMenuCommands();
			bQuit = true;
			break;
		}
		else if (bUp) {
			if (activeLineIndexPlace == 0)
				activeLineIndexPlace = lineCount;
			activeLineIndexPlace--;
			waitTime = 150;
		}
		else if (bDown) {
			activeLineIndexPlace++;
			if (activeLineIndexPlace == lineCount)
				activeLineIndexPlace = 0;
			waitTime = 150;
		}

		resetMenuCommands();

	}
}

void MyScenarioCreator::time_menu() 
{
	// the method controls the time in the game
	const float lineWidth = 250.0;
	const int lineCount = 4;
	menuActive = true;

	std::string caption = "Time control";

	static LPCSTR lines[lineCount] = {
		"Hour Forward",
		"Hour Backward",
		"Time Freeze",
		"Time Active"
	};

	char timeText[32];
	DWORD waitTime = 150;
	while (true) 
	{
		// timed menu draw, used for pause after active line switch. Because waitTime is 150 expect some operation changes it.
		DWORD maxTickCount = GetTickCount() + waitTime;

		sprintf_s(timeText, "~y~time %d:%d", TIME::GET_CLOCK_HOURS(), TIME::GET_CLOCK_MINUTES());

		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexTime)
					draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexTime], lineWidth, mHeight, mTopFlat + activeLineIndexTime * mTop, mLeft, mHeight, true, false);

			// show time, not the real time, just the rough time
			draw_menu_line(timeText, lineWidth, mHeight, mTopFlat + lineCount * mTop, mLeft, mHeight, false, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);  
		waitTime = 0;
		
		update();

		//process buttons
		if (bEnter) {
			int h = TIME::GET_CLOCK_HOURS();
			if (activeLineIndexTime == 0) 
			{
				h = (h == 23) ? 0 : h + 1;
			}
			else if (activeLineIndexTime == 1) 
			{
				h = (h == 0) ? 23 : h - 1;
			}
			else if (activeLineIndexTime == 2)
			{
				TIME::PAUSE_CLOCK(true);
			}
			else if (activeLineIndexTime == 3) 
			{
				TIME::PAUSE_CLOCK(false);
			}
			
			int m = TIME::GET_CLOCK_MINUTES();
			TIME::SET_CLOCK_TIME(h, m, 0);

			char log_message[50];
			record_hour = h;
			record_minute = m;
			sprintf_s(log_message, "the record time is %d:%d", h, m);
			log_info(log_message);

			sprintf_s(timeText, "~y~time %d:%d", h, m);
			waitTime = 200;
		}
		else if (bBack) {
			resetMenuCommands();
			break;
		}
		else if (bQuit) {
			resetMenuCommands();
			break;
		}
		else if (bUp) {
			if (activeLineIndexTime == 0)
				activeLineIndexTime = lineCount;
			activeLineIndexTime--;
			waitTime = 150;
		}
		else if (bDown) {
			activeLineIndexTime++;
			if (activeLineIndexTime == lineCount)
				activeLineIndexTime = 0;
			waitTime = 150;
		}
		resetMenuCommands();
	}
}

void MyScenarioCreator::weather_menu()
{
	// the method controls the weather in the game
	const float lineWidth = 250.0;
	const int lineCount = 14;
	menuActive = true;

	std::string caption = "Weather";

	static LPCSTR lines[lineCount] = {
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

	DWORD waitTime = 150;
	while (true) {
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		if (wind)
			lines[0] = "WIND		~g~ON";
		else
			lines[0] = "WIND		~r~OFF";

		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexWeather)
					draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexWeather], lineWidth, mHeight, mTopFlat + activeLineIndexWeather * mTop, mLeft, mHeight, true, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();

		//process buttons
		if (bEnter) {
			switch (activeLineIndexWeather) 
			{
			case 0:
				wind = !wind;
				if (wind) {
					GAMEPLAY::SET_WIND(1.0);
					GAMEPLAY::SET_WIND_SPEED(11.99f);
					GAMEPLAY::SET_WIND_DIRECTION(ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID()));
				}
				else if(wind){
					GAMEPLAY::SET_WIND(0.0);
					GAMEPLAY::SET_WIND_SPEED(0.0);
				}
				weather_code = 0;
				break;
			default:
				GAMEPLAY::CLEAR_OVERRIDE_WEATHER();
				GAMEPLAY::SET_WEATHER_TYPE_NOW_PERSIST((char *)lines[activeLineIndexWeather]);
				GAMEPLAY::CLEAR_WEATHER_TYPE_PERSIST();
				weather_code = activeLineIndexWeather;
				break;
			}
			log_info((char *)lines[activeLineIndexWeather]);
			waitTime = 200;
		}
		else if (bBack) {
			resetMenuCommands();
			break;
		}
		else if (bQuit) {
			resetMenuCommands();
			bQuit = true;
			break;
		}
		else if (bUp)
		{
			if (activeLineIndexWeather == 0)
				activeLineIndexWeather = lineCount;
			activeLineIndexWeather--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexWeather++;
			if (activeLineIndexWeather == lineCount)
				activeLineIndexWeather = 0;
			waitTime = 150;
		}
		resetMenuCommands();
	}
}

int MyScenarioCreator::myreadLine(FILE *f, Vector3 *pos, int *nPeds, int *ngroup, int *currentBehaviour, float *speed, Vector3 *goFrom, Vector3 *goTo, int *task_time, int *type,
	int *radius, int *min_lenght, int *time_between_walks, int *spawning_radius, int *actionType, int *actionSn)
{
	//read the lines of creating the peds
	int result = fscanf_s(f, "%d %f %f %f %d %d %f %f %f %f %f %f %f %d %d %d %d %d %d %d %d\n", nPeds, &(*pos).x, &(*pos).y, &(*pos).z,
		ngroup, currentBehaviour, speed,
		&(*goFrom).x, &(*goFrom).y, &(*goFrom).z, &(*goTo).x, &(*goTo).y, &(*goTo).z,
		task_time, type, radius, min_lenght, time_between_walks, spawning_radius, actionType, actionSn);

	return result; // in order to indicate wheather we come the end of the file
}

void MyScenarioCreator::spawn_peds_flow(Vector3 pos, Vector3 goFrom, Vector3 goTo, int npeds, int ngroup, int currentBehaviour,
	int task_time, int type, int radius, int min_lenght, int time_between_walks, int spawning_radius, float speed, int actionType, int actionSn)
{
	// generate the walking flow of the peds
	int max_waiting_time = 0;
	// 
	Ped peds[1024];
	//Ped ped_speculars[100];
	Vector3 current;
	//int i = 0; // ? use? 

	float rnX, rnY;

	//if (currentBehaviour == 8) {
	// when currentBehaviour = 8, the task is MOVE
	for (int i = 0; i < npeds; i++) {
		peds[i] = PED::CREATE_RANDOM_PED(goFrom.x, goFrom.y, goFrom.z); // create the random ped
		//add the peds into the array
		addPed(peds[i]);
		char log_message[200];
		sprintf_s(log_message, "In spawning flow(cb:8) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], goFrom.x, goFrom.y, goFrom.z);
		log_info(log_message);
		WAIT(100);
	}
	//dummy_wait(DEMO);

	for (int i = 0; i < npeds; i++) {
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

		current = ENTITY::GET_ENTITY_COORDS(peds[i], TRUE);

		// Not use in this function
		//Ped target = peds[0];
		//if (npeds > 1) {
		//	target = peds[1];
		//}
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
		//addwPed(peds[i], coordsToVector(goFrom.x + rnX, goFrom.y + rnY, goFrom.z), coordsToVector(goTo.x + rnX, goTo.y + rnY, goTo.z), time_between_walks, speed_rnd);
		addwPed(peds[i], coordsToVector(goFrom.x + rnX, goFrom.y + rnY, goFrom.z), coordsToVector(goTo.x + rnX, goTo.y + rnY, goTo.z), time_between_walks, speed);
		Object seq; // actually it is a int

		// waiting time proportional to distance
		float distance = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(goFrom.x, goFrom.y, goFrom.z, goTo.x, goTo.y, goTo.z, 1);
		int max_time = (int)((distance / 2.5) * 1000);

		if (max_time > max_waiting_time)
			max_waiting_time = max_time;

		AI::OPEN_SEQUENCE_TASK(&seq);
		//AI::TASK_USE_MOBILE_PHONE_TIMED(0, rand() % max_time);

		
		for (int round = 0; round < walking_round; round++) {
			// same two line code
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed, 0, 0, 786603, 0xbf800000);
			//AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed, 0, 0, 786603, 0xbf800000);
			//AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
		}

		//AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
		//AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
		//AI::SET_SEQUENCE_TO_REPEAT(seq, TRUE);

		AI::CLOSE_SEQUENCE_TASK(seq);
		AI::TASK_PERFORM_SEQUENCE(peds[i], seq);
		AI::CLEAR_SEQUENCE_TASK(&seq);
	}

}

void MyScenarioCreator::spawn_peds(Vector3 pos, Vector3 goFrom, Vector3 goTo, int npeds, int ngroup, int currentBehaviour,
	int task_time, int type, int radius, int min_lenght, int time_between_walks, int spawning_radius, float speed, int actionType, int actionSn)
{
	// generate the standard peds
	Ped peds[1024]; // a maximun number of each line is 100
	Vector3 current;
	int i = 0;

	int rn;

	for (int i = 0; i < npeds; i++) {
		peds[i] = PED::CREATE_RANDOM_PED(pos.x, pos.y, pos.z);
		WAIT(50);
		addPed(peds[i]);
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

		//Vector3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_ID(), TRUE);
		char log_message[200];
		switch (currentBehaviour)
		{
		case 0:
			rn = rand() % 12 + 2;
			if (type == 0) {
				AI::TASK_USE_NEAREST_SCENARIO_TO_COORD(peds[i], current.x, current.y, current.z, 100.0, task_time);
				//char log_message[200];
				sprintf_s(log_message, "In spawning (cb:0,type:nearst) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
				log_info(log_message);
			}
			else if (type == 1)
			{
				AI::TASK_START_SCENARIO_IN_PLACE(peds[i], scenarioTypes[rn], 0, true);
				//char log_message[200];
				sprintf_s(log_message, "In spawning (cb:0,type:random) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
				log_info(log_message);
			}
			else {
				AI::TASK_STAND_STILL(peds[i], 200);
				AI::TASK_START_SCENARIO_IN_PLACE(peds[i], scenarioTypes[type], 0, true);
				//char log_message[200];
				sprintf_s(log_message, "In spawning (cb:0,type:WORLD_HUMAN_MUSCLE_FLEX) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
				log_info(log_message);
			}
			break;
		case 1:
			AI::TASK_STAND_STILL(peds[i], task_time);
			//char log_message[200];
			sprintf_s(log_message, "In spawning (cb:1) the pedid:%d, the location:(%.3f, %.3f,%.3f)", peds[i], current.x, current.y, current.z);
			log_info(log_message);
			break;
		case 2:
			AI::TASK_USE_MOBILE_PHONE_TIMED(peds[i], task_time);
			//char log_message[200];
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

}

void MyScenarioCreator::loadFile(char *file_name)
{
	// load the file to re-construct the scenario
	char fname[50] = "";
	strcat(fname, files_path);
	strcat(fname, file_name);
	f = fopen(fname, "r");
	Vector3 cCoords, cRot;
	float cFov;
	Vector3 vTP1, vTP2, vTP1_rot, vTP2_rot;
	int camMov;

	int cWeather_code;
	int cRecord_hour;
	int cRecord_minute;

	int stop;
	fscanf_s(f, "%d ", &camMov);

	if (camMov == 0) {
		fscanf_s(f, "%f %f %f %d %f %f %f %f %d %d %d\n", &cCoords.x, &cCoords.y, &cCoords.z, &stop, &cRot.x, &cRot.y, &cRot.z, &cFov, &cWeather_code, &cRecord_hour, &cRecord_minute);
	}
	else {
		fscanf_s(f, "%f %f %f %d %f %f %f %f %f %f %d %d %d\n", &A.x, &A.y, &A.z, &stop, &B.x, &B.y, &B.z, &C.x, &C.y, &C.z, &cWeather_code, &cRecord_hour, &cRecord_minute);
	}

	fscanf_s(f, "%f %f %f %f %f %f\n", &vTP1.x, &vTP1.y, &vTP1.z, &vTP1_rot.x, &vTP1_rot.y, &vTP1_rot.z);
	fscanf_s(f, "%f %f %f %f %f %f\n", &vTP2.x, &vTP2.y, &vTP2.z, &vTP2_rot.x, &vTP2_rot.y, &vTP2_rot.z);
	
	Entity e = PLAYER::PLAYER_PED_ID();

	// store a copy of the perserve
	perserve_cam_coords = cCoords;
	perserve_cam_rot = cRot;
	perserve_cam_fov = cFov;
	perserve_weather = cWeather_code;
	perserve_hour = cRecord_hour;
	perserve_minute = cRecord_minute;
	
	ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, vTP1.x, vTP1.y, vTP1.z, 0, 0, 1); // why we set 0,0,1 as the last 3. Default
	lockCam(vTP1, vTP1_rot);
	WAIT(500);

	ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, vTP2.x, vTP2.y, vTP2.z, 0, 0, 1);
	lockCam(vTP2, vTP2_rot);
	WAIT(500);

	if (camMov == 0)
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, cCoords.x, cCoords.y, cCoords.z, 0, 0, 1);
	else
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, B.x, B.y, B.z, 0, 0, 1);


	fixCoords = cCoords;
	WAIT(100);

	TIME::SET_CLOCK_TIME(cRecord_hour, cRecord_minute, 0);
	//TIME::PAUSE_CLOCK(TRUE);
	GAMEPLAY::CLEAR_WEATHER_TYPE_PERSIST();
	GAMEPLAY::SET_WEATHER_TYPE_NOW_PERSIST((char *)weathers[cWeather_code]);

	camLock = false;
	camLockChange(); // get the active cam
	//visible = true;
	//ENTITY::SET_ENTITY_VISIBLE(PLAYER::PLAYER_PED_ID(), FALSE, true);// no need to set the player is invisible
	CAM::SET_CAM_ROT(activeCam, cRot.x, cRot.y, cRot.z, 2);
	CAM::SET_CAM_COORD(activeCam, cCoords.x, cCoords.y, cCoords.z);
	CAM::SET_CAM_FOV(activeCam, cFov);

	/*
	Vector3 pos;
	while (readLine(f, &pos) >= 0) {
		// based on the inforamtion of each peds in the log file to generate the peds
		spawn_peds(pos, nPeds);
		update();
	}
	*/

	Vector3 pos, goFrom, goTo;
	int npeds, ngroup, currentBehaviour, task_time, type, radius, min_lenght, time_between_walks, spawning_radius;
	float speed;
	int actionType;
	int actionSn;

	while (myreadLine(f, &pos, &npeds, &ngroup, &currentBehaviour, &speed,
		&goFrom, &goTo, &task_time, &type, &radius, &min_lenght,
		&time_between_walks, &spawning_radius, &actionType, &actionSn) >= 0)
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

	walking_peds();
	if (stop == 1)
		stopCoords = true;

	ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, cCoords.x, cCoords.y, cCoords.z, 0, 0, 1);
	sprintf_s(fname, "\"%s\" LOADED", file_name);
	set_status_text(fname);

}

void MyScenarioCreator::file_menu()
{
	// save the setting into the log file, in order to re-construct the scene
	// we just make the camera, the tasks of ped, and so on same. we can't make the ped is same.

	const float lineWidth = 250.0;
	const int lineCount = 5;
	menuActive = true;
	std::string caption = "File Manager";

	char lines[lineCount][60] = {
		"Load",  // 0
		"Overwrite", //1
		"Save new",  //2
		"Clear Log Data", //3
		"copy with cam"  //4  // indicate we will overwrite the log keeping the original information of the camera in the original log
	};

	char loadedFile[40];
	DWORD waitTime = 150;

	while (true) {
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		sprintf_s(loadedFile, "file:~y~ %s", file_name);

		readDirr();

		if (nFiles == 0) {
			strcpy(file_name, "None");
			sprintf_s(lines[0], "LOAD	~y~[ no files ]");
			sprintf_s(lines[4], "Copy with cam ~y~[ no files ]");
		}
		else {
			sprintf_s(lines[0], "LOAD	~y~[ log_%03d.txt ]", currentFile);
			sprintf_s(lines[4], "Copy with cam ~y~[ log_%03d.txt ]", currentFile);
		}

		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexFile)
					draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexFile], lineWidth, mHeight, mTopFlat + activeLineIndexFile * mTop, mLeft, mHeight, true, false);

			// show time
			draw_menu_line(loadedFile, lineWidth, mHeight, mTopFlat + lineCount * mTop, mLeft, mHeight, false, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		// process buttons

		if (bEnter)
		{
			switch (activeLineIndexFile)
			{
			case 0:
				if (nFiles > 0) {
					sprintf_s(file_name, "log_%03d.txt", currentFile);
					loadFile(file_name);
				}
				break;
			case 1:
				if (strcmp(file_name, "None") != 0)
					saveFile();
				break;
			case 2:
				sprintf_s(file_name, "log_%03d.txt", nFiles + 1);
				saveFile();
				break;
			case 3:
				cancelLastLog();
				break;
			case 4:
				sprintf_s(file_name, "log_%03d.txt", nFiles + 1);
				copy_with_cam();
				break;
			//default:
			//	break;
			}
			waitTime = 200;
		}
		else if (bBack) {
			resetMenuCommands();
			break;
		}
		else if (bQuit) {
			resetMenuCommands();
			bQuit = true;
			break;
		} 
		else if (bUp)
		{
			if (activeLineIndexFile == 0)
				activeLineIndexFile = lineCount;
			activeLineIndexFile--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexFile++;
			if (activeLineIndexFile == lineCount)
				activeLineIndexFile = 0;
			waitTime = 150;
		}
		else if (bLeft)
		{
			if (activeLineIndexFile == 0) {
				if (currentFile == 1)
					currentFile = nFiles;
				else
					currentFile--;
			}
			waitTime = 150;
		}
		else if (bRight)
		{
			if (activeLineIndexFile == 0) {
				if (currentFile == nFiles)
					currentFile = 1;
				else
					currentFile++;
			}
			waitTime = 150;
		}
		resetMenuCommands();
	}
}


// new features!!!
void MyScenarioCreator::addPed(Ped p) {
	present_peds[nPresent_peds].ped = p;
	nPresent_peds++;
}

void MyScenarioCreator::clear_method() {
	for (int i = 0; i < nPresent_peds; i++) {
		PED::DELETE_PED(&present_peds[i].ped);
	}
	nPresent_peds = 0;
}

void MyScenarioCreator::copy_with_cam() {
	std::string fname = "";
	int stop = 0;
	if (stopCoords)
		stop = 1;

	fname = fname + std::string(files_path) + std::string(file_name);
	f = fopen(fname.c_str(), "w");

	if (camMoving) {
		// Not implemented present
		fprintf_s(f, "%d %f %f %f %d %f %f %f %f %f %f %d %d %d\n", (int)camMoving, A.x, A.y, A.z, stop, B.x, B.y, B.z, C.x, C.y, C.z, weather_code, record_hour, record_minute);
	}
	else {
		fprintf_s(f, "%d %f %f %f %d %f %f %f %f %d %d %d\n", (int)camMoving, perserve_cam_coords.x, perserve_cam_coords.y, perserve_cam_coords.z, stop, perserve_cam_rot.x, perserve_cam_rot.y, perserve_cam_rot.z, perserve_cam_fov, perserve_weather, perserve_hour, perserve_minute);
	}

	fprintf_s(f, "%f %f %f %f %f %f\n", TP1.x, TP1.y, TP1.z, TP1_rot.x, TP1_rot.y, TP1_rot.z);
	fprintf_s(f, "%f %f %f %f %f %f\n", TP2.x, TP2.y, TP2.z, TP2_rot.x, TP2_rot.y, TP2_rot.z);

	fprintf_s(f, "%s", log_string);
	fclose(f);

	fname = fname + "  copy with cam!";
	set_status_text(fname);
}