// ttd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define VARDEF
#include "ttd.h"
#include "gfx.h"
#include "gui.h"
#include "station.h"
#include "vehicle.h"
#include "viewport.h"
#include "window.h"
#include "player.h"
#include "command.h"
#include "city.h"
#include "industry.h"
#include "news.h"
#include "engine.h"
#include "sound.h"
#include "economy.h"
#include "fileio.h"
#include "hal.h"

#include <stdarg.h>

void GameLoop();

void IncreaseSpriteLRU();
void InitializeGame();
void GenerateWorld(bool empty);
void CallLandscapeTick();
void IncreaseDate();
void RunOtherPlayersLoop();
void DoPaletteAnimations();
void MusicLoop();

extern void SetDifficultyLevel(int mode);
extern void DoStartupNewPlayer(bool is_ai);
extern void UpdateAllSignVirtCoords();


void redsq_debug(int tile);
bool LoadSavegame(const char *filename);

extern void HalGameLoop();

uint32 _pixels_redrawn;
bool _dbg_screen_rect;
bool disable_computer;

#if defined(WIN32)
void ShowWin32ErrorBox(const char *buf);
#endif

void CDECL error(const char *s, ...) {
	va_list va;
	char buf[512];
	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);
	
#if defined(WIN32)
	ShowWin32ErrorBox(buf);
	if (_video_driver)
		_video_driver->stop();
#else
	if (_video_driver)
		_video_driver->stop();
	fprintf(stderr, "Error: %s\n", buf);
#endif
	assert(0);
	exit(1);
}

void CDECL debug(const char *s, ...)
{
	va_list va;
	char buf[1024];
	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);

	fprintf(stderr, "dbg: %s\n", buf);
}

void CDECL ShowInfoF(const char *str, ...)
{
	va_list va;
	char buf[1024];
	va_start(va, str);
	vsprintf(buf, str, va);
	va_end(va);
	ShowInfo(buf);
}

// NULL midi driver
static char *NullMidiStart(char **parm) { return NULL; }
static void NullMidiStop() {}
static void NullMidiPlaySong(const char *filename) {}
static void NullMidiStopSong() {}
static bool NullMidiIsSongPlaying() { return true; }
static void NullMidiSetVolume(byte vol) {}

const HalMusicDriver _null_music_driver = {
	NullMidiStart,
	NullMidiStop,
	NullMidiPlaySong,
	NullMidiStopSong,
	NullMidiIsSongPlaying,
	NullMidiSetVolume,
};

// NULL video driver
static void *_null_video_mem;
static const char *NullVideoStart(char **parm) {
	_screen.width = _screen.pitch = _cur_resolution[0];
	_screen.height = _cur_resolution[1];
	_null_video_mem = malloc(_cur_resolution[0]*_cur_resolution[1]);
	return NULL;
}
static void NullVideoStop() { free(_null_video_mem); }
static void NullVideoMakeDirty(int left, int top, int width, int height) {}
static int NullVideoMainLoop() {
	int i = 1000;
	do {
		GameLoop();
		_screen.dst_ptr = _null_video_mem;
		UpdateWindows();
	}	while (--i);
	return ML_QUIT;
}

static bool NullVideoChangeRes(int w, int h) { return false; }

	
const HalVideoDriver _null_video_driver = {
	NullVideoStart,
	NullVideoStop,
	NullVideoMakeDirty,
	NullVideoMainLoop,
	NullVideoChangeRes,
};

// NULL sound driver
static char *NullSoundStart(char **parm) { return NULL; }
static void NullSoundStop() {}
const HalSoundDriver _null_sound_driver = {
	NullSoundStart,
	NullSoundStop,
};

enum {
	DF_PRIORITY_MASK = 0xf,
};

typedef struct {
	const DriverDesc *descs;
	const char *name;
	void *var;
} DriverClass;

static DriverClass _driver_classes[] = {
	{_video_driver_descs, "video", &_video_driver},
	{_sound_driver_descs, "sound", &_sound_driver},
	{_music_driver_descs, "music", &_music_driver},
};

static const DriverDesc *GetDriverByName(const DriverDesc *dd, const char *name)
{
	do {
		if (!strcmp(dd->name, name))
			return dd;
	} while ((++dd)->name);
	return NULL;
}

static const DriverDesc *ChooseDefaultDriver(const DriverDesc *dd)
{
	const DriverDesc *best = NULL;
	int best_pri = -1;
	do {
		if ((int)(dd->flags&DF_PRIORITY_MASK) > best_pri) {
			best_pri = dd->flags&DF_PRIORITY_MASK;
			best = dd;
		}
	} while ((++dd)->name);
	return best;
}

void ttd_strlcpy(char *dst, const char *src, size_t len)
{
	assert(len > 0);
	while (--len && *src)
		*dst++=*src++;
	*dst = 0;
}

char *strecpy(char *dst, const char *src)
{
	while ( (*dst++ = *src++) != 0) {}
	return dst - 1;
}

byte *ReadFileToMem(const char *filename, size_t *lenp, size_t maxsize)
{
	FILE *in;
	void *mem;
	size_t len;

	in = fopen(filename, "rb");
	if (in == NULL)
		return NULL;

	fseek(in, 0, SEEK_END);
	len = ftell(in);
	fseek(in, 0, SEEK_SET);
	if (len > maxsize || (mem=(byte*)malloc(len + 1)) == NULL) {
		fclose(in);
		return NULL;
	}
	((byte*)mem)[len] = 0;
	if (fread(mem, len, 1, in) != 1) {
		fclose(in);
		free(mem);
		return NULL;
	}
	fclose(in);

	*lenp = len;
	return mem;
}

void LoadDriver(int driver, const char *name)
{
	const DriverClass *dc = &_driver_classes[driver];
	const DriverDesc *dd;
	const void **var;
	const void *drv;
	const char *err;
	char *parm;
	char buffer[256];
	char *parms[32];

	parms[0] = NULL;

	if (!*name) {
		dd = ChooseDefaultDriver(dc->descs);
	} else {
		// Extract the driver name and put parameter list in parm
		ttd_strlcpy(buffer, name, sizeof(buffer));
		parm = strchr(buffer, ':');
		if (parm) {
			int np = 0;
			// Tokenize the parm.
			do {
				*parm++ = 0;
				if (np < lengthof(parms) - 1)
					parms[np++] = parm;
				while (*parm != 0 && *parm != ',')
					parm++;
			} while (*parm == ',');
			parms[np] = NULL;
		}
		dd = GetDriverByName(dc->descs, buffer);
		if (dd == NULL)
			error("No such %s driver: %s\n", dc->name, buffer);
	}
	var = dc->var;
	if (*var != NULL) ((HalCommonDriver*)*var)->stop();
	*var = NULL;
	drv = dd->drv;
	if ((err=((HalCommonDriver*)drv)->start(parms)) != NULL)
		error("Unable to load driver %s(%s). The error was: %s\n", dd->name, dd->longname, err);
	*var = drv;
}

void PrintDriverList()
{
}

static void showhelp()
{
	char buf[4096], *p;
	const DriverClass *dc = _driver_classes;
	const DriverDesc *dd;
	int i;

	p = strecpy(buf, 
		"Command line options:\n"
		"  -v drv = Set video driver (see below)\n"
		"  -s drv = Set sound driver (see below)\n"
		"  -m drv = Set music driver (see below)\n"
		"  -r res = Set resolution (for instance 800x600)\n"
		"  -h     = Display this help text\n"
		"  -t date= Set starting date\n"
		"  -d dbg = Debug mode\n"
		"  -l lng = Select Language\n"
		"  -e     = Start Editor\n"
		"  -g     = Start new game immediately (can optionally take a game to load)\n"
		"  -G seed= Set random seed\n"
	);

	for(i=0; i!=lengthof(_driver_classes); i++,dc++) { 
		p += sprintf(p, "List of %s drivers:\n", dc->name);
		dd = dc->descs;
		do {
			p += sprintf(p, "%10s: %s\n", dd->name, dd->longname);
		} while ((++dd)->name);
	}

	ShowInfo(buf);
}


char *GetDriverParam(char **parm, const char *name)
{
	char *p;
	int len = strlen(name);
	while ((p = *parm++) != NULL) {
		if (!strncmp(p,name,len)) {
			if (p[len] == '=') return p + len + 1;
			if (p[len] == 0)   return p + len;
		}
	}
	return NULL;
}

bool GetDriverParamBool(char **parm, const char *name)
{
	char *p = GetDriverParam(parm, name);
	return p != NULL;
}

int GetDriverParamInt(char **parm, const char *name, int def)
{
	char *p = GetDriverParam(parm, name);
	return p != NULL ? atoi(p) : def;
}

typedef struct {
	char *opt;
	int numleft;
	char **argv;
	const char *options;
	char *cont;
} MyGetOptData;

static void MyGetOptInit(MyGetOptData *md, int argc, char **argv, const char *options)
{
	md->cont = NULL;
	md->numleft = argc;
	md->argv = argv;
	md->options = options;
}

static int MyGetOpt(MyGetOptData *md)
{
	char *s,*r,*t;

	if ((s=md->cont) != NULL)
		goto md_continue_here;

	while(true) {
		if (--md->numleft < 0)
			return -1;

		s = *md->argv++;
		if (*s == '-') {
md_continue_here:;
			s++;
			if (*s != 0) {
				// Found argument, try to locate it in options.
				if (*s == ':' || (r = strchr(md->options, *s)) == NULL) {
					// ERROR!
					return -2;
				}
				if (r[1] == ':') {
					// Item wants an argument. Check if the argument follows, or if it comes as a separate arg.
					if (!*(t = s + 1)) {
						// It comes as a separate arg. Check if out of args?
						if (--md->numleft < 0 || *(t = *md->argv) == '-') {
							// Check if item is optional?
							if (r[2] != ':')
								return -2;
							md->numleft++;
							t = NULL;
						} else {
							md->argv++;
						}
					}
					md->opt = t;
					md->cont = NULL;
					return *s;
				}
				md->opt = NULL;
				md->cont = s;
				return *s;
			}
		} else {
			// This is currently not supported.
			return -2;
		}
	}
}

void SetDebugString(const char *s)
{
	int v;
	char *end;
	const char *t;
	int *p;

	// global debugging level?
	if (*s >= '0' && *s <= '9') {
		v = strtoul(s, &end, 0);
		s = end;
		
		_debug_spritecache_level = v;
		_debug_misc_level = v;
	}

	// individual levels
	for(;;) {
		// skip delimiters
		while (*s == ' ' || *s == ',' || *s == '\t') s++;
		if (*s == 0) break;

		t = s;
		while (*s >= 'a' && *s <= 'z') s++;

#define IS_LVL(x) (s - t == sizeof(x)-1 && !memcmp(t, x, sizeof(x)-1))
		// check debugging levels
		if IS_LVL("misc") p = &_debug_misc_level;
		else if IS_LVL("spritecache") p = &_debug_spritecache_level;
		else {
			ShowInfoF("Unknown debug level '%.*s'", s-t, t);
			return;
		}
#undef IS_LVL
		if (*s == '=') s++;
		v = strtoul(s, &end, 0);
		s = end;
		if (p) *p = v;
	}		
}

void ParseResolution(int res[2], char *s)
{
	char *t = strchr(s, 'x');
	if (t == NULL) {
		ShowInfoF("Invalid resolution '%s'", s);
		return;
	}

	res[0] = (strtoul(s, NULL, 0) + 7) & ~7;
	res[1] = (strtoul(t+1, NULL, 0) + 7) & ~7;
} 

int ttd_main(int argc, char* argv[])
{
	MyGetOptData mgo;
	int i;
	int network = 0;
	char *network_conn;
	char *language = 0;

	char musicdriver[32], sounddriver[32], videodriver[32];
	int resolution[2] = {0,0};
	musicdriver[0] = sounddriver[0] = videodriver[0] = 0;

	_game_mode = GM_MENU;
	_switch_mode = SM_MENU;

	MyGetOptInit(&mgo, argc-1, argv+1, "m:s:v:hn::l:et:d::r:g::G:c");
	while ((i = MyGetOpt(&mgo)) != -1) {
		switch(i) {
		case 'm': ttd_strlcpy(musicdriver, mgo.opt, sizeof(musicdriver)); break;
		case 's': ttd_strlcpy(sounddriver, mgo.opt, sizeof(sounddriver)); break;
		case 'v': ttd_strlcpy(videodriver, mgo.opt, sizeof(videodriver)); break;
		case 'n': network=1; if (mgo.opt) {network_conn = mgo.opt; network++;} break;
		case 'r': ParseResolution(resolution, mgo.opt); break;
		case 'l': {
				language = mgo.opt;
			} break;
		case 't': {
				_starting_date = atoi(mgo.opt);
			} break;
		case 'd': {
#if defined(WIN32)
				CreateConsole();
#endif
				if (mgo.opt)
					SetDebugString(mgo.opt);
			} break;
		case 'e': _switch_mode = SM_EDITOR; break;
		case 'g': 
			if (mgo.opt) {
				strcpy(_file_to_saveload, mgo.opt);
				_switch_mode = SM_LOAD;
			} else
				_switch_mode = SM_NEWGAME;
			break;
		case 'G':
			_random_seed_1 = atoi(mgo.opt);
			break;
		case -2:
 		case 'h':
			showhelp();
			return 0;
		}
	}

	LoadFromConfig();

	// override config?
	if (musicdriver[0]) strcpy(_ini_musicdriver, musicdriver);
	if (sounddriver[0]) strcpy(_ini_sounddriver, sounddriver);
	if (videodriver[0]) strcpy(_ini_videodriver, videodriver);
	if (resolution[0]) { _cur_resolution[0] = resolution[0]; _cur_resolution[1] = resolution[1]; }
	
	// enumerate language files
	InitializeLanguagePacks();
	
	// Sample catalogue
	DEBUG(misc, 1) ("Loading sound effects...");
	MxInitialize(11025, "sample.cat"); 

	// This must be done early, since functions use the InvalidateWindow* calls
	InitWindowSystem();

	GfxLoadSprites();
	LoadStringWidthTable();

	DEBUG(misc, 1) ("Loading drivers...");
	LoadDriver(SOUND_DRIVER, _ini_sounddriver);
	LoadDriver(MUSIC_DRIVER, _ini_musicdriver);
	LoadDriver(VIDEO_DRIVER, _ini_videodriver); // load video last, to prevent an empty window while sound and music loads
	MusicLoop();

	_num_human_players = 1;
	
	// Default difficulty level
	_opt_mod_ptr = &_new_opt;
	
	// ugly hack, if diff_level is 9, it means we got no setting from the config file, so we load the default settings.
	if (_opt_mod_ptr->diff_level == 9)
		SetDifficultyLevel(0);

#define PORTNUM 1234
	if (network) {
		DoStartupNewPlayer(false);
		_players[1].player_money = 1000000;
		_num_human_players = 2;
		
		NetworkInitialize();
		if (network==1) {
			DEBUG(misc, 1) ("Listening on port %d\n", PORTNUM);
			NetworkListen(PORTNUM);
		} else {
			DEBUG(misc, 1) ("Connecting to %s %d\n", network_conn, PORTNUM);
			NetworkConnect(network_conn,PORTNUM);
		}
	}

	while (_video_driver->main_loop() == ML_SWITCHDRIVER) {}

	_video_driver->stop();
	_music_driver->stop();
	_sound_driver->stop();

	SaveToConfig();

	return 0;
}

static void ShowScreenshotResult(bool b)
{
	if (b) {
		SET_DPARAM16(0, STR_SPEC_SCREENSHOT_NAME);
		ShowErrorMessage(INVALID_STRING_ID, STR_031B_SCREENSHOT_SUCCESSFULLY, 0, 0);
	} else {
		ShowErrorMessage(INVALID_STRING_ID, STR_031C_SCREENSHOT_FAILED, 0, 0);
	}

}

static void SwitchMode(int new_mode)
{
	_in_state_game_loop = true;
	
	switch(new_mode) {
	case SM_EDITOR: // Switch to scenario editor
		_game_mode = GM_EDITOR;

		// Copy in game options
		_opt_mod_ptr = &_opt;
		memcpy(&_opt, &_new_opt, sizeof(_opt));
		
		GfxLoadSprites();

		// Re-init the windowing system
		InitWindowSystem();

		// Create toolbars
		SetupColorsAndInitialWindow();

		// Startup the game system
		GenerateWorld(true);

		_local_player = 0x10;
		MarkWholeScreenDirty();
		break;

	case SM_NEWGAME: // Make new world
		_game_mode = GM_NORMAL;

		// Copy in game options
		_opt_mod_ptr = &_opt;
		memcpy(&_opt, &_new_opt, sizeof(_opt));

		GfxLoadSprites();

		// Reinitialize windows
		InitWindowSystem();
		LoadStringWidthTable();

		SetupColorsAndInitialWindow();
		
		// Randomize world
		GenerateWorld(false);
		
		// Create a single player
		DoStartupNewPlayer(false);
		_local_player = 0;
		MarkWholeScreenDirty();
		break;

	case SM_LOAD: // Load game
		_game_mode = GM_NORMAL;
		_local_player = 0;
		
		{
			char *s = strrchr(_file_to_saveload, '.');
			SaveOrLoad(_file_to_saveload, s && strcmp(s, ".sv1") == 0 ? 2 : 0  );
		}

		_opt_mod_ptr = &_opt;

		break;

	case SM_MENU: // Switch to game menu
		_game_mode = GM_MENU;

		_opt_mod_ptr = &_new_opt;
		GfxLoadSprites();
		LoadStringWidthTable();
		// Setup main window
		InitWindowSystem();
		SetupColorsAndInitialWindow();

		// Generate a world.
		if (!SaveOrLoad("opntitle.dat", 0 ))
			GenerateWorld(false);
		
		_opt.currency = _new_opt.currency;

		_local_player = 0;
		MarkWholeScreenDirty();
		break;

	case SM_SAVE: // Save game
		if (!SaveOrLoad(_file_to_saveload, true))
			ShowErrorMessage(INVALID_STRING_ID, STR_4007_GAME_SAVE_FAILED, 0, 0);
		else
			DeleteWindowById(WC_SAVELOAD, 0);
		break;

	case SM_GENRANDLAND:
		GenerateWorld(false);
		// XXX: set date
		_local_player = 0x10;
		MarkWholeScreenDirty();
		break;

	case SM_SCREENSHOT:
		UndrawMouseCursor();
		ShowScreenshotResult(MakeScreenshot());
		break;

	case SM_BIG_SCREENSHOT:
		ShowScreenshotResult(MakeWorldScreenshot(-8160, 0, 8192 + 8160, 8192, 0));
		break;
	}

	_in_state_game_loop = false;
}


// State controlling game loop.
// The state must not be changed from anywhere
// but here.
// That check is enforced in DoCommand.
void StateGameLoop()
{
	_in_state_game_loop = true;
	_frame_counter++;
	
	if (_game_mode == GM_EDITOR) {
		RunTileLoop();
		CallVehicleTicks();
		CallLandscapeTick();
		CallWindowTickEvent();
		NewsLoop();
	} else {
		AnimateAnimatedTiles();
		IncreaseDate();
		RunTileLoop();
		CallVehicleTicks();
		CallLandscapeTick();

		if (!disable_computer)
			RunOtherPlayersLoop();

		CallWindowTickEvent();
		NewsLoop();
	}
	_in_state_game_loop = false;
}

static void DoAutosave()
{
	char buf[20];
	int n;

	n = _autosave_ctr;
	_autosave_ctr = (_autosave_ctr + 1) & 3;
	sprintf(buf, "autosave%d.sav", n);
	if (!SaveOrLoad(buf, 1))
		ShowErrorMessage(INVALID_STRING_ID, STR_AUTOSAVE_FAILED, 0, 0);
}

//
void GameLoop()
{
	if (_do_autosave) {
		_do_autosave = false;
		DoAutosave();
		RedrawAutosave();
	}

	if (_switch_mode != SM_NONE) {
		int m = _switch_mode;
		_switch_mode = SM_NONE;
		SwitchMode(m);
	}
	IncreaseSpriteLRU();
	InteractiveRandom();

	if (_scroller_click_timeout > 3)
		_scroller_click_timeout -= 3;
	else
		_scroller_click_timeout = 0;

	_caret_timer += 3;
	_timer_counter+=8;
	CursorTick();

	NetworkPoll();
	RecorderPoll();

	MouseLoop();

	if (_game_paused == 0) {
		StateGameLoop();
		MoveAllTextEffects();
		if (_display_opt & DO_FULL_ANIMATION)
			DoPaletteAnimations();
	}

	if (_game_mode != GM_MENU)
		MusicLoop();
}

void BeforeSaveGame()
{
	Window *w = FindWindowById(WC_MAIN_WINDOW, 0);

	_saved_scrollpos_x = WP(w,vp_d).scrollpos_x;
	_saved_scrollpos_y = WP(w,vp_d).scrollpos_y;
	_saved_scrollpos_zoom = w->viewport->zoom;
}

void AfterLoadGame()
{
	Window *w;
	ViewPort *vp;

	// convert road side to my format.
	if (_opt.road_side) _opt.road_side = 1;

	// Load the sprites
	GfxLoadSprites();

	// Update current year
	{
		YearMonthDay ymd;
		ConvertDayToYMD(&ymd, _date);
		_cur_month = ymd.month;
		_cur_year = ymd.year;
	}

	// reinit the landscape variables (landscape might have changed)
	InitializeLandscapeVariables(true);
	
	// Update all vehicles
	AfterLoadVehicles();
	UpdateAllStationVirtCoord();

	// Setup city coords
	AfterLoadCity();
	UpdateAllSignVirtCoords();

	// Initialize windows
	InitWindowSystem();
	SetupColorsAndInitialWindow();

	w = FindWindowById(WC_MAIN_WINDOW, 0);

	WP(w,vp_d).scrollpos_x = _saved_scrollpos_x;
	WP(w,vp_d).scrollpos_y = _saved_scrollpos_y;
	
	vp = w->viewport;
	vp->zoom = _saved_scrollpos_zoom;
	vp->virtual_width = vp->width << vp->zoom;
	vp->virtual_height = vp->height << vp->zoom;

	// in case the player doesn't exist, create one
	if (_players[0].name_1 == 0)
		DoStartupNewPlayer(false);


	DoZoomInOut(ZOOM_NONE); // update button status
	
	MarkWholeScreenDirty();
}


void DebugProc(int i)
{
	switch(i) {
	case 0:
		*(byte*)0 = 0;
		break;
	case 1:
		SubtractMoneyFromPlayer(-10000000);
		MarkWholeScreenDirty();
		break;
	case 2:
		UpdateAllStationVirtCoord();
		break;
	}
}

void PauseGame()
{
	_game_paused++;
	InvalidateWindow(WC_STATUS_BAR, 0);
}

void UnpauseGame()
{
	_game_paused--;
	InvalidateWindow(WC_STATUS_BAR, 0);
}

