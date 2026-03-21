#include "stdafx.h"
#include "ttd.h"
#include "vehicle.h"
#include "command.h"
#include "pathfind.h"
#include "station.h"
#include "table/train_cmd.h"
#include "gfx.h"
#include "news.h"
#include "engine.h"
#include "player.h"

static const byte _vehicle_initial_x_fract[4] = {10,8,4,8};
static const byte _vehicle_initial_y_fract[4] = {8,4,8,10};

uint GetVehicleWeight(Vehicle *v)
{
	uint weight = 0;

	for(;;) {
		weight += _rail_vehicle_info[v->engine_type].weight;
		weight += (_cargoc.weights[v->cargo_type] * v->cargo_count) >> 4;

		if (v->next_in_chain == 0xffff)
			break;

		v = &_vehicles[v->next_in_chain];
	}

	return weight;
}

static void UpdateVehicleAcceleration(Vehicle *v)
{
	uint weight = GetVehicleWeight(v);
	uint acc = 0;

	if (weight != 0)
		acc = _rail_vehicle_info[v->engine_type].power / weight * 4;

	if (acc >= 255) acc=255;
	if (acc == 0) acc++;

	v->acceleration = (byte)acc;
}

int GetTrainImage(Vehicle *v, byte direction)
{
	int img = v->spritenum;
	int base;
	
	base = _engine_sprite_base[img] + ((direction + _engine_sprite_add[img]) & _engine_sprite_and[img]);

	if (v->cargo_count >= (v->cargo_cap >> 1))
		base += _wagon_full_adder[img];
	return base;
}

void DrawTrainEngine(int x, int y, int engine, uint32 image_ormod)
{
	const RailVehicleInfo *rvi = &_rail_vehicle_info[engine];
	
	int img = rvi->image_index;
	uint32 image;

	image = (6 & _engine_sprite_and[img]) + _engine_sprite_base[img];

	if (rvi->flags & RVI_MULTIHEAD) {
		DrawSprite(image | image_ormod, x-14, y);
		x += 15;
		image = ((6 + _engine_sprite_add[img+1]) & _engine_sprite_and[img+1]) + _engine_sprite_base[img+1];	
	}
	DrawSprite(image | image_ormod, x, y);
}

void DrawTrainEngineInfo(int engine, int x, int y, int maxw)
{
	const RailVehicleInfo *rvi = &_rail_vehicle_info[engine];
//	const Engine *e = &_engines[engine];
	int cap;

	SET_DPARAM32(0, ((_price.build_railvehicle >> 3) * rvi->base_cost) >> 5);
	SET_DPARAM16(2, rvi->max_speed * 10 >> 4);
	SET_DPARAM16(3, rvi->power);
	SET_DPARAM16(1, rvi->weight << ((rvi->flags & RVI_MULTIHEAD) ? 1 : 0));

	SET_DPARAM32(4,rvi->running_cost_base * _price.running_rail[rvi->running_cost_class-1] >> 8);
	
	cap = rvi->capacity;
	if (rvi->flags & RVI_MULTIHEAD) cap*=2;
	SET_DPARAM16(5, STR_8838_N_A);
	if (cap != 0) {
		SET_DPARAM16(6, cap);
		SET_DPARAM16(5, _cargoc.names_long_p[rvi->cargo_type]);
	}
	DrawStringMultiCenter(x, y, STR_885B_COST_WEIGHT_T_SPEED_POWER, maxw);
}


int32 CmdBuildRailWagon(uint engine, uint tile, uint32 flags)
{
	int32 value;
	Vehicle *v;
	const RailVehicleInfo *rvi;
	int dir;
	const Engine *e;
	int x,y;

	rvi = &_rail_vehicle_info[engine];
	value = (rvi->base_cost * _price.build_railwagon) >> 8;

	if (!(flags & DC_QUERY_COST)) {
		_error_message = STR_00E1_TOO_MANY_VEHICLES_IN_GAME;

		v = AllocateVehicle();
		if (v == NULL)
			return CMD_ERROR;

		if (flags & DC_EXEC) {
			byte img = rvi->image_index;
			Vehicle *u;

			v->spritenum = img;

			u = _vehicles;
			for(;;) {
				if (u->type == VEH_Train && u->tile == (TileIndex)tile &&
						u->subtype == 4 && u->spritenum == img) {
					
					u = GetLastVehicleInChain(u);
					break;
				}

				if (++u == endof(_vehicles)) {
					u = NULL;
					break;
				}
			}

			v->engine_type = engine;

			dir = _map5[tile] & 3;

			v->direction = (byte)(dir*2+1);
			v->tile = (TileIndex)tile;
			
			x = GET_TILE_X(tile)*16 | _vehicle_initial_x_fract[dir];
			y = GET_TILE_Y(tile)*16 | _vehicle_initial_y_fract[dir];

			v->x_pos = x;
			v->y_pos = y;
			v->z_pos = (byte)GetSlopeZ(x,y);
			v->owner = _current_player;
			v->z_height = 6;
			v->u.rail.track = 0x80;
			v->vehstatus = VS_HIDDEN | VS_DEFPAL;

			v->subtype = 4;
			if (u != NULL) {
				u->next_in_chain = v->index;
				v->subtype = 2;
			}

			v->cargo_type = rvi->cargo_type;
			v->cargo_cap = rvi->capacity;
			v->value = value;
//			v->day_counter = 0;

			e = &_engines[engine];
			v->u.rail.railtype = e->railtype;
			
			v->build_year = _cur_year;
			v->type = VEH_Train;
			v->cur_image = 0xAC2;
	
			_new_wagon_id = v->index;

			VehiclePositionChanged(v);

			InvalidateWindow(WC_VEHICLE_DEPOT, v->tile);
		}
	}

	return value;
}

static const byte _railveh_unk1[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 1, 0, 1, 0,
	0, 1, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
};

static const byte _railveh_score[] = {
	1, 4, 7, 19, 20, 30, 31, 19,
	20, 21, 22, 10, 11, 30, 31, 32,
	33, 34, 35, 29, 45, 32, 50, 40,
	41, 51, 52, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 60, 62,
	63, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 70, 71, 72, 73,
	74, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
};


/* Build a railroad vehicle
 * p1 = vehicle type id
 */

int32 CmdBuildRailVehicle(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	const RailVehicleInfo *rvi;
	int value,dir;
	Vehicle *v, *u;
	byte unit_num;
	Engine *e;
	uint tile;

	_cmd_build_rail_veh_var1 = 0;
	
	SET_EXPENSES_TYPE(EXPENSES_NEW_VEHICLES);

	tile = TILE_FROM_XY(x,y);
	rvi = &_rail_vehicle_info[p1];
	
	if (rvi->flags & RVI_WAGON) {
		return CmdBuildRailWagon(p1, tile, flags);
	}

	value = (rvi->base_cost * (_price.build_railvehicle >> 3)) >> 5;

	if (!(flags & DC_QUERY_COST)) {
		v = AllocateVehicle();
		if (v == NULL || _ptr_to_next_order >= endof(_order_array))
			return_cmd_error(STR_00E1_TOO_MANY_VEHICLES_IN_GAME);

		unit_num = GetFreeUnitNumber(VEH_Train);
		if (unit_num > 80)
			return_cmd_error(STR_00E1_TOO_MANY_VEHICLES_IN_GAME);

		if (flags & DC_EXEC) {
			v->unitnumber = unit_num;

			dir = _map5[tile] & 3;

			v->direction = (byte)(dir*2+1);
			v->tile = (TileIndex)tile;
			v->owner = _current_player;
			v->x_pos = (x |= _vehicle_initial_x_fract[dir]);
			v->y_pos = (y |= _vehicle_initial_y_fract[dir]);
			v->z_pos = (byte)GetSlopeZ(x,y);
			v->z_height = 6;
			v->u.rail.track = 0x80;
			v->vehstatus = VS_HIDDEN | VS_STOPPED | VS_DEFPAL;
//			v->subtype = 0;
			v->spritenum = rvi->image_index;
			v->cargo_type = rvi->cargo_type;
			v->cargo_cap = rvi->capacity;
			v->max_speed = rvi->max_speed;
//			v->cargo_count = 0;
			v->value = value;
//			v->day_counter = 0;
//			v->next_order = 0;
//			v->next_station = 0;
//			v->load_unload_time_rem = 0;
//			v->progress = 0;
//			v->targetairport = 0;
//			v->crash_anim_pos = 0;
			v->last_station_visited = 0xff;
			v->dest_tile = 0;
//			v->profit_last_year = 0;
//			v->profit_this_year = 0;
			
			v->engine_type = (byte)p1;
			e = &_engines[p1];

			v->reliability = e->reliability;
			v->reliability_spd_dec = e->reliability_spd_dec;
			v->max_age = e->lifelength * 366;
			
			v->string_id = STR_SV_TRAIN_NAME;
//			v->cur_speed = 0;
//			v->subspeed = 0;
			v->u.rail.railtype = e->railtype;
			_new_train_id = v->index;
//			v->cur_order_index = 0;
//			v->num_orders = 0;
			
			*(v->schedule_ptr = _ptr_to_next_order++) = 0;
			v->next_in_chain = 0xffff;
			
			v->service_interval = 150;
//			v->breakdown_ctr = 0;
//			v->breakdowns_since_last_service = 0;
//			v->unk4D = 0;
			v->date_of_last_service = _date;
			v->build_year = _cur_year;
			v->type = VEH_Train;
			v->cur_image = 0xAC2;

			UpdateVehicleAcceleration(v);
			VehiclePositionChanged(v);

			if (rvi->flags&RVI_MULTIHEAD && (u=AllocateVehicle()) != NULL) {
				u->direction = v->direction;
				u->owner = v->owner;
				u->tile = v->tile;
				u->x_pos = v->x_pos;
				u->y_pos = v->y_pos;
				u->z_pos = v->z_pos;
				u->z_height = 6;
				u->u.rail.track = 0x80;
				u->vehstatus = VS_HIDDEN | VS_DEFPAL;
				u->subtype = 2;
				u->spritenum = v->spritenum + 1;
				u->cargo_type = v->cargo_type;
				u->cargo_cap = v->cargo_cap;
				u->u.rail.railtype = v->u.rail.railtype;
				u->next_in_chain = 0xffff;
				v->next_in_chain = u->index;
				u->engine_type = v->engine_type;
				u->build_year = v->build_year;
				v->value = u->value = v->value >> 1;
//				u->day_counter = 0;
				u->type = VEH_Train;
				u->cur_image = 0xAC2;
				UpdateVehicleAcceleration(u);
				VehiclePositionChanged(u);
			}

			InvalidateWindow(WC_VEHICLE_DEPOT, tile);
			InvalidateWindow(WC_TRAINS_LIST, v->owner);
			InvalidateWindow(WC_COMPANY, v->owner);
		}
	}
	_cmd_build_rail_veh_var1 = _railveh_unk1[p1];
	_cmd_build_rail_veh_score = _railveh_score[p1];
	return value;		
}


bool IsTrainDepotTile(TileIndex tile)
{
	return IS_TILETYPE(tile, MP_RAILWAY) &&
					(_map5[tile] & 0xFC) == 0xC0;
}

bool IsTunnelTile(TileIndex tile)
{
	return IS_TILETYPE(tile, MP_TUNNELBRIDGE) &&
				 (_map5[tile]&0x80) == 0;
}


static int CheckStoppedInDepot(Vehicle *v)
{
	int count;
	TileIndex tile = v->tile;
	
	/* check if stopped in a depot */
	if (!IsTrainDepotTile(tile)) {
errmsg:
		_error_message = STR_881A_TRAINS_CAN_ONLY_BE_ALTERED;
		return -1;
	}

	for(count=0;;) {
		count++;
		if (v->u.rail.track != 0x80 || v->tile != (TileIndex)tile || 
				(v->subtype==0 && !(v->vehstatus&VS_STOPPED)))
			goto errmsg;
		if (v->next_in_chain == INVALID_VEHICLE)
			return count;
		v = &_vehicles[v->next_in_chain];
	}
}

/* p1 = source vehicle index
   p2 = dest vehicle index
 */

int32 CmdMoveRailVehicle(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *src, *dst, *src_head, *dst_head;
	uint tile;

	src = &_vehicles[p1];

	if (src->spritenum < 0x21)
		return 0;

	tile = src->tile;

	/* if nothing is selected as destination, try and find a matching vehicle to drag to. */
	if (p2 == (uint32)-1) {	
		uint16 eng = src->engine_type;

		dst = _vehicles;
		while(!(dst->type==VEH_Train && dst->subtype==4 && 
					dst->tile==(TileIndex)tile && dst->engine_type==eng)) {
			if (++dst == endof(_vehicles)) {
				dst = NULL;
				break;
			}
		}
	} else {
		dst = &_vehicles[p2];
	}
	
	/* the player must be the owner */
	if (!CheckOwnership(src->owner) ||
			(dst!=NULL && !CheckOwnership(dst->owner)))
				return CMD_ERROR;

	/* locate the head of the two chains */
	src_head = GetFirstVehicleInChain(src);
	dst_head = NULL;
	if (dst != NULL)
		dst_head = GetFirstVehicleInChain(dst);
	
	/* check if all vehicles in the source train are stopped */
	if (CheckStoppedInDepot(src_head) < 0)
		return CMD_ERROR;

	/* check if all the vehicles in the dest train are stopped,
	 * and that the length of the dest train is no longer than 9 vehicles */
	if (dst_head != NULL) {
		int num = CheckStoppedInDepot(dst_head);
		if (num < 0)
			return CMD_ERROR;

		if (num > (_patches.mammoth_trains ? 100 : 9) && dst_head->subtype==0)
			return_cmd_error(STR_8819_TRAIN_TOO_LONG);

		/* check something?? might be if dest is the 2nd loco of two. */
		if (_engine_sprite_add[dst->spritenum] != 0) {
			dst = GetPrevVehicleInChain(dst);
			assert(dst != NULL);
		}

		assert(dst_head->tile == src_head->tile);
	}

	/* do it? */
	if (flags & DC_EXEC) {
		Vehicle *v;
		VehicleID veh;

		/* if both src and dest are already unconnected, do nothing */
		if (src_head->subtype==4 && (dst_head==NULL || dst_head->subtype==4))
			return 0;
		
		v = src_head;
		veh = src->index;
		for(;;) {
			if (v->next_in_chain == veh) {
				v->next_in_chain = src->next_in_chain;
				break;
			}
			if (v->next_in_chain == INVALID_VEHICLE) {
				if (src->next_in_chain != INVALID_VEHICLE) {
					(&_vehicles[src->next_in_chain])->subtype = 4;
				}	
				break;
			}			
			v = &_vehicles[v->next_in_chain];
		}

		if (dst == NULL) {
			src->subtype = 4;
			src->next_in_chain = INVALID_VEHICLE;
		} else {
			src->subtype = 2;
			src->next_in_chain = dst->next_in_chain;
			dst->next_in_chain = src->index;
		}

		if (src_head->subtype == 0)
			UpdateVehicleAcceleration(src_head);

		if (dst_head!=NULL && dst_head->subtype==0)
			UpdateVehicleAcceleration(dst_head);

		InvalidateWindow(WC_VEHICLE_DEPOT, tile);
		InvalidateWindow(WC_TRAINS_LIST, _current_player);
		InvalidateWindow(WC_VEHICLE_DETAILS, src_head->index);
		if (dst_head != NULL)
			InvalidateWindow(WC_VEHICLE_DETAILS, dst_head->index);
	}

	return 0;
}

/* p1 = train to start / stop */
int32 CmdStartStopTrain(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v;

	v = &_vehicles[p1];

	if (!CheckOwnership(v->owner))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		v->u.rail.days_since_order_progr = 0;
		v->vehstatus ^= VS_STOPPED;
		InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
		InvalidateWindow(WC_VEHICLE_DEPOT, v->tile);
	}
	return 0;
}

static void SellRailLocoExec(Vehicle *v)
{
	VehicleID veh;
	Vehicle *u = v;

	veh = u->next_in_chain;
	while (veh != INVALID_VEHICLE) {
		u = &_vehicles[veh];
		veh = u->next_in_chain;
		DoCommandByTile(u->tile, u->index, (uint32)-1, DC_EXEC, CMD_MOVE_RAIL_VEHICLE);		
	}	
	
	DeleteVehicle(v);
	
	if (v->next_in_chain != INVALID_VEHICLE)
		DeleteVehicle(&_vehicles[v->next_in_chain]);
	
	InvalidateWindow(WC_VEHICLE_DEPOT, v->tile);
	InvalidateWindow(WC_TRAINS_LIST, v->owner);
	InvalidateWindow(WC_COMPANY, v->owner);
	
	DeleteWindowById(WC_VEHICLE_VIEW, v->index);	
}

/* p1 = vehicle */

int32 CmdSellRailLoco(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v;

	SET_EXPENSES_TYPE(EXPENSES_NEW_VEHICLES);

	v = &_vehicles[p1];
	
	/* must be locomotive, owned by current player */
	if (v->subtype != 0 || !CheckOwnership(v->owner))
		return CMD_ERROR;

	/* the whole train must be stopped in a depot */
	if (CheckStoppedInDepot(v) < 0)
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		SellRailLocoExec(v);
	}
	return - (int32)(v->value << ((_rail_vehicle_info[v->engine_type].flags&RVI_MULTIHEAD)?1:0));
}

static void SellRailWagonExec(Vehicle *v, Vehicle *first)
{
	if (v->subtype == 4) {
		if (v->next_in_chain != INVALID_VEHICLE) {
			(&_vehicles[v->next_in_chain])->subtype = 4;
		}
	} else {
		Vehicle *u;
		for(u=first; u->next_in_chain!=v->index; u=&_vehicles[u->next_in_chain]) {}
		u->next_in_chain = v->next_in_chain;
	}

	DeleteVehicle(v);
	if (first->subtype == 0)
		UpdateVehicleAcceleration(first);

	InvalidateWindow(WC_VEHICLE_DETAILS, first->index);
	InvalidateWindow(WC_VEHICLE_DEPOT, first->tile);
	InvalidateWindow(WC_TRAINS_LIST, first->owner);			
}

/* p1 = vehicle */
int32 CmdSellRailWagon(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v, *first;
	int eng;

	SET_EXPENSES_TYPE(EXPENSES_NEW_VEHICLES);

	v = &_vehicles[p1];

	if (v->subtype == 0)
		return CMD_ERROR;
	
	eng = v->engine_type;

	if (eng < 27 || (eng>=54 && eng<57) ||
			(eng>=84 && eng<89))
				return CMD_ERROR;

	if (!CheckOwnership(v->owner))
		return CMD_ERROR;

	first = GetFirstVehicleInChain(v);

	if (CheckStoppedInDepot(first) < 0)
		return CMD_ERROR;

	if (flags & DC_EXEC) SellRailWagonExec(v, first);

	return -(int32)v->value;
}

static void UpdateTrainDeltaXY(Vehicle *v, int direction)
{
#define MKIT(a,b,c,d) ((a&0xFF)<<24) | ((b&0xFF)<<16) | ((c&0xFF)<<8) | ((d&0xFF)<<0)
	static const uint32 _delta_xy_table[8] = {
		MKIT(3, 3, -1, -1),
		MKIT(3, 7, -1, -3),
		MKIT(3, 3, -1, -1),
		MKIT(7, 3, -3, -1),
		MKIT(3, 3, -1, -1),
		MKIT(3, 7, -1, -3),
		MKIT(3, 3, -1, -1),
		MKIT(7, 3, -3, -1),
	};
#undef MKIT

	uint32 x = _delta_xy_table[direction];

	v->x_offs = (byte)x;
	v->y_offs = (byte)(x>>=8);
	v->sprite_width = (byte)(x>>=8);
	v->sprite_height = (byte)(x>>=8);
}

static void UpdateVarsAfterSwap(Vehicle *v)
{
	UpdateTrainDeltaXY(v, v->direction);
	v->cur_image = GetTrainImage(v, v->direction);
	BeginVehicleMove(v);
	VehiclePositionChanged(v);
	EndVehicleMove(v);
}

static void ReverseTrainSwapVeh(Vehicle *v, int l, int r)
{
	Vehicle *a, *b;

	/* locate vehicles to swap */
	for(a=v; l!=0; l--) { a = &_vehicles[a->next_in_chain]; }
	for(b=v; r!=0; r--) { b = &_vehicles[b->next_in_chain]; }

	if (a != b) {
		/* swap the hidden bits */
		{
			uint16 tmp = (a->vehstatus & ~VS_HIDDEN) | (b->vehstatus&VS_HIDDEN);
			b->vehstatus = (b->vehstatus & ~VS_HIDDEN) | (a->vehstatus&VS_HIDDEN);
			a->vehstatus = tmp;
		}
		
		/* swap variables */
		swap_byte(&a->u.rail.track, &b->u.rail.track);
		swap_byte(&a->direction, &b->direction);

		/* toggle direction */
		if (!(a->u.rail.track & 0x80)) a->direction ^= 4;
		if (!(b->u.rail.track & 0x80)) b->direction ^= 4;
		
		/* swap more variables */
		swap_int16(&a->x_pos, &b->x_pos);
		swap_int16(&a->y_pos, &b->y_pos);
		swap_uint16(&a->tile, &b->tile);
		swap_byte(&a->z_pos, &b->z_pos);

		/* update other vars */
		UpdateVarsAfterSwap(a);
		UpdateVarsAfterSwap(b);
	} else {
		if (!(a->u.rail.track & 0x80)) a->direction ^= 4;
		UpdateVarsAfterSwap(a);		
	}
}

static void ReverseTrainDirection(Vehicle *v)
{
	int l = 0;
	int r = -1;
	Vehicle *u;

	/* count amount of vehicles */
	for(u=v; r++, u->next_in_chain != INVALID_VEHICLE; u = &_vehicles[u->next_in_chain]) {}

	/* swap start<>end, start+1<>end-1, ... */
	do {
		ReverseTrainSwapVeh(v, l++, r--);
	} while (l <= r);
}

/* p1 = vehicle */
int32 CmdReverseTrainDirection(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v;

	v = &_vehicles[p1];

	if (!CheckOwnership(v->owner))
		return CMD_ERROR;

	_error_message = STR_EMPTY;

	if (v->u.rail.track & 0x80 || IsTrainDepotTile(v->tile))
		return CMD_ERROR;

	if (v->u.rail.crash_anim_pos != 0 || v->breakdown_ctr != 0)
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		v->cur_speed = 0;
		ReverseTrainDirection(v);
	}
	return 0;
}

int32 CmdForceTrainProceed(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v;

	v = &_vehicles[p1];

	if (!CheckOwnership(v->owner))
		return CMD_ERROR;

	if (flags & DC_EXEC)
		v->u.rail.force_proceed = 0x50;
	
	return 0;
}

typedef struct TrainFindDepotData {
	uint best_length;
	uint tile;
	byte owner;
} TrainFindDepotData;

static bool TrainFindDepotEnumProc(uint tile, TrainFindDepotData *tfdd, int track, uint length, byte *state)
{
	if (IS_TILETYPE(tile, MP_RAILWAY) &&
			(_map5[tile] & ~0x3) == 0xC0 &&
			_map_owner[tile] == tfdd->owner) {
			
		if (length < tfdd->best_length) {
			tfdd->best_length = length;
			tfdd->tile = tile;
		}
	}
	return false;
}

int GetDepotByTile(uint tile)
{
	Depot *d;
	int i=0;
	for(d=_depots; d->xy != (TileIndex)tile; d++) { i++; }
	return i;
}

static int FindClosestTrainDepot(Vehicle *v)
{
	uint tile = v->tile;
	
	int i;
	TrainFindDepotData tfdd;

	if (!IsTrainDepotTile(tile)) {	
		if (v->u.rail.track == 0x40) { tile = GetVehicleOutOfTunnelTile(v); }
			
		tfdd.owner = v->owner;
		tfdd.best_length = (uint)-1;

		/* search in all directions */
		for(i=0; i!=4; i++)
			FollowTrack(tile, 0x3000, i, (TPFEnumProc*)TrainFindDepotEnumProc, NULL, &tfdd);

		if (tfdd.best_length == (uint)-1)
			return -1;
		tile = tfdd.tile;
	}

	return GetDepotByTile(tile);
}

int32 CmdTrainGotoDepot(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v = &_vehicles[p1];
	int depot;

	if ((v->next_order & OT_MASK) == OT_GOTO_DEPOT) {
		if (flags & DC_EXEC) {

			if (v->next_order & OF_UNLOAD) {
				v->u.rail.days_since_order_progr = 0;
				v->cur_order_index++;
			}
			
			v->next_order = OT_DUMMY;
			InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
		}
		return 0;
	}

	depot = FindClosestTrainDepot(v);
	if (depot < 0)
		return_cmd_error(STR_883A_UNABLE_TO_FIND_ROUTE_TO);

	if (flags & DC_EXEC) {
		v->next_order = OF_NON_STOP | OF_FULL_LOAD | OT_GOTO_DEPOT;
		v->next_order_param = (byte)depot;
		v->dest_tile = _depots[depot].xy;
		InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
	}
	
	return 0;
}

/* p1 = vehicle
 * p2 = new service int
 */
int32 CmdChangeTrainServiceInt(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v = &_vehicles[p1];

	if (!CheckOwnership(v->owner))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		v->service_interval = (uint16)p2;
		InvalidateWindowWidget(WC_VEHICLE_DETAILS, v->index, 8);
	}
	
	return 0;
}

void OnTick_Train()
{
	_age_cargo_skip_counter = (_age_cargo_skip_counter == 0) ? 184 : (_age_cargo_skip_counter - 1);
}

static void AgeTrainCargo(Vehicle *v)
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

static const int8 _vehicle_smoke_pos[16] = {
	-4, -4, -4, 0, 4, 4, 4, 0,
	-4,  0,  4, 4, 4, 0,-4,-4,
};

static void HandleSmokeCloudEngine0_to_4(Vehicle *v)
{		
	if ( (v->tick_counter&0x1F) == 0 &&
			 v->cur_speed >= 2 &&
			 v->load_unload_time_rem == 0 &&
			 !(v->vehstatus&VS_HIDDEN) &&
			 !(v->u.rail.track & 0xC0) &&
			 !IsTrainDepotTile(v->tile) &&
			 !IsTunnelTile(v->tile)) {
		
		CreateEffectVehicleRel(v, 
			(_vehicle_smoke_pos[v->direction]),
			(_vehicle_smoke_pos[v->direction+8]),
			10,
			EV_1);
	}
}

static void HandleSmokeCloudEngine4_to_14(Vehicle *v)
{
	if (v->cur_speed < 2 ||
			v->cur_speed > 40 ||
			v->load_unload_time_rem != 0)
				return;
			
	if (!(v->vehstatus&VS_HIDDEN) &&
			!(v->u.rail.track & 0xC0) &&
			!IsTrainDepotTile(v->tile) &&
			!IsTunnelTile(v->tile) &&
			(uint16)Random() <= 0x1E00) {
		CreateEffectVehicleRel(v,0,0,10,EV_SMOKE_3);
	}

	if (!(_rail_vehicle_info[v->engine_type].flags & RVI_MULTIHEAD))
		return;
	
	v = GetLastVehicleInChain(v);
	if (!(v->vehstatus&VS_HIDDEN) &&
			!(v->u.rail.track & 0xC0) &&
			!IsTrainDepotTile(v->tile) &&
			!IsTunnelTile(v->tile) &&
			(uint16)Random() <= 0x1E00) {
		CreateEffectVehicleRel(v, 0,0,10,EV_SMOKE_3);	
	}
}

static void HandleSmokeCloudEngine14_to_19(Vehicle *v)
{
	if ( (v->tick_counter&0x7) == 0 &&
			 v->cur_speed >= 2 &&
			 v->load_unload_time_rem == 0 &&
			 !(v->vehstatus&VS_HIDDEN) &&
			 !(v->u.rail.track & 0xC0) &&
			 !IsTrainDepotTile(v->tile) &&
			 !IsTunnelTile(v->tile) &&
			 (uint16)Random() <= 0x5B0
			 ) {
		
		CreateEffectVehicleRel(v, 0,0,10, EV_SMOKE_2);
	}
}

static void HandleLocomotiveSmokeCloud(Vehicle *v)
{
	if (v->vehstatus & VS_TRAIN_SLOWING)
		return;

	if (v->spritenum < 4) {
		HandleSmokeCloudEngine0_to_4(v);
	} else if (v->spritenum < 0x14) {
		HandleSmokeCloudEngine4_to_14(v);
	} else if (v->spritenum < 0x19) {
		HandleSmokeCloudEngine14_to_19(v);
	}
}

static void TrainPlayLeaveStationSound(Vehicle *v)
{
	if (v->spritenum < 4) {
		SndPlayVehicleFx(2, v);
	} else if (v->spritenum < 0x19) {
		SndPlayVehicleFx(8, v);
	} else if (v->spritenum < 0x1C) {
		SndPlayVehicleFx(0x47, v);
	} else {
		SndPlayVehicleFx(0x41, v);
	}
}

static bool CheckTrainStayInDepot(Vehicle *v)
{
	Vehicle *u;
	if (v->u.rail.track != 0x80)
		return false;

	// make sure that all vehicles are in the depot
	u = GetLastVehicleInChain(v);
	if (u->u.rail.track != 0x80)
		return false;

	if (v->u.rail.force_proceed == 0) {
		if (++v->load_unload_time_rem < 37)
			return true;
		v->load_unload_time_rem = 0;

		if (UpdateSignalsOnSegment(v->tile, v->direction))
			return true;
	}

	TrainPlayLeaveStationSound(v);
	
	v->u.rail.track = 1;
	if (v->direction & 2)
		v->u.rail.track = 2;
	
	v->vehstatus &= ~VS_HIDDEN;
	v->cur_speed = 0;
	
	UpdateTrainDeltaXY(v, v->direction);
	v->cur_image = GetTrainImage(v, v->direction);
	VehiclePositionChanged(v);
	UpdateSignalsOnSegment(v->tile, v->direction);
	UpdateVehicleAcceleration(v);
	InvalidateWindow(WC_VEHICLE_DEPOT, v->tile);

	return false;
}

typedef struct TrainTrackFollowerData {
	TileIndex dest_coords;
	TileIndex station_coord[3];
	uint best_bird_dist;
	uint best_track_dist;
} TrainTrackFollowerData;

static const byte _signal_onedir[14] = {
	0x80, 0x80, 0x80, 0x20, 0x40, 0x10, 0, 0, 
	0x40, 0x40, 0x40, 0x10, 0x80, 0x20
};

static const byte _signal_otherdir[14] = {
	0x40, 0x40, 0x40, 0x10, 0x80, 0x20, 0, 0,
	0x80, 0x80, 0x80, 0x20, 0x40, 0x10
};



static bool TrainTrackFollower(uint tile, TrainTrackFollowerData *ttfd, int track, uint length, byte *state){
	if (IS_TILETYPE(tile, MP_RAILWAY) && (_map5[tile]&0xC0) == 0x40) {
		// the tile has a signal
		byte m3 = _map3_lo[tile];

		if (m3 & _signal_onedir[track] && _map2[tile] & _signal_onedir[track]) {
			*state |= 1;
			/* a green light */
		} else {
			if (m3 & _signal_otherdir[track] && !(*state & 1)) {
				return true; /* don't continue if it's a two way red signal. */
			}
		}
	}

	if (ttfd->dest_coords == 0)
		return false;

	if ( (TileIndex)tile != ttfd->dest_coords &&
			 (TileIndex)tile != ttfd->station_coord[0] &&
			 (TileIndex)tile != ttfd->station_coord[1] &&
			 (TileIndex)tile != ttfd->station_coord[2]) {
		uint dist = GetTileDist(tile, ttfd->dest_coords);
		
		if (dist < ttfd->best_bird_dist)
			ttfd->best_bird_dist = dist;

		return false;
	} else {
		/* found station */
		ttfd->best_bird_dist = 0;
		if (length < ttfd->best_track_dist)
			ttfd->best_track_dist = length;
		return true;
	}
}

static void FillWithStationData(TrainTrackFollowerData *fd, Vehicle *v)
{
	uint tile;
	TileIndex *t;
	int delta;

	fd->dest_coords = tile = v->dest_tile;
	fd->station_coord[0] = 0;
	fd->station_coord[1] = 0;
	fd->station_coord[2] = 0;

	if (IS_TILETYPE(tile, MP_STATION) &&
			IS_BYTE_INSIDE(_map5[tile], 0, 8) ) {
		t = fd->station_coord;		
		delta = (_map5[tile] & 1) ? TILE_XY(1,0) : TILE_XY(0,1);
		for(;;) {
			tile += delta;
			if (!(IS_TILETYPE(tile, MP_STATION) && IS_BYTE_INSIDE(_map5[tile], 0, 8)))
				break;
			*t++ = (TileIndex)tile;
		}
	}
}

static const byte _initial_tile_subcoord[6][4][3] = {
{{ 15, 8, 1 },{ 0, 0, 0 },{ 0, 8, 5 },{ 0, 0, 0 }},
{{  0, 0, 0 },{ 8, 0, 3 },{ 0, 0, 0 },{ 8,15, 7 }},
{{  0, 0, 0 },{ 7, 0, 2 },{ 0, 7, 6 },{ 0, 0, 0 }},
{{ 15, 8, 2 },{ 0, 0, 0 },{ 0, 0, 0 },{ 8,15, 6 }},
{{ 15, 7, 0 },{ 8, 0, 4 },{ 0, 0, 0 },{ 0, 0, 0 }},
{{  0, 0, 0 },{ 0, 0, 0 },{ 0, 8, 4 },{ 7,15, 0 }},
};

static const uint32 _reachable_tracks[4] = {
	0x10091009,
	0x00160016,
	0x05200520,
	0x2A002A00,
};

static const byte _search_directions[6][4] = {
	{ 0, 9, 2, 9 },
	{ 9, 1, 9, 3 },
	{ 9, 0, 3, 9 },
	{ 1, 9, 9, 2 },
	{ 3, 2, 9, 9 },
	{ 9, 9, 1, 0 },
};

static const byte _pick_track_table[6] = {1, 3, 2, 2, 0, 0};

/* choose a track */
static byte ChooseTrainTrack(Vehicle *v, uint tile, int direction, byte trackbits)
{	
	TrainTrackFollowerData fd;
	int bits = trackbits;
	int i, r;
	int best_track;
	uint best_bird_dist;
	uint best_track_dist;
	byte train_dir = v->direction & 3;

	assert( (bits & ~0x3F) == 0);

	/* quick return in case only one possible direction is available */
	if (KILL_FIRST_BIT(bits) == 0)
		return FIND_FIRST_BIT(bits);

	FillWithStationData(&fd, v);

	best_track = -1;

	do {
		i = FIND_FIRST_BIT(bits);
		bits = KILL_FIRST_BIT(bits);

		fd.best_bird_dist = (uint)-1;
		fd.best_track_dist = (uint)-1;
		FollowTrack(tile, 0x3000, _search_directions[i][direction], (TPFEnumProc*)TrainTrackFollower, NULL, &fd);

		if (best_track != -1) {
			if (best_bird_dist != 0) {
				if (fd.best_bird_dist != 0) {
					/* neither reached the destination, pick the one with the smallest bird dist */
					if (fd.best_bird_dist > best_bird_dist) goto bad;
					if (fd.best_bird_dist < best_bird_dist) goto good;
				} else {
					/* we found the destination for the first time */
					goto good;		
				}
			} else {
				if (fd.best_bird_dist != 0) {
					/* didn't find destination, but we've found the destination previously */
					goto bad;
				} else {
					/* both old & new reached the destination, compare track length */
					if (fd.best_track_dist > best_track_dist) goto bad;
					if (fd.best_track_dist < best_track_dist) goto good;
				}
			}
			
			/* if we reach this position, there's two paths of equal value so far. 
			 * pick one randomly. */
			r = (byte)Random();
			if (_pick_track_table[i] == train_dir) r += 80;
			if (_pick_track_table[best_track] == train_dir) r -= 80;
	
			if (r <= 127) goto bad;
		}
good:;
		best_track = i;
		best_bird_dist = fd.best_bird_dist;
		best_track_dist = fd.best_track_dist;
bad:;

	} while (bits != 0);

	assert(best_track != -1);

	return best_track;
}


static bool CheckReverseTrain(Vehicle *v)
{
	TrainTrackFollowerData fd;
	int i, r;
	int best_track;
	uint best_bird_dist;
	uint best_track_dist;
	uint reverse, reverse_best;

	if (_opt.diff.line_reverse_mode != 0 ||
			v->u.rail.track & 0xC0 ||
			!(v->direction & 1))
		return false;

	FillWithStationData(&fd, v);

	best_track = -1;
	reverse_best = reverse = 0;

	assert(v->u.rail.track);

	i = _search_directions[FIND_FIRST_BIT(v->u.rail.track)][v->direction>>1];

	while(true) {
		fd.best_bird_dist = (uint)-1;
		fd.best_track_dist = (uint)-1;
		FollowTrack(v->tile, 0x3000, reverse ^ i, (TPFEnumProc*)TrainTrackFollower, NULL, &fd);

		if (best_track != -1) {
			if (best_bird_dist != 0) {
				if (fd.best_bird_dist != 0) {
					/* neither reached the destination, pick the one with the smallest bird dist */
					if (fd.best_bird_dist > best_bird_dist) goto bad;
					if (fd.best_bird_dist < best_bird_dist) goto good;
				} else {
					/* we found the destination for the first time */
					goto good;		
				}
			} else {
				if (fd.best_bird_dist != 0) {
					/* didn't find destination, but we've found the destination previously */
					goto bad;
				} else {
					/* both old & new reached the destination, compare track length */
					if (fd.best_track_dist > best_track_dist) goto bad;
					if (fd.best_track_dist < best_track_dist) goto good;
				}
			}
			
			/* if we reach this position, there's two paths of equal value so far. 
			 * pick one randomly. */
			r = (byte)Random();
			if (_pick_track_table[i] == (v->direction & 3)) r += 80;
			if (_pick_track_table[best_track] == (v->direction & 3)) r -= 80;
			if (r <= 127) goto bad;
		}
good:;
		best_track = i;
		best_bird_dist = fd.best_bird_dist;
		best_track_dist = fd.best_track_dist;
		reverse_best = reverse;
bad:;
		if (reverse != 0)
			break;
		reverse = 2;
	}

	return reverse_best != 0;
}

static bool ProcessTrainOrder(Vehicle *v)
{
	uint order;
	bool result;

	// These are un-interruptible
	if ((v->next_order & OT_MASK) >= OT_GOTO_DEPOT && (v->next_order & OT_MASK) <= OT_LEAVESTATION) {
		
		// Let a depot order in the schedule interrupt.
		if ((v->next_order & (OT_MASK|OF_UNLOAD)) != (OT_GOTO_DEPOT|OF_UNLOAD))
			return false;
	}

	// check if we've reached the checkpoint?
	if ((v->next_order & OT_MASK) == OT_GOTO_CHECKPOINT && v->tile == v->dest_tile) {
		v->cur_order_index++;
	}


	// Get the current order
	if (v->cur_order_index >= v->num_orders)
		v->cur_order_index = 0;
	order = v->schedule_ptr[v->cur_order_index];

	// If no order, do nothing.
	if (order == 0) {
		v->next_order = OT_NOTHING;
		v->dest_tile = 0;
		return false;
	}

	// If it is unchanged, keep it.
	if (order == (uint)((v->next_order | (v->next_order_param<<8))))
		return false;

	// Otherwise set it, and determine the destination tile.
	v->next_order = (byte)order;
	v->next_order_param = (byte)(order >> 8);

	v->dest_tile = 0;

	result = false;
	if ((order & OT_MASK) == OT_GOTO_STATION) {
		if ( (byte)(order >> 8) == v->last_station_visited)
			v->last_station_visited = 0xFF;
		v->dest_tile = DEREF_STATION(order >> 8)->xy;
		result = CheckReverseTrain(v);
	} else if ((order & OT_MASK) == OT_GOTO_DEPOT) {
		v->dest_tile = _depots[order >> 8].xy;
		result = CheckReverseTrain(v);
	} else if ((order & OT_MASK) == OT_GOTO_CHECKPOINT) {
		v->dest_tile = _checkpoints[order >> 8].xy;
		result = CheckReverseTrain(v);
	}

	InvalidateVehicleOrderWidget(v);

	return result;
}

static void MarkTrainDirty(Vehicle *v)
{
	for(;;) {
		v->cur_image = GetTrainImage(v, v->direction);
		MarkAllViewportsDirty(v->left_coord, v->top_coord, v->right_coord + 1, v->bottom_coord + 1);
		if (v->next_in_chain == INVALID_VEHICLE)
			break;
		v = &_vehicles[v->next_in_chain];
	}
}

static void HandleTrainLoading(Vehicle *v, bool mode)
{
	if (v->next_order == OT_NOTHING)
		return;
	
	if (v->next_order != OT_DUMMY) {
		if ((v->next_order&OT_MASK) != OT_LOADING)
			return;

		if (mode)
			return;

		if (--v->load_unload_time_rem)
			return;

		if (v->next_order&OF_FULL_LOAD && CanFillVehicle(v)) {
			SET_EXPENSES_TYPE(EXPENSES_TRAIN_INC);
			if (LoadUnloadVehicle(v)) {
				InvalidateWindow(WC_TRAINS_LIST, v->owner);
				MarkTrainDirty(v);
			}
			UpdateVehicleAcceleration(v);
			return;
		}
		
		TrainPlayLeaveStationSound(v);
		
		{
			byte b = v->next_order;
			v->next_order = OT_LEAVESTATION;
			
			// If this was not the final order, don't remove it from the list.
			if (!(b & OF_NON_STOP))
				return;
		}
	}

	v->u.rail.days_since_order_progr = 0;
	v->cur_order_index++;
	InvalidateVehicleOrderWidget(v);
}

static int UpdateTrainSpeed(Vehicle *v)
{
	uint spd;

	spd = v->subspeed + v->acceleration * 2;
	v->subspeed = (byte)spd;
	v->cur_speed = spd = min(v->cur_speed + (spd >> 8), v->max_speed);

	if (!(v->direction & 1)) spd = spd * 3 >> 2;

	spd += v->progress;
	v->progress = (byte)spd;
	return (spd >> 8);
}

static void TrainEnterStation(Vehicle *v, int station)
{
	Station *st;
	uint32 flags;

	v->last_station_visited = station;

	/* check if a train ever visited this station before */
	st = DEREF_STATION(station);
	if (!(st->had_vehicle_of_type & HVOT_TRAIN)) {
		st->had_vehicle_of_type |= HVOT_TRAIN;
		SET_DPARAM16(0, st->index);
		flags = (v->owner == _local_player) ? NEWS_FLAGS(NM_THIN, NF_VIEWPORT|NF_VEHICLE, NT_ARRIVAL_PLAYER, 0) : NEWS_FLAGS(NM_THIN, NF_VIEWPORT|NF_VEHICLE, NT_ARRIVAL_OTHER, 0);
		AddNewsItem(
			STR_8801_CITIZENS_CELEBRATE_FIRST,
			flags,
			v->index,
			0);
	}

	// Did we reach the final destination?
	if ((v->next_order&OT_MASK) == OT_GOTO_STATION && v->next_order_param == (byte)station) {
		// Yeah, keep the load/unload flags
		// Non Stop now means if the order should be increased.
		v->next_order = (v->next_order & (OF_FULL_LOAD|OF_UNLOAD)) | OF_NON_STOP | OT_LOADING;
	} else {
		// No, just do a simple load
		v->next_order = OT_LOADING;
	}
	v->next_order_param = 0;

	SET_EXPENSES_TYPE(EXPENSES_TRAIN_INC);
	if (LoadUnloadVehicle(v) != 0) {
		InvalidateWindow(WC_TRAINS_LIST, v->owner);
		MarkTrainDirty(v);
	}

	UpdateVehicleAcceleration(v);
	InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
}

static byte AfterSetVehiclePos(Vehicle *v)
{
	uint r = GetSlopeZ(v->x_pos, v->y_pos);
	byte old_z = v->z_pos;

	if (r & 256 && (byte)r != v->z_pos)
		r+=8;

	v->z_pos = (byte)r;
	
	VehiclePositionChanged(v);
	EndVehicleMove(v);
	return old_z;
}

static const byte _new_vehicle_direction_table[11] = {
	0, 7, 6, 0,
	1, 0, 5, 0,
	2, 3, 4,
};

static int GetNewVehicleDirectionByTile(uint new_tile, uint old_tile)
{
	uint offs = (GET_TILE_Y(new_tile) - GET_TILE_Y(old_tile) + 1) * 4 + 
							GET_TILE_X(new_tile) - GET_TILE_X(old_tile) + 1;
	assert(offs < 11);
	return _new_vehicle_direction_table[offs];
}

static int GetNewVehicleDirection(Vehicle *v, int x, int y)
{
	uint offs = (y - v->y_pos + 1) * 4 + (x - v->x_pos + 1);
	assert(offs < 11);
	return _new_vehicle_direction_table[offs];
}

static int GetDirectionToVehicle(Vehicle *v, int x, int y)
{
	byte offs;

	x -= v->x_pos;
	if (x >= 0) {
		offs = (x > 2) ? 0 : 1;
	} else {
		offs = (x < -2) ? 2 : 1;
	}

	y -= v->y_pos;
	if (y >= 0) {
		offs += ((y > 2) ? 0 : 1) * 4;
	} else {
		offs += ((y < -2) ? 2 : 1) * 4;
	}

	assert(offs < 11);
	return _new_vehicle_direction_table[offs];
}

/* Check if the vehicle is compatible with the specified tile */
static bool CheckCompatibleRail(Vehicle *v, uint tile)
{
	if (IS_TILETYPE(tile, MP_RAILWAY) ||
			IS_TILETYPE(tile, MP_STATION)) {

	} else if (IS_TILETYPE(tile, MP_TUNNELBRIDGE)) {
		if ((_map5[tile] & 0xC0) == 0xC0)
			return true;
	} else
		return true;

	if (_map_owner[tile] != v->owner ||
			(v->subtype == 0 && (_map3_lo[tile] & 0xF) != v->u.rail.railtype))
		return false;
	
	return true;
}

/* Modify the speed of the vehicle due to a turn */
static void AffectSpeedByDirChange(Vehicle *v, byte new_dir)
{
	byte diff = (v->direction - new_dir) & 7;
	if (diff == 0)
		return;

	if (diff == 1 || diff == 7) {
		v->cur_speed -= v->cur_speed >> 2;
	} else {
		v->cur_speed >>= 1;
	}
}

/* Modify the speed of the vehicle due to a change in altitude */
static void AffectSpeedByZChange(Vehicle *v, byte old_z)
{
	if (old_z == v->z_pos)
		return;

	if (old_z < v->z_pos) {
		v->cur_speed -= v->cur_speed >> 2;
	} else {
		uint16 spd = v->cur_speed + 2;
		if (spd <= v->max_speed)
			v->cur_speed = spd;
	}
}

static const byte _otherside_signal_directions[14] = {
	1, 3, 1, 3, 5, 3, 0, 0,
	5, 7, 7, 5, 7, 1,
};

static void TrainMovedChangeSignals(uint tile, int dir)
{
	int i;
	if (IS_TILETYPE(tile, MP_RAILWAY) && (_map5[tile]&0xC0)==0x40) {
		i = FindFirstBit2x64((_map5[tile]+(_map5[tile]<<8)) & _reachable_tracks[dir]);
		UpdateSignalsOnSegment(tile, _otherside_signal_directions[i]);
	}
}


typedef struct TrainCollideChecker {
	Vehicle *v, *v_skip;

} TrainCollideChecker;

void *FindTrainCollideEnum(Vehicle *v, TrainCollideChecker *tcc)
{
	if (v == tcc->v || v == tcc->v_skip || v->type != VEH_Train || v->u.rail.track==0x80)
		return 0;

	if ( myabs(v->z_pos - tcc->v->z_pos) > 6 ||
			 myabs(v->x_pos - tcc->v->x_pos) >= 6 ||
			 myabs(v->y_pos - tcc->v->y_pos) >= 6)
				return NULL;
	return v;
}

static void SetVehicleCrashed(Vehicle *v)
{
	Vehicle *u;

	if (v->u.rail.crash_anim_pos != 0)
		return;

	v->u.rail.crash_anim_pos++;
	
	u = v;
	BEGIN_ENUM_WAGONS(v)
		v->vehstatus |= VS_CRASHED;
	END_ENUM_WAGONS(v)

	InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
}

static int CountPassengersInTrain(Vehicle *v)
{
	int num = 0;
	
	BEGIN_ENUM_WAGONS(v)
		if (v->cargo_type == 0) num += v->cargo_count;
	END_ENUM_WAGONS(v)
	return num;
}

static void CheckTrainCollision(Vehicle *v)
{
	TrainCollideChecker tcc;
	Vehicle *coll;
	int num;

	/* can't collide in depot */
	if (v->u.rail.track == 0x80)
		return;
	
	assert(TILE_FROM_XY(v->x_pos, v->y_pos) == v->tile);

	tcc.v = v;
	tcc.v_skip = v->next_in_chain == INVALID_VEHICLE ? NULL : &_vehicles[v->next_in_chain];

	/* find colliding vehicle */
	coll = VehicleFromPos(v->tile, &tcc, (VehicleFromPosProc*)FindTrainCollideEnum);
	if (coll == NULL)
		return;
	
	coll = GetFirstVehicleInChain(coll);
	
	/* it can't collide with its own wagons */
	if (v == coll)
		return;

	SetVehicleCrashed(v);
	if (coll->subtype == 0)
		SetVehicleCrashed(coll);

	num = 4 + CountPassengersInTrain(v) + CountPassengersInTrain(coll);
	
	SET_DPARAM16(0, num);
	
	AddNewsItem(STR_8868_TRAIN_CRASH_DIE_IN_FIREBALL,
		NEWS_FLAGS(NM_THIN, NF_VIEWPORT|NF_VEHICLE, NT_ACCIDENT, 0),
		v->index,
		0);

	ModifyStationRatingAround(v->tile, v->owner, -160, 30);
	SndPlayVehicleFx(17, v);
}

static void *CheckVehicleAtSignal(Vehicle *v, void *data)
{
	uint32 d = (uint32)data;

	if (v->type == VEH_Train && v->subtype == 0 && v->tile == (TileIndex)(d >> 8)) {
		byte diff = (v->direction - (byte)d + 2) & 7;
		if (diff == 2 || (v->cur_speed <= 5 && diff <= 4))
			return (void*)1;
	}
	return 0;
}

static void TrainController(Vehicle *v)
{
	Vehicle *prev = NULL;
	GetNewVehiclePosResult gp;
	uint32 r, tracks,ts;
	int dir, i;
	byte chosen_dir;
	byte chosen_track;
	byte old_z;

	for(;;) {
		BeginVehicleMove(v);
		
		if (v->u.rail.track != 0x40) {
			if (GetNewVehiclePos(v, &gp)) {
				/* Statying in the old tile */
				if (v->u.rail.track == 0x80) {
					/* inside depot */
					gp.x = v->x_pos;
					gp.y = v->y_pos;
				} else {
					/* isnot inside depot */
					r = VehicleEnterTile(v, gp.new_tile, gp.x, gp.y);
					if (r & 0x8)
						goto invalid_rail;
					if (r & 0x2) {
						TrainEnterStation(v, r >> 8);
						return;
					}

					if (v->next_order == OT_LEAVESTATION) {
						v->next_order = OT_NOTHING;
						InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
					}
				}
			} else {
				/* A new tile is about to be entered. */

				/* Determine what direction we're entering the new tile from */
				dir = GetNewVehicleDirectionByTile(gp.new_tile, gp.old_tile);
				assert(dir==1 || dir==3 || dir==5 || dir==7);
				
				/* Get the status of the tracks in the new tile and mask
				 * away the bits that aren't reachable. */
				ts = GetTileTrackStatus(gp.new_tile, 0) & _reachable_tracks[dir >> 1];

				/* Combine the from & to directions.
				 * Now, the lower byte contains the track status, and the byte at bit 16 contains
				 * the signal status. */
				tracks = ts|(ts >> 8);
				if ( (byte) tracks == 0)
					goto invalid_rail;

				/* Check if the new tile contrains tracks that are compatible
				 * with the current train, if not, bail out. */
				if (!CheckCompatibleRail(v, gp.new_tile))
					goto invalid_rail;

				if (prev == NULL) {
					/* Currently the locomotive is active. Determine which one of the
					 * available tracks to choose */
					chosen_track = 1 << ChooseTrainTrack(v, gp.new_tile, dir>>1, (byte)tracks);

					/* Check if it's a red signal and that force proceed is not clicked. */
					if ( (tracks>>16)&chosen_track && v->u.rail.force_proceed == 0) goto red_light;
				} else {
					static byte _matching_tracks[8] = {0x30, 1, 0xC, 2, 0x30, 1, 0xC, 2};
					
					/* The wagon is active, simply follow the prev vehicle. */
					chosen_track = (byte)(_matching_tracks[GetDirectionToVehicle(prev, gp.x, gp.y)] & tracks);
				}

				/* make sure chosen track is a valid track */
				assert(chosen_track==1 || chosen_track==2 || chosen_track==4 || chosen_track==8 || chosen_track==16 || chosen_track==32);

				/* Update XY to reflect the entrance to the new tile, and select the direction to use */
				{
					const byte *b = _initial_tile_subcoord[FIND_FIRST_BIT(chosen_track)][dir>>1];
					gp.x = (gp.x & ~0xF) | b[0];
					gp.y = (gp.y & ~0xF) | b[1];
					chosen_dir = b[2];
				}
				
				/* Call the landscape function and tell it that the vehicle entered the tile */
				r = VehicleEnterTile(v, gp.new_tile, gp.x, gp.y);
				if (r&0x8)
					goto invalid_rail;

				if (v->subtype == 0) v->load_unload_time_rem = 0;

				if (!(r&0x4)) {
					v->tile = gp.new_tile;
					v->u.rail.track = chosen_track;
				}

				if (v->subtype == 0)
					TrainMovedChangeSignals(gp.new_tile, dir>>1);

				if (v->next_in_chain == INVALID_VEHICLE)
					TrainMovedChangeSignals(gp.old_tile, (dir>>1) ^ 2);
				
				if (prev == NULL) {
					AffectSpeedByDirChange(v, chosen_dir);
				}

				v->direction = chosen_dir;
			}
		} else {
			/* in tunnel */
			GetNewVehiclePos(v, &gp);
			
			if (IS_TILETYPE(gp.new_tile, MP_TUNNELBRIDGE) &&
					!(_map5[gp.new_tile] & 0xF0)) {
				r = VehicleEnterTile(v, gp.new_tile, gp.x, gp.y);
				if (r & 0x4) goto common;
			}

			v->x_pos = gp.x;
			v->y_pos = gp.y;
			VehiclePositionChanged(v);
			goto next_vehicle;
		}
common:;

		/* update image of train, as well as delta XY */
		dir = GetNewVehicleDirection(v, gp.x, gp.y);
		UpdateTrainDeltaXY(v, dir);
		v->cur_image = GetTrainImage(v, dir);

		v->x_pos = gp.x;
		v->y_pos = gp.y;

		/* update the Z position of the vehicle */
		old_z = AfterSetVehiclePos(v);
		if (prev == NULL) {
			AffectSpeedByZChange(v, old_z);
			CheckTrainCollision(v);
		}

		/* continue with next vehicle */
next_vehicle:;
		prev = v;
		if (v->next_in_chain == INVALID_VEHICLE)
			return;
		v = &_vehicles[v->next_in_chain];
	}

invalid_rail:
	if (prev != NULL) {
		error("Disconnecting train");
		//v->subtype = 4;
		//prev->next_in_chain = INVALID_VEHICLE;
	}
	goto reverse_train_direction;

red_light: {
		/* find the first set bit in ts. need to do it in 2 steps, since
		 * FIND_FIRST_BIT only handles 6 bits at a time. */
		i = FindFirstBit2x64(ts);
		
		if (!(_map3_lo[gp.new_tile] & _signal_otherdir[i])) {
			v->cur_speed = 0;
			v->subspeed = 0;
			v->progress = 255-100;
			if (++v->load_unload_time_rem < 254)
				return;
		} else if (_map3_lo[gp.new_tile] & _signal_onedir[i]){
			v->cur_speed = 0;
			v->subspeed = 0;
			v->progress = 255-10;
			if (++v->load_unload_time_rem < 254) {
				uint o_tile = gp.new_tile + _tileoffs_by_dir[dir>>1];
				/* check if a train is waiting on the other side */
				if (VehicleFromPos(o_tile, (void*)( (o_tile<<8) | (dir^4)), (VehicleFromPosProc*)CheckVehicleAtSignal) == NULL)
					return;
			}
		}
	}
		
reverse_train_direction:
	v->load_unload_time_rem = 0;
	v->cur_speed = 0;
	v->subspeed = 0;
	ReverseTrainDirection(v);

}

static void DeleteLastWagon(Vehicle *v)
{
	Vehicle *u = v;
	int t;

	while (v->next_in_chain != INVALID_VEHICLE) {
		u = v;
		v = &_vehicles[v->next_in_chain];
	}
	u->next_in_chain = 0xFFFF;

	InvalidateWindow(WC_VEHICLE_DETAILS, v->index);
	DeleteWindowById(WC_VEHICLE_VIEW, v->index);
	InvalidateWindow(WC_TRAINS_LIST, v->owner);
	InvalidateWindow(WC_COMPANY, v->owner);

	BeginVehicleMove(v);
	EndVehicleMove(v);
	DeleteVehicle(v);

	if (!((t=v->u.rail.track) & 0xC0)) {
		SetSignalsOnBothDir(v->tile, FIND_FIRST_BIT(t));
	}
}

static void ChangeTrainDirRandomly(Vehicle *v)
{
	static int8 _random_dir_change[4] = { -1, 0, 0, 1};
	
	for(;;) {
		v->direction = (v->direction + _random_dir_change[Random()&3]) & 7;
		if (!(v->vehstatus & VS_HIDDEN)) {
			BeginVehicleMove(v);
			UpdateTrainDeltaXY(v, v->direction);
			v->cur_image = GetTrainImage(v, v->direction);
			AfterSetVehiclePos(v);
		}

		if (v->next_in_chain == INVALID_VEHICLE)
			break;
		v = &_vehicles[v->next_in_chain];
	}
}

static void HandleCrashedTrain(Vehicle *v)
{
	int state = ++v->u.rail.crash_anim_pos, index;
	uint32 r;
	Vehicle *u;
	
	if (state == 4) {
		CreateEffectVehicleRel(v, 4, 4, 8, EV_CRASHED_SMOKE);
	}

	if (state <= 200 && (uint16)(r=Random()) <= 0x2492) {
		index = (r * 10 >> 16);

		u = v;
		for(;;) {
			if (--index < 0) {
				r = Random();

				CreateEffectVehicleRel(u,
					2 + ((r>>8)&7),
					2 + ((r>>16)&7),
					5 + (r&7),
					EV_DEMOLISH);
				break;
			}
			if (u->next_in_chain == INVALID_VEHICLE)
				break;
			u = &_vehicles[u->next_in_chain];
		}
	}

	if (state <= 240 && !(v->tick_counter&7)) {
		ChangeTrainDirRandomly(v);
	}

	if (state >= 4440 && !(v->tick_counter&0x3F))
		DeleteLastWagon(v);
}

static void HandleBrokenTrain(Vehicle *v)
{
	if (v->breakdown_ctr != 1) {
		v->breakdown_ctr = 1;
		v->cur_speed = 0;

		if (v->breakdowns_since_last_service != 255)
			v->breakdowns_since_last_service++;
		
		InvalidateWindow(WC_VEHICLE_VIEW, v->index);
		InvalidateWindow(WC_VEHICLE_DETAILS, v->index);
		
		SndPlayVehicleFx((_opt.landscape != LT_CANDY) ? 0xE : 0x3A, v);

		if (!(v->vehstatus & VS_HIDDEN)) {
			Vehicle *u = CreateEffectVehicleRel(v, 4, 4, 5, EV_BREAKDOWN_SMOKE);
			if (u)
				u->u.special.unk0 = v->breakdown_delay * 2;
		}
	}

	if (!(v->tick_counter & 3)) {
		if (!--v->breakdown_delay) {
			v->breakdown_ctr = 0;
			InvalidateWindow(WC_VEHICLE_VIEW, v->index);
		}
	}
}

static const byte _breakdown_speeds[16] = {
	225, 210, 195, 180, 165, 150, 135, 120, 105, 90, 75, 60, 45, 30, 15, 15
};

static const byte _state_dir_table[4] = {
	0x20, 8, 0x10, 4, 
};

static void TrainCheckIfLineEnds(Vehicle *v)
{
	uint tile;
	uint x,y;
	int t;
	uint32 ts;

	if ((uint)(t=v->breakdown_ctr) > 1) {
		v->vehstatus |= VS_TRAIN_SLOWING;
		
		t = _breakdown_speeds[ (-t) >> 4];
		if ((uint16)t <= v->cur_speed)
			v->cur_speed = t;
	} else {
		v->vehstatus &= ~VS_TRAIN_SLOWING;
	}

	// exit if inside a tunnel
	if (v->u.rail.track & 0x40)
		return;

	tile = v->tile;

	// tunnel entrance?
	if (IS_TILETYPE(tile, MP_TUNNELBRIDGE) &&
			(_map5[tile] & 0xF0) == 0 && (byte)((_map5[tile] & 3)*2+1) == v->direction)
				return;
	
	// depot?
	if (IS_TILETYPE(tile, MP_RAILWAY) && (_map5[tile] & 0xFC) == 0xC0)
		return;

	// determine the tracks on the next tile.
	t = v->direction >> 1;
	if (!(v->direction & 1) && v->u.rail.track != _state_dir_table[t]) {
		t = (t - 1) & 3;
	}
	tile += _tileoffs_by_dir[t];
	ts = GetTileTrackStatus(tile, 0) & _reachable_tracks[t];
	
	x = v->x_pos & 0xF;
	y = v->y_pos & 0xF;
	
	switch(v->direction) {
	case 0:
		x = (~x) + (~y) + 24;
		break;
	case 7:
		x = y;
		/* fall through */
	case 1:
		x = (~x) + 16;
		break;
	case 2:
		x = (~x) + y + 8;
		break;
	case 3:
		x = y;
		break;
	case 4:
		x = x + y - 8;
		break;
	case 6:
		x = (~y) + x + 8;
		break;
	}

	if ( (uint16)ts != 0) {
		if ((ts &= (ts >> 16)) == 0) {
			// make a rail/road crossing red
			if (IS_TILETYPE(tile, MP_STREET) && (_map5[tile] & 0xF0)==0x10) {
				if (!(_map5[tile] & 4)) {
					_map5[tile] |= 4;
					SndPlayVehicleFx(12, v);
					MarkTileDirtyByTile(tile);
				}
			}
			return;
		}
	} else if (x + 4 > 15) {
		v->cur_speed = 0;
		ReverseTrainDirection(v);
		return;
	}

	// slow down
	v->vehstatus |= VS_TRAIN_SLOWING;
	t = _breakdown_speeds[x & 0xF];
	if (!(v->direction&1)) t>>=1;
	if ((uint16)t < v->cur_speed)
		v->cur_speed = t;
}

static void TrainLocoHandler(Vehicle *v, bool mode)
{
	int j;
	
	v->tick_counter++;

	/* train has crashed? */
	if (v->u.rail.crash_anim_pos != 0) {
		HandleCrashedTrain(v);
		return;
	}

	if (v->u.rail.force_proceed != 0)
		v->u.rail.force_proceed--;

	/* train is broken down? */
	if (v->breakdown_ctr != 0) {
		if (v->breakdown_ctr <= 2) {
			HandleBrokenTrain(v);
			return;
		}
		v->breakdown_ctr--;
	}

	/* exit if train is stopped */
	if (v->vehstatus & VS_STOPPED)
		return;

	HandleLocomotiveSmokeCloud(v);
	if (ProcessTrainOrder(v)) {
		v->load_unload_time_rem = 0;
		v->cur_speed = 0;
		v->subspeed = 0;
		ReverseTrainDirection(v);
		return;
	}

	HandleTrainLoading(v, mode);

	if ((v->next_order & OT_MASK) == OT_LOADING)
		return;

	if (CheckTrainStayInDepot(v))
		return;

	j = UpdateTrainSpeed(v);

	if (j == 0)
		return;

	TrainCheckIfLineEnds(v);

	do {
		TrainController(v);
		if (v->cur_speed <= 0x100)
			break;
	} while (--j != 0);

	if (v->cur_speed != v->u.rail.last_speed) {
		v->u.rail.last_speed = v->cur_speed;
		if (_patches.vehicle_speed)
			InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
	}
}


void Train_Tick(Vehicle *v)
{
	AgeTrainCargo(v);

	if (v->subtype == 0) {
		TrainLocoHandler(v, false);
		
		/* need to check again in case vehicle was deleted */
		if (v->type == VEH_Train && v->subtype == 0)
			TrainLocoHandler(v, true);
	}
}


static const byte _depot_track_ind[4] = {0,1,0,1};

void TrainEnterDepot(Vehicle *v, uint tile)
{
	byte t;

	SetSignalsOnBothDir(tile, _depot_track_ind[_map5[tile]&3]);

	if (v->subtype != 0)
		v = GetFirstVehicleInChain(v);

	v->date_of_last_service = _date;
	v->breakdowns_since_last_service = 0;
	v->reliability = _engines[v->engine_type].reliability;
	InvalidateWindow(WC_VEHICLE_DETAILS, v->index);

	v->load_unload_time_rem = 0;
	v->cur_speed = 0;

	if ((v->next_order&OT_MASK) == OT_GOTO_DEPOT) {
		InvalidateWindow(WC_VEHICLE_VIEW, v->index);
	
		t = v->next_order;
		v->next_order = OT_DUMMY;

		// Part of the schedule?
		if (t & OF_UNLOAD) { v->u.rail.days_since_order_progr = 0; v->cur_order_index++; }

		// User initiated?
		if (t & OF_FULL_LOAD) {
			v->vehstatus |= VS_STOPPED;
			if (v->owner == _local_player) {
				SET_DPARAM16(0, v->unitnumber);
				AddNewsItem(
					STR_8814_TRAIN_IS_WAITING_IN_DEPOT,
					NEWS_FLAGS(NM_SMALL, NF_VIEWPORT|NF_VEHICLE, NT_ADVICE, 0),
					v->index,
					0);
			}
		}


	}
}

static void CheckIfTrainNeedsService(Vehicle *v)
{
	int i;

	if (v->date_of_last_service + v->service_interval > _date)
		return;

	if (v->vehstatus & VS_STOPPED)
		return;
	
	// Don't interfere with a depot visit scheduled by the user, or a
	// depot visit by the order list.
	if ((v->next_order & OT_MASK) == OT_GOTO_DEPOT &&
			(v->next_order & (OF_FULL_LOAD|OF_UNLOAD)) != 0)
		return;

	i = FindClosestTrainDepot(v);
	if (i < 0 || GetTileDist(v->tile, (&_depots[i])->xy) > 12) {
		if ((v->next_order & OT_MASK) == OT_GOTO_DEPOT) {
			v->next_order = OT_DUMMY;
			InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
		}
		return;
	}

	if ((v->next_order & OT_MASK) == OT_GOTO_DEPOT && v->next_order_param != (byte)i && !CHANCE16(3,16))
		return;

	v->next_order = OT_GOTO_DEPOT | OF_NON_STOP;
	v->next_order_param = (byte)i;
	v->dest_tile = (&_depots[i])->xy;
	InvalidateWindowWidget(WC_VEHICLE_VIEW, v->index, 4);
}

void OnNewDay_Train(Vehicle *v)
{
	TileIndex tile;

	if ((++v->day_counter & 7) == 0)
		DecreaseVehicleValue(v);

	if (v->subtype == 0) {
		CheckVehicleBreakdown(v);
		AgeVehicle(v);
		
		if (!_patches.no_train_service || !IS_HUMAN_PLAYER(v->owner))
			CheckIfTrainNeedsService(v);
		
		// check if train hasn't advanced in its order list for a set number of days
		if (_patches.lost_train_days && v->num_orders && !(v->vehstatus & VS_STOPPED) && ++v->u.rail.days_since_order_progr >= _patches.lost_train_days && v->owner == _local_player) {
			v->u.rail.days_since_order_progr = 0;
			SET_DPARAM16(0, v->unitnumber);
			AddNewsItem(
				STR_TRAIN_IS_LOST,
				NEWS_FLAGS(NM_SMALL, NF_VIEWPORT|NF_VEHICLE, NT_ADVICE, 0),
				v->index,
				0);
		}

		/* update destination */
		if ((v->next_order & OT_MASK) == OT_GOTO_STATION &&
				(tile=DEREF_STATION(v->next_order_param)->train_tile) != 0)
					v->dest_tile = tile;

		if ((v->vehstatus & VS_STOPPED) == 0) {
			/* running costs */
			const RailVehicleInfo *rvi = &_rail_vehicle_info[v->engine_type];
			int32 cost = (rvi->running_cost_base * _price.running_rail[rvi->running_cost_class-1]) / 364;

			v->profit_this_year -= cost >> 8;

			SET_EXPENSES_TYPE(EXPENSES_TRAIN_RUN);
			SubtractMoneyFromPlayerFract(v->owner, cost);

			InvalidateWindow(WC_VEHICLE_DETAILS, v->index);
			InvalidateWindow(WC_TRAINS_LIST, v->owner);
		}
	}
}

void TrainsYearlyLoop()
{
	Vehicle *v;

	for(v=_vehicles; v != endof(_vehicles); v++) {
		if (v->type == VEH_Train && v->subtype == 0) {
			
			// show warning if train is not generating enough income last 2 years
			if (v->owner == _local_player && v->age >= 730 && v->profit_last_year < _patches.train_income_warn && v->profit_this_year < _patches.train_income_warn) {
				SET_DPARAM32(1, v->profit_this_year);
				SET_DPARAM16(0, v->unitnumber);
				AddNewsItem(
					STR_TRAIN_IS_UNPROFITABLE,
					NEWS_FLAGS(NM_SMALL, NF_VIEWPORT|NF_VEHICLE, NT_ADVICE, 0),
					v->index,
					0);
			}

			v->profit_last_year = v->profit_this_year;
			v->profit_this_year = 0;
			InvalidateWindow(WC_VEHICLE_DETAILS, v->index);
		}
	}
}

extern void ShowTrainViewWindow(Vehicle *v);

void HandleClickOnTrain(Vehicle *v)
{
	if (v->subtype != 0) v = GetFirstVehicleInChain(v);
	ShowTrainViewWindow(v);
}

void NormalizeTrainVehInDepot(TileIndex tile, int veh)
{
	Vehicle *v;

restart:
	for(v=_vehicles; v != endof(_vehicles); v++) {
		if (v->type == VEH_Train && v->subtype==4 &&
				v->tile == tile &&
				v->u.rail.track == 0x80) {
			if (DoCommandByTile(0,v->index,veh, 0, CMD_MOVE_RAIL_VEHICLE) == CMD_ERROR)
				break;

			DoCommandByTile(0, v->index,veh, DC_EXEC, CMD_MOVE_RAIL_VEHICLE);
			goto restart;
		}
	}
}


void InitializeTrains()
{
	_age_cargo_skip_counter = 1;
}
