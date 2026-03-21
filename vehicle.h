#ifndef VEHICLE_H
#define VEHICLE_H

typedef struct VehicleRail {
	uint16 last_speed;
	uint16 crash_anim_pos;
	uint16 days_since_order_progr;
	byte track;
	byte force_proceed;
	byte railtype;
} VehicleRail;

typedef struct VehicleAir {
	uint16 crashed_counter;
	byte unk0;
	byte targetairport;
	byte state;
	
} VehicleAir;

typedef struct VehicleRoad {
	byte state;
	byte frame;
	uint16 unk2;
	byte overtaking;
	byte overtaking_ctr;
	uint16 crashed_ctr;
	byte reverse_ctr;
} VehicleRoad;

typedef struct VehicleSpecial {
	uint16 unk0;
	byte unk2;
} VehicleSpecial;

typedef struct VehicleDisaster {
	uint16 image_override;
	uint16 unk2;
} VehicleDisaster;

typedef struct VehicleShip {
	byte state;
} VehicleShip;

struct Vehicle {
	byte type;				// type, ie roadven,train,ship,aircraft,special
	byte subtype;			// subtype

	uint16 index;			// NOSAVE: Index in vehicle array
	uint16 next_in_chain; // Next vehicle index for chained vehicles

	StringID string_id; // Displayed string

	byte unitnumber;	// unit number, for display purposes only
	byte owner;				// which player owns the vehicle?

	TileIndex tile;		// Current tile index
	TileIndex dest_tile; // Heading for this tile

	int16 x_pos;			// coordinates
	int16 y_pos;
	byte z_pos;
	byte direction;		// facing

	uint16 cur_image; // sprite number for this vehicle
	byte spritenum;		// currently displayed sprite index
	byte sprite_width;// width of vehicle sprite
	byte sprite_height;// height of vehicle sprite
	byte z_height;		// z-height of vehicle sprite
	int8 x_offs;			// x offset for vehicle sprite
	int8 y_offs;			// y offset for vehicle sprite
	uint16 engine_type;

	uint16 max_speed;	// maximum speed
	uint16 cur_speed;	// current speed
	byte subspeed;		// fractional speed
	byte acceleration; // used by train & aircraft
	byte progress;

	byte vehstatus;		// Status
	byte last_station_visited;
	
	byte cargo_type;	// type of cargo this vehicle is carrying
	byte cargo_days; // how many days have the pieces been in transit
	byte cargo_source;// source of cargo
	uint16 cargo_cap;	// total capacity
	uint16 cargo_count;// how many pieces are used

	byte day_counter; // increased by one for each day
	byte tick_counter;// increased by one for each tick

	// related to the current order
	byte cur_order_index;
	byte num_orders;
	byte next_order;
	byte next_order_param;
	uint16 *schedule_ptr;

	// Boundaries for the current position in the world and a next hash link.
	// NOSAVE: All of those can be updated with VehiclePositionChanged()
	int16 left_coord, top_coord, right_coord, bottom_coord;
	uint16 next_hash;

	// Related to age and service time
	uint16 age;				// Age in days
	uint16 max_age;		// Maximum age
	uint16 date_of_last_service;
	uint16 service_interval;
	uint16 reliability;
	uint16 reliability_spd_dec;
	byte breakdown_ctr;
	byte breakdown_delay;
	byte breakdowns_since_last_service;
	byte breakdown_chance;
	byte build_year;

	uint16 load_unload_time_rem;
	
	int32 profit_this_year;
	int32 profit_last_year;
	uint32 value;

	union {
		VehicleRail rail;
		VehicleAir air;
		VehicleRoad road;
		VehicleSpecial special;
		VehicleDisaster disaster;
		VehicleShip ship;
	} u;
};

struct Depot {
	TileIndex xy;
	uint16 city_index;
};

// train checkpoint
struct Checkpoint {
	TileIndex xy;
	uint16 city_or_string; // if this is 0xC000, it's a string id, otherwise a city.
	ViewportSign sign;
	byte deleted;					 // this is a delete counter. when it reaches 0, the checkpoint struct is deleted.
};

enum {
	VEH_Train = 0x10,
	VEH_Road = 0x11,
	VEH_Ship = 0x12,
	VEH_Aircraft = 0x13,
	VEH_Special = 0x14,
	VEH_Disaster = 0x15,
};

/* Order types */
enum {
	OT_NOTHING = 0,
	OT_GOTO_STATION = 1,
	OT_GOTO_DEPOT = 2,
	OT_LOADING = 3,
	OT_LEAVESTATION = 4,
	OT_DUMMY = 5,
	OT_GOTO_CHECKPOINT = 6,

	OT_MASK = 0x1F,
};

/* Order flags */
enum {
	OF_UNLOAD = 0x20,
	OF_FULL_LOAD = 0x40, // Also used when to force an aircraft into a depot.
	OF_NON_STOP = 0x80,
	OF_MASK = 0xE0,
};


enum VehStatus {
	VS_HIDDEN = 1,
	VS_STOPPED = 2,
	VS_UNCLICKABLE = 4,
	VS_DEFPAL = 0x8,
	VS_TRAIN_SLOWING = 0x10,
	VS_DISASTER = 0x20,
	VS_AIRCRAFT_BROKEN = 0x40,
	VS_CRASHED = 0x80,
};



/* Effect vehicle types */
enum {
	EV_INDUSTRYSMOKE = 0,
	EV_1 = 1,

	EV_SMOKE_1 = 2,
	EV_SMOKE_2 = 3,
	EV_SMOKE_3 = 4,

	EV_CRASHED_SMOKE = 5,
	EV_BREAKDOWN_SMOKE = 6,

	EV_DEMOLISH = 7,
	EV_ROADWORK = 8,

	EV_INDUSTRY_SMOKE = 9,

};

typedef void VehicleTickProc(Vehicle *v);
typedef void *VehicleFromPosProc(Vehicle *v, void *data);

void BackupVehicleOrders(Vehicle *v, uint16 *order);
void RestoreVehicleOrders(Vehicle *v, uint16 *order);
Vehicle *AllocateVehicle();
Vehicle *ForceAllocateVehicle();
Vehicle *ForceAllocateSpecialVehicle();
void UpdateVehiclePosHash(Vehicle *v, int x, int y);
void InitializeVehicles();
void VehiclePositionChanged(Vehicle *v);
void AfterLoadVehicles();
Vehicle *GetLastVehicleInChain(Vehicle *v);
Vehicle *GetPrevVehicleInChain(Vehicle *v);
Vehicle *GetFirstVehicleInChain(Vehicle *v);
int CountVehiclesInChain(Vehicle *v);
void DeleteVehicle(Vehicle *v);
void *VehicleFromPos(TileIndex tile, void *data, VehicleFromPosProc *proc);
void CallVehicleTicks();
void DeleteVehicleSchedule(Vehicle *v);
Vehicle *IsScheduleShared(Vehicle *v);

Depot *AllocateDepot();
Checkpoint *AllocateCheckpoint();
void UpdateCheckpointSign(Checkpoint *cp);
void RedrawCheckpointSign(Checkpoint *cp);

void NormalizeTrainVehInDepot(TileIndex tile, int veh);

void InitializeTrains();
bool IsTrainDepotTile(TileIndex tile);
bool IsRoadDepotTile(TileIndex tile);

bool CanFillVehicle(Vehicle *v);

void ViewportAddVehicles(DrawPixelInfo *dpi);

void TrainEnterDepot(Vehicle *v, uint tile);

/* train_cmd.h */
int GetTrainImage(Vehicle *v, byte direction);
int GetAircraftImage(Vehicle *v, byte direction);
int GetRoadVehImage(Vehicle *v, byte direction);
int GetShipImage(Vehicle *v, byte direction);

Vehicle *CreateEffectVehicle(int x, int y, int z, int type);
Vehicle *CreateEffectVehicleAbove(int x, int y, int z, int type);
Vehicle *CreateEffectVehicleRel(Vehicle *v, int x, int y, int z, int type);

uint32 VehicleEnterTile(Vehicle *v, uint tile, int x, int y);

void VehicleInTheWayErrMsg(Vehicle *v);
Vehicle *FindVehicleBetween(TileIndex from, TileIndex to, byte z);
uint GetVehicleOutOfTunnelTile(Vehicle *v);

bool UpdateSignalsOnSegment(uint tile, byte direction);
void SetSignalsOnBothDir(uint tile, byte track);

void CheckClickOnVehicle(ViewPort *vp, int x, int y);
uint GetVehicleWeight(Vehicle *v);

void DecreaseVehicleValue(Vehicle *v);
void CheckVehicleBreakdown(Vehicle *v);
void AgeVehicle(Vehicle *v);

void DeleteCommandFromVehicleSchedule(uint cmd);

void BeginVehicleMove(Vehicle *v);
void EndVehicleMove(Vehicle *v);

bool IsAircraftHangarTile(TileIndex tile);
void ShowAircraftViewWindow(Vehicle *v);

void InvalidateVehicleOrderWidget(Vehicle *v);

bool IsShipDepotTile(TileIndex tile);
uint GetFreeUnitNumber(byte type);

int LoadUnloadVehicle(Vehicle *v);
int GetDepotByTile(uint tile);
uint GetCheckpointByTile(uint tile);

void DoDeleteDepot(uint tile);

typedef struct GetNewVehiclePosResult {
	int x,y;
	uint old_tile;
	uint new_tile;
} GetNewVehiclePosResult;

/* returns true if staying in the same tile */
bool GetNewVehiclePos(Vehicle *v, GetNewVehiclePosResult *gp);
byte GetDirectionTowards(Vehicle *v, int x, int y);

#define BEGIN_ENUM_WAGONS(v) for(;;) {
#define END_ENUM_WAGONS(v) if (v->next_in_chain == INVALID_VEHICLE) break; v = &_vehicles[v->next_in_chain]; }

/* vehicle.c */
enum {
	NUM_NORMAL_VEHICLES = 690,
	NUM_SPECIAL_VEHICLES = 170,
	NUM_VEHICLES = NUM_NORMAL_VEHICLES + NUM_SPECIAL_VEHICLES
};	

VARDEF Vehicle _vehicles[NUM_VEHICLES];

VARDEF uint16 _order_array[5000];
VARDEF uint16 *_ptr_to_next_order;

VARDEF Depot _depots[255];

// 128 checkpoints
VARDEF Checkpoint _checkpoints[128];

// NOSAVE: Can be regenerated by inspecting the vehicles.
VARDEF VehicleID _vehicle_position_hash[0x1000];

// NOSAVE: Return values from various commands.
VARDEF VehicleID _new_train_id;
VARDEF VehicleID _new_wagon_id;
VARDEF VehicleID _new_aircraft_id;
VARDEF VehicleID _new_ship_id;
VARDEF VehicleID _new_roadveh_id;
VARDEF uint16 _aircraft_refit_capacity;
VARDEF byte _cmd_build_rail_veh_score;
VARDEF byte _cmd_build_rail_veh_var1;

// NOSAVE: Player specific info
VARDEF TileIndex _last_built_train_depot_tile;
VARDEF TileIndex _last_built_road_depot_tile;
VARDEF TileIndex _last_built_aircraft_depot_tile;
VARDEF TileIndex _last_built_ship_depot_tile;
VARDEF TileIndex _backup_orders_tile;
VARDEF uint16 _backup_orders_data[41];

#define INVALID_VEHICLE 0xffff

#endif /* VEHICLE_H */
