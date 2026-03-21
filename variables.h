//enum { DPARAM_SIZE = 32 };


// ********* START OF SAVE REGION

#if !defined(MAX_PATH)
# define MAX_PATH 260
#endif

// Prices and also the fractional part.
VARDEF Prices _price;
VARDEF uint16 _price_frac[NUM_PRICES];

VARDEF uint32 _cargo_payment_rates[NUM_CARGO];
VARDEF uint16 _cargo_payment_rates_frac[NUM_CARGO];

typedef struct {
	GameDifficulty diff;
	byte diff_level;
	byte currency;
	bool kilometers;
	byte city_name;
	byte landscape;
	byte snow_line;
	byte autosave;
	byte road_side;
} GameOptions;

// These are the options for the current game
VARDEF GameOptions _opt;

// These are the options for the new game
VARDEF GameOptions _new_opt;

// Current date
VARDEF uint16 _date;
VARDEF uint16 _date_fract;

// Amount of game ticks
VARDEF uint16 _tick_counter;

// Used when calling OnNewDay
VehicleID _vehicle_id_ctr_day;

// Skip aging of cargo?
VARDEF byte _age_cargo_skip_counter;

// Available aircraft types
VARDEF byte _avail_aircraft;

// Position in tile loop
VARDEF TileIndex _cur_tileloop_tile;

// Also save scrollpos_x, scrollpos_y and zoom
VARDEF uint16 _disaster_delay;

// Determines what station to operate on in the
//  tick handler.
VARDEF uint16 _station_tick_ctr;

VARDEF uint32 _random_seed_1, _random_seed_2;
// Iterator through all towns in OnTick_Town
VARDEF byte _cur_town_ctr;

VARDEF uint _cur_player_tick_index;
VARDEF uint _next_competitor_start;

// Determines how often to run the tree loop
VARDEF byte _trees_tick_ctr;

// Keep track of current game position
VARDEF int _saved_scrollpos_x;
VARDEF int _saved_scrollpos_y;
VARDEF byte _saved_scrollpos_zoom;


// ********* END OF SAVE REGION

typedef struct Patches {
	bool vehicle_speed;			// show vehicle speed
	bool build_on_slopes;		// allow building on slopes
	bool mammoth_trains;		// allow very long trains
	bool join_stations;			// allow joining of train stations
	bool full_load_any;			// new full load calculation, any cargo must be full
	byte station_spread;		// amount a station may spread
	bool noinflation;				// disable inflation
	bool no_train_service;	// never automatically send trains to service
	bool selectgoods;       // only send the goods to station if a train has been there
	bool longbridges;				// allow 100 tile long bridges
	bool gotodepot;					// allow goto depot in orders
	bool crossing_tunnels;	// allow tunnels that cross each other
	bool multiple_industry_per_town;	// allow many industries of the same type per town
	bool same_industry_close;	// allow same type industries to be built close to each other
	uint16 lost_train_days;	// if a train doesn't switch order in this amount of days, a train is lost warning is shown
	int32 train_income_warn; // if train is generating less income than this last 2 years, show a warning
	bool status_long_date;		// always show long date in status bar
	bool signal_side;				// show signals on right side
	bool show_finances;			// show finances at end of year
	bool new_nonstop;				// ttdpatch compatible nonstop handling
	bool roadveh_queue;			// buggy road vehicle queueing
	bool autoscroll;				// scroll when moving mouse to the edge.
} Patches;

VARDEF Patches _patches;

// Which options struct does options modify?
VARDEF GameOptions *_opt_mod_ptr;

// NOSAVE: Used in palette animations only, not really important.
VARDEF int _timer_counter;


// NOSAVE: can be determined from _date
VARDEF byte _cur_year;
VARDEF byte _cur_month;

VARDEF int _starting_date;

// NOSAVE: can be determined from player structs
VARDEF byte _player_colors[8];

VARDEF bool _in_state_game_loop;
VARDEF uint32 _frame_counter;

VARDEF bool _is_ai_player; // current player is an AI player?

VARDEF bool _do_autosave;
VARDEF int _autosave_ctr;

VARDEF byte _local_player;
VARDEF byte _num_human_players;
VARDEF byte _display_opt;
VARDEF bool _game_paused;
VARDEF int _caret_timer;
VARDEF uint16 _news_display_opt;
VARDEF byte _game_mode;

VARDEF StringID _error_message;
VARDEF StringID _error_message_2;
VARDEF uint32 _decode_parameters[10];
VARDEF byte _current_player;

VARDEF uint32 _pressed_key; // Low 8 bits = ASCII, High 16 bits = keycode
VARDEF bool _ctrl_pressed;  // Is Ctrl pressed?
VARDEF bool _fullscreen;
VARDEF bool _double_size;
VARDEF uint _display_hz;
VARDEF bool _force_full_redraw;

// IN/OUT parameters to commands
VARDEF byte _yearly_expenses_type;
VARDEF TileIndex _terraform_err_tile;
VARDEF uint _build_tunnel_endtile;
VARDEF bool _generating_world;
VARDEF int _new_town_size;

// Used when switching from the intro menu.
VARDEF byte _switch_mode;
VARDEF bool _exit_game;
VARDEF byte _file_to_saveload[MAX_PATH];

VARDEF char _ini_videodriver[16], _ini_musicdriver[16], _ini_sounddriver[16];

VARDEF bool _cache_sprites;

typedef struct {
	char *name;
	char *file;
} DynLangEnt;

// Used for dynamic language support
typedef struct {
	int num; // number of languages
	int curr; // currently selected language index
	char curr_file[32]; // currently selected language file
	StringID dropdown[32 + 1]; // used in settings dialog
	DynLangEnt ent[32];
} DynamicLanguages;

VARDEF DynamicLanguages _dynlang;

int _num_resolutions;
uint16 _resolutions[32][2];
uint16 _cur_resolution[2];

// NOSAVE: These can be recalculated from InitializeLandscapeVariables
typedef struct {
	StringID names_s[NUM_CARGO];
	StringID names_p[NUM_CARGO];
	StringID names_long_s[NUM_CARGO];
	StringID names_long_p[NUM_CARGO];
	StringID names_short[NUM_CARGO];
	byte weights[NUM_CARGO];
	SpriteID sprites[NUM_CARGO];
	byte transit_days_1[NUM_CARGO];
	byte transit_days_2[NUM_CARGO];
	byte ai_railwagon[3][NUM_CARGO];
	byte ai_roadveh_start[NUM_CARGO];
	byte ai_roadveh_count[NUM_CARGO];
} CargoConst;

VARDEF CargoConst _cargoc;

typedef byte CityNameGenerator(byte *buf, uint32 seed);
extern CityNameGenerator * const _city_name_generators[];

#define SET_DPARAM32(n, v) (_decode_parameters[n] = (v))
#define SET_DPARAMX32(s, n, v) ((s)[n] = (v))
#define GET_DPARAM32(n) (_decode_parameters[n])

#define SET_DPARAM(n, v) (_decode_parameters[n] = (v))
#define SET_DPARAMX(s, n, v) ((s)[n] = (v))
#define GET_DPARAM(n) (_decode_parameters[n])

static void FORCEINLINE SET_DPARAM64(int n, int64 v)
{
	_decode_parameters[n] = (uint32)v;
	_decode_parameters[n+1] = (uint32)((uint64)v >> 32);
}

#if defined(TTD_LITTLE_ENDIAN)
#define SET_DPARAMX16(s, n, v) ( ((uint16*)(s+n))[0] = (v))
#define SET_DPARAMX8(s, n, v) ( ((uint8*)(s+n))[0] = (v))
#define GET_DPARAMX16(s, n) ( ((uint16*)(s+n))[0])
#define GET_DPARAMX8(s, n) ( ((uint8*)(s+n))[0])
#elif defined(TTD_BIG_ENDIAN)
#define SET_DPARAMX16(s, n, v) ( ((uint16*)(s+n))[1] = (v))
#define SET_DPARAMX8(s, n, v) ( ((uint8*)(s+n))[3] = (v))
#define GET_DPARAMX16(s, n) ( ((uint16*)(s+n))[1])
#define GET_DPARAMX8(s, n) ( ((uint8*)(s+n))[3])
#endif

#define SET_DPARAM16(n, v) SET_DPARAMX16(_decode_parameters, n, v)
#define SET_DPARAM8(n, v)  SET_DPARAMX8(_decode_parameters, n, v)
#define GET_DPARAM16(n)    GET_DPARAMX16(_decode_parameters, n)
#define GET_DPARAM8(n)     GET_DPARAMX8(_decode_parameters, n)

#define COPY_IN_DPARAM(offs,src,num) memcpy(_decode_parameters + offs, src, sizeof(uint32) * (num))
#define COPY_OUT_DPARAM(dst,offs,num) memcpy(dst,_decode_parameters + offs, sizeof(uint32) * (num))

#define INJECT_DPARAM(n) InjectDparam(n);

#define SET_EXPENSES_TYPE(x) if (x) _yearly_expenses_type=x;

/* landscape.c */
extern const byte _tileh_to_sprite[32];
extern byte _map_type_and_height[TILES_X * TILES_Y];
extern byte _map5[TILES_X * TILES_Y];
extern byte _map3_lo[TILES_X * TILES_Y];
extern byte _map3_hi[TILES_X * TILES_Y];
extern byte _map_owner[TILES_X * TILES_Y];
extern byte _map2[TILES_X * TILES_Y];
extern byte _map_extra_bits[TILES_X * TILES_Y/4];

extern const TileTypeProcs * const _tile_type_procs[16];

/* station_cmd.c */
extern const byte _airport_size_x[3];
extern const byte _airport_size_y[3];
extern const int16 _tileoffs_by_dir[4];

/* misc */
VARDEF byte str_buffr[512];
VARDEF char _screenshot_name[128];
VARDEF char _userstring[128];
VARDEF byte _vehicle_design_names;

VARDEF SignStruct _sign_list[40];
VARDEF SignStruct *_new_sign_struct;


/* Debugging levels */
VARDEF int _debug_spritecache_level;
VARDEF int _debug_misc_level;

void CDECL debug(const char *s, ...);
#define DEBUG(name, level) if (level == 0 || _debug_ ## name ## _level >= level) debug


