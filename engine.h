#ifndef ENGINE_H
#define ENGINE_H

typedef struct RailVehicleInfo {
	byte image_index;
	byte flags; /* 1=multihead engine, 2=wagon */
	byte base_cost;
	uint16 max_speed;
	uint16 power;
	byte weight;
	byte running_cost_base;
	byte running_cost_class;
	byte capacity;
	byte cargo_type; 
} RailVehicleInfo;


typedef struct Engine {
	uint16 intro_date;
	uint16 age;
	uint16 reliability;
	uint16 reliability_spd_dec;
	uint16 reliability_start, reliability_max, reliability_final;
	uint16 duration_phase_1, duration_phase_2, duration_phase_3;
	byte lifelength;
	byte flags;
	byte preview_player;
	byte preview_wait;
	byte railtype;
	byte player_avail;
} Engine;


enum {
	RVI_MULTIHEAD = 1,
	RVI_WAGON = 2,
};


void StartupEngines();

void DrawTrainEngine(int x, int y, int engine, uint32 image_ormod);
void DrawRoadVehEngine(int x, int y, int engine, uint32 image_ormod);
void DrawShipEngine(int x, int y, int engine, uint32 image_ormod);
void DrawAircraftEngine(int x, int y, int engine, uint32 image_ormod);

void DrawTrainEngineInfo(int engine, int x, int y, int maxw);
void DrawRoadVehEngineInfo(int engine, int x, int y, int maxw);
void DrawShipEngineInfo(int engine, int x, int y, int maxw);
void DrawAircraftEngineInfo(int engine, int x, int y, int maxw);

void AcceptEnginePreview(Engine *e, int player);

void LoadCustomEngineNames();
void DeleteCustomEngineNames();


enum {
	NUM_NORMAL_RAIL_ENGINES = 54,
	NUM_MONORAIL_ENGINES = 30,
	NUM_MAGLEV_ENGINES = 32,
	NUM_TRAIN_ENGINES = NUM_NORMAL_RAIL_ENGINES + NUM_MONORAIL_ENGINES + NUM_MAGLEV_ENGINES,
	NUM_ROAD_ENGINES = 88,
	NUM_SHIP_ENGINES = 11,
	NUM_AIRCRAFT_ENGINES = 41,
	TOTAL_NUM_ENGINES = NUM_NORMAL_RAIL_ENGINES+NUM_MONORAIL_ENGINES+NUM_MAGLEV_ENGINES+NUM_ROAD_ENGINES+NUM_SHIP_ENGINES+NUM_AIRCRAFT_ENGINES,
	AIRCRAFT_ENGINES_INDEX = NUM_TRAIN_ENGINES + NUM_ROAD_ENGINES + NUM_SHIP_ENGINES,
	SHIP_ENGINES_INDEX = NUM_TRAIN_ENGINES + NUM_ROAD_ENGINES,
	ROAD_ENGINES_INDEX = NUM_TRAIN_ENGINES,
};
VARDEF Engine _engines[TOTAL_NUM_ENGINES];
VARDEF StringID _engine_name_strings[TOTAL_NUM_ENGINES];

extern const RailVehicleInfo _rail_vehicle_info[];


#endif
