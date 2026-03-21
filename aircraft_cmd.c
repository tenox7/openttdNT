#include "stdafx.h"
#include "ttd.h"
#include "vehicle.h"
#include "engine.h"
#include "command.h"
#include "station.h"
#include "news.h"
#include "gfx.h"
#include "player.h"

static const SpriteID _aircraft_sprite[] = {
	0x0EB5, 0x0EBD, 0x0EC5, 0x0ECD,
	0x0ED5, 0x0EDD, 0x0E9D, 0x0EA5,
	0x0EAD, 0x0EE5, 0x0F05, 0x0F0D,
	0x0F15, 0x0F1D, 0x0F25, 0x0F2D,
	0x0EED, 0x0EF5, 0x0EFD, 0x0F35,
	0x0E9D, 0x0EA5, 0x0EAD, 0x0EB5,
	0x0EBD, 0x0EC5
};

int GetAircraftImage(Vehicle *v, byte direction)
{
	return direction + _aircraft_sprite[v->spritenum];
}

const byte _aircraft_cost_table[NUM_AIRCRAFT_ENGINES] = {
	14, 15, 16, 75, 15, 18, 17, 18,
	19, 20, 16, 18, 17, 30, 18, 19,
	27, 25, 20, 19, 18, 26, 16, 17,
	16, 16, 17, 18, 20, 21, 19, 24,
	80, 13, 18, 25, 32, 80, 15, 17,
	15
};

const byte _aircraft_speed[NUM_AIRCRAFT_ENGINES] = {
	37, 37, 74, 181, 37, 74, 74, 74,
	74, 74, 74, 74, 74, 74, 74, 74,
	74, 74, 74, 74, 74, 74, 74, 74,
	74, 74, 74, 74, 74, 74, 181, 74,
	181, 37, 37, 74, 74, 181, 25, 40,
	25
};

const byte _aircraft_running_cost[NUM_AIRCRAFT_ENGINES] = {
	85, 100, 130, 250, 98, 240, 150, 245,
	192, 190, 135, 240, 155, 253, 210, 220,
	230, 225, 235, 220, 170, 210, 125, 145,
	130, 149, 170, 210, 230, 220, 160, 248,
	251, 85, 100, 140, 220, 255, 81, 77,
	80
};

const byte _aircraft_num_mail[NUM_AIRCRAFT_ENGINES] = {
	 4,  8, 10, 20,  6, 30, 15, 30,
	40, 25, 10, 35, 15, 50, 25, 25,
	40, 35, 30, 25, 20, 20, 10, 10,
	10, 10, 18, 25, 60, 65, 45, 80,
	45,  5,  9, 12, 40, 30, 15, 20,
	10
};

const uint16 _aircraft_num_pass[NUM_AIRCRAFT_ENGINES] = {
	 25, 65, 90,100, 30,200,100,150,
	220,230, 95,170,110,300,200,240,
	260,240,260,210,160,220, 80, 85,
	 75, 85, 65,110,180,150, 85,400,
	130, 25, 60, 90,200,100, 40, 55,
	 40
};

const byte _aircraft_sounds[NUM_AIRCRAFT_ENGINES] = {
	 6,  6,  7, 59,  6,  7,  7,  7,
	 7,  7,  7,  7,  7, 61,  7,  7,
	 7,  7,  7,  7,  7,  7,  7,  7,
	 7,  7,  7,  7,  7,  7,  7, 61,
	59, 69, 70,  7, 61, 59,  7,  7,
	 7
};

const byte _aircraft_table_3[NUM_AIRCRAFT_ENGINES] = {
	 1,  0,  2,  8,  5,  6,  2,  2,
	 3,  3,  2,  2,  4,  7,  4,  4,
	 4,  3,  4,  4,  4,  4,  6,  2,
	11, 10, 15, 12, 13, 14, 16, 17,
	18, 20, 21, 22, 23, 24,  9, 19,
	25
};

// &1 = regular aircraft
// &2 = crashes easily on small airports
const byte _aircraft_subtype[NUM_AIRCRAFT_ENGINES] = {
	1,  1,  3,  3,  1,  3,  1,  3,
	3,  3,  3,  3,  3,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  1,  1,
	3,  3,  3,  3,  3,  3,  3,  3,
	3,  1,  1,  1,  3,  3,  0,  0,
	0
};

const byte _aircraft_acceleration[NUM_AIRCRAFT_ENGINES] = {
	18, 20, 35, 50, 20, 40, 35, 40,
	40, 40, 35, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 50, 40,
	40, 40, 40, 40, 40, 40, 40, 40,
	50, 18, 20, 40, 40, 50, 20, 20,
	20,
};

enum {
	AS_IN_HANGAR = 0,

	AS_AT_TERM_1 = 1,
	AS_AT_TERM_2 = 2,
	AS_AT_TERM_3 = 3,

	AS_HANGAR_TO_TERM_1 = 4,
	AS_HANGAR_TO_TERM_2 = 5,
	AS_HANGAR_TO_TERM_3 = 6,

	AS_LANDED_TO_TERM_1 = 7,
	AS_LANDED_TO_TERM_2 = 8,
	AS_LANDED_TO_TERM_3 = 9,

	AS_TERM_1_TO_HANGAR = 10,
	AS_TERM_2_TO_HANGAR = 11,
	AS_TERM_3_TO_HANGAR = 12,

	AS_TAKE_OFF_TERM_1 = 13,
	AS_TAKE_OFF_TERM_2 = 14,
	AS_TAKE_OFF_TERM_3 = 15,

	AS_TAKE_OFF_HANGAR = 16,
	AS_LAND_HANGAR = 17,
	AS_IN_FLIGHT = 18,
};

void DrawAircraftEngine(int x, int y, int engine, uint32 image_ormod)
{
	DrawSprite((6 + _aircraft_sprite[_aircraft_table_3[engine - AIRCRAFT_ENGINES_INDEX]]) | image_ormod, x, y);
	
	if ((_aircraft_subtype[engine - AIRCRAFT_ENGINES_INDEX]&1) == 0)
		DrawSprite(0xF3D, x, y-5);
}

void DrawAircraftEngineInfo(int engine, int x, int y, int maxw)
{
	engine -= AIRCRAFT_ENGINES_INDEX;

	SET_DPARAM32(0, ((_price.aircraft_base >> 3) * _aircraft_cost_table[engine]) >> 5);
	SET_DPARAM16(1, _aircraft_speed[engine] << 3);
	SET_DPARAM16(2, _aircraft_num_pass[engine]);
	SET_DPARAM16(3, _aircraft_num_mail[engine]);
	SET_DPARAM32(4, _aircraft_running_cost[engine] * _price.aircraft_running >> 8);
	
	DrawStringMultiCenter(x, y, STR_A02E_COST_MAX_SPEED_CAPACITY, maxw);
}

/* Allocate many vehicles */
bool AllocateVehicles(Vehicle **vl, int num) {
	int i;
	Vehicle *v;
	bool success = true;

	for(i=0; i!=num; i++) {
		vl[i] = v = AllocateVehicle();
		if (v == NULL) {
			success = false;
			break;
		}
		v->type = 1;
	}

	while (--i >= 0) {
		vl[i]->type = 0;
	}
	
	return success;
}

/* p1 = engine */
int32 CmdBuildAircraft(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	int32 value;
	Vehicle *vl[3], *v, *u, *w;
	byte unit_num;
	uint tile = TILE_FROM_XY(x,y);
	Engine *e;

	SET_EXPENSES_TYPE(EXPENSES_NEW_VEHICLES);

	value = _aircraft_cost_table[p1 - AIRCRAFT_ENGINES_INDEX] * (_price.aircraft_base>>3)>>5;

	if (flags & DC_QUERY_COST)
		return value;

	// allocate 2 or 3 vehicle structs, depending on type
	if (!AllocateVehicles(vl, (_aircraft_subtype[p1 - AIRCRAFT_ENGINES_INDEX]&1) == 0 ? 3 : 2) ||
			_ptr_to_next_order >= endof(_order_array))
					return_cmd_error(STR_00E1_TOO_MANY_VEHICLES_IN_GAME);

	unit_num = GetFreeUnitNumber(VEH_Aircraft);
	if (unit_num > 40)
		return_cmd_error(STR_00E1_TOO_MANY_VEHICLES_IN_GAME);

	if (flags & DC_EXEC) {
		v = vl[0];
		u = vl[1];

		v->unitnumber = unit_num;
		v->type = u->type = VEH_Aircraft;
		v->direction = 3;

		v->owner = u->owner = _current_player;

		v->tile = tile;
//		u->tile = 0;

		x = GET_TILE_X(tile)*16 + 5;
		y = GET_TILE_Y(tile)*16 + 3;

		v->x_pos = u->x_pos = x;
		v->y_pos = u->y_pos = y;

		u->z_pos = (byte)GetSlopeZ(x, y);
		v->z_pos = u->z_pos + 1;

		v->x_offs = v->y_offs = -1;
//		u->delta_x = u->delta_y = 0;

		v->sprite_width = v->sprite_height = 2;
		v->z_height = 5;

		u->sprite_width = u->sprite_height = 2;
		u->z_height = 1;

		v->vehstatus = VS_HIDDEN | VS_STOPPED | VS_DEFPAL;
		u->vehstatus = VS_HIDDEN | VS_UNCLICKABLE | VS_DISASTER;

		v->spritenum = _aircraft_table_3[p1 - AIRCRAFT_ENGINES_INDEX];
//		v->cargo_count = u->number_of_pieces = 0;

		v->cargo_cap = _aircraft_num_pass[p1 - AIRCRAFT_ENGINES_INDEX];
		u->cargo_cap = _aircraft_num_mail[p1 - AIRCRAFT_ENGINES_INDEX];
		
		v->cargo_type = CT_PASSENGERS;
		u->cargo_type = CT_MAIL;

		v->string_id = STR_SV_AIRCRAFT_NAME;
//		v->next_order_param = v->next_order = 0;

//		v->load_unload_time_rem = 0;
//		v->progress = 0;
		v->last_station_visited = 0xFF;
//		v->destination_coords = 0;

		v->max_speed = _aircraft_speed[p1 - AIRCRAFT_ENGINES_INDEX];
		v->acceleration = _aircraft_acceleration[p1 - AIRCRAFT_ENGINES_INDEX];
		v->engine_type = (byte)p1;

		v->subtype = (_aircraft_subtype[p1 - AIRCRAFT_ENGINES_INDEX]&1) == 0 ? 0 : 2;
		v->value = value;
		
		u->subtype = 4;

		e = &_engines[p1];
		v->reliability = e->reliability;
		v->reliability_spd_dec = e->reliability_spd_dec;
		v->max_age = e->lifelength * 366;

		_new_aircraft_id = v->index;

		*(v->schedule_ptr = _ptr_to_next_order++) = 0;

		v->u.air.state = AS_IN_HANGAR;
		v->u.air.targetairport = _map2[tile];
		v->next_in_chain = u->index;

		v->service_interval = 100;

		v->date_of_last_service = _date;
		v->build_year = _cur_year;

		v->cur_image = u->cur_image = 0xEA0;

		VehiclePositionChanged(v);
		VehiclePositionChanged(u);

		// Aircraft with 3 vehicles?
		if (v->subtype == 0) {
			w = vl[2];
			
			u->next_in_chain = w->index;
			
			w->type = VEH_Aircraft;
			w->direction = 0;
			w->owner = _current_player;
			w->x_pos = v->x_pos;
			w->y_pos = v->y_pos;
			w->z_pos = v->z_pos + 5;
			w->x_offs = w->y_offs = -1;
			w->sprite_width = w->sprite_height = 2;
			w->z_height = 1;
			w->vehstatus = VS_HIDDEN | VS_UNCLICKABLE;
			w->subtype = 6;
			w->cur_image = 0xF3D;
			VehiclePositionChanged(w);
		}

		InvalidateWindow(WC_VEHICLE_DEPOT, v->tile);
		InvalidateWindow(WC_AIRCRAFT_LIST, v->owner);
		InvalidateWindow(WC_COMPANY, v->owner);
	}
	
	return value;
}

bool IsAircraftHangarTile(TileIndex tile) {
	return IS_TILETYPE(tile, MP_STATION) &&
				(_map5[tile] == 0x20 || _map5[tile] == 0x41);
}

static bool CheckStoppedInHangar(Vehicle *v) {
	if (!(v->vehstatus&VS_STOPPED) ||
			!IsAircraftHangarTile(v->tile)) {
		_error_message = STR_A01B_AIRCRAFT_MUST_BE_STOPPED;
		return false;
	}

	return true;
}

void DoDeleteAircraft(Vehicle *v)
{
	Vehicle *u;

	DeleteWindowById(WC_VEHICLE_VIEW, v->index);
	InvalidateWindow(WC_AIRCRAFT_LIST, v->owner);
	InvalidateWindow(WC_COMPANY, v->owner);
	
	DeleteVehicle(v);
	if (v->next_in_chain != INVALID_VEHICLE) {
		u = &_vehicles[v->next_in_chain];
		DeleteVehicle(u);
		if (u->next_in_chain != INVALID_VEHICLE)
			DeleteVehicle(&_vehicles[u->next_in_chain]);
	}
}

// p1 = vehicle
int32 CmdSellAircraft(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v;

	SET_EXPENSES_TYPE(EXPENSES_NEW_VEHICLES);
	
	v = &_vehicles[p1];

	if (!CheckOwnership(v->owner) || !CheckStoppedInHangar(v))
		return CMD_ERROR;
	
	if (flags & DC_EXEC) {
		// Invalidate depot
		InvalidateWindow(WC_VEHICLE_DEPOT, v->tile);

		DoDeleteAircraft(v);
	
	}
	
	return -(int32)v->value;
}

// p1 = vehicle
int32 CmdStartStopAircraft(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v;

	v = &_vehicles[p1];

	if (!CheckOwnership(v->owner))
		return CMD_ERROR;

	if (v->u.air.state >= AS_HANGAR_TO_TERM_1 && v->u.air.unk0 >= 13)
		return_cmd_error(STR_A017_AIRCRAFT_IS_IN_FLIGHT);

	if (flags & DC_EXEC) {
		v->vehstatus ^= VS_STOPPED;
		InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
		InvalidateWindow(WC_VEHICLE_DEPOT, v->tile);
	}

	return 0;
}

// p1 = vehicle
int32 CmdSendAircraftToHangar(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v;
	Station *st;

	v = &_vehicles[p1];

	if (!CheckOwnership(v->owner))
		return CMD_ERROR;

	if ((v->next_order&OT_MASK) == OT_GOTO_DEPOT) {
		if (flags & DC_EXEC) {
			v->next_order = OT_DUMMY;
			InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);			
		}
	} else {
		st = DEREF_STATION(v->u.air.targetairport);
		if (st->xy == 0 || st->airport_tile == 0 ||
				st->airport_flags&(1<<6))
					return CMD_ERROR;

		if (flags & DC_EXEC) {
			v->next_order = OF_NON_STOP | OF_FULL_LOAD | OT_GOTO_DEPOT;
			v->next_order_param = v->u.air.targetairport;
			InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
		}
	}
	
	return 0;
}

// p1 = vehicle
// p2 = new service int
int32 CmdChangeAircraftServiceInt(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v;

	v = &_vehicles[p1];

	if (!CheckOwnership(v->owner))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		v->service_interval = (uint16)p2;
		InvalidateWindowWidget(WC_VEHICLE_DETAILS, v->index, 7);
	}

	return 0;
}

// p1 = vehicle
// p2 = new cargo type
int32 CmdRefitAircraft(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v,*u;
	int pass, mail;
	int32 cost;

	SET_EXPENSES_TYPE(EXPENSES_AIRCRAFT_RUN);
	
	v = &_vehicles[p1];
	if (!CheckOwnership(v->owner) || !CheckStoppedInHangar(v))
		return CMD_ERROR;

	pass = _aircraft_num_pass[v->engine_type - AIRCRAFT_ENGINES_INDEX];
	if (p2 != 0) {
		pass >>= 1;
		if (p2 != 5)
			pass >>= 1;
	}
	_aircraft_refit_capacity = pass;

	cost = 0;
	if (IS_HUMAN_PLAYER(v->owner) && (byte)p2 != v->cargo_type) {
		cost = _price.aircraft_base >> 7;
	}

	if (flags & DC_EXEC) {
		v->cargo_cap = pass;

		u = &_vehicles[v->next_in_chain];
		mail = _aircraft_num_mail[v->engine_type - AIRCRAFT_ENGINES_INDEX];
		if (p2 != 0) {
			mail = 0;
		}
		u->cargo_cap = mail;
		v->cargo_count = u->cargo_count = 0;
		v->cargo_type = (byte)p2;
		InvalidateWindow(WC_VEHICLE_DETAILS, v->index);
	}

	return cost;
}

void HandleClickOnAircraft(Vehicle *v)
{
	ShowAircraftViewWindow(v);
}

static void CheckIfAircraftNeedsService(Vehicle *v)
{
	Station *st;

	if (v->date_of_last_service + v->service_interval > _date)
		return;

	if (v->vehstatus & VS_STOPPED)
		return;
	
	if ((v->next_order & (OT_MASK | OF_FULL_LOAD)) == (OT_GOTO_DEPOT | OF_FULL_LOAD))
		return;

	st = DEREF_STATION(v->next_order_param);
	if (st->xy != 0 && st->airport_tile != 0 && !(st->airport_flags & (1 << 6))) {
		v->next_order = OF_NON_STOP | OT_GOTO_DEPOT;
		InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
	} else if ((v->next_order & OT_MASK) == OT_GOTO_DEPOT) {
		v->next_order = OT_DUMMY;
		InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
	}
}

void OnNewDay_Aircraft(Vehicle *v)
{
	int32 cost;

	if (v->subtype > 2)
		return;

	if ((++v->day_counter & 7) == 0)
		DecreaseVehicleValue(v);

	CheckVehicleBreakdown(v);
	AgeVehicle(v);
	CheckIfAircraftNeedsService(v);

	if (v->vehstatus & VS_STOPPED)
		return;

	cost = _aircraft_running_cost[v->engine_type - AIRCRAFT_ENGINES_INDEX] * _price.aircraft_running / 364;

	v->profit_this_year -= cost >> 8;

	SET_EXPENSES_TYPE(EXPENSES_AIRCRAFT_RUN);
	SubtractMoneyFromPlayerFract(v->owner, cost);

	InvalidateWindow(WC_VEHICLE_DETAILS, v->index);
	InvalidateWindow(WC_AIRCRAFT_LIST, v->owner);
}

void AircraftYearlyLoop()
{
	Vehicle *v;

	for(v=_vehicles; v != endof(_vehicles); v++) {
		if (v->type == VEH_Aircraft && v->subtype <= 2) {
			v->profit_last_year = v->profit_this_year;
			v->profit_this_year = 0;
			InvalidateWindow(WC_VEHICLE_DETAILS, v->index);
		}
	}
}

static void AgeAircraftCargo(Vehicle *v)
{
	if (_age_cargo_skip_counter != 0)
		return;

	for(;;) {
		if (v->cargo_days != 0xFF)
			v->cargo_days++;

		if (v->next_in_chain == INVALID_VEHICLE)
			break;
		v = &_vehicles[v->next_in_chain];
	}
}

static void HelicopterTickHandler(Vehicle *v)
{
	Vehicle *u;
	int tick,spd;
	uint16 img;

	u = &_vehicles[v->next_in_chain];
	u = &_vehicles[u->next_in_chain];

	if (u->vehstatus & VS_HIDDEN)
		return;

	if (IS_BYTE_INSIDE(v->u.air.state, AS_AT_TERM_1, AS_AT_TERM_3+1)) {
		if (u->cur_speed != 0) {
			u->cur_speed++;
			if (u->cur_speed >= 0x80 && u->cur_image == 0xF40) {
				u->cur_speed = 0;
			}
		}
	} else {
		if (u->cur_speed == 0)
			u->cur_speed = 0x70;

		if (u->cur_speed >= 0x50)
			u->cur_speed--;
	}

	tick = ++u->tick_counter;
	spd = u->cur_speed >> 4;

	if (spd == 0) {
		img = 0xF3D;
		if (u->cur_image == img)
			return;
	} else if (tick >= spd) {
		u->tick_counter = 0;
		img = u->cur_image + 1;
		if (img > 0xF40)
			img = 0xF3E;
	} else
		return;

	u->cur_image=img;

	BeginVehicleMove(u);
	VehiclePositionChanged(u);
	EndVehicleMove(u);
}

static void SetAircraftPosition(Vehicle *v, int x, int y, int z)
{
	Vehicle *u;
	int yt;

	v->x_pos = x;
	v->y_pos = y;
	v->z_pos = z;
	
	v->cur_image = GetAircraftImage(v, v->direction);

	BeginVehicleMove(v);
	VehiclePositionChanged(v);
	EndVehicleMove(v);

	u = &_vehicles[v->next_in_chain];
	
	yt = y - ((v->z_pos-(byte)GetSlopeZ(x, y-1)) >> 3);
	u->x_pos = x;
	u->y_pos = yt;
	u->z_pos = (byte)GetSlopeZ(x,yt);
	u->cur_image = v->cur_image;

	BeginVehicleMove(u);
	VehiclePositionChanged(u);
	EndVehicleMove(u);

	if (u->next_in_chain != INVALID_VEHICLE) {
		u = &_vehicles[u->next_in_chain];

		u->x_pos = x;
		u->y_pos = y;
		u->z_pos = z + 5;
		
		BeginVehicleMove(u);
		VehiclePositionChanged(u);
		EndVehicleMove(u);
	}
}


static void ServiceAircraft(Vehicle *v)
{
	Station *st;
	Vehicle *u;

	st = DEREF_STATION(v->u.air.targetairport);
	CLRBIT(st->airport_flags, 7);
	v->cur_speed = 0;
	v->subspeed = 0;
	v->progress = 0;
	v->vehstatus |= VS_HIDDEN;

	u = &_vehicles[v->next_in_chain];
	u->vehstatus |= VS_HIDDEN;
	if (u->next_in_chain != INVALID_VEHICLE) {
		u = &_vehicles[u->next_in_chain];
		u->vehstatus |= VS_HIDDEN;
		u->cur_speed = 0;
	}

	SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
	InvalidateWindow(WC_VEHICLE_DEPOT, v->tile);

	v->date_of_last_service = _date;
	v->breakdowns_since_last_service = 0;
	v->reliability = _engines[v->engine_type].reliability;
	InvalidateWindow(WC_VEHICLE_DETAILS, v->index);
}

static void PlayAircraftSound(Vehicle *v)
{
	SndPlayVehicleFx(_aircraft_sounds[v->engine_type - AIRCRAFT_ENGINES_INDEX], v);
}

static bool UpdateAircraftSpeed(Vehicle *v)
{
	uint spd = v->acceleration * 2;
	byte t;

	v->subspeed = (t=v->subspeed) + (byte)spd;
	spd = min(v->cur_speed + (spd >> 8) + (v->subspeed < t), (v->vehstatus&VS_AIRCRAFT_BROKEN)?27:v->max_speed);

	//updates statusbar only if speed have changed to save CPU time
	if (spd != v->cur_speed) {
		v->cur_speed = spd;	
		if (_patches.vehicle_speed)
			InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
	}

	if (!(v->direction & 1)) {
		spd = spd * 3 >> 2;
	}

	if (spd == 0)
		return false;

	if ((byte)++spd == 0)
		return true;

	v->progress = (t = v->progress) - (byte)spd;

	return (t < v->progress);
}

typedef struct AirportMovingData {
	int16 x,y;
	byte flag;
	byte direction;
} AirportMovingData;

enum {
	AMED_NOSPDCLAMP = 1<<0,
	AMED_TAKEOFF = 1<<1,
	AMED_SLOWTURN = 1<<2,
	AMED_LAND = 1<<3,
	AMED_EXACTPOS = 1<<4,
	AMED_BRAKE = 1<<5,
	AMED_HELI_RAISE = 1<<6,
	AMED_HELI_LOWER = 1<<7,
};

static const AirportMovingData _airport_moving_data_1[29] = {
	{53,3,AMED_EXACTPOS,7},
	{53,27,0,0},			// 1 - taxi to right outside depot
	{8,23,AMED_EXACTPOS,7},	// 2 - taxi to terminal 1
	{32,23,AMED_EXACTPOS,7},
	{0,0,AMED_EXACTPOS,5},
	{53,24,0,0},
	{53,24,0,0},
	{48,37,0,0},			// 7 - taxi to asphalt outside terminal 1
	{21,37,0,0},			// 8 - just landed, turn around and taxi 1 square
	{21,37,0,0},			// 9 - taxi from runway to crossing
	{56,37,0,0},			// 10 - taxi to runway
	{56,37,0,0},			// 11 - taxi from terminal 1 to right before runway
	{56,37,0,0},			// 12 - middle state
	{3,37,AMED_NOSPDCLAMP,0}, // 13 - accelerate to end of runway
	{57,37,0,0},			// 14 - taxi to start of runway
	{-79,37,AMED_NOSPDCLAMP | AMED_TAKEOFF,0}, // 15 - take off
	{177,37,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},// 16 - fly to landing position in air.
	{-31,193,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{1,1,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},	// 18 - travel to north corner of airport
	{257,1,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},// 19 - travel to east of airport
	{273,49,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},//20 - travel to east #2 of airport 
	{57,37,AMED_NOSPDCLAMP | AMED_LAND,0},		// 21 - going down for land
	{3,37,AMED_NOSPDCLAMP | AMED_BRAKE,0},    // 22 - just landed, brake until end of runway
	{44,37,AMED_HELI_RAISE,0},						
	{40,37,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},	
	{40,37,AMED_HELI_LOWER,0},
	{23,8,AMED_HELI_RAISE,0},
	{23,8,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{23,8,AMED_NOSPDCLAMP | AMED_HELI_LOWER,0},
};


static const AirportMovingData _airport_moving_data_2[29] = {
	{85,3,AMED_EXACTPOS,7},
	{85,27,0,0},
	{26,38,AMED_EXACTPOS,5},
	{56,20,AMED_EXACTPOS,3},
	{38,8,AMED_EXACTPOS,5},
	{65,6,0,0},
	{70,33,0,0},
	{44,63,0,0},
	{21,85,0,0},
	{37,69,0,0},
	{72,85,0,0},
	{56,69,0,0},
	{56,69,0,0},
	{3,85,AMED_NOSPDCLAMP,0},
	{89,85,0,0},
	{-79,85,AMED_NOSPDCLAMP | AMED_TAKEOFF,0},
	{177,85,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{-31,193,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{1,1,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{257,1,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{273,49,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{89,85,AMED_NOSPDCLAMP | AMED_LAND,0},
	{3,85,AMED_NOSPDCLAMP | AMED_BRAKE,0},
	{44,63,AMED_HELI_RAISE,0},
	{40,65,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{40,65,AMED_HELI_LOWER,0},
	{23,8,AMED_HELI_RAISE,0},
	{23,8,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{23,8,AMED_NOSPDCLAMP | AMED_HELI_LOWER,0},
};	

static const AirportMovingData _airport_moving_data_3[29] = {
	{53,3,AMED_EXACTPOS,7},
	{53,27,0,0},
	{8,23,AMED_EXACTPOS,7},
	{32,23,AMED_EXACTPOS,7},
	{0,0,AMED_EXACTPOS,5},
	{53,24,0,0},
	{53,24,0,0},
	{48,37,0,0},
	{21,37,0,0},
	{21,37,0,0},
	{56,37,0,0},
	{56,37,0,0},
	{56,37,0,0},
	{3,37,AMED_NOSPDCLAMP,0},
	{57,37,0,0},
	{-79,37,AMED_NOSPDCLAMP | AMED_TAKEOFF,0},
	{33,21,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{-31,33,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{1,1,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{49,1,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{65,49,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{57,37,AMED_NOSPDCLAMP | AMED_LAND,0},
	{3,37,AMED_NOSPDCLAMP | AMED_BRAKE,0},
	{44,37,AMED_HELI_RAISE,0},
	{40,37,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{40,37,AMED_HELI_LOWER,0},
	{-2,9,AMED_HELI_RAISE,0},
	{-2,9,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{-2,9,AMED_NOSPDCLAMP | AMED_HELI_LOWER,0},
};

static const AirportMovingData _airport_moving_data_4[29] = {
	{53,3,AMED_EXACTPOS,7},
	{53,27,0,0},
	{8,23,AMED_EXACTPOS,7},
	{32,23,AMED_EXACTPOS,7},
	{0,0,AMED_EXACTPOS,5},
	{53,24,0,0},
	{53,24,0,0},
	{48,37,0,0},
	{21,37,0,0},
	{21,37,0,0},
	{56,37,0,0},
	{56,37,0,0},
	{56,37,0,0},
	{3,37,AMED_NOSPDCLAMP,0},
	{57,37,0,0},
	{-79,37,AMED_NOSPDCLAMP | AMED_TAKEOFF,0},
	{65,37,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{-31,33,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{1,1,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{81,1,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{97,49,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{57,37,AMED_NOSPDCLAMP | AMED_LAND,0},
	{3,37,AMED_NOSPDCLAMP | AMED_BRAKE,0},
	{44,37,AMED_HELI_RAISE,0},
	{40,37,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{40,37,AMED_HELI_LOWER,0},
	{23,8,AMED_HELI_RAISE,0},
	{23,8,AMED_NOSPDCLAMP | AMED_SLOWTURN,0},
	{23,8,AMED_NOSPDCLAMP | AMED_HELI_LOWER,0},
};


static const AirportMovingData * const _airport_moving_datas[4] = {
	_airport_moving_data_1,
	_airport_moving_data_2,
	_airport_moving_data_3,
	_airport_moving_data_4,
};

static bool Aircraft_5(Vehicle *v)
{
	Station *st;
	const AirportMovingData *amd;
	Vehicle *u;
	byte z,dirdiff,newdir,maxz,curz;
	GetNewVehiclePosResult gp;
	uint dist;
	int x,y;

	st = DEREF_STATION(v->u.air.targetairport);

	amd = &_airport_moving_datas[st->airport_type][v->u.air.unk0];

	x = GET_TILE_X(st->airport_tile)*16;
	y = GET_TILE_Y(st->airport_tile)*16;

// Helicopter raise
	if (amd->flag & AMED_HELI_RAISE) {
		u = &_vehicles[v->next_in_chain];
		u = &_vehicles[u->next_in_chain];

		// Make sure the rotors don't rotate too fast		
		if (u->cur_speed > 32) {
			v->cur_speed = 0;
			if (--u->cur_speed == 32) {
				SndPlayVehicleFx(0x16, v);
			}
		} else {
			u->cur_speed = 32;
			if (UpdateAircraftSpeed(v)) {
				v->tile = 0;
				
				// Reached altitude?
				if (v->z_pos >= 184) {
					v->cur_speed = 0;
					return true;
				}
				SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos+1);
			}
		}
		return false;
	} 
	
	// Helicopter landing.
	if (amd->flag & AMED_HELI_LOWER) {
		if (UpdateAircraftSpeed(v)) {
			if (st->airport_tile == 0) {
				v->u.air.unk0 = 17;
				v->u.air.state = AS_IN_FLIGHT;
				return false;
			}	

			// Vehicle is now at the airport.
			v->tile = st->airport_tile;
			
			// Find altitude of landing position.
			z = (byte)GetSlopeZ(x, y) + 1;
			if (st->airport_type == AT_OILFIELD) z += 54;
			if (st->airport_type == AT_HELIPORT) z += 60;

			if (z == v->z_pos) {
				u = &_vehicles[v->next_in_chain];
				u = &_vehicles[u->next_in_chain];

				// Increase speed of rotors. When speed is 80, we've landed.
				if (u->cur_speed >= 80)
					return true;
				u->cur_speed+=4;
			} else if (v->z_pos > z) {
				SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos-1);
			} else {
				SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos+1);
			}
		}
		return false;
	}

	// Get distance from destination pos to current pos.
	dist = myabs(x + amd->x - v->x_pos) +  myabs(y + amd->y - v->y_pos);

	// Need exact position?
	if (!(amd->flag & AMED_EXACTPOS) && dist <= (uint)((amd->flag&AMED_SLOWTURN)?8:4))
		return true;

	// At final pos?
	if (dist == 0) {
		
		// Clamp speed to 12.
		if (v->cur_speed > 12)
			v->cur_speed = 12;

		if (!UpdateAircraftSpeed(v))
			return false;

		// Change direction smoothly to final direction.
		dirdiff = amd->direction - v->direction;
		if (dirdiff == 0) {
			v->cur_speed = 0;
			return true;
		}
		v->direction = (v->direction+((dirdiff&7)<5?1:-1)) & 7;
		v->cur_speed >>= 1;

		SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
		return false;
	}
		
	// Clamp speed?
	if (!(amd->flag & AMED_NOSPDCLAMP) && v->cur_speed > 12)
		v->cur_speed = 12;

	if (!UpdateAircraftSpeed(v))
		return false;

	// Decrease animation counter.
	if (v->load_unload_time_rem != 0)
		v->load_unload_time_rem--;

	// Turn. Do it slowly if in the air.
	newdir = GetDirectionTowards(v, x + amd->x, y + amd->y);
	if (newdir != v->direction) {
		if (amd->flag & AMED_SLOWTURN) {
			if (v->load_unload_time_rem == 0) {
				v->load_unload_time_rem = 8;
				v->direction = newdir;
			}
		} else {
			v->cur_speed >>= 1;
			v->direction = newdir;
		}
	}

	// Move vehicle.
	GetNewVehiclePos(v, &gp);
	v->tile = gp.new_tile;

	// If vehicle is in the air, use tile coordinate 0.
	if (amd->flag & (AMED_TAKEOFF | AMED_SLOWTURN | AMED_LAND)) {
		v->tile = 0;
	}

	// Adjust Z for land or takeoff?
	z = v->z_pos;

	if (amd->flag & AMED_TAKEOFF) {
		z+=2;
		// Determine running altitude
		maxz = 162;
		if (v->max_speed != 37) {
			maxz = 171;
			if (v->max_speed != 74) {
				maxz = 180;
			}
		}
		if (z > maxz)
			z = maxz;
	}

	if (amd->flag & AMED_LAND) {
		if (st->airport_tile == 0) {
			v->u.air.unk0 = 17;
			v->u.air.state = AS_IN_FLIGHT;
			SetAircraftPosition(v, gp.x, gp.y, z);
			return false;
		}

		curz = (byte)GetSlopeZ(x, y) + 1;
		
		if (curz > z) {
			z++;
		} else {
			int t = max(1, dist-4);
			
			z -= ((z - curz) + t - 1) / t;
			if (z < curz) z = curz;
		}
	}

	// We've landed. Decrase speed when we're reaching end of runway.
	if (amd->flag & AMED_BRAKE) {
		curz = (byte)GetSlopeZ(x, y) + 1;

		if (z > curz) z--;
		else if (z < curz) z++;

		if (dist < 64 && v->cur_speed > 12)
			v->cur_speed -= 4;
	}

	SetAircraftPosition(v, gp.x, gp.y, z);
	return false;
}

static const int8 _crashed_aircraft_moddir[4] = {
	-1,0,0,1
};

static void HandleCrashedAircraft(Vehicle *v)
{
	uint32 r;
	Station *st;

	v->u.air.crashed_counter++;

	if (v->u.air.crashed_counter < 650) {
		if (CHANCE16R(1,32,r)) {
			v->direction = (v->direction+_crashed_aircraft_moddir[(r >> 16)&3]) & 7;
			SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
			r = Random();
			CreateEffectVehicleRel(v,
				4 + (r&0xF),
				4 + ((r>>4)&0xF),
				((r>>8)&0xF),
				EV_DEMOLISH);
		}
	} else if (v->u.air.crashed_counter >= 10000) {
		st = DEREF_STATION(v->u.air.targetairport);

		if (HASBIT(st->airport_flags, 8))
			CLRBIT(st->airport_flags, 11);
		else
			CLRBIT(st->airport_flags, 7);

		BeginVehicleMove(v);
		EndVehicleMove(v);

		DoDeleteAircraft(v);
	}
}


static void HandleBrokenAircraft(Vehicle *v)
{
	if (v->breakdown_ctr != 1) {
		v->breakdown_ctr = 1;
		v->vehstatus |= VS_AIRCRAFT_BROKEN;

		if (v->breakdowns_since_last_service != 255)
			v->breakdowns_since_last_service++;
		InvalidateWindow(WC_VEHICLE_VIEW, v->index);
		InvalidateWindow(WC_VEHICLE_DETAILS, v->index);
	}
}

static const int8 _aircraft_smoke_xy[16] = {
	5,6,5,0,-5,-6,-5,0, /* x coordinates */
	5,0,-5,-6,-5,0,5,6, /* y coordinate */

};

static void HandleAircraftSmoke(Vehicle *v)
{
	if (!(v->vehstatus&VS_AIRCRAFT_BROKEN))
		return;

	if (v->cur_speed < 10) {
		v->vehstatus &= ~VS_AIRCRAFT_BROKEN;
		v->breakdown_ctr = 0;
		return;
	}

	if ((v->tick_counter & 0x1F) == 0) {
		CreateEffectVehicleRel(v,
			_aircraft_smoke_xy[v->direction],
			_aircraft_smoke_xy[v->direction + 8],
			2,
			EV_SMOKE_3
		);
	}
}

static void ProcessAircraftOrder(Vehicle *v)
{
	uint order;

	// OT_GOTO_DEPOT, OT_LOADING, OT_UNLOADING
	if ((v->next_order & OT_MASK) >= OT_GOTO_DEPOT && (v->next_order & OT_MASK) <= OT_LEAVESTATION)
		return;

	if (v->cur_order_index >= v->num_orders)
		v->cur_order_index = 0;

	order = v->schedule_ptr[v->cur_order_index];

	if (order == 0) {
		v->next_order = OT_NOTHING;
		return;
	}

	if (order == (uint)((v->next_order | (v->next_order_param<<8))))
		return;

	v->next_order = (byte)order;
	v->next_order_param = (byte)(order >> 8);

	if ((order & OT_MASK) == OT_GOTO_STATION && v->u.air.state == AS_IN_FLIGHT) {
		v->u.air.targetairport = order >> 8;
		v->u.air.unk0 = 18;
	}

	InvalidateVehicleOrderWidget(v);
}

static void HandleAircraftLoading(Vehicle *v, int mode)
{
	if (v->next_order == OT_NOTHING)
		return;
	
	if (v->next_order != OT_DUMMY) {
		if ((v->next_order&OT_MASK) != OT_LOADING)
			return;

		if (mode != 0)
			return;

		if (--v->load_unload_time_rem)
			return;

		if (v->next_order&OF_FULL_LOAD && CanFillVehicle(v)) {
			SET_EXPENSES_TYPE(EXPENSES_AIRCRAFT_INC);
			LoadUnloadVehicle(v);
			return;
		}
		
		{
			byte b = v->next_order;
			v->next_order = OT_NOTHING;
			if (!(b & OF_NON_STOP))
				return;
		}
	}
	v->cur_order_index++;
	InvalidateVehicleOrderWidget(v);
}

static void MaybeCrashAirplane(Vehicle *v)
{
	Station *st;
	uint16 prob;
	int i;
	uint16 amt;

	st = DEREF_STATION(v->u.air.targetairport);

	prob = 0x10000 / 1500;
	if (st->airport_type == AT_SMALL && (_aircraft_subtype[v->engine_type - AIRCRAFT_ENGINES_INDEX]&2)) {
		prob = 0x10000 / 20;
	}

	if ((uint16)Random() > prob)
		return;

	// Crash the airplane. Remove all goods stored at the station.
	for(i=0; i!=NUM_CARGO; i++) {
		st->goods[i].rating = 1;
		st->goods[i].waiting_acceptance &= ~0xFFF;
	}

	v->vehstatus |= VS_CRASHED;
	v->u.air.crashed_counter = 0;

	CreateEffectVehicleRel(v, 4, 4, 8, EV_CRASHED_SMOKE);
	
	InvalidateWindow(WC_VEHICLE_VIEW, v->index);

	amt = 2;
	if (v->cargo_type == CT_PASSENGERS) amt += v->cargo_count;
	SET_DPARAM16(0, amt);

	v->cargo_count = 0;
	(&_vehicles[v->next_in_chain])->cargo_count = 0,
	
	SET_DPARAM16(1, st->index);
	AddNewsItem(STR_A034_PLANE_CRASH_DIE_IN_FIREBALL,
		NEWS_FLAGS(NM_THIN, NF_VIEWPORT|NF_VEHICLE, NT_ACCIDENT, 0),
		v->index,
		0);

	ModifyStationRatingAround(TILE_FROM_XY(v->x_pos, v->y_pos), v->owner, -160, 30);
	SndPlayVehicleFx(16, v);
}


static void AircraftEntersTerminal(Vehicle *v, byte cmd)
{
	Station *st;
	byte old_order;
		
	st = DEREF_STATION(v->u.air.targetairport);

	if (cmd != 255) {
		v->u.air.state = cmd;
		CLRBIT(st->airport_flags, 7);
	}

	if ((v->next_order & OT_MASK) == OT_GOTO_DEPOT)
		return;

	v->last_station_visited = v->u.air.targetairport;
	
	/* Check if station was ever visited before */
	if (!(st->had_vehicle_of_type & HVOT_AIRCRAFT)) {
		uint32 flags;

		st->had_vehicle_of_type |= HVOT_AIRCRAFT;
		SET_DPARAM16(0, st->index);
		flags = (v->owner == _local_player) ? NEWS_FLAGS(NM_THIN, NF_VIEWPORT|NF_VEHICLE, NT_ARRIVAL_PLAYER, 0) : NEWS_FLAGS(NM_THIN, NF_VIEWPORT|NF_VEHICLE, NT_ARRIVAL_OTHER, 0);
		AddNewsItem(
			STR_A033_CITIZENS_CELEBRATE_FIRST,
			flags,
			v->index,
			0);
	}

	old_order = v->next_order;
	v->next_order = OT_LOADING;

	if ((old_order & OT_MASK) == OT_GOTO_STATION &&
			v->next_order_param == v->last_station_visited) {
		v->next_order = OT_LOADING | (old_order & (OF_UNLOAD|OF_FULL_LOAD)) | OF_NON_STOP;
	}

	SET_EXPENSES_TYPE(EXPENSES_AIRCRAFT_INC);
	LoadUnloadVehicle(v);
	InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
}

// In depot
static void AircraftEventHandler_State0(Vehicle *v) {
	Station *st;
	byte state;

	if ((v->next_order&OT_MASK) == OT_GOTO_DEPOT) {
		v->next_order = OT_NOTHING;
		return;
	}

	if ((v->next_order&OT_MASK) != OT_GOTO_STATION)
		return;

	st = DEREF_STATION(v->u.air.targetairport);

	// Is the runway in use by someone else?
	if (HASBIT(st->airport_flags,7))
		return;

	// We are already at the target airport?
	if (v->next_order_param == v->u.air.targetairport) {
		
		// Find a free terminal and go to it.
		state = AS_HANGAR_TO_TERM_1;
		if (st->airport_flags & 1) {
			state++;
			if (st->airport_flags & 2) {
				state++;
				if (st->airport_flags & 4)
					return;
			}
		}
		v->u.air.state = state;
		SETBIT(st->airport_flags, state-AS_HANGAR_TO_TERM_1); /* set bit 0 to 2 */
	} else {
		// Else prepare for launch.
		v->u.air.state = AS_TAKE_OFF_HANGAR;
	}

	v->u.air.unk0 = 1;
	
	// Runway is occupied now
	SETBIT(st->airport_flags, 7);
		
	v->cur_speed = 0;
	v->subspeed = 0;
	v->progress = 0;
	v->direction = 3;
	v->vehstatus &= ~VS_HIDDEN;

	{
		Vehicle *u = &_vehicles[v->next_in_chain];
		u->vehstatus &= ~VS_HIDDEN;

		// Rotor blades
		if (u->next_in_chain != INVALID_VEHICLE) {
			u = &_vehicles[u->next_in_chain];
			u->vehstatus &= ~VS_HIDDEN;
			u->cur_speed = 80;
		}
	}

	SetAircraftPosition(v, v->x_pos, v->y_pos, v->z_pos);
	InvalidateWindow(WC_VEHICLE_DEPOT, v->tile);
}

// At terminal 1..3
static void AircraftEventHandler_State1to3(Vehicle *v)
{
	Station *st;
	byte state, unk0;

	st = DEREF_STATION(v->u.air.targetairport);

	if (!HASBIT(st->airport_flags,6) && 
			HASBIT(st->airport_flags,7))
				return;
	
	// removed &0x1F
	if (v->next_order == OT_NOTHING)
		return;

	if (HASBIT(st->airport_flags,6)) {
		state = AS_TAKE_OFF_TERM_1;
		unk0 = 26;
		goto set_state_and_unk0_and_exit;
	}

	if (HASBIT(st->airport_flags,8) && 
			(v->next_order&OT_MASK) == OT_GOTO_STATION &&
			v->subtype != 0) {
		if (HASBIT(st->airport_flags, 9)) {
			CLRBIT(st->airport_flags, 7);
			return;
		}
		SETBIT(st->airport_flags,9);
	}

	CLRBIT(st->airport_flags, v->u.air.state-AS_AT_TERM_1);
	SETBIT(st->airport_flags, 7);

	state = v->u.air.state + (AS_TERM_1_TO_HANGAR - AS_AT_TERM_1);

	unk0 = 7;
	if ((v->next_order&OT_MASK) == OT_GOTO_STATION) {
		state += AS_TAKE_OFF_TERM_1 - AS_TERM_1_TO_HANGAR;
		if (v->subtype != 0)
			unk0 = 11;
	}

	if (v->u.air.state != AS_AT_TERM_1)
		unk0 = 5;

set_state_and_unk0_and_exit:
	v->u.air.state = state;
	v->u.air.unk0 = unk0;
}

// Going from hangar to terminal 1..3
static void AircraftEventHandler_State4to6(Vehicle *v)
{
	byte state, unk0;

	if (!Aircraft_5(v))
		return;

	state = v->u.air.state;
	unk0 = v->u.air.unk0;
	
	if (unk0 == 1) {
		v->u.air.unk0 = (state!=AS_HANGAR_TO_TERM_1) ? 6 : 7;
		return;
	} 
	
	if (unk0 == 6) {
		v->u.air.unk0 = 5;
		return;
	}
	
	if (unk0 == 7) {
		v->u.air.unk0 = 2;
		return;
	}
	
	if (unk0 == 5) {
		v->u.air.unk0 = (state==AS_HANGAR_TO_TERM_2) ? 3 : 4;
		return;
	}

	AircraftEntersTerminal(v, state-(AS_HANGAR_TO_TERM_1 - AS_AT_TERM_1));
}

static void AircraftUnk0Is21(Vehicle *v)
{
	v->sprite_width = v->sprite_height = 2;
	SndPlayVehicleFx(0x15, v);
	MaybeCrashAirplane(v);
	v->u.air.unk0 = 22;
}

static void AircraftUnk0Is24(Vehicle *v)
{
	v->sprite_width = v->sprite_height = 2;
	v->u.air.unk0 = 25;
}


// Landing and going to terminal 1..3
static void HandleAircraft_LandedToTerminal(Vehicle *v)
{
	byte unk0;
	Station *st;

	if (!Aircraft_5(v))
		return;
	
	unk0 = v->u.air.unk0;

	if (unk0 == 21) {
		AircraftUnk0Is21(v);
	} else if (unk0 == 22) {
		v->u.air.unk0 = 8;
	} else if (unk0 == 8) {
		st = DEREF_STATION(v->u.air.targetairport);
		if (HASBIT(st->airport_flags, 8)) {
			if (HASBIT(st->airport_flags, 10)) {
				v->cur_speed = 0;
				v->subspeed = 0;
				return;
			}
			SETBIT(st->airport_flags, 10);
			CLRBIT(st->airport_flags, 11);
		}
		v->u.air.unk0 = 9;
	} else if (unk0 == 9 || unk0 == 25) {
		if (v->subtype != 0) {
			st = DEREF_STATION(v->u.air.targetairport);
			if (HASBIT(st->airport_flags, 8)) {
				if (HASBIT(st->airport_flags, 7)) {
					v->cur_speed = 0;
					v->subspeed = 0;
					return;
				}
				SETBIT(st->airport_flags, 7);
				CLRBIT(st->airport_flags, 10);
			}
		}

		v->u.air.unk0 = v->u.air.state!=AS_LANDED_TO_TERM_1 ? 6 : 7;
	} else if (unk0 == 7) {
		v->u.air.unk0 = 2;
	} else if (unk0 == 6) {
		v->u.air.unk0 = 5;
	} else if (unk0 == 5) {
		v->u.air.unk0 = v->u.air.state==AS_LANDED_TO_TERM_2 ? 3 : 4;
	} else if (unk0 == 24) {
		AircraftUnk0Is24(v);
	} else if (unk0 == 27) {
		v->sprite_width = v->sprite_height = 2;
		v->u.air.unk0 = 28;
	} else if (unk0 == 28) {
		v->u.air.state = AS_AT_TERM_1;
		AircraftEntersTerminal(v, 255);	
	} else {
		AircraftEntersTerminal(v, v->u.air.state-(AS_LANDED_TO_TERM_1 - AS_AT_TERM_1));	
	}
}


static void AircraftEnterHangar(Vehicle *v)
{
	byte old_order;

	if (v->u.air.unk0 == 1) {
		v->u.air.unk0 = 0;
	} else {	
		ServiceAircraft(v);
		v->u.air.state = AS_IN_HANGAR;

		if ((v->next_order & OT_MASK) == OT_GOTO_DEPOT) {
			InvalidateWindow(WC_VEHICLE_VIEW, v->index);

			old_order = v->next_order;
			v->next_order = OT_NOTHING;

			if (old_order & OF_FULL_LOAD) { // force depot visit
				v->vehstatus |= VS_STOPPED;

				if (v->owner == _local_player) {
					SET_DPARAM16(0, v->unitnumber);
					AddNewsItem(
						STR_A014_AIRCRAFT_IS_WAITING_IN,
						NEWS_FLAGS(NM_SMALL, NF_VIEWPORT|NF_VEHICLE, NT_ADVICE, 0),
						v->index,
						0);
				}
			}
		}
	}
}


// Going from terminal 1..3 to hangar 
static void HandleAircraft_TerminalToHangar(Vehicle *v)
{
	byte unk0;
	
	if (!Aircraft_5(v))
		return;

	unk0 = v->u.air.unk0;

	if (unk0 == 5 || unk0 == 7) {
		v->u.air.unk0 = 6;
	} else if (unk0 == 6) {
		v->u.air.unk0 = 1;
	} else {
		AircraftEnterHangar(v);
	}
}

static void StartAircraftTakeOff(Vehicle *v)
{
	byte unk0;
	Station *st;

	unk0 = v->u.air.unk0;
	if (unk0 == 5) {
		v->u.air.unk0 = 6;
	} else if (unk0 == 6) {
		v->u.air.unk0 = 7;
	} else if (unk0 == 7) {
		v->u.air.unk0 = v->subtype==0 ? 23 : 11;
	} else if (unk0 == 11) {
		st = DEREF_STATION(v->u.air.targetairport);
		if (HASBIT(st->airport_flags, 8))
			CLRBIT(st->airport_flags, 7);
		v->u.air.unk0 = 12;
	} else if (unk0 == 12) {
		st = DEREF_STATION(v->u.air.targetairport);
		if (HASBIT(st->airport_flags, 8)) {
			if (HASBIT(st->airport_flags, 11)) {
				v->cur_speed = 0;
				v->subspeed = 0;
				return;
			}
			SETBIT(st->airport_flags, 11);
			CLRBIT(st->airport_flags, 9);
		}
		v->u.air.unk0 = 10;
	} else if (unk0 == 10) {
		v->u.air.unk0 = 14;
	} else if (unk0 == 14) {
		PlayAircraftSound(v);
		v->u.air.unk0 = 13;
	} else if (unk0 == 13) {
		st = DEREF_STATION(v->u.air.targetairport);
		if (HASBIT(st->airport_flags, 8))
			CLRBIT(st->airport_flags, 11);
		else
			CLRBIT(st->airport_flags, 7);
		v->sprite_width = v->sprite_height = 24;
		v->u.air.unk0 = 15;
	} else if (unk0 == 23 || unk0 == 26) {
		st = DEREF_STATION(v->u.air.targetairport);
		CLRBIT(st->airport_flags, 7);
		v->sprite_width = v->sprite_height = 24;

		v->u.air.unk0 = 20;

		if ((v->next_order&OT_MASK) == OT_GOTO_STATION ||
			  (v->next_order&OT_MASK) == OT_GOTO_DEPOT)
					v->u.air.targetairport = v->next_order_param;

		v->u.air.state = AS_IN_FLIGHT;
	} else {
		v->u.air.unk0 = 18;

		if ((v->next_order&OT_MASK) == OT_GOTO_STATION ||
			  (v->next_order&OT_MASK) == OT_GOTO_DEPOT)
					v->u.air.targetairport = v->next_order_param;

		v->u.air.state = AS_IN_FLIGHT;
	}
}

// Taking off from terminal 1..3
static void HandleAircraft_TerminalTakeOff(Vehicle *v)
{
	if (!Aircraft_5(v))
		return;

	StartAircraftTakeOff(v);
}

// Taking off from hangar
static void HandleAircraft_HangarTakeOff(Vehicle *v)
{
	if (!Aircraft_5(v))
		return;

	if (v->u.air.unk0 == 1) {
		v->u.air.unk0 = 7;
	} else {
		StartAircraftTakeOff(v);
	}
}

// Landing, going to hangar
static void HandleAircraft_ToHangar(Vehicle *v)
{
	byte unk0;
	Station *st;

	if (!Aircraft_5(v))
		return;

	unk0 = v->u.air.unk0;

	if (unk0 == 21) {
		AircraftUnk0Is21(v);
	} else if (unk0 == 24) {
		AircraftUnk0Is24(v);
	} else if (unk0 == 22) {
		v->u.air.unk0 = 8;
	} else if (unk0 == 25) {
		v->u.air.unk0 = 1;
	} else if (unk0 == 9) {
		st = DEREF_STATION(v->u.air.targetairport);
		if (HASBIT(st->airport_flags, 8)) {
			if (HASBIT(st->airport_flags, 7)) {
				v->cur_speed = 0;
				v->subspeed = 0;
				return;
			}
			SETBIT(st->airport_flags, 7);
			CLRBIT(st->airport_flags, 10);
		}
		v->u.air.unk0 = 1;
	} else if (unk0 == 8) {
		st = DEREF_STATION(v->u.air.targetairport);
		if (HASBIT(st->airport_flags, 8)) {
			if (HASBIT(st->airport_flags, 10)) {
				v->cur_speed = 0;
				v->subspeed = 0;
				return;
			}
			SETBIT(st->airport_flags, 10);
			CLRBIT(st->airport_flags, 11);
		}
		v->u.air.unk0 = 9;
	} else {
		AircraftEnterHangar(v);
	}
}

// In flight
static void HandleAircraft_InFlight(Vehicle *v)
{
	byte unk0, state;
	Station *st;
	int i;

	if (!Aircraft_5(v))
		return;

	unk0 = v->u.air.unk0;

	if (unk0 == 18) {
		v->u.air.unk0 = 19;
	} else if (unk0 == 19) {
		v->u.air.unk0 = 20;
	} else if (unk0 == 20) {
		v->u.air.unk0 = 16;
	} else if (unk0 == 17) {
		v->u.air.unk0 = 18;
	} else {
		st = DEREF_STATION(v->u.air.targetairport);
		if (st->airport_tile == 0 || (st->owner != 0x10 && st->owner != v->owner)) {
invalid_dest:;
			v->u.air.unk0 = 17;
			return;
		}

		i = (HASBIT(st->airport_flags,8) && v->subtype != 0) ? 11 : 7;
		if (HASBIT(st->airport_flags, i)) goto invalid_dest;
		if (HASBIT(st->airport_flags, 6)) {
			if (v->subtype != 0) goto invalid_dest;
			SETBIT(st->airport_flags, 7);
			v->u.air.state = AS_LANDED_TO_TERM_1;
			v->u.air.unk0 = 27;
		} else {
			SETBIT(st->airport_flags, i);
			
			state = AS_LAND_HANGAR;
			if ((v->next_order&OT_MASK) == OT_GOTO_STATION) {
				state = AS_LANDED_TO_TERM_1;
				if (HASBIT(st->airport_flags, 0)) {
					state++;
					if (HASBIT(st->airport_flags, 1)) {
						state++;
						if (HASBIT(st->airport_flags,2)) {
							state = AS_LAND_HANGAR;
						}
					}
				}
			}

			if (state != AS_LAND_HANGAR) {
				SETBIT(st->airport_flags, state - AS_LANDED_TO_TERM_1);
			}
			v->u.air.state = state;
			v->u.air.unk0 = v->subtype!=0 ? 21 : 24;
		}
	}

}


typedef void AircraftStateHandler(Vehicle *v);
static AircraftStateHandler * const _aircraft_state_handlers[] = {
	AircraftEventHandler_State0,

	AircraftEventHandler_State1to3,
	AircraftEventHandler_State1to3,
	AircraftEventHandler_State1to3,

	AircraftEventHandler_State4to6,
	AircraftEventHandler_State4to6,
	AircraftEventHandler_State4to6,

	HandleAircraft_LandedToTerminal,
	HandleAircraft_LandedToTerminal,
	HandleAircraft_LandedToTerminal,

	HandleAircraft_TerminalToHangar,
	HandleAircraft_TerminalToHangar,
	HandleAircraft_TerminalToHangar,

	HandleAircraft_TerminalTakeOff,
	HandleAircraft_TerminalTakeOff,
	HandleAircraft_TerminalTakeOff,

	HandleAircraft_HangarTakeOff,
	HandleAircraft_ToHangar,
	HandleAircraft_InFlight,
};

static void AircraftEventHandler(Vehicle *v, int loop)
{
	v->tick_counter++;

	if (v->vehstatus & VS_CRASHED) {
		HandleCrashedAircraft(v);
		return;
	}

	/* exit if aircraft is stopped */
	if (v->vehstatus & VS_STOPPED)
		return;

	/* aircraft is broken down? */
	if (v->breakdown_ctr != 0) {
		if (v->breakdown_ctr <= 2) {
			HandleBrokenAircraft(v);
		} else {
			v->breakdown_ctr--;
		}
	}

	HandleAircraftSmoke(v);
	ProcessAircraftOrder(v);
	HandleAircraftLoading(v, loop);

	if ((v->next_order&OT_MASK) >= OT_LOADING)
		return;

#if 0
	{
		static int oldst, oldunk0;

		static const char * const state_names[] = {
			"in depot",
			"at terminal 1",
			"at terminal 2",
			"at terminal 3",
			"moving from hangar to terminal 1",
			"moving from hangar to terminal 2",
			"moving from hangar to terminal 3",
			"landing, to terminal 1",
			"landing, to terminal 2",
			"landing, to terminal 3",
			"moving from terminal 1 to hangar",
			"moving from terminal 2 to hangar",
			"moving from terminal 3 to hangar",
			"take off from terminal 1",
			"take off from terminal 2",
			"take off from terminal 3",
			"take off from hangar",
			"landing, to hangar",
			"in flight",
		};

		if (oldst != v->u.air.state || oldunk0 != v->u.air.unk0) {
			oldst = v->u.air.state;
			oldunk0 = v->u.air.unk0;
			printf("Calling state '%s', unk0 is %d\n", state_names[v->u.air.state], v->u.air.unk0);
		}
	}
#endif

	_aircraft_state_handlers[v->u.air.state](v);
}

void Aircraft_Tick(Vehicle *v)
{
	int i;

	if (v->subtype > 2)
		return;

	if (v->subtype == 0)
		HelicopterTickHandler(v);

	AgeAircraftCargo(v);

	for(i=0; i!=6; i++) {
		AircraftEventHandler(v, i);
		if (v->type != VEH_Aircraft) // In case it was deleted
			break;
	}
}
