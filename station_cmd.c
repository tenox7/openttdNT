#include "stdafx.h"
#include "ttd.h"
#include "station.h"
#include "gfx.h"
#include "window.h"
#include "viewport.h"
#include "command.h"
#include "city.h"
#include "vehicle.h"
#include "news.h"
#include "saveload.h"
#include "economy.h"
#include "player.h"

const byte _airport_size_x[3] = {4, 6, 1 };
const byte _airport_size_y[3] = {3, 6, 1 };

void ShowAircraftDepotWindow(uint tile);

static void MarkStationDirty(Station *st)
{
	if (st->sign.width_1 != 0) {
		InvalidateWindowWidget(WC_STATION_VIEW, st->index, 1);

		MarkAllViewportsDirty(
			st->sign.left - 6,
			st->sign.top,
			st->sign.left + (st->sign.width_1 << 2) + 12,
			st->sign.top + 48);
	}
}

#define CHECK_STATIONS_ERR ((Station*)-1)

static Station *GetStationAround(uint tile, int w, int h)
{
	Station *st;

	/* check around to see if there's any stations there */
	{
		int closest_station = -1;
BEGIN_TILE_LOOP(tile_cur, w + 2, h + 2, tile - TILE_XY(1,1))
			if (IS_TILETYPE(tile_cur, MP_STATION)) {
				int t;
				t = _map2[tile_cur];
				if (closest_station == -1) {
					closest_station = t;
				} else if (closest_station != t) {
					_error_message = STR_3006_ADJOINS_MORE_THAN_ONE_EXISTING;
					return CHECK_STATIONS_ERR;
				}
			}
END_TILE_LOOP(tile_cur, w + 2, h + 2, tile - TILE_XY(1,1))
		st = (closest_station == -1) ? NULL : DEREF_STATION(closest_station);
	}

	return st;
}


static bool CheckStationSpreadOut(Station *st, uint tile, int w, int h)
{
	byte station_index = st->index;
	uint i;
	uint x1 = GET_TILE_X(tile);
	uint y1 = GET_TILE_Y(tile);
	uint x2 = x1 + w - 1;
	uint y2 = y1 + h - 1;
	uint t;

	for(i=0; i!=TILES_X*TILES_Y; i++) {
		if (IS_TILETYPE(i, MP_STATION) && _map2[i] == station_index) {
			t = GET_TILE_X(i);
			if (t < x1) x1 = t;
			if (t > x2) x2 = t;

			t = GET_TILE_Y(i);
			if (t < y1) y1 = t;
			if (t > y2) y2 = t;
		}
	}

	if (y2-y1 >= _patches.station_spread || x2-x1 >= _patches.station_spread) {
		_error_message = STR_306C_STATION_TOO_SPREAD_OUT;
		return false;
	}

	return true;
}

static Station *AllocateStation()
{
	Station *st, *a_free = NULL;
	int count;
	int num_free = 0;
	int i;

	for(st = _stations, count = lengthof(_stations); count != 0; count--, st++) {
		if (st->xy == 0) {
			num_free++;
			if (a_free==NULL)
				a_free = st;
		}
	}

	if (a_free == NULL ||
			(num_free < 30 && IS_HUMAN_PLAYER(_current_player))) {
		_error_message = STR_3008_TOO_MANY_STATIONS_LOADING;
		return NULL;
	}

	i = a_free->index;	
	memset(a_free, 0, sizeof(Station));
	a_free->index = i;
	return a_free;
}


static int CountMapSquareAround(uint tile, byte type, byte min, byte max) {
	static const int16 _count_square_table[7*7+1] = {
		TILE_XY(-3,-3), 1, 1, 1, 1, 1, 1,
		TILE_XY(-6,1),  1, 1, 1, 1, 1, 1,
		TILE_XY(-6,1),  1, 1, 1, 1, 1, 1,
		TILE_XY(-6,1),  1, 1, 1, 1, 1, 1,
		TILE_XY(-6,1),  1, 1, 1, 1, 1, 1,
		TILE_XY(-6,1),  1, 1, 1, 1, 1, 1,
		TILE_XY(-6,1),  1, 1, 1, 1, 1, 1,
		0,
	};	
	int j;
	const int16 *p = _count_square_table;
	int num = 0;

	while ( (j=*p++) != 0 ) {
		tile = TILE_MASK(tile + j);

		if (IS_TILETYPE(tile, type) && _map5[tile] >= min && _map5[tile] <= max)
			num++;
	}

	return num;
}

#define M(x) ((x) - STR_SV_STNAME)

static bool GenerateStationName(Station *st, uint tile, int flag)
{
	static const uint32 _gen_station_name_bits[] = {
		0,																/* 0 */
		1 << M(STR_SV_STNAME_AIRPORT),		/* 1 */
		1 << M(STR_SV_STNAME_OILFIELD),		/* 2 */
		1 << M(STR_SV_STNAME_DOCKS),			/* 3 */
		0x1FF << M(STR_SV_STNAME_BUOY_1),	/* 4 */
		1 << M(STR_SV_STNAME_HELIPORT),		/* 5 */
	};

	City *city = st->city;
	uint32 free_names = (uint32)-1;
	int found;
	uint z,z2;
	unsigned long tmp;

	{
		Station *s;

		FOR_ALL_STATIONS(s) {
			if (s != st && s->xy != 0 && s->city==city) {
				uint str = M(s->string_id);
				if (str <= 0x20) {
					if (str == M(STR_SV_STNAME_FOREST))
						str = M(STR_SV_STNAME_WOODS);
					CLRBIT(free_names, str);
				}
			}
		}	
	}
	
	/* check default names */
	tmp = free_names & _gen_station_name_bits[flag];
	if (tmp != 0) {
		found = FindFirstBit(tmp);
		goto done;
	}

	/* check mine? */
	if (HASBIT(free_names, M(STR_SV_STNAME_MINES))) {
		if (CountMapSquareAround(tile, MP_INDUSTRY, 0, 6) >= 2 ||
		    CountMapSquareAround(tile, MP_INDUSTRY, 0x64, 0x73) >= 2 ||
				CountMapSquareAround(tile, MP_INDUSTRY, 0x2F, 0x33) >= 2 ||
				CountMapSquareAround(tile, MP_INDUSTRY, 0x48, 0x58) >= 2 ||
				CountMapSquareAround(tile, MP_INDUSTRY, 0x5B, 0x63) >= 2) {
					found = M(STR_SV_STNAME_MINES);
					goto done;
				}
	}

	/* check close enough to city to get central as name? */
	if (GetTileDist1D(tile,city->xy) < 8) {
		found = M(STR_SV_STNAME);
		if (HASBIT(free_names, M(STR_SV_STNAME))) goto done;

		found = M(STR_SV_STNAME_CENTRAL);
		if (HASBIT(free_names, M(STR_SV_STNAME_CENTRAL))) goto done;
	}

	/* Check lakeside */
	if (HASBIT(free_names, M(STR_SV_STNAME_LAKESIDE)) &&
			!CheckDistanceFromEdge(tile, 20) &&
			CountMapSquareAround(tile, MP_WATER, 0, 0) >= 5) {
				found = M(STR_SV_STNAME_LAKESIDE);
				goto done;
			}

	/* Check woods */
	if (HASBIT(free_names, M(STR_SV_STNAME_WOODS)) && (
			CountMapSquareAround(tile, MP_TREES, 0, 255) >= 8 ||
			CountMapSquareAround(tile, MP_INDUSTRY, 0x10, 0x11) >= 2 )) {
				found = (_opt.landscape==LT_DESERT) ? M(STR_SV_STNAME_FOREST) : M(STR_SV_STNAME_WOODS);
				goto done;
	}

	/* check elevation compared to city */
	z = GetTileZ(tile);
	z2 = GetTileZ(city->xy);
	if (z < z2) {
		found = M(STR_SV_STNAME_VALLEY);
		if (HASBIT(free_names, M(STR_SV_STNAME_VALLEY))) goto done;
	} else if (z > z2) {
		found = M(STR_SV_STNAME_HEIGHTS);
		if (HASBIT(free_names, M(STR_SV_STNAME_HEIGHTS))) goto done;
	}

	/* check direction compared to city */
	{
		static const int8 _direction_and_table[] = {
			~( (1<<M(STR_SV_STNAME_WEST)) | (1<<M(STR_SV_STNAME_EAST)) | (1<<M(STR_SV_STNAME_NORTH)) ),
			~( (1<<M(STR_SV_STNAME_SOUTH)) | (1<<M(STR_SV_STNAME_WEST)) | (1<<M(STR_SV_STNAME_NORTH)) ),
			~( (1<<M(STR_SV_STNAME_SOUTH)) | (1<<M(STR_SV_STNAME_EAST)) | (1<<M(STR_SV_STNAME_NORTH)) ),
			~( (1<<M(STR_SV_STNAME_SOUTH)) | (1<<M(STR_SV_STNAME_WEST)) | (1<<M(STR_SV_STNAME_EAST)) ),
		};
		
		free_names &= _direction_and_table[ 
			(GET_TILE_X(tile) < GET_TILE_X(city->xy)) + 
			(GET_TILE_Y(tile) < GET_TILE_Y(city->xy))*2];
	}

	tmp = free_names & ((1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<6)|(1<<7)|(1<<12)|(1<<26)|(1<<27)|(1<<28)|(1<<29)|(1<<30));
	if (tmp == 0) {
		_error_message = STR_3007_TOO_MANY_STATIONS_LOADING;
		return false;
	}
	found = FindFirstBit(tmp);

done:
	st->string_id = found + STR_SV_STNAME;
	return true;
}
#undef M

static Station *GetClosestStationFromTile(uint tile, uint threshold, byte owner)
{
	Station *st, *best_station = NULL;
	uint cur_dist;

	FOR_ALL_STATIONS(st) {
		cur_dist = GetTileDist(tile, st->xy);
		if (cur_dist < threshold && (owner == 0xFF || st->owner == owner)) {
			threshold = cur_dist;
			best_station = st;
		}
	}

	return best_station;
}

static void RemoveStationFromAlphaList(Station *st)
{
	byte b;
	Station *s;

	b =  st->alpha_order;
	FOR_ALL_STATIONS(s) {
		if (s->xy != 0 && b < s->alpha_order)
			s->alpha_order--;
	}

	st->alpha_order = 0xFF;
}

static void UpdateStationAlphaList(Station *myst)
{
	Station *st;
	int n;
	char buf1[64];
	char buf2[64];

	RemoveStationFromAlphaList(myst);

	SET_DPARAM16(0, myst->city->citynametype);
	SET_DPARAM32(1, myst->city->citynameparts);
	GetString(buf1, myst->string_id);

	n = 0;
restart:;
	FOR_ALL_STATIONS(st) {
		if (st->xy != 0 && st->alpha_order == n) {
			SET_DPARAM16(0, st->city->citynametype);
			SET_DPARAM32(1, st->city->citynameparts);
			GetString(buf2, st->string_id);
			if (!str_is_below(buf1,buf2)) {
				n++;
				goto restart;
			}
			break;
		}
	}

	FOR_ALL_STATIONS(st) {
		if (st->xy != 0 && n <= st->alpha_order)
			st->alpha_order++;
	}

	myst->alpha_order = n;
}

static void StationInitialize(Station *st, TileIndex tile)
{
	int i;
	GoodsEntry *ge;

	st->xy = tile;
	st->bus_tile = st->lorry_tile = st->airport_tile = st->dock_tile = st->train_tile = 0;
	st->had_vehicle_of_type = 0;
	st->time_since_load = 255;
	st->time_since_unload = 255;
	st->delete_ctr = 0;
	st->facilities = 0;

	st->blocked_months = 0;
	st->last_vehicle = INVALID_VEHICLE;

	for(i=0,ge=st->goods; i!=NUM_CARGO; i++, ge++) {
		ge->waiting_acceptance = 0;
		ge->days_since_pickup = 0;
		ge->enroute_from = 0xFF;
		ge->rating = 175;
		ge->last_speed = 0;
		ge->last_age = 0xFF;
	}

	st->alpha_order = 0xFF;
	UpdateStationAlphaList(st);
}

// Update the virtual coords needed to draw the station sign.
// st = Station to update for.
static void UpdateStationVirtCoord(Station *st)
{
	Point pt = RemapCoords2(GET_TILE_X(st->xy) * 16, GET_TILE_Y(st->xy) * 16);
	pt.y -= 32;
	if (st->facilities&FACIL_AIRPORT && st->airport_type==AT_OILFIELD) pt.y -= 16;

	SET_DPARAM16(0, st->index);
	SET_DPARAM8(1, st->facilities);
	UpdateViewportSignPos(&st->sign, pt.x, pt.y, STR_305C_0);
}

// Update the virtual coords needed to draw the station sign for all stations.
void UpdateAllStationVirtCoord()
{
	Station *st;
	FOR_ALL_STATIONS(st) {
		if (st->xy != 0)
			UpdateStationVirtCoord(st);			
	}
}

// Update the station virt coords while making the modified parts dirty.
static void UpdateStationVirtCoordDirty(Station *st)
{
	MarkStationDirty(st);
	UpdateStationVirtCoord(st);
	MarkStationDirty(st);
}

// Get a mask of the cargo types that the station accepts.
static uint GetAcceptanceMask(Station *st)
{
	uint mask = 0;
	uint cur_mask = 1;
	int i;
	for(i=0; i!=NUM_CARGO; i++,cur_mask*=2) {
		if (st->goods[i].waiting_acceptance & 0x8000)
			mask |= cur_mask;
	}
	return mask;
}

// Items contains the two cargo names that are to be accepted or rejected.
// msg is the string id of the message to display.
static void ShowRejectOrAcceptNews(Station *st, uint32 items, StringID msg)
{
	if (items) {
		SET_DPARAM32(2, items >> 16);
		SET_DPARAM32(1, items & 0xFFFF);
		SET_DPARAM16(0, st->index);
		AddNewsItem(msg + ((items >> 16)?1:0), NEWS_FLAGS(NM_SMALL, NF_VIEWPORT|NF_TILE, NT_ACCEPTANCE, 0), st->xy, 0);
	}
}

// Get a list of the cargo types being produced around the tile.
void GetProductionAroundTiles(uint *produced, uint tile, int w, int h)
{
	int x,y;
	int x1,y1,x2,y2;
	int xc,yc;
	byte cargos[2];

	memset(produced, 0, NUM_CARGO * sizeof(uint));

	x = GET_TILE_X(tile);
	y = GET_TILE_Y(tile);

	// expand the region by 4 tiles on each side
	// while making sure that we remain inside the board.
	x2 = min(x + w+4, TILE_X_MAX+1);
	x1 = max(x-4, 0);

	y2 = min(y + h+4, TILE_Y_MAX+1);
	y1 = max(y-4, 0);

	assert(x1 < x2);
	assert(y1 < y2);
	assert(w > 0);
	assert(h > 0);

	yc = y1;
	do {
		xc = x1;
		do {
			if (!(IS_INSIDE_1D(xc, x, w) && IS_INSIDE_1D(yc, y, h))) {
				GetProducedCargoProc *gpc;
				uint tile = TILE_XY(xc, yc);
				gpc = _tile_type_procs[GET_TILETYPE(tile)]->get_produced_cargo_proc;
				if (gpc != NULL) {
					cargos[0] = cargos[1] = 0xFF;
					gpc(tile, cargos);
					if (cargos[0] != 0xFF) {
						produced[cargos[0]]++;
						if (cargos[1] != 0xFF) {
							produced[cargos[1]]++;
						}
					}
				}
			}
		} while (++xc != x2);
	} while (++yc != y2);
}

// Get a list of the cargo types that are accepted around the tile.
void GetAcceptanceAroundTiles(uint *accepts, uint tile, int w, int h)
{
	int x,y;
	int x1,y1,x2,y2;
	int xc,yc;
	AcceptedCargo ac;

	memset(accepts, 0, NUM_CARGO * sizeof(uint));

	x = GET_TILE_X(tile);
	y = GET_TILE_Y(tile);

	// expand the region by 4 tiles on each side
	// while making sure that we remain inside the board.
	x2 = min(x + w + 4, TILE_X_MAX+1);
	y2 = min(y + h + 4, TILE_Y_MAX+1);
	x1 = max(x-4, 0);
	y1 = max(y-4, 0);

	assert(x1 < x2);
	assert(y1 < y2);
	assert(w > 0);
	assert(h > 0);

	yc = y1;
	do {
		xc = x1;
		do {
			if (!(IS_INSIDE_1D(xc, x, w) && IS_INSIDE_1D(yc, y, h))) {
				GetAcceptedCargo(TILE_XY(xc, yc), &ac);
				accepts[ac.type_1] += ac.amount_1;
				accepts[ac.type_2] += ac.amount_2;
				accepts[ac.type_3] += ac.amount_3;
			}
		} while (++xc != x2);
	} while (++yc != y2);
}

// Update the acceptance for a station.
// show_msg controls whether to display a message that acceptance was changed.
static void UpdateStationAcceptance(Station *st, bool show_msg)
{
	uint old_acc, new_acc;
	TileIndex span[1+1+2+2+1];
	int i;
	int min_x, min_y, max_x, max_y;
	uint accepts[NUM_CARGO];

	// Don't update acceptance for a buoy
	if (st->had_vehicle_of_type & HVOT_BUOY)
		return;

	/* old accepted goods types */
	old_acc = GetAcceptanceMask(st);

	// Put all the tiles that span an area in the table.
	span[3] = span[5] = 0;
	span[0] = st->bus_tile;
	span[1] = st->lorry_tile;
	span[2] = st->train_tile;
	if (st->train_tile != 0) {
		int w = (st->station_platforms & 0xF)-1;
		int h = (st->station_platforms >> 4) -1;
		if (!(_map5[st->train_tile] & 1))
			intswap(w,h);
		span[3] = st->train_tile + TILE_XY(w,h);
	}
	span[4] = st->airport_tile;
	if (st->airport_tile != 0) {
		span[5] = st->airport_tile + TILE_XY(_airport_size_x[st->airport_type]-1, _airport_size_y[st->airport_type]-1); 
	}
	span[6] = st->dock_tile;

	// Construct a rectangle from those points
	min_x = min_y = 0x7FFFFFFF;
	max_x = max_y = 0;

	for(i=0; i!=7; i++) {
		uint tile = span[i];
		if (tile) {
			min_x = min(min_x,GET_TILE_X(tile));
			max_x = max(max_x,GET_TILE_X(tile));
			min_y = min(min_y,GET_TILE_Y(tile));
			max_y = max(max_y,GET_TILE_Y(tile));
		}
	}

	// And retrieve the acceptance.
	if (max_x != 0) {
		GetAcceptanceAroundTiles(accepts, TILE_XY(min_x, min_y), max_x - min_x + 1, max_y-min_y+1);
	} else {
		memset(accepts, 0, sizeof(accepts));
	}

	// Adjust in case our station only accepts fewer kinds of goods
	for(i=0; i!=NUM_CARGO; i++) {
		uint amt = min(accepts[i], 15);

		// Make sure the station can accept the goods type.
		if ((i != CT_PASSENGERS && !(st->facilities & (byte)~FACIL_BUS_STOP)) ||
				(i == CT_PASSENGERS && !(st->facilities & (byte)~FACIL_TRUCK_STOP)))
			amt = 0;

		st->goods[i].waiting_acceptance = (st->goods[i].waiting_acceptance & ~0xF000) + (amt << 12);
	}

	// Only show a message in case the acceptance was actually changed.
	new_acc = GetAcceptanceMask(st);
	if (old_acc == new_acc)
		return;
	
	// show a message to report that the acceptance was changed?
	if (show_msg && st->owner == _local_player && st->facilities) {
		uint32 accept=0, reject=0; /* these contain two string ids each */
		const StringID *str = _cargoc.names_s;

		do {
			if (new_acc & 1) {
				if (!(old_acc & 1)) accept = (accept << 16) | *str; 
			} else {
				if (old_acc & 1) reject = (reject << 16) | *str;
			}
		} while (str++,(new_acc>>=1) != (old_acc>>=1));

		ShowRejectOrAcceptNews(st, accept, STR_3040_NOW_ACCEPTS);
		ShowRejectOrAcceptNews(st, reject, STR_303E_NO_LONGER_ACCEPTS);
	}

	// redraw the station view since acceptance changed
	InvalidateWindowWidget(WC_STATION_VIEW, st->index, 4);
}

// This is called right after a station was deleted.
// It checks if the whole station is free of substations, and if so, the station will be
// deleted after a little while.
static void DeleteStationIfEmpty(Station *st) {
	if (st->facilities == 0) {
		st->delete_ctr = 0;
		InvalidateWindow(WC_STATION_LIST, st->owner);
	}
}

// Tries to clear the given area. Returns the cost in case of success.
// Or an error code if it failed.
int32 CheckFlatLandBelow(uint tile, uint w, uint h, uint flags, uint invalid_dirs)
{
	int32 cost = 0, ret;

	uint tileh;
	int z, allowed_z = -1, flat_z;

	BEGIN_TILE_LOOP(tile_cur, w, h, tile)
		if (!EnsureNoVehicle(tile_cur))
			return CMD_ERROR;

		tileh = GetTileSlope(tile_cur, &z);

		// steep slopes are completely prohibited
		if (tileh & 0x10 || (_is_ai_player || !_patches.build_on_slopes) && tileh != 0) {
			_error_message = STR_0007_FLAT_LAND_REQUIRED;
			return CMD_ERROR;
		}

		flat_z = z;
		if (tileh) {
			// need to check so the entrance to the station is not pointing at a slope.
			if (invalid_dirs&1 && !(tileh & 0xC) && (uint)w_cur == w ||
					invalid_dirs&2 && !(tileh & 6) &&	h_cur == 1 ||
					invalid_dirs&4 && !(tileh & 3) && w_cur == 1 ||
					invalid_dirs&8 && !(tileh & 9) && (uint)h_cur == h) {
				_error_message = STR_0007_FLAT_LAND_REQUIRED;
				return CMD_ERROR;
			}
			cost += _price.terraform;
			flat_z += 8;
		}
		
		// get corresponding flat level and make sure that all parts of the station have the same level.
		if (allowed_z == -1) {
			// first tile
			allowed_z = flat_z;
		} else if (allowed_z != flat_z) {
			_error_message = STR_0007_FLAT_LAND_REQUIRED;
			return CMD_ERROR;
		}

	
		ret = DoCommandByTile(tile_cur, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		if (ret == CMD_ERROR)
			return CMD_ERROR;
			cost += ret;
	END_TILE_LOOP(tile_cur, w, h, tile)

	return cost;
}

static bool CanExpandRailroadStation(Station *st, uint tile, int w, int h, int direction, uint *fin)
{
	int curw, curh;

	// check so the direction is the same
	if ((_map5[st->train_tile] & 1) != direction) return false;

	// get current size of station
	curw = st->station_platforms >> 4;
	curh = st->station_platforms & 0xF;
	if (direction) intswap(curw,curh);

	// check if the new station adjoins the old station in either direction
	if (curw == w && st->train_tile == tile + TILE_XY(0, h)) {
		// above
		curh += h;
	} else if (curw == w && st->train_tile == tile - TILE_XY(0, curh)) {
		// below
		tile -= TILE_XY(0, curh);
		curh += h;
	} else if (curh == h && st->train_tile == tile + TILE_XY(w, 0)) {
		// to the left
		curw += w;
	} else if (curh == h && st->train_tile == tile - TILE_XY(curw, 0)) {
		// to the right
		tile -= TILE_XY(curw, 0);
		curw += w;
	} else
		return false;

	// make sure the final size is not too big.
	if (curw > 15 || curh > 15) return false;

	// now tile contains the new value for st->train_tile
	// curw, curh contain the new value for width and height
	fin[0] = tile;

	if (direction) intswap(curw, curh);
	fin[1] = (curw << 4) + curh;

	return true;
}

static byte FORCEINLINE *CreateSingle(byte *layout, int n)
{
	int i = n;
	do *layout++ = 0; while (--i);
	layout[((n-1) >> 1)-n] = 2;
	return layout;
}

static byte FORCEINLINE *CreateMulti(byte *layout, int n, byte b)
{
	int i = n;
	do *layout++ = b; while (--i);
	if (n > 4) {
		layout[0-n] = 0;
		layout[n-1-n] = 0;
	}
	return layout;
}

// stolen from TTDPatch
static void GetStationLayout(byte *layout, int numtracks, int plat_len)
{
	if (plat_len == 1) {
		CreateSingle(layout, numtracks);
	} else {
		if (numtracks & 1)
			layout = CreateSingle(layout, plat_len);
		numtracks>>=1;

		while (--numtracks >= 0) {
			layout = CreateMulti(layout, plat_len, 4);
			layout = CreateMulti(layout, plat_len, 6);
		}
	}
}

/* build railroad station
 * p1 & 0xFF - orientation
 * (p1 >> 8) & 0xFF - numtracks
 * (p1 >> 16) & 0xFF - platform length
 * p2  - railtype
 */

/* DL = 0..4, DH=0..3 */
/* DL = platform len, DH = tracks */

int32 CmdBuildRailroadStation(int x_org, int y_org, uint32 flags, uint32 p1, uint32 p2)
{
	/* unpack params */
	int w_org,h_org;
	uint tile_org;
	int32 cost, ret;
	Station *st;
	int plat_len, numtracks;
	int direction;
	uint finalvalues[2];
	byte layout_buff[7*7];

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	tile_org = TILE_FROM_XY(x_org,y_org);

	/* Does the authority allow this? */
	if (!(flags & DC_NO_CITY_RATING) && !CheckIfAuthorityRefuses(tile_org))
		return CMD_ERROR;

	{
		/* unpack parameters */
		direction = p1 & 1;
		plat_len = (p1 >> 16) & 0xFF;
		numtracks = (p1 >> 8) & 0xFF;
		/* w = length, h = num_tracks */
		if (direction) {
			h_org = plat_len;
			w_org = numtracks;
		} else {
			w_org = plat_len;
			h_org = numtracks;
		}
	}

	// these values are those that will be stored in train_tile and station_platforms
	finalvalues[0] = tile_org;
	finalvalues[1] = (plat_len << 4) + numtracks;

	// Make sure the area below consists of clear tiles.
	if ((ret=CheckFlatLandBelow(tile_org, w_org, h_org, flags, 5 << direction)) == CMD_ERROR) return CMD_ERROR;
	cost = ret + (numtracks * _price.train_station_track + _price.train_station_length) * plat_len;

	// Make sure there are no similar stations around us.
	st = GetStationAround(tile_org, w_org, h_org);
	if (st == CHECK_STATIONS_ERR) return CMD_ERROR;

	// See if there is a deleted station close to us.
	if (st == NULL) {
		st = GetClosestStationFromTile(tile_org, 8, _current_player);
		if (st != NULL && st->facilities) st = NULL;
	}

	if (st != NULL) {
		// Reuse an existing station.
		if (st->owner != 0x10 && st->owner != _current_player)
			return_cmd_error(STR_3009_TOO_CLOSE_TO_ANOTHER_STATION);

		if (st->train_tile != 0) {
			// check if we want to expanding an already existing station? Only human players can do this.
			if (_is_ai_player || !_patches.join_stations || !CanExpandRailroadStation(st, tile_org, w_org, h_org, direction, finalvalues))
				return_cmd_error(STR_3005_TOO_CLOSE_TO_ANOTHER_RAILROAD);
		}

		if (!CheckStationSpreadOut(st, tile_org, w_org, h_org))
			return CMD_ERROR;

	}	else {
		// Create a new station
		st = AllocateStation();
		if (st == NULL)
			return CMD_ERROR;
		
		st->city = ClosestCityFromTile(tile_org, (uint)-1);
		if (_current_player < 8 && flags&DC_EXEC)
			SETBIT(st->city->have_ratings, _current_player);

		if (!GenerateStationName(st, tile_org, 0))
			return CMD_ERROR;

		if (flags & DC_EXEC)
			StationInitialize(st, tile_org);
	}

	if (flags & DC_EXEC) {
		int tile_delta;
		const byte *layout_ptr;
		uint station_index = st->index;

		st->train_tile = finalvalues[0];
		if (!st->facilities) st->xy = finalvalues[0];
		st->facilities |= FACIL_TRAIN;
		st->owner = _current_player;

		st->station_platforms = finalvalues[1];
		tile_delta = direction ? TILE_XY(0,1) : TILE_XY(1,0);
		
		GetStationLayout(layout_buff, numtracks, plat_len);
		layout_ptr = layout_buff;
		do {
			int tile = tile_org;
			int w = plat_len;
			do {

				ModifyTile(tile, 
					MP_SETTYPE(MP_STATION) | MP_MAPOWNER_CURRENT |
					MP_MAP2 | MP_MAP5 | MP_MAP3LO | MP_MAP3HI_CLEAR,
					station_index, /* map2 parameter */
					p2,				/* map3lo parameter */
					(*layout_ptr++) + direction   /* map5 parameter */
				);

				tile += tile_delta;
			} while (--w);
			tile_org += tile_delta ^ TILE_XY(1,1);
		} while (--numtracks);

		UpdateStationVirtCoordDirty(st);
		UpdateStationAcceptance(st, false);
		InvalidateWindow(WC_STATION_LIST, st->owner);
	}

	return cost;
}

static int32 RemoveRailroadStation(Station *st, uint32 flags)
{
	uint tile;
	int w,h;
	int32 cost;

	/* Current player owns the station? */
	if (_current_player != 0x11 && !CheckOwnership(st->owner))
		return CMD_ERROR;

	/* determine width and height of platforms */
	tile = st->train_tile;
	w = st->station_platforms >> 4;
	h = st->station_platforms & 0xF;
	if (_map5[tile]&1) intswap(w,h);
	
	/* cost is area * constant */
	cost = w*h*_price.remove_rail_station;

	/* clear all areas of the station */
	do {
		int w_bak = w;
		do {
			if (!EnsureNoVehicle(tile))
				return CMD_ERROR;
			if (flags & DC_EXEC)
				DoClearSquare(tile);
			tile += TILE_XY(1, 0);
		} while (--w);
		w = w_bak;
		tile = tile + TILE_XY(-w, 1);
	} while (--h);

	if (flags & DC_EXEC) {
		st->train_tile = 0;
		st->facilities &= ~FACIL_TRAIN;

		UpdateStationVirtCoordDirty(st);
		DeleteStationIfEmpty(st);
	}
	
	return cost;
}

/* Build a bus station
 * p1 - direction
 * p2 - unused
 */

int32 CmdBuildBusStation(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile;
	int32 cost;
	Station *st;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	tile = TILE_FROM_XY(x,y);

	if (!(flags & DC_NO_CITY_RATING) && !CheckIfAuthorityRefuses(tile))
		return CMD_ERROR;

	if ((cost=CheckFlatLandBelow(tile, 1, 1, flags, 1 << p1)) == CMD_ERROR)
		return CMD_ERROR;

	st = GetStationAround(tile, 1, 1);
	if (st == CHECK_STATIONS_ERR)
		return CMD_ERROR;

	/* Find a station close to us */
	if (st == NULL) {
		st = GetClosestStationFromTile(tile, 8, _current_player);
		if (st!=NULL && st->facilities) st = NULL;
	}

	if (st != NULL) {
		if (st->owner != 0x10 && st->owner != _current_player)
			return_cmd_error(STR_3009_TOO_CLOSE_TO_ANOTHER_STATION);
		
		if (!CheckStationSpreadOut(st, tile, 1, 1))
			return CMD_ERROR;

		if (st->bus_tile != 0)
			return_cmd_error(STR_3044_TOO_CLOSE_TO_ANOTHER_BUS);
	} else {
		City *cit;

		st = AllocateStation();
		if (st == NULL)
			return CMD_ERROR;

		st->city = cit = ClosestCityFromTile(tile, (uint)-1);

		if (_current_player < 8 && flags&DC_EXEC)
			SETBIT(cit->have_ratings, _current_player);

		st->sign.width_1 = 0;

		if (!GenerateStationName(st, tile, 0))
			return CMD_ERROR;

		if (flags & DC_EXEC)
			StationInitialize(st, tile);
	}

	cost += _price.build_bus_station;

	if (flags & DC_EXEC) {
		st->bus_tile = tile;
		if (!st->facilities) st->xy = tile;
		st->facilities |= FACIL_BUS_STOP;
		st->bus_stop_status = 3;
		st->owner = _current_player;

		ModifyTile(tile, 
			MP_SETTYPE(MP_STATION) | MP_MAPOWNER_CURRENT |
			MP_MAP2 | MP_MAP5 | MP_MAP3LO_CLEAR | MP_MAP3HI_CLEAR,
			st->index,			/* map2 parameter */
			p1 + 0x47       /* map5 parameter */
		);

		UpdateStationVirtCoordDirty(st);
		UpdateStationAcceptance(st, false);
		InvalidateWindow(WC_STATION_LIST, st->owner);
	}
	return cost;
}

// Remove a bus station
static int32 RemoveBusStation(Station *st, uint32 flags)
{
	uint tile;

	if (_current_player != 0x11 && !CheckOwnership(st->owner))
		return CMD_ERROR;

	tile = st->bus_tile;
	
	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		DoClearSquare(tile);

		st->bus_tile = 0;
		st->facilities &= ~FACIL_BUS_STOP;

		UpdateStationVirtCoordDirty(st);
		DeleteStationIfEmpty(st);
	}

	return _price.remove_bus_station;	
}


/* Build a truck station
 * p1 - direction
 * p2 - unused
 */
int32 CmdBuildTruckStation(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile;
	int32 cost = 0;
	Station *st;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	tile = TILE_FROM_XY(x,y);

	if (!(flags & DC_NO_CITY_RATING) && !CheckIfAuthorityRefuses(tile))
		return CMD_ERROR;

	if ((cost=CheckFlatLandBelow(tile, 1, 1, flags, 1 << p1)) == CMD_ERROR)
		return CMD_ERROR;

	st = GetStationAround(tile, 1, 1);
	if (st == CHECK_STATIONS_ERR)
		return CMD_ERROR;

	/* Find a station close to us */
	if (st == NULL) {
		st = GetClosestStationFromTile(tile, 8, _current_player);
		if (st!=NULL && st->facilities) st = NULL;
	}

	if (st != NULL) {
		if (st->owner != 0x10 && st->owner != _current_player)
			return_cmd_error(STR_3009_TOO_CLOSE_TO_ANOTHER_STATION);
		
		if (!CheckStationSpreadOut(st, tile, 1, 1))
			return CMD_ERROR;

		if (st->lorry_tile != 0)
			return_cmd_error(STR_3045_TOO_CLOSE_TO_ANOTHER_TRUCK);
	} else {
		City *cit;

		st = AllocateStation();
		if (st == NULL)
			return CMD_ERROR;

		st->city = cit = ClosestCityFromTile(tile, (uint)-1);

		if (_current_player < 8 && flags&DC_EXEC)
			SETBIT(cit->have_ratings, _current_player);

		st->sign.width_1 = 0;

		if (!GenerateStationName(st, tile, 0))
			return CMD_ERROR;

		if (flags & DC_EXEC)
			StationInitialize(st, tile);
	}

	cost += _price.build_truck_station;

	if (flags & DC_EXEC) {
		st->lorry_tile = tile;
		if (!st->facilities) st->xy = tile;
		st->facilities |= FACIL_TRUCK_STOP;
		st->truck_stop_status = 3;
		st->owner = _current_player;

		ModifyTile(tile, 
			MP_SETTYPE(MP_STATION) | MP_MAPOWNER_CURRENT | 
			MP_MAP2 | MP_MAP3LO_CLEAR | MP_MAP3HI_CLEAR | MP_MAP5,
			st->index,			/* map2 parameter */
			p1 + 0x43       /* map5 parameter */
		);

		UpdateStationVirtCoordDirty(st);
		UpdateStationAcceptance(st, false);
		InvalidateWindow(WC_STATION_LIST, st->owner);
	}
	return cost;
}

// Remove a truck station
static int32 RemoveTruckStation(Station *st, uint32 flags)
{
	uint tile;

	if (_current_player != 0x11 && !CheckOwnership(st->owner))
		return CMD_ERROR;
	
	tile = st->lorry_tile;

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		DoClearSquare(tile);
		
		st->lorry_tile = 0;
		st->facilities &= ~FACIL_TRUCK_STOP;

		UpdateStationVirtCoordDirty(st);
		DeleteStationIfEmpty(st);
	}

	return _price.remove_truck_station;	
}

static const uint16 _initial_airport_flags[3] = {
	4,
	0x100,
	0x40,
};

static const byte _airport_map5_tiles_small[] = {
	54, 53, 52, 65,
	58, 57, 56, 55,
	64, 63, 63, 62
};

static const byte _airport_map5_tiles_big[] = {
	31, 9, 33, 9, 9, 32,
	27, 36, 29, 34, 8, 10,
	30, 11, 35, 13, 20, 21,
	51, 12, 14, 17, 19, 28,
	38, 13, 15, 16, 18, 39,
	26, 22, 23, 24, 25, 26
};

static const byte _airport_map5_tiles_heliport[] = {
	66,
};

static const byte * const _airport_map5_tiles[] = {
	_airport_map5_tiles_small,
	_airport_map5_tiles_big,
	_airport_map5_tiles_heliport,
};

static const int8 _airport_depot_offs[3] = {
	TILE_XY(3,0),
	TILE_XY(5,0),
	-TILE_XY(1,0),
};

/* Place an Airport
 * p1 - airport type
 * p2 - unused
 */
int32 CmdBuildAirport(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile;
	City *city;
	Station *st;
	int32 cost;
	int w,h;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	tile = TILE_FROM_XY(x,y);
	
	if (!(flags & DC_NO_CITY_RATING) && !CheckIfAuthorityRefuses(tile))
		return CMD_ERROR;

	city = ClosestCityFromTile(tile, (uint)-1);

	/* Check if local auth refuses a new airport */
	{
		uint num = 0;
		FOR_ALL_STATIONS(st) {
			if (st->xy != 0 && st->city == city && st->facilities&FACIL_AIRPORT && st->airport_type != AT_OILFIELD)
				num++;
		}
		if (num >= 2) {
			SET_DPARAM16(0, city->index);
			return_cmd_error(STR_2035_LOCAL_AUTHORITY_REFUSES);
		}
	}

	w = _airport_size_x[p1];
	h = _airport_size_y[p1];

	cost = CheckFlatLandBelow(tile, w, h, flags, 0);
	if (cost == CMD_ERROR)
		return CMD_ERROR;

	st = GetStationAround(tile, w, h);
	if (st == CHECK_STATIONS_ERR)
		return CMD_ERROR;

	/* Find a station close to us */
	if (st == NULL) {
		st = GetClosestStationFromTile(tile, 8, _current_player);
		if (st!=NULL && st->facilities) st = NULL;
	}

	if (st != NULL) {
		if (st->owner != 0x10 && st->owner != _current_player)
			return_cmd_error(STR_3009_TOO_CLOSE_TO_ANOTHER_STATION);
		
		if (!CheckStationSpreadOut(st, tile, 1, 1))
			return CMD_ERROR;

		if (st->airport_tile != 0)
			return_cmd_error(STR_300D_TOO_CLOSE_TO_ANOTHER_AIRPORT);
	} else {
		City *cit;

		st = AllocateStation();
		if (st == NULL)
			return CMD_ERROR;

		st->city = cit = ClosestCityFromTile(tile, (uint)-1);

		if (_current_player < 8 && flags&DC_EXEC)
			SETBIT(cit->have_ratings, _current_player);

		st->sign.width_1 = 0;

		if (!GenerateStationName(st, tile, p1 == 2 ? 5 : 1))
			return CMD_ERROR;

		if (flags & DC_EXEC)
			StationInitialize(st, tile);
	}

	cost += _price.build_airport * w * h;

	if (flags & DC_EXEC) {
		st->owner = _current_player;
		if (_current_player == _local_player) {
			_last_built_aircraft_depot_tile = tile + _airport_depot_offs[p1];
		}

		st->airport_tile = tile;
		if (!st->facilities) st->xy = tile;
		st->facilities |= FACIL_AIRPORT;
		st->airport_type = (byte)p1;
		st->airport_flags = _initial_airport_flags[p1];
	
		{
			const byte *b = _airport_map5_tiles[p1];
BEGIN_TILE_LOOP(tile_cur,w,h,tile)
				ModifyTile(tile_cur, 
					MP_SETTYPE(MP_STATION) | MP_MAPOWNER_CURRENT |
					MP_MAP2 | MP_MAP3LO_CLEAR | MP_MAP3HI_CLEAR | MP_MAP5,
					st->index, *b++);
END_TILE_LOOP(tile_cur,w,h,tile)
		}

		UpdateStationVirtCoordDirty(st);
		UpdateStationAcceptance(st, false);
		InvalidateWindow(WC_STATION_LIST, st->owner);
	}

	return cost;
}

static int32 RemoveAirport(Station *st, uint32 flags)
{
	uint tile;
	int w,h;
	int32 cost;

	if (_current_player != 0x11 && !CheckOwnership(st->owner))
		return CMD_ERROR;

	tile = st->airport_tile;

	w = _airport_size_x[st->airport_type];
	h = _airport_size_y[st->airport_type];

	cost = w * h * _price.remove_airport;

	{
BEGIN_TILE_LOOP(tile_cur,w,h,tile)
		if (!EnsureNoVehicle(tile_cur))
			return CMD_ERROR;

		if (flags & DC_EXEC) {
			DeleteAnimatedTile(tile_cur);
			DoClearSquare(tile_cur);
		}
END_TILE_LOOP(tile_cur, w,h,tile)
	}

	if (flags & DC_EXEC) {
		DeleteWindowById(WC_VEHICLE_DEPOT, tile + _airport_depot_offs[st->airport_type]);

		st->airport_tile = 0;
		st->facilities &= ~FACIL_AIRPORT;

		UpdateStationVirtCoordDirty(st);
		DeleteStationIfEmpty(st);
	}

	return cost;
}

/* Build a buoy 
 * p1,p2 unused
 */

int32 CmdBuildBuoy(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	TileInfo ti;
	Station *st;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	FindLandscapeHeight(&ti, x, y);

	if (ti.type != MP_WATER || ti.tileh != 0 || ti.map5 != 0 || ti.tile == 0)
		return_cmd_error(STR_304B_SITE_UNSUITABLE);

	st = AllocateStation();
	if (st == NULL)
		return CMD_ERROR;

	st->city = ClosestCityFromTile(ti.tile, (uint)-1);
	st->sign.width_1 = 0;

	if (!GenerateStationName(st, ti.tile, 4))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		StationInitialize(st, ti.tile);
		st->dock_tile = ti.tile;
		st->facilities |= FACIL_DOCK;
		st->had_vehicle_of_type |= HVOT_BUOY;
		st->owner = 0x10;

		ModifyTile(ti.tile,
			MP_SETTYPE(MP_STATION) |
			MP_MAP2 | MP_MAP3LO_CLEAR | MP_MAP3HI_CLEAR | MP_MAPOWNER | MP_MAP5,
			st->index,		 /* map2 */
			0x10,						/* map_owner */
			0x52						/* map5 */
		);

		UpdateStationVirtCoordDirty(st);
		
		UpdateStationAcceptance(st, false);
		InvalidateWindow(WC_STATION_LIST, st->owner);
	}

	return _price.build_dock;
}

static int32 RemoveBuoy(Station *st, uint32 flags)
{
	uint tile;

	if (_current_player >= 8) {
		/* XXX: strange stuff */
		return_cmd_error(INVALID_STRING_ID);
	}

	tile = st->dock_tile;

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		st->dock_tile = 0;
		st->facilities &= ~FACIL_DOCK;
		st->had_vehicle_of_type &= ~HVOT_BUOY;

		ModifyTile(tile, 
			MP_SETTYPE(MP_WATER) |
			MP_MAP2_CLEAR | MP_MAP3LO_CLEAR | MP_MAP3HI_CLEAR | MP_MAPOWNER | MP_MAP5,
			0x11, /* map_owner */
			0			/* map5 */
		);

		UpdateStationVirtCoordDirty(st);
		DeleteStationIfEmpty(st);
	}

	return _price.remove_truck_station;
}

static const int16 _dock_tileoffs_chkaround[4] = {
	TILE_XY(-1,0),
	0,0,
	TILE_XY(0,-1),
};
static const byte _dock_w_chk[4] = { 2,1,2,1 };
static const byte _dock_h_chk[4] = { 1,2,1,2 };

int32 CmdBuildDock(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	TileInfo ti;
	int direction;
	int32 cost;
	uint tile, tile_cur;
	Station *st;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	FindLandscapeHeight(&ti, x, y);

	if ((direction=0,ti.tileh) != 3 &&
			(direction++,ti.tileh) != 9 &&
			(direction++,ti.tileh) != 12 &&
			(direction++,ti.tileh) != 6)
		return_cmd_error(STR_304B_SITE_UNSUITABLE);

	if (!EnsureNoVehicle(ti.tile))
		return CMD_ERROR;

	cost = DoCommandByTile(ti.tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (cost == CMD_ERROR)
		return CMD_ERROR;

	tile_cur = (tile=ti.tile) + _tileoffs_by_dir[direction];

	if (!EnsureNoVehicle(tile_cur))
		return CMD_ERROR;

	FindLandscapeHeightByTile(&ti, tile_cur);
	if (ti.tileh != 0 || ti.type != MP_WATER)
		return_cmd_error(STR_304B_SITE_UNSUITABLE);

	cost = DoCommandByTile(tile_cur, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (cost == CMD_ERROR)
		return CMD_ERROR;

	tile_cur = tile_cur + _tileoffs_by_dir[direction];
	FindLandscapeHeightByTile(&ti, tile_cur);
	if (ti.tileh != 0 || ti.type != MP_WATER)
		return_cmd_error(STR_304B_SITE_UNSUITABLE);
	
	/* middle */
	st = GetStationAround(tile + _dock_tileoffs_chkaround[direction], 
		_dock_w_chk[direction], _dock_h_chk[direction]);
	if (st == CHECK_STATIONS_ERR)
		return CMD_ERROR;
	
	/* Find a station close to us */
	if (st == NULL) {
		st = GetClosestStationFromTile(tile, 8, _current_player);
		if (st!=NULL && st->facilities) st = NULL;
	}

	if (st != NULL) {
		if (st->owner != 0x10 && st->owner != _current_player)
			return_cmd_error(STR_3009_TOO_CLOSE_TO_ANOTHER_STATION);
		
		if (!CheckStationSpreadOut(st, tile, 1, 1))
			return CMD_ERROR;

		if (st->dock_tile != 0)
			return_cmd_error(STR_304C_TOO_CLOSE_TO_ANOTHER_DOCK);
	} else {
		City *cit;

		st = AllocateStation();
		if (st == NULL)
			return CMD_ERROR;

		st->city = cit = ClosestCityFromTile(tile, (uint)-1);

		if (_current_player < 8 && flags&DC_EXEC)
			SETBIT(cit->have_ratings, _current_player);

		st->sign.width_1 = 0;

		if (!GenerateStationName(st, tile, 3))
			return CMD_ERROR;

		if (flags & DC_EXEC)
			StationInitialize(st, tile);
	}

	if (flags & DC_EXEC) {
		st->dock_tile = tile;
		if (!st->facilities) st->xy = tile;
		st->facilities |= FACIL_DOCK;
		st->owner = _current_player;

		ModifyTile(tile, 
			MP_SETTYPE(MP_STATION) | MP_MAPOWNER_CURRENT | 
			MP_MAP2 | MP_MAP3LO_CLEAR | MP_MAP3HI_CLEAR |
			MP_MAP5,
			st->index,
			direction + 0x4C);

		ModifyTile(tile + _tileoffs_by_dir[direction], 
			MP_SETTYPE(MP_STATION) | MP_MAPOWNER_CURRENT | 
			MP_MAP2 | MP_MAP3LO_CLEAR | MP_MAP3HI_CLEAR |
			MP_MAP5,
			st->index,
			(direction&1) + 0x50);
		
		UpdateStationVirtCoordDirty(st);
		UpdateStationAcceptance(st, false);
		InvalidateWindow(WC_STATION_LIST, st->owner);
	}
	return _price.build_dock;
}

static int32 RemoveDock(Station *st, uint32 flags)
{
	uint tile1, tile2;

	if (!CheckOwnership(st->owner))
		return CMD_ERROR;

	tile1 = st->dock_tile;
	tile2 = tile1 + _tileoffs_by_dir[_map5[tile1] - 0x4C];

	if (!EnsureNoVehicle(tile1))
		return CMD_ERROR;

	if (!EnsureNoVehicle(tile2))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		DoClearSquare(tile1);
		DoClearSquare(tile2);

		st->dock_tile = 0;
		st->facilities &= ~FACIL_DOCK;

		UpdateStationVirtCoordDirty(st);
		DeleteStationIfEmpty(st);
	}

	return _price.remove_dock;
}

#include "table/station_land.h"


typedef struct DrawTileSeqStruct {
	int8 delta_x;
	int8 delta_y;
	int8 delta_z;
	byte width,height;
	byte unk;
	SpriteID image;
} DrawTileSeqStruct;


static void DrawTile_Station(TileInfo *ti)
{
	uint32 image_or_modificator;
	uint32 base_img, image;
	const DrawTileSeqStruct *dtss;
	const byte *t;

	base_img = (_map3_lo[ti->tile] & 0xF) * 82;

	{
		uint owner = _map_owner[ti->tile];
		image_or_modificator = 0x315 << 16; /* NOTE: possible bug in ttd here? */
		if (owner < 8)
			image_or_modificator = PLAYER_SPRITE_COLOR(owner);
	}

	// don't show foundation for docks
	if (ti->tileh != 0 && ti->map5 < 0x4C)
		DrawFoundation(ti, ti->tileh);

	t = _station_display_datas[ti->map5];

	image = READ_LE_UINT16(t);
	t += sizeof(uint16);
	if (image & 0x8000)
		image |= image_or_modificator;
	DrawGroundSprite(image + base_img);
	OffsetGroundSpriteEnd();

	for(dtss = (DrawTileSeqStruct *)t; (byte)dtss->delta_x != 0x80; dtss++) {
		if ((byte)dtss->delta_z != 0x80) {
			image =	dtss->image + base_img;
			if (_display_opt & DO_TRANS_BUILDINGS) {
				if (image&0x8000) image |= image_or_modificator;
			} else {
				image = (image & 0x3FFF) | 0x03224000;
			}

			AddSortableSpriteToDraw(image, ti->x + dtss->delta_x, ti->y + dtss->delta_y, dtss->width, dtss->height, dtss->unk, ti->z + dtss->delta_z);
		} else {
			image = READ_LE_UINT32(&dtss->height) + base_img;

			if (_display_opt & DO_TRANS_BUILDINGS) {
				if (image&0x8000) image |= image_or_modificator;	
			} else {
				image = (image & 0x3FFF) | 0x03224000;
			}
			AddChildSpriteScreen(image, dtss->delta_x, dtss->delta_y);
		}
	}
}

void StationPickerDrawSprite(int x, int y, int railtype, int image)
{
	uint32 ormod, img;
	const DrawTileSeqStruct *dtss;
	const byte *t;

	/* baseimage */
	railtype *= TRACKTYPE_SPRITE_PITCH;

	ormod = PLAYER_SPRITE_COLOR(_local_player);

	t = _station_display_datas[image];

	img = READ_LE_UINT16(t);
	t += sizeof(uint16);
	if (img & 0x8000)
		img |= ormod;
	DrawSprite(img, x, y);

	for(dtss = (DrawTileSeqStruct *)t; (byte)dtss->delta_x != 0x80; dtss++) {
		Point pt = RemapCoords(dtss->delta_x, dtss->delta_y, dtss->delta_z);
		DrawSprite((dtss->image | ormod) + railtype, x + pt.x, y + pt.y);
	}
}

static uint16 GetSlopeZ_Station(TileInfo *ti)
{
	uint z = ti->z;
	if (ti->tileh != 0)
		z += 8;
	return z;
}

static void GetAcceptedCargo_Station(uint tile, AcceptedCargo *ac)
{
	/* not used */
}

static void GetTileDesc_Station(uint tile, TileDesc *td)
{
	byte m5;
	StringID str;

	td->owner = _map_owner[tile];

	m5 = _map5[tile];
	(str=STR_305E_RAILROAD_STATION, m5 < 8) ||
	(str=STR_305F_AIRCRAFT_HANGAR, m5==0x20 || m5==0x41) ||
	(str=STR_3060_AIRPORT, m5 < 0x43) ||
	(str=STR_3061_TRUCK_LOADING_AREA, m5 < 0x47) ||
	(str=STR_3062_BUS_STATION, m5 < 0x4B) ||
	(str=STR_4807_OIL_RIG, m5 == 0x4B) ||
	(str=STR_3063_SHIP_DOCK, m5 != 0x52) ||
	(str=STR_3069_BUOY, true);
	td->str = str;
}


static const byte _tile_track_status_rail[8] = { 1,2,1,2,1,2,1,2 };

static uint32 GetTileTrackStatus_Station(uint tile, int mode) {
	uint i = _map5[tile];
	uint j = 0;

	if (mode == 0) {
		if (i < 8)
			j = _tile_track_status_rail[i];
	} else if (mode == 2) {
		// not needed
	} else if (mode == 4) {
		// buoy
		if (i == 0x52) j = 0x3F;
	}

	return j + (j << 8);
}

static void TileLoop_Station(uint tile)
{
	if (_map5[tile] == 0x27 || _map5[tile] == 0x3A)
		AddAnimatedTile(tile);
}


static void AnimateTile_Station(uint tile)
{
	byte m5 = _map5[tile];

	if (m5 >= 0x27 && m5 <= 0x32) {
		if (_tick_counter & 3)
			return;
		
		if (++m5 == 0x32+1)
			m5 = 0x27;

		_map5[tile] = m5;
		MarkTileDirtyByTile(tile);
	} else if (m5 >= 0x3A && m5 <= 0x3D) {
		if (_tick_counter & 1)
			return;

		if (++m5 == 0x3D+1)
			m5 = 0x3A;
		
		_map5[tile] = m5;
		MarkTileDirtyByTile(tile);
	}
}

static void ClickTile_Station(uint tile)
{
	if (_map5[tile] == 0x20 || _map5[tile] == 0x41) {
		ShowAircraftDepotWindow(tile);
	} else {
		ShowStationViewWindow(_map2[tile]);
	}
}

static INLINE bool IsTrainStationTile(uint tile) {
	return IS_TILETYPE(tile, MP_STATION) && IS_BYTE_INSIDE(_map5[tile], 0, 8);
}

static const byte _enter_station_speedtable[12] = {
	215, 195, 175, 155, 135, 115, 95, 75, 55, 35, 15, 0
};

static uint32 VehicleEnter_Station(Vehicle *v, uint tile, int x, int y)
{
	byte station_id;
	byte dir;
	uint16 spd;

	if (v->type == VEH_Train) {
		if (IS_BYTE_INSIDE(_map5[tile], 0, 8) && v->subtype == 0 && 
			!IsTrainStationTile(tile + _tileoffs_by_dir[v->direction >> 1])) {
			
			station_id = _map2[tile];
			if (!(v->next_order & OF_NON_STOP) && !_patches.new_nonstop || 
					 ((v->next_order & OT_MASK) == OT_GOTO_STATION && v->next_order_param == station_id)) {

				if (_patches.new_nonstop && (v->next_order & OF_NON_STOP)) {
					v->cur_order_index++;
				} else if (v->next_order != OT_LEAVESTATION && v->last_station_visited != station_id) {
					x &= 0xF;
					y &= 0xF;
					
					dir = v->direction & 6;
					if (dir & 2) intswap(x,y);
					if (y == 8) {
						if (dir != 2 && dir != 4) {
							x = (~x)&0xF;
						}
						if (x == 12) return 2 | (station_id << 8); /* enter station */
						if (x < 12) {
							v->vehstatus |= VS_TRAIN_SLOWING;
							spd = _enter_station_speedtable[x];
							if (spd < v->cur_speed)
								v->cur_speed = spd;
						}
					}
				}
			}	
		}
	} else if (v->type == VEH_Road) {
		if (v->u.road.state < 16 && (v->u.road.state&4)==0 && v->u.road.frame==0) {
			Station *st = DEREF_STATION(_map2[tile]);
			byte m5 = _map5[tile];
			byte *b, bb,state;

			if (IS_BYTE_INSIDE(m5, 0x43, 0x4B)) {
				b = (m5 >= 0x47) ? &st->bus_stop_status : &st->truck_stop_status;

				bb = *b;

				// Station busy?
				if (bb & 0x80 || (bb&3) == 0)
					return 8;

				state = v->u.road.state + 32;
				if (bb & 1) {
					bb &= ~1;
				} else {
					bb &= ~2;
					state += 2;
				}
				*b = bb;
				v->u.road.state = state;
			}
		}
	}
	
	return 0;
}

static void DeleteStation(Station *st)
{
	int index;

	st->xy = 0;

	DeleteName(st->string_id);
	MarkStationDirty(st);
	RemoveStationFromAlphaList(st);

	index = st->index;
	DeleteWindowById(WC_STATION_VIEW, index);
	DeleteCommandFromVehicleSchedule((index << 8) + OT_GOTO_STATION);
	DeleteSubsidyWithStation(index);
}

/* this function is called for one station each tick */
static void StationHandleBigTick(Station *st)
{
	UpdateStationAcceptance(st, true);

	if (st->facilities == 0) {
		if (++st->delete_ctr >= 8)
			DeleteStation(st);
	}
}

static INLINE void byte_inc_sat(byte *p) { byte b = *p + 1; if (b != 0) *p = b; }

static byte _rating_boost[3] = { 0, 31, 63};

static void UpdateStationRating(Station *st)
{
	GoodsEntry *ge;
	int rating, index;
	int waiting;
	bool waiting_changed = false;

	byte_inc_sat(&st->time_since_load);
	byte_inc_sat(&st->time_since_unload);

	ge = st->goods;
	do {
		if (ge->enroute_from != 0xFF) {
			byte_inc_sat(&ge->enroute_time);
			byte_inc_sat(&ge->days_since_pickup);

			rating = 0;

			{
				int b = ge->last_speed;
				if ((b-=85) >= 0)
					rating += b >> 2;
			}

			{
				byte age = ge->last_age;
				(age >= 3) ||
				(rating += 10, age >= 2) ||
				(rating += 10, age >= 1) ||
				(rating += 13, true);
			}

			{
				if (!IS_HUMAN_PLAYER(st->owner) && st->owner != 0x10)
							rating += _rating_boost[_opt.diff.competitor_intelligence];
			}

			if (st->owner < 8 && HASBIT(st->city->statues, st->owner))
				rating += 26;

			{
				byte days = ge->days_since_pickup;
				if (st->last_vehicle != INVALID_VEHICLE &&
						(&_vehicles[st->last_vehicle])->type == VEH_Ship)
							days >>= 2;
				(days > 21) ||
				(rating += 25, days > 12) ||
				(rating += 25, days > 6) ||
				(rating += 45, days > 3) ||
				(rating += 35, true);
			}
			
			{
				waiting = ge->waiting_acceptance & 0xFFF;
				(rating -= 90, waiting > 1500) ||
				(rating += 55, waiting > 1000) ||
				(rating += 35, waiting > 600) ||
				(rating += 10, waiting > 300) ||
				(rating += 20, waiting > 100) ||
				(rating += 10, true);
			}

			{
				byte nr=rating,or = ge->rating;
				if (rating < 0) nr = 0;
				if (rating > 255) nr = 255;

				if (nr >= or) {
					nr -= or;
					if (nr > 2) nr = 2;
				} else {
					nr -= or;
					if (nr < (byte)-2) nr = (byte)-2;
				}
				ge->rating = (nr += or);
				if (nr <= 64 && waiting >= 200) {
					int dec = Random() & 0x1F;
					if (waiting < 400) dec &= 7;
					waiting -= dec + 1;
					waiting_changed = true;
				}

				if (nr <= 128 && waiting != 0) {
					uint32 r = Random();
					if ( ((byte)r >> 1) >= nr ) {
						waiting = max(waiting - ((r >> 8)&3) - 1, 0);
						waiting_changed = true;
					}
				}

				if (waiting_changed)
					ge->waiting_acceptance = (ge->waiting_acceptance & ~0xFFF) + waiting;
			}
		}
	} while (++ge != endof(st->goods));
	
	index = st->index;

	if (waiting_changed)
		InvalidateWindow(WC_STATION_VIEW, index);
	else
		InvalidateWindowWidget(WC_STATION_VIEW, index, 5);
}

/* called for every station each tick */
static void StationHandleSmallTick(Station *st)
{
	byte b;

	if (st->facilities == 0)
		return;

	b = st->delete_ctr + 1;
	if (b >= 185) b = 0;
	st->delete_ctr = b;

	if (b == 0)
		UpdateStationRating(st);
}

void OnTick_Station()
{
	int i;
	Station *st;

	if (_game_mode == GM_EDITOR)
		return;

	i = _station_tick_ctr;
	if (++_station_tick_ctr == lengthof(_stations))
		_station_tick_ctr = 0;

	st = DEREF_STATION(i);
	if (st->xy != 0)
		StationHandleBigTick(st);

	FOR_ALL_STATIONS(st) {
		if (st->xy != 0)
			StationHandleSmallTick(st);
	}

}

void StationMonthlyLoop()
{
	Station *st;

	FOR_ALL_STATIONS(st) {
		if (st->blocked_months != 0)
			st->blocked_months--;
	}

}


void ModifyStationRatingAround(TileIndex tile, byte owner, int amount, uint radius)
{
	Station *st;
	GoodsEntry *ge;
	int i;

	FOR_ALL_STATIONS(st) {
		if (st->xy != 0 && st->owner == owner && GetTileDist(tile, st->xy) <= radius) {
			ge = st->goods;
			for(i=0; i!=NUM_CARGO; i++,ge++) {
				if (ge->enroute_from != 0xFF) {
					ge->rating = clamp(ge->rating + amount, 0, 255);
				}
			}
		}
	}
}

static void UpdateStationWaiting(Station *st, int type, uint amount)
{
	st->goods[type].waiting_acceptance = 
		(st->goods[type].waiting_acceptance & ~0xFFF) + 
			min(0xFFF, (st->goods[type].waiting_acceptance & 0xFFF) + amount);

	st->goods[type].enroute_time = 0;
	st->goods[type].enroute_from = st->index;
	InvalidateWindow(WC_STATION_VIEW, st->index);
}

int32 CmdRenameStation(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	StringID str,old_str;
	Station *st;

	str = AllocateName((byte*)_decode_parameters, 6);
	if (str == 0)
		return CMD_ERROR;
	
	if (flags & DC_EXEC) {
		st = DEREF_STATION(p1);
		old_str = st->string_id;
		st->string_id = str;
		UpdateStationVirtCoord(st);
		DeleteName(old_str);
		MarkWholeScreenDirty();
	} else {
		DeleteName(str);
	}

	return 0;
}


uint MoveGoodsToStation(uint tile, int w, int h, int type, uint amount)
{
	Station *around_ptr[8];
	byte around[8], st_index;
	int i;
	Station *st;
	uint moved;
	uint best_rating, best_rating2;
	Station *st1, *st2;
	int t;

	memset(around, 0xff, sizeof(around));

	w += 8;
	h += 8;

	BEGIN_TILE_LOOP(cur_tile, w, h, tile - TILE_XY(4,4))
		cur_tile = TILE_MASK(cur_tile);
		if (IS_TILETYPE(cur_tile, MP_STATION)) {
			st_index = _map2[cur_tile];
			for(i=0; i!=8; i++)	{
				if (around[i] == 0xFF) {
					st = DEREF_STATION(st_index);
					if ((st->had_vehicle_of_type & HVOT_BUOY) == 0 &&
							st->blocked_months == 0 &&
							(!_patches.selectgoods || HASBIT(st->accepted_goods, type)) &&
							((st->facilities & (byte)~FACIL_BUS_STOP)!=0 || type==CT_PASSENGERS) &&
							((st->facilities & (byte)~FACIL_TRUCK_STOP)!=0 || type!=CT_PASSENGERS)) {
						
						around[i] = st_index;
						around_ptr[i] = st;
					}
					break;
				} else if (around[i] == st_index)
					break;
			}
		}
	END_TILE_LOOP(cur_tile, w, h, tile - TILE_XY(4,4))

	/* no stations around at all? */
	if (around[0] == 0xFF)
		return 0;

	if (around[1] == 0xFF) {
		/* only one station around */
		moved = (amount * around_ptr[0]->goods[type].rating >> 8) + 1;
		UpdateStationWaiting(around_ptr[0], type, moved);
		return moved;
	}

	/* several stations around, find the two with the highest rating */
	st2 = st1 = NULL;
	best_rating = best_rating2 = 0;
	for(i=0; i!=8 && around[i] != 0xFF; i++) {
		if (around_ptr[i]->goods[type].rating >= best_rating) {
			best_rating2 = best_rating;
			st2 = st1;

			best_rating = around_ptr[i]->goods[type].rating;
			st1 = around_ptr[i];
		} else if (around_ptr[i]->goods[type].rating >= best_rating2) {
			best_rating2 = around_ptr[i]->goods[type].rating;
			st2 = around_ptr[i];			
		}
	}
	
	assert(st1 != NULL);
	assert(st2 != NULL);
	assert(best_rating != 0 || best_rating2 != 0);

	/* the 2nd highest one gets a penalty */
	best_rating2 >>= 1;

	/* amount given to station 1 */
	t = (best_rating * (amount + 1)) / (best_rating + best_rating2);

	moved = 0;
	if (t != 0) {
		moved = (t * best_rating >> 8) + 1;
		amount -= t;
		UpdateStationWaiting(st1, type, moved); 
	}

	assert(amount >= 0);

	if (amount != 0) {
		moved += (amount = (amount * best_rating2 >> 8) + 1);
		UpdateStationWaiting(st2, type, amount);
	}

	return moved;
}

void BuildOilRig(uint tile)
{
	Station *st;
	int j;

	FOR_ALL_STATIONS(st) {
		if (st->xy == 0) {
			st->city = ClosestCityFromTile(tile, (uint)-1);
			st->sign.width_1 = 0;
			if (!GenerateStationName(st, tile, 2))
				return;

			_map_type_and_height[tile] &= 0xF;
			_map_type_and_height[tile] |= MP_STATION << 4;
			_map5[tile] = 0x4B;
			_map_owner[tile] = 0x10;
			_map3_lo[tile] = 0;
			_map3_hi[tile] = 0;
			_map2[tile] = st->index;

			st->owner = 0x10;
			st->airport_flags = 0x40;
			st->airport_type = AT_OILFIELD;
			st->xy = tile;
			st->bus_tile = 0;
			st->lorry_tile = 0;
			st->airport_tile = tile;
			st->dock_tile = tile;
			st->train_tile = 0;
			st->had_vehicle_of_type = 0;
			st->time_since_load = 255;
			st->time_since_unload = 255;
			st->delete_ctr = 0;
			st->last_vehicle = INVALID_VEHICLE;
			st->facilities = FACIL_AIRPORT | FACIL_DOCK;
			for(j=0; j!=NUM_CARGO; j++) {
				st->goods[j].waiting_acceptance = 0;
				st->goods[j].days_since_pickup = 0;
				st->goods[j].enroute_from = 0xFF;
				st->goods[j].rating = 175;
				st->goods[j].last_speed = 0;
				st->goods[j].last_age = 255;
			}

			UpdateStationVirtCoordDirty(st);
			UpdateStationAcceptance(st, false);
			return;		
		}
	}
}

void DeleteOilRig(uint tile)
{
	Station *st = DEREF_STATION(_map2[tile]);

	DoClearSquare(tile);

	st->dock_tile = 0;
	st->airport_tile = 0;
	st->facilities &= ~(FACIL_AIRPORT | FACIL_DOCK);
	st->airport_flags = 0;
	UpdateStationVirtCoordDirty(st);
	DeleteStation(st);
}

static void ChangeTileOwner_Station(uint tile, byte old_player, byte new_player)
{
	if (_map_owner[tile] != old_player)
		return;

	if (new_player != 255) {
		Station *st = DEREF_STATION(_map2[tile]);
		_map_owner[tile] = new_player;
		st->owner = new_player;
	} else {
		DoCommandByTile(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
	}
}

static int32 ClearTile_Station(uint tile, byte flags) {
	byte m5 = _map5[tile];
	Station *st;

	if (flags & DC_AUTO) {
		if (m5 < 8) return_cmd_error(STR_300B_MUST_DEMOLISH_RAILROAD);
		if (m5 < 0x43) return_cmd_error(STR_300E_MUST_DEMOLISH_AIRPORT_FIRST);
		if (m5 < 0x47) return_cmd_error(STR_3047_MUST_DEMOLISH_TRUCK_STATION);
		if (m5 < 0x4B) return_cmd_error(STR_3046_MUST_DEMOLISH_BUS_STATION);
		if (m5 == 0x52) return_cmd_error(STR_306A_BUOY_IN_THE_WAY);
		if (m5 != 0x4B && m5 < 0x53) return_cmd_error(STR_304D_MUST_DEMOLISH_DOCK_FIRST);
		SET_DPARAM16(0, STR_4807_OIL_RIG);
		return_cmd_error(STR_4800_IN_THE_WAY);
	}

	st = DEREF_STATION(_map2[tile]);

	if (m5 < 8)
		return RemoveRailroadStation(st, flags);

	if (m5 < 0x43)
		return RemoveAirport(st, flags);
		
	if (m5 < 0x47)
		return RemoveTruckStation(st, flags);
	
	if (m5 < 0x4B)
		return RemoveBusStation(st, flags);

	if (m5 == 0x52)
		return RemoveBuoy(st, flags);

	if (m5 != 0x4B && m5 < 0x53)
		return RemoveDock(st, flags);

	return CMD_ERROR;

}

void InitializeStations()
{
	int i;
	memset(_stations, 0, sizeof(_stations));
	for(i=0;i!=lengthof(_stations); i++)
		_stations[i].index=i;
	_station_tick_ctr = 0;
}


const TileTypeProcs _tile_type_station_procs = {
	DrawTile_Station,						/* draw_tile_proc */
	GetSlopeZ_Station,					/* get_slope_z_proc */
	ClearTile_Station,					/* clear_tile_proc */
	GetAcceptedCargo_Station,		/* get_accepted_cargo_proc */
	GetTileDesc_Station,				/* get_tile_desc_proc */
	GetTileTrackStatus_Station,	/* get_tile_track_status_proc */
	ClickTile_Station,					/* click_tile_proc */
	AnimateTile_Station,				/* animate_tile_proc */
	TileLoop_Station,						/* tile_loop_clear */
	ChangeTileOwner_Station,		/* change_tile_owner_clear */
	NULL,												/* get_produced_cargo_proc */
	VehicleEnter_Station,				/* vehicle_enter_tile_proc */
	NULL,												/* vehicle_leave_tile_proc */
};


static const byte _station_desc[] = {
	SLE_VAR(Station,xy,							SLE_UINT16),
	SLE_VAR(Station,bus_tile,				SLE_UINT16),
	SLE_VAR(Station,lorry_tile,			SLE_UINT16),
	SLE_VAR(Station,train_tile,			SLE_UINT16),
	SLE_VAR(Station,airport_tile,		SLE_UINT16),
	SLE_VAR(Station,dock_tile,			SLE_UINT16),
	SLE_REF(Station,city,						REF_CITY),
	SLE_VAR(Station,station_platforms,	SLE_UINT8),
	SLE_VAR(Station,alpha_order,		SLE_UINT8),
	SLE_VAR(Station,string_id,			SLE_STRINGID),

	SLE_VAR(Station,had_vehicle_of_type,SLE_UINT16),

	SLE_VAR(Station,time_since_load,		SLE_UINT8),
	SLE_VAR(Station,time_since_unload,	SLE_UINT8),
	SLE_VAR(Station,delete_ctr,					SLE_UINT8),
	SLE_VAR(Station,owner,							SLE_UINT8),
	SLE_VAR(Station,facilities,					SLE_UINT8),
	SLE_VAR(Station,airport_type,				SLE_UINT8),
	SLE_VAR(Station,truck_stop_status,	SLE_UINT8),
	SLE_VAR(Station,bus_stop_status,		SLE_UINT8),
	SLE_VAR(Station,blocked_months,			SLE_UINT8),

	SLE_VAR(Station,airport_flags,			SLE_UINT16),
	SLE_VAR(Station,last_vehicle,				SLE_UINT16),

	SLE_END()
};

static const byte _goods_desc[] = {
	SLE_VAR(GoodsEntry,waiting_acceptance,SLE_UINT16),
	SLE_VAR(GoodsEntry,days_since_pickup,	SLE_UINT8),
	SLE_VAR(GoodsEntry,rating,						SLE_UINT8),
	SLE_VAR(GoodsEntry,enroute_from,			SLE_UINT8),
	SLE_VAR(GoodsEntry,enroute_time,			SLE_UINT8),
	SLE_VAR(GoodsEntry,last_speed,				SLE_UINT8),
	SLE_VAR(GoodsEntry,last_age,					SLE_UINT8),

	SLE_END()
};


static void SaveLoad_STNS(Station *st)
{
	int i;
	SlObject(st, _station_desc);
	for(i=0; i!=NUM_CARGO; i++)
		SlObject(&st->goods[i], _goods_desc);
}

static void Save_STNS()
{
	Station *st;
	// Write the vehicles
	FOR_ALL_STATIONS(st) {
		if (st->xy != 0) {
			SlSetArrayIndex(st->index);
			SlAutolength((AutolengthProc*)SaveLoad_STNS, st);
		}
	}
}

static void Load_STNS()
{
	int index;
	while ((index = SlIterateArray()) != -1) {
		Station *st = &_stations[index];
		SaveLoad_STNS(st);
	}
}

const ChunkHandler _station_chunk_handlers[] = {
	{ 'STNS', Save_STNS, Load_STNS, CH_ARRAY | CH_LAST},
};

