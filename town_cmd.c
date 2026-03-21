#include "stdafx.h"
#include "ttd.h"
#include "viewport.h"
#include "city.h"
#include "command.h"
#include "pathfind.h"
#include "gfx.h"
#include "industry.h"
#include "station.h"
#include "player.h"
#include "news.h"
#include "saveload.h"
#include "economy.h"

// DoCommand flag
static bool _allow_build_house_when_clear;

// Local
static int _grow_city_result;


typedef struct DrawCityTileStruct {
	uint32 sprite_1;
	uint32 sprite_2;

	byte subtile_xy;
	byte width_height;
	byte dz;
	byte proc;
} DrawCityTileStruct;

#include "table/city_land.h"


static void CityDrawTileProc1(TileInfo *ti)
{
	AddChildSpriteScreen(0x5A3, 0xE, 0x3C - (_map_owner[ti->tile]&0x7F));
}

typedef void CityDrawTileProc(TileInfo *ti);
static CityDrawTileProc * const _city_draw_tile_procs[1] = {
	CityDrawTileProc1
};


static void DrawTile_Town(TileInfo *ti)
{
	const DrawCityTileStruct *dcts;
	byte z;
	uint32 image;

	/* Retrieve pointer to the draw city tile struct */
	{
		uint hash, t;
		hash  = ti->x >> 4;
		hash ^= hash>>2;
		hash ^= (t=(ti->y >> 4));
		hash -= t>>2;
		dcts = &_city_draw_tile_data[((_map2[ti->tile]<<4)|(_map3_lo[ti->tile]>>6)|((hash&3)<<2))];
	}

	z = ti->z;

	/* Add bricks below the house? */
	if (ti->tileh & 0xF) {
		AddSortableSpriteToDraw((ti->tileh & 0xF) + 0x3DD, ti->x, ti->y, 16, 16, 7, z);
		AddChildSpriteScreen(dcts->sprite_1, 0x1F, 1);
		z += 8;
	} else {
		/* Else draw regular ground */
		DrawGroundSprite(dcts->sprite_1);
	}

	/* Add a house on top of the ground? */
	if ((image = dcts->sprite_2) != 0) {
		if (!(_display_opt & DO_TRANS_BUILDINGS))
			image = (image & 0x3FFF) | 0x3224000;
		
		AddSortableSpriteToDraw(image, 
			ti->x | (dcts->subtile_xy>>4),
			ti->y | (dcts->subtile_xy&0xF),
			(dcts->width_height>>4)+1,
			(dcts->width_height&0xF)+1,
			dcts->dz,
			z);

		if (!(_display_opt & DO_TRANS_BUILDINGS))
			return;
	}

	{
		int proc;
		if ((proc=dcts->proc-1) >= 0 )
			_city_draw_tile_procs[proc](ti);
	}
}


static uint16 GetSlopeZ_Town(TileInfo *ti)
{
	uint z = GetPartialZ(ti->x&0xF, ti->y&0xF, ti->tileh) + ti->z;
	if (ti->tileh != 0) z = (z & ~7) + 4;
	return (uint16) z;	
}

static void AnimateTile_Town(uint tile)
{
	int old;
	int i;
	int a,b;

	if (_tick_counter & 3)
		return;

	if (_map2[tile] != 4 && _map2[tile] != 5)
		return;

	if (!((old=_map_owner[tile])&0x80)) {
		_map_owner[tile] |= 0x80;

		do {
			i = (Random()&7) - 1;
		} while (i < 0 || i == 1 || i*6==old);

		_map5[tile] = (_map5[tile] & ~0x3F) | i;
	}

	a = _map_owner[tile]&0x7F;
	b = (_map5[tile]&0x3F) * 6;
	a += (a < b) ? 1 : -1;
	_map_owner[tile] = (_map_owner[tile]&0x80)|a;

	if (a == b) {
		_map5[tile] &= 0x40;
		_map_owner[tile] &= 0x7F;
		DeleteAnimatedTile(tile);
	}
	
	MarkTileDirtyByTile(tile);
}

static void UpdateCityRadius(City *c);

static bool IsCloseToTown(uint tile, uint dist)
{
	City *c;

	FOR_ALL_CITIES(c) {
		if (c->xy != 0 && GetTileDist(tile, c->xy) < dist)
			return true;
	}
	return false;
}


static void ChangePopulation(City *c, int mod)
{
	c->population += mod;
	InvalidateWindow(WC_TOWN_VIEW, c->index);

	if (_town_sort_order & 2) _town_sort_dirty = true;
}

static void MakeSingleHouseBigger(uint tile)
{
	byte b;
	
	assert(IS_TILETYPE(tile, MP_HOUSE));
	
	b = _map5[tile];
	if (b & 0x80)
		return;

	_map5[tile] = (b & 0xC0) | ((b+1)&7);

	if ((_map5[tile]&7) != 0)
		return;

	_map3_lo[tile] = _map3_lo[tile] + 0x40;

	if ( (_map3_lo[tile] & 0xC0) == 0xC0) {
		City *c = ClosestCityFromTile(tile, (uint)-1);
		ChangePopulation(c, _housetype_population[_map2[tile]]); 
	}
	MarkTileDirtyByTile(tile);
}

static void MakeTownHouseBigger(uint tile)
{
	uint flags = _house_more_flags[_map2[tile]]; 
	if (flags & 8) MakeSingleHouseBigger(TILE_ADDXY(tile, 0, 0));
	if (flags & 4) MakeSingleHouseBigger(TILE_ADDXY(tile, 0, 1));
	if (flags & 2) MakeSingleHouseBigger(TILE_ADDXY(tile, 1, 0));
	if (flags & 1) MakeSingleHouseBigger(TILE_ADDXY(tile, 1, 1));
}

static void TileLoop_Town(uint tile)
{
	int house;
	City *c;
	uint32 r;

	if ((_map3_lo[tile] & 0xC0) != 0xC0) {
		MakeTownHouseBigger(tile);
		return;
	}

	house = _map2[tile];
	if (_housetype_extra_flags[house] & 0x20 &&
			!(_map5[tile] & 0x80) &&
			CHANCE16(1,2) &&
			AddAnimatedTile(tile)) {
		_map5[tile] = (_map5[tile] & 0x40)|0x80;
	}

	c = ClosestCityFromTile(tile, (uint)-1);

	r = Random();

	if ( (byte)r < _housetype_population[house] ) {
		uint amt = ((byte)r >> 3) + 1, moved;
		if (_economy.fluct <= 0) amt = (amt + 1) >> 1;
		c->new_max_pass += amt;
		moved = MoveGoodsToStation(tile, 1, 1, CT_PASSENGERS, amt);
		c->new_act_pass += moved;
	}

	if ( (byte)(r>>8) < _housetype_mailamount[house] ) {
		uint amt = ((byte)(r>>8) >> 3) + 1, moved;
		if (_economy.fluct <= 0) amt = (amt + 1) >> 1;
		c->new_max_mail += amt;
		moved = MoveGoodsToStation(tile, 1, 1, CT_MAIL, amt);
		c->new_act_mail += moved;
	}

	if (_house_more_flags[house]&8 && (c->flags12&1) && --c->time_until_rebuild == 0) {
		r>>=16;
		c->time_until_rebuild = (r & 63) + 130;
		_current_player = c->index | 0x80;

		DoCommandByTile(tile, 0, 0, DC_EXEC, CMD_CLEAR_TOWN_HOUSE);

		if ( (byte) (r >> 8) >= 12) {
			_allow_build_house_when_clear = true;
			DoCommandByTile(tile, 0, 0, DC_EXEC, CMD_BUILD_TOWN_HOUSE);
			_allow_build_house_when_clear = false;
		}
	}
}

static void ClickTile_Town(uint tile)
{
	/* not used */
}

static int32 ClearTile_Town(uint tile, byte flags)
{
	return DoCommandByTile(tile, 0, 0, flags, CMD_CLEAR_TOWN_HOUSE);
}

static void GetAcceptedCargo_Town(uint tile, AcceptedCargo *ac)
{
	int type = _map2[tile];
	
	ac->type_1 = CT_PASSENGERS;
	ac->amount_1 = _housetype_cargo_passengers[type];

	ac->type_2 = CT_GOODS;
	ac->amount_2 = _housetype_cargo_goods[type];
	if (ac->amount_2 & 0x80) {
		ac->amount_2 &= 0x7F;
		ac->type_2 = CT_FOOD;
	}

	ac->type_3 = CT_MAIL;
	ac->amount_3 = _housetype_cargo_mail[type];
}

static void GetTileDesc_Town(uint tile, TileDesc *td)
{
	City *c;

	td->str = _city_tile_names[_map2[tile]];
	if ((_map3_lo[tile] & 0xC0) != 0xC0) {
		SET_DPARAMX16(td->dparam, 0, td->str);
		td->str = STR_2058_UNDER_CONSTRUCTION;
	}

	c = ClosestCityFromTile(tile, (uint)-1);
	assert(c != NULL);
	td->owner = c->index | 0x80;
}

static uint32 GetTileTrackStatus_Town(uint tile, int mode)
{
	/* not used */
	return 0;
}

static void ChangeTileOwner_Town(uint tile, byte old_player, byte new_player)
{
	/* not used */
}


static const int _roadblock_tileadd[4+3] = {
	TILE_XY(0,-1),
	TILE_XY(1,0),
	TILE_XY(0,1),
	TILE_XY(-1,0),
	
	// Store the first 3 elements again.
	// Lets us rotate without using &3.
	TILE_XY(0,-1),
	TILE_XY(1,0),
	TILE_XY(0,1),
};

static void TownTickHandler(City *c)
{
	if (c->flags12&1) {
		int i = c->grow_counter - 1;
		if (i < 0) {
			if (GrowCity(c)) {
				i = c->growth_rate;
			} else {
				i = 0;	
			}
		}
		c->grow_counter = i;
	}

	UpdateCityRadius(c);
}

void OnTick_Town()
{
	uint i;
	City *c;

	if (_game_mode == GM_EDITOR)
		return;

	i = _cur_town_ctr;
	c = DEREF_CITY(i);
	if (++i == lengthof(_cities)) i = 0;
	_cur_town_ctr = i;

	if (c->xy != 0)
		TownTickHandler(c);

}

static byte GetCityRoadMask(TileIndex tile)
{
	byte b = GetRoadBitsByTile(tile);
	byte r=0;
	if (b&1) r|=10;
	if (b&2) r|=5;
	if (b&4) r|=9;
	if (b&8) r|=6;
	if (b&16) r|=3;
	if (b&32) r|=12;
	return r;
}

static bool IsRoadAllowedHere(uint tile, int dir)
{
	uint k;
	uint slope;

	// If this assertion fails, it might be because the world contains
	//  land at the edges. This is not ok.
	TILE_ASSERT(tile);
	
	for(;;) {
		// Check if there already is a road at this point?
		if (GetRoadBitsByTile(tile) == 0) {
			// No, try to build one in the direction.
			// if that fails clear the land, and if that fails exit.
			// This is to make sure that we can build a road here later.
			if (DoCommandByTile(tile, (dir&1)?0xA:0x5, 0, DC_AUTO, CMD_BUILD_ROAD) == CMD_ERROR &&
					DoCommandByTile(tile, 0, 0, DC_AUTO, CMD_LANDSCAPE_CLEAR) == CMD_ERROR)
				return false;
		}

		slope = GetTileSlope(tile, NULL);
		if (slope == 0) {
			// Tile has no slope
			// Disallow the road if any neighboring tile has a road.
			if (HASBIT(GetCityRoadMask(TILE_ADD(tile, _roadblock_tileadd[dir+1])), dir^2) ||
					HASBIT(GetCityRoadMask(TILE_ADD(tile, _roadblock_tileadd[dir+3])), dir^2) ||
					HASBIT(GetCityRoadMask(TILE_ADD(tile, _roadblock_tileadd[dir+1] + _roadblock_tileadd[dir+2])), dir) ||
					HASBIT(GetCityRoadMask(TILE_ADD(tile, _roadblock_tileadd[dir+3] + _roadblock_tileadd[dir+2])), dir))
				return false;
			
			// Otherwise allow
			return true;
		}
		
		// If the tile is not a slope in the right direction, then
		// maybe terraform some.
		if ((k = (dir&1)?0xC:0x9) != slope && (k^0xF) != slope) {
			uint32 r = Random();

			if (CHANCE16I(1,8, r) && !_generating_world) {
				if (CHANCE16I(1,16,r))
					DoCommandByTile(tile, slope, 0, DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_TERRAFORM_LAND);
				else
					DoCommandByTile(tile, slope^0xF, 1, DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_TERRAFORM_LAND);
			}
			return false;
		}

		tile = TILE_ADD(tile, _roadblock_tileadd[dir]);
	}
}

static bool TerraformCityTile(uint tile, int edges, int dir)
{
	int32 r;
	
	TILE_ASSERT(tile);

	r = DoCommandByTile(tile, edges, dir, DC_AUTO | DC_NO_WATER, CMD_TERRAFORM_LAND);
	if (r == CMD_ERROR || r >= 126*16)
		return false;
	DoCommandByTile(tile, edges, dir, DC_AUTO | DC_NO_WATER | DC_EXEC, CMD_TERRAFORM_LAND);
	return true;
}

static void LevelCityLand(uint tile)
{
	TileInfo ti;

	TILE_ASSERT(tile);

	// Don't terraform if land is plain or if there's a house there.
	FindLandscapeHeightByTile(&ti, tile);
	if (ti.tileh == 0 || ti.type == MP_HOUSE)
		return;

	// First try up, then down
	if (!TerraformCityTile(tile, ~ti.tileh & 0xF, 1)) {
		TerraformCityTile(tile, ti.tileh & 0xF, 0);
	}
}

#define IS_WATER_TILE(t) (IS_TILETYPE((t), MP_WATER) && _map5[(t)] == 0)

static void GrowCityInTile(uint *tile_ptr, uint mask, int block, City *city)
{
	City *c;
	uint16 r;
	int a,b,rcmd;
	uint tmptile;
	TileInfo ti;
	int i;
	int j;
	uint tile = *tile_ptr;

	TILE_ASSERT(tile);

	if (mask == 0) {
		// Tile has no road. First reset the status counter
		// to say that this is the last iteration.	
		_grow_city_result = 0;

		// Then check if the tile we are at belongs to the city,
		// if not, bail out.
		c = ClosestCityFromTile(tile, (uint)-1);
		if (c != city)
			return;

		// Remove hills etc
		LevelCityLand(tile);

		// Is a road allowed here?
		if (!IsRoadAllowedHere(tile, block))
			return;

		// Randomize new road block numbers
		a = block;
		b = block ^ 2;
		r = (uint16)Random();
		if (r <= 0x4000) do {
			a = (int)Random() & 3;
		} while(a == b);

		if (!IsRoadAllowedHere(TILE_ADD(tile,_roadblock_tileadd[a]), a)) {
			// A road is not allowed to continue the randomized road,
			//   return if the road we're trying to build is curved.
			if ( a != (b^2))
				return;
			
			// Return if neither side of the new road is a house
			if (!IS_TILETYPE(TILE_ADD(tile,_roadblock_tileadd[a+1]), MP_HOUSE) &&
					!IS_TILETYPE(TILE_ADD(tile,_roadblock_tileadd[a+3]), MP_HOUSE))
				return;

			// That means that the road is only allowed if there is a house
			//  at any side of the new road.
		}
		rcmd = (1 << a) + (1 << b);

	} else if (block < 5 && !HASBIT(mask,block^2)) {
		// Continue building on a partial road.
		// Always OK.
		_grow_city_result = 0;
		rcmd = 1 << (block^2);
	} else {

		// Reached a tunnel? Then continue at the other side of it.
		if (IS_TILETYPE(tile, MP_TUNNELBRIDGE) && (_map5[tile]&~3)==4) {
			FindLengthOfTunnelResult flotr = FindLengthOfTunnel(tile, _map5[tile]&3, 2);
			*tile_ptr = flotr.tile;
			return;
		}
		
		// For any other kind of tunnel/bridge, bail out.
		if (IS_TILETYPE(tile, MP_TUNNELBRIDGE))
			return;

		// Possibly extend the road in a direction.
		// Randomize a direction and if it has a road, bail out.
		i = (int)Random() & 3;
		if (HASBIT(mask, i))
			return;

		// This is the tile we will reach if we extend to this direction.
		tmptile = TILE_ADD(tile,_roadblock_tileadd[i]);
		
		// Don't do it if it reaches to water.
		if (IS_WATER_TILE(tmptile))
			return;

		// If the new tile belongs to another city,
		//  then stop the search altogether.
		if (ClosestCityFromTile(tmptile, (uint)-1) != city) {
			_grow_city_result = 0;
			return;
		}

		// Build a house at the edge. 60% chance or 
		//  always ok if no road allowed.
		if (!IsRoadAllowedHere(tmptile, i) || CHANCE16(6,10)) {
			// But not if there already is a house there.
			if (!IS_TILETYPE(tmptile, MP_HOUSE)) {
				// Level the land if possible
				LevelCityLand(tmptile);

				// And build a house.
				// Set result to -1 if we managed to build it.
				if (DoCommandByTile(tmptile, 0, 0, DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_BUILD_TOWN_HOUSE) != CMD_ERROR)
					_grow_city_result = -1;
			}
			return;
		}

		_grow_city_result = 0;
		rcmd = 1 << i;
	}

	FindLandscapeHeightByTile(&ti, tile);

	// Return if a water tile
	if (ti.type == MP_WATER && ti.map5==0)
		return;

	// Determine direction of slope,
	//  and build a road if not a special slope.
	if ((i=0,ti.tileh != 3) &&
			(i++,ti.tileh != 9) &&
			(i++,ti.tileh != 12) &&
			(i++,ti.tileh != 6)) {
build_road_and_exit:
		if (DoCommandByTile(tile, rcmd, 0, DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_BUILD_ROAD) != CMD_ERROR)
			_grow_city_result = -1;
		return;
	}

	tmptile = tile;

	// Now i contains the direction of the slope
	j = -11;
	do {
		if (++j == 0)
			goto build_road_and_exit;
		tmptile = TILE_MASK(tmptile + _tileoffs_by_dir[i]);
	} while (IS_WATER_TILE(tmptile));

	// no water tiles in between?
	if (j == -10)
		goto build_road_and_exit;

	// Quit if it fails a large number of times.
	j = 22;
	do {
		if (DoCommandByTile(tile, tmptile, 0x8000 + RandomRange(11), DC_EXEC | DC_AUTO, CMD_BUILD_BRIDGE) != CMD_ERROR) {
			_grow_city_result = -1;
			return;
		}
	} while(_error_message == STR_500C && --j != 0);
}
#undef IS_WATER_TILE


// Returns true if a house was built, or no if the build failed.
static int GrowCityAtRoad(City *c, uint tile)
{
	uint mask;
	int block = 5; // special case

	TILE_ASSERT(tile);

	// Number of times to search.
	_grow_city_result = 20;

	do {
		// Get a bitmask of the road blocks on a tile
		mask = GetCityRoadMask(tile);

		// Try to grow the city from this point
		GrowCityInTile(&tile,mask,block,c);

		// Exclude the source position from the bitmask
		// and return if no more road blocks available
		CLRBIT(mask, (block ^ 2));
		if (mask == 0)
			return _grow_city_result;

		// Select a random bit from the blockmask, walk a step
		// and continue the search from there.
		do block = Random() & 3; while (!HASBIT(mask,block));
		tile += _roadblock_tileadd[block];

		// Max number of times is checked.
	} while (--_grow_city_result >= 0);

	return (_grow_city_result == -2);
}

// Generate a random road block
// The probability of a straight road
// is somewhat higher than a curved.
static int GenRandomRoadBits()
{
	uint32 r = Random();
	int a = r&3, b = (r >> 8) & 3;
	if (a == b) b ^= 2;
	return (1<<a)+(1<<b);
}

// Grow the city
// Returns true if a house was built, or no if the build failed.
bool GrowCity(City *c)
{
	uint tile;
	const int16 *ptr;
	int offs;
	TileInfo ti;

	static const int16 _city_coord_mod[] = {
		TILE_XY(-1,0),
		TILE_XY(1,1),
		TILE_XY(1,-1),
		TILE_XY(-1,-1),
		TILE_XY(-1,0),
		TILE_XY(0,2),
		TILE_XY(2,0),
		TILE_XY(0,-2),
		TILE_XY(-1,-1),
		TILE_XY(-2,2),
		TILE_XY(2,2),
		TILE_XY(2,-2),
		0,
	};

	// Current player is the city index
	_current_player = c->index | 0x80;

	// Find a road that we can base the construction on.
	tile = c->xy;
	ptr = _city_coord_mod;
	do {
		if (GetRoadBitsByTile(tile) != 0) {
			return GrowCityAtRoad(c, tile);
		}
		offs = *ptr++;
		
		tile = TILE_ADD(tile, offs);
	} while (offs);

	// No road available, try to build a random road block by
	// clearing some land and then building a road there.
	tile = c->xy;
	ptr = _city_coord_mod;
	do {
		FindLandscapeHeightByTile(&ti, tile);
		
		// Only work with plain land that not already has a house with map5=0
		if (ti.tileh == 0 && !(ti.type==MP_HOUSE && ti.map5==0)) {
			if (DoCommandByTile(tile, 0, 0, DC_AUTO, CMD_LANDSCAPE_CLEAR) != CMD_ERROR) {
				DoCommandByTile(tile, GenRandomRoadBits(), 0, DC_EXEC | DC_AUTO, CMD_BUILD_ROAD);
				return true;
			}
		}
		offs = *ptr++;
		tile = TILE_ADD(tile, offs);
	} while (offs != 0);

	return false;
}

static void UpdateCityRadius(City *c)
{
	static const uint16 _city_radius_data[23][5] = {
		{ 4,  0,  0,  0,  0},
		{16,  0,  0,  0,  0},
		{25,  0,  0,  0,  0},
		{36,  0,  0,  0,  0},
		{49,  0,  4,  0,  0},
		{64,  0,  4,  0,  0},
		{64,  0,  9,  0,  1},
		{64,  0,  9,  0,  4},
		{64,  0, 16,  0,  4},
		{81,  0, 16,  0,  4},
		{81,  0, 16,  0,  4},
		{81,  0, 25,  0,  9},
		{81, 36, 25,  0,  9},
		{81, 36, 25, 16,  9},
		{81, 49,  0, 25,  9},
		{81, 64,  0, 25,  9},
		{81, 64,  0, 36,  9},
		{81, 64,  0, 36, 16},
		{ 0, 81,  0, 49, 16},
		{ 0, 81,  0, 49, 25},
		{ 0,100,  0, 49, 25},
		{ 0,100,  0, 64, 25},
		{ 0,100,  0, 64, 36},
	};
	int i = min(c->num_houses, 88) >> 2;
	memcpy(c->radius, _city_radius_data[i], sizeof(c->radius));
}

static void UpdateCityVirtCoord(City *c)
{
	Point pt = RemapCoords2(GET_TILE_X(c->xy)*16, GET_TILE_Y(c->xy)*16);
	SET_DPARAM32(0, c->citynameparts);
	UpdateViewportSignPos(&c->sign, pt.x, pt.y - 24, c->citynametype);
}

static void CreateCityName(City *c)
{
	City *cc;
	char buf1[64];
	char buf2[64];
	uint32 r;

	c->citynametype = SPECSTR_TOWNNAME_START + _opt.city_name;

	for(;;) {
restart:
		r = Random();

		SET_DPARAM32(0, r);
		GetString(buf1, c->citynametype);
			
		// Check size and width
		if (strlen(buf1) >= 31 || GetStringWidth(buf1) > 130)
			continue;

		FOR_ALL_CITIES(cc) {
			if (cc->xy != 0) {
				SET_DPARAM32(0, cc->citynameparts);
				GetString(buf2, cc->citynametype);
				if (str_eq(buf1, buf2))
					goto restart;
			}
		}
		c->citynameparts = r;
		
		return;
	}
}

static void UpdateCityMaxPass(City *c)
{
	c->max_pass = c->population >> 3;
	c->max_mail = c->population >> 4;
}

static void DoCreateTown(City *c, TileIndex tile)
{
	int x, i;

	// clear the city struct
	i = c->index;
	memset(c, 0, sizeof(City));
	c->index = i;

	c->xy = tile;
	c->num_houses = 0;
	c->time_until_rebuild = 10;
	UpdateCityRadius(c);
	c->flags12 = 0;
	c->population = 0;
	c->grow_counter = 0;
	c->growth_rate = 250;
	c->new_max_pass = 0;
	c->new_max_mail = 0;
	c->new_act_pass = 0;
	c->new_act_mail = 0;
	c->max_pass = 0;
	c->max_mail = 0;
	c->act_pass = 0;
	c->act_mail = 0;

	c->pct_pass_transported = 0;
	c->pct_mail_transported = 0;
	c->fund_buildings_months = 0;
	c->new_act_food = 0;
	c->new_act_paper = 0;
	c->act_food = 0;
	c->act_paper = 0;

	for(i=0; i!=8; i++)
		c->ratings[i] = 500;

	c->have_ratings = 0;
	c->statues = 0;

	CreateCityName(c);
		
	UpdateCityVirtCoord(c);
	_town_sort_dirty = true;

	x = (Random() & 0xF) + 8;
	if (_game_mode == GM_EDITOR)
		x = _new_town_size * 16 + 3;

	c->num_houses += x;
	UpdateCityRadius(c);

	i = x * 4;
	do {
		GrowCity(c);
	} while (--i);

	c->num_houses -= x;
	UpdateCityRadius(c);
	UpdateCityMaxPass(c);
}

static City *AllocateTown()
{
	City *c;
	FOR_ALL_CITIES(c) {
		if (c->xy == 0)
			return c;
	}
	return NULL;
}

int32 CmdBuildTown(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile = TILE_FROM_XY(x,y);
	TileInfo ti;
	City *c;
	
	SET_EXPENSES_TYPE(EXPENSES_OTHER);

	// Check if too close to the edge of map
	if (!CheckDistanceFromEdge(tile, 12))
		return_cmd_error(STR_0237_TOO_CLOSE_TO_EDGE_OF_MAP);

	// Can only build on clear flat areas.
	FindLandscapeHeightByTile(&ti, tile);
	if (ti.type != MP_CLEAR || ti.tileh != 0)
		return_cmd_error(STR_0239_SITE_UNSUITABLE);

	// Check distance to all other cities.
	if (IsCloseToTown(tile, 20))
		return_cmd_error(STR_0238_TOO_CLOSE_TO_ANOTHER_TOWN);

	// Allocate town struct
	c = AllocateTown();
	if (c == NULL)
		return_cmd_error(STR_023A_TOO_MANY_TOWNS);	

	// Create the town
	if (flags & DC_EXEC) {
		_generating_world = true;
		DoCreateTown(c, tile);
		_generating_world = false;
	}
	return 0;
}

City *CreateRandomTown()
{
	uint tile;
	TileInfo ti;
	City *c;
	int n;

	// Try 20 times.
	n = 20;
	do {
		// Generate a tile index not too close from the edge
		tile = TILE_MASK(Random());
		if (!CheckDistanceFromEdge(tile, 20))
			continue;

		// Make sure the tile is plain
		FindLandscapeHeightByTile(&ti, tile);
		if (ti.type != MP_CLEAR || ti.tileh != 0)
			continue;

		// Check not too close to a town
		if (IsCloseToTown(tile, 20))
			continue;
		
		// Allocate a town struct
		c = AllocateTown();
		if (c == NULL)
			break;

		DoCreateTown(c, tile);
		return c;
	} while (--n);
	return NULL;
}

static const byte _num_initial_cities[3] = {
	11, 23, 46
};

void GenerateTowns()
{
	uint n;
	n = _num_initial_cities[_opt.diff.number_cities] + (Random()&7);
	do CreateRandomTown(); while (--n);
}

static bool CheckBuildHouseMode(City *c, uint tile, uint tileh, int mode) {
	City *c2 = ClosestCityFromTile(tile, (uint)-1);
	int b;
	uint slope;

	static const byte _masks[8] = {
		0xC,0x3,0x9,0x6,
		0x3,0xC,0x6,0x9,
	};

	if (c2 != c)
		return false;

	slope = GetTileSlope(tile, NULL);
	if (slope & 0x10)
		return false;

	b = 0;
	if ((slope & 0xF && ~slope & _masks[mode])) b = ~b;
	if ((tileh & 0xF && ~tileh & _masks[mode+4])) b = ~b;
	if (b)
		return false;

	return DoCommandByTile(tile, 0, 0, DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_LANDSCAPE_CLEAR) != CMD_ERROR;
}

int GetCityRadiusGroup(City *c, uint tile)
{
	uint dist;
	int i,smallest;

	dist = GetTileDistAdv(tile, c->xy);
	if (c->fund_buildings_months && dist <= 25)
		return 4;

	smallest = 0;
	for(i=0; i!=lengthof(c->radius); i++) {
		if (dist < c->radius[i])
			smallest = i;
	}

	return smallest;
}

static bool CheckFree2x2Area(City *c, uint tile)
{
	City *c2;
	int i;

	static const int _tile_add[4] = {
		TILE_XY(0,0),
		TILE_XY(0,1) - TILE_XY(0,0),
		TILE_XY(1,0) - TILE_XY(0,1),
		TILE_XY(1,1) - TILE_XY(1,0),
	};

	for(i=0; i!=4; i++) {
		tile += _tile_add[i];

		c2 = ClosestCityFromTile(tile, (uint)-1);
		if (c != c2) 
			return false;

		if (GetTileSlope(tile, NULL))
			return false;

		if (DoCommandByTile(tile, 0, 0, DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_LANDSCAPE_CLEAR) == CMD_ERROR)
			return false;
	}

	return true;
}

static void DoBuildTownHouse(City *c, uint tile)
{
	int i;
	uint bitmask;
	int house;
	uint slope;
	int z;
	uint oneof;
	
	// Above snow?
	slope = GetTileSlope(tile, &z);

	// Get the city zone type
	{
		uint rad = GetCityRadiusGroup(c, tile);

		int land = _opt.landscape;
		if (land == LT_HILLY && z >= _opt.snow_line)
			land = -1;

		bitmask = (1 << rad) + (1 << (land + 12));
	}

	// bits 0-4 are used
	// bits 11-15 are used
	// bits 5-10 are not used.
	{
		byte houses[lengthof(_housetype_flags)];
		int num = 0;

		// Generate a list of all possible houses that can be built.
		for(i=0; i!=lengthof(_housetype_flags); i++) {
			if ((~_housetype_flags[i] & bitmask) == 0)
				houses[num++] = (byte)i;
		}

		for(;;) {
			house = houses[RandomRange(num)];

			if (_cur_year < _housetype_years[house].min || _cur_year > _housetype_years[house].max)
				continue;

			// Special houses that there can be only one of.
			oneof = 0;
			if (house == 0x5B || house == 0x53 || house == 3 ||	house == 0x3C || house == 0x3D)
				oneof = 2;
			else if (house == 0x20 || house == 0x14)
				oneof = 4;

			if (c->flags12 & oneof)
				continue;

			// Make sure there is no slope?
			if (_housetype_extra_flags[house]&0x12 && slope)
				continue;
			
			if (_housetype_extra_flags[house]&0x10) {
				if (CheckFree2x2Area(c,tile) ||
						CheckFree2x2Area(c,(tile+=TILE_XY(-1,0))) ||
						CheckFree2x2Area(c,(tile+=TILE_XY(0,-1))) ||
						CheckFree2x2Area(c,(tile+=TILE_XY(1,0))))
							break;
				tile += TILE_XY(0,1);
			} else if (_housetype_extra_flags[house]&4) {
				if (CheckBuildHouseMode(c, tile+TILE_XY(1,0), slope, 0))
					break;
				
				if (CheckBuildHouseMode(c, tile+TILE_XY(-1,0), slope, 1)) {
					tile += TILE_XY(-1,0);
					break;
				}
			} else if (_housetype_extra_flags[house]&8) {
				if (CheckBuildHouseMode(c, tile+TILE_XY(0,1), slope, 2))
					break;

				if (CheckBuildHouseMode(c, tile+TILE_XY(0,-1), slope, 3)) {
					tile += TILE_XY(0,-1);
					break;
				}
			} else
				break;
		}
	}

	c->num_houses++;

	// Special houses that there can be only one of.
	c->flags12 |= oneof;
	
	{
		int m3lo,m5,eflags;

		// ENDING_2
		m3lo = 0;
		m5 = 0;
		if (_generating_world) {
			uint32 r = Random();
			
			// Value for map3lo
			m3lo = 0xC0;
			if ((byte)r >= 220) m3lo &= (r>>8);

			if (m3lo == 0xC0)
				ChangePopulation(c, _housetype_population[house]);
			
			// Initial value for map5.
			m5 = (r >> 16) & 0x3F;
		}
		
		assert(IS_TILETYPE(tile, MP_CLEAR));

		ModifyTile(tile, 
			MP_SETTYPE(MP_HOUSE) | MP_MAP2 | MP_MAP3LO | MP_MAP3HI_CLEAR | MP_MAP5 | MP_MAPOWNER,
			house, /* map2 */
			m3lo,  /* map3_lo */
			0,     /* map_owner */
			m5		 /* map5 */
		);

		eflags = _housetype_extra_flags[house];

		if (eflags&0x18) {
			assert(IS_TILETYPE(tile + TILE_XY(0,1), MP_CLEAR));
			ModifyTile(tile + TILE_XY(0,1),
				MP_SETTYPE(MP_HOUSE) | MP_MAP2 | MP_MAP3LO | MP_MAP3HI_CLEAR | MP_MAP5 | MP_MAPOWNER,
				++house,	/* map2 */
				m3lo,			/* map3_lo */
				0,				/* map_owner */
				m5				/* map5 */
			);
		}

		if (eflags&0x14) {
			assert(IS_TILETYPE(tile + TILE_XY(1,0), MP_CLEAR));
			ModifyTile(tile + TILE_XY(1,0),
				MP_SETTYPE(MP_HOUSE) | MP_MAP2 | MP_MAP3LO | MP_MAP3HI_CLEAR | MP_MAP5 | MP_MAPOWNER,
				++house,	/* map2 */
				m3lo,			/* map3_lo */
				0,				/* map_owner */
				m5				/* map5 */
			);
		}

		if (eflags&0x10) {
			assert(IS_TILETYPE(tile + TILE_XY(1,1), MP_CLEAR));
			ModifyTile(tile + TILE_XY(1,1),
				MP_SETTYPE(MP_HOUSE) | MP_MAP2 | MP_MAP3LO | MP_MAP3HI_CLEAR | MP_MAP5 | MP_MAPOWNER,
				++house,	/* map2 */
				m3lo,			/* map3_lo */
				0,				/* map_owner */
				m5				/* map5 */
			);
		}
	}

	// ENDING
}

int32 CmdBuildTownHouse(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile = TILE_FROM_XY(x,y);
	
	int32 r;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	if (GetTileSlope(tile, NULL) & 0x10)
		return_cmd_error(STR_EMPTY);

	r = DoCommandByTile(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (r == CMD_ERROR || (!_allow_build_house_when_clear && r == 0))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		DoBuildTownHouse(DEREF_CITY((int)_current_player - 0x80), tile);
	}

	return 0;
}

static void DoClearTownHouse(uint tile)
{
	DoClearSquare(tile);
	DeleteAnimatedTile(tile);
}

int32 CmdClearTownHouse(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile = TILE_FROM_XY(x,y);
	int32 cost;
	uint house, eflags;
	int rating;
	City *c;
	
	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	house = _map2[tile];

	cost = _price.remove_house * _housetype_remove_cost[house] >> 8;

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	if (flags&DC_AUTO && !(flags&DC_AI_BUILDING))
		return_cmd_error(STR_2004_BUILDING_MUST_BE_DEMOLISHED);

	rating = _housetype_remove_ratingmod[house];
	_cleared_city_rating += rating;
	_cleared_city = c = ClosestCityFromTile(tile, (uint)-1);
	
	if (_current_player < 8) {
		if (rating > c->ratings[_current_player] && !(flags & DC_NO_CITY_RATING)) {
			SET_DPARAM16(0, c->index);
			return_cmd_error(STR_2009_LOCAL_AUTHORITY_REFUSES);
		}
		if (flags & DC_EXEC) {
			c->ratings[_current_player] -= rating;
			SETBIT(c->have_ratings, _current_player);
		}
	}

	if (flags & DC_EXEC) {
		// need to align house to point to the
		// upper left corner of the house
		if (_housetype_extra_flags[house-1] & 0x04) {
			house--;
			tile += TILE_XY(-1,0);
		} else if (_housetype_extra_flags[house-1] & 0x18) {
			house--;
			tile += TILE_XY(0,-1);
		} else if (_housetype_extra_flags[house-2] & 0x10) {
			house-=2;
			tile += TILE_XY(-1,0);
		} else if (_housetype_extra_flags[house-3] & 0x10) {
			house-=3;
			tile += TILE_XY(-1,-1);
		}
		
		// Remove population from the town if the
		// house is finished.
		if ((~_map3_lo[tile] & 0xC0) == 0) {
			ChangePopulation(c, -_housetype_population[house]);
		}

		c->num_houses--;

		// Clear flags for houses that only may exist once/town.
		if (house == 0x5B || house == 0x53 || house == 0x3C ||
				house == 0x3D || house == 0x03)
			c->flags12 &= ~2;
		if (house == 0x14 || house == 0x20)
			c->flags12 &= ~4;
		
		// Do the actual clearing of tiles
		eflags = _housetype_extra_flags[house];
		DoClearTownHouse(tile);
		if (eflags & 0x14) DoClearTownHouse(tile + TILE_XY(1,0));
		if (eflags & 0x18) DoClearTownHouse(tile + TILE_XY(0,1));
		if (eflags & 0x10) DoClearTownHouse(tile + TILE_XY(1,1));
	}
	return cost;
}

int32 CmdRenameTown(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	StringID str;
	City *c = DEREF_CITY(p1);
		
	str = AllocateName((byte*)_decode_parameters, 4);
	if (str == 0)
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		StringID old_str = c->citynametype;
		c->citynametype = str;
		DeleteName(old_str);

		UpdateCityVirtCoord(c);
		_town_sort_dirty = true;
		UpdateAllStationVirtCoord();
		MarkWholeScreenDirty();
	} else {
		DeleteName(str);
	}
	return 0;
}

// Called from GUI
void DeleteTown(City *c)
{
	Industry *i;
	uint tile;
	byte cb;

	// Delete town authority window
	//  and remove from list of sorted cities
	DeleteWindowById(WC_TOWN_VIEW, c->index);
	_town_sort_dirty = true;

	// Delete all industries belonging to the city
	for(i=_industries; i != endof(_industries); i++) {
		if (i->xy && i->city == c)
			DeleteIndustry(i);
	}

	// Go through all tiles and delete those belonging to the town
	cb = c->index | 0x80;

	tile = 0;
	do {
		if (IS_TILETYPE(tile, MP_HOUSE)) {
			if (ClosestCityFromTile(tile, (uint)-1) == c) {
				DoCommandByTile(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
			}
		} else if (IS_TILETYPE(tile, MP_TUNNELBRIDGE) || IS_TILETYPE(tile, MP_STREET)) {
			if (_map_owner[tile] == cb) {
				DoCommandByTile(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
			}
		}
	} while (++tile != TILES_X * TILES_Y);

	c->xy = 0;
	DeleteName(c->citynametype);

	MarkWholeScreenDirty();
}

// Called from GUI
void ExpandTown(City *c)
{
	int amount, n;

	_generating_world = true;
	
	amount = ((int)Random()&3) + 3;
	c->num_houses += amount;
	UpdateCityRadius(c);

	n = amount * 4;
	do GrowCity(c); while (--n);

	c->num_houses -= amount;
	UpdateCityRadius(c);

	UpdateCityMaxPass(c);
	_generating_world = false;
}

const byte _town_action_costs[7] = {
	2, 4, 9, 35, 48, 53, 117
};

typedef void TownActionProc(City *c, int action);

static void TownActionAdvertise(City *c, int action)
{
	static const byte _advertising_amount[3] = {0x40, 0x70, 0xA0};
	static const byte _advertising_radius[3] = {10,15,20};
	ModifyStationRatingAround(c->xy, _current_player, 
		_advertising_amount[action],
		_advertising_radius[action]);
}

static void TownActionRoadRebuild(City *c, int action)
{
	Player *p;

	c->road_build_months = 6;
	
	SET_DPARAM16(0, c->index);

	p = DEREF_PLAYER(_current_player);
	SET_DPARAM16(1, p->name_1);
	SET_DPARAM32(2, p->name_2);

	AddNewsItem(STR_2055_TRAFFIC_CHAOS_IN_ROAD_REBUILDING, 
		NEWS_FLAGS(NM_NORMAL, NF_TILE, NT_GENERAL, 0), c->xy, 0);
}

static bool DoBuildStatueOfCompany(uint tile)
{
	TileInfo ti;
	byte old;
	int32 r;

	FindLandscapeHeightByTile(&ti, tile);
	if (ti.tileh != 0)
		return false;

	if (ti.type != MP_HOUSE && ti.type != MP_CLEAR && ti.type != MP_TREES)
		return false;


	old = _current_player;
	_current_player = 0x10;
	r = DoCommandByTile(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
	_current_player = old;

	if (r == CMD_ERROR)
		return false;

	ModifyTile(tile, MP_SETTYPE(MP_UNMOVABLE) | MP_MAPOWNER_CURRENT | MP_MAP5,
		2 /* map5 */
	);

	return true;
}

static void TownActionBuildStatue(City *c, int action)
{
	// Layouted as an outward spiral
	static const int16 _statue_tiles[] = {
			-1, 256,   1,   1,-256,-256,  -1,  -1,
			-1, 256, 256, 256,   1,   1,   1,   1,
		-256,-256,-256,-256,  -1,  -1,  -1,  -1,
			-1, 256, 256, 256, 256, 256,   1,   1,
			 1,   1,   1,   1,-256,-256,-256,-256,
		-256,-256,  -1,  -1,  -1,  -1,  -1,  -1,
			-1, 256, 256, 256, 256, 256, 256, 256,
			 1,   1,   1,   1,   1,   1,   1,   1,
		-256,-256,-256,-256,-256,-256,-256,-256,
			-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
			 0,
	};
	int offs;
	uint tile = c->xy;
	const int16 *p = _statue_tiles;

	SETBIT(c->statues, _current_player);

	do {
		if (DoBuildStatueOfCompany(tile))
			return;
		offs = *p++;
		tile = TILE_ADD(tile, offs);
	} while (offs);
}

static void TownActionFundBuildings(City *c, int action)
{
	c->grow_counter = 1;
	c->flags12 |= 1;
	c->fund_buildings_months = 3;
}

static void TownActionBuyRights(City *c, int action)
{
	Station *st;

	FOR_ALL_STATIONS(st) {
		if (st->xy && st->city == c && st->owner < 8 && 
				st->owner != _current_player)
			st->blocked_months = 12;
	}

	ModifyStationRatingAround(c->xy, _current_player, 130, 17);
}

static TownActionProc * const _town_action_proc[] = {
	TownActionAdvertise,
	TownActionAdvertise,
	TownActionAdvertise,
	TownActionRoadRebuild,
	TownActionBuildStatue,
	TownActionFundBuildings,
	TownActionBuyRights,
};

// p1 = town
// p2 = action
int32 CmdDoTownAction(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	int32 cost;

	SET_EXPENSES_TYPE(EXPENSES_OTHER);

	cost = (_price.build_industry >> 8) * _town_action_costs[p2];

	if (flags & DC_EXEC) {
		_town_action_proc[p2](DEREF_CITY(p1), p2);
	}

	return cost;
}

static void UpdateCityGrowRate(City *c)
{
	int n;
	Station *st;
	byte m;
	Player *p;

	// Reset player ratings if they're low
	FOR_ALL_PLAYERS(p) {
		if (p->name_1 != 0 && c->ratings[p->index] <= 200) {
			c->ratings[p->index] += 5;
		}
	}

	n = 0;
	FOR_ALL_STATIONS(st) {
		if (GetTileDistAdv(st->xy, c->xy) <= c->radius[0]) {
			if (st->time_since_load <= 20 || st->time_since_unload <= 20) {
				n++;
				if (st->owner < 8 && c->ratings[st->owner] <= 1000-12)
					c->ratings[st->owner] += 12;
			} else {
				if (st->owner < 8 && c->ratings[st->owner] >= -1000+15)
					c->ratings[st->owner] -= 15;
			}
		}
	}

	c->flags12 &= ~1;

	if (c->fund_buildings_months != 0) {
		c->fund_buildings_months--;
		m = 60;
	} else if (n == 0) {
		m = 160;
		if (!CHANCE16(1, 12))
			return;
	} else {
		static const byte _grow_count_values[5] = {
			210, 150, 110, 80, 50
		};
		m = _grow_count_values[min(n, 5) - 1];
	}

	if (_opt.landscape == LT_HILLY) {
		if ((_map_type_and_height[c->xy] & 0xF) * 8 >= _opt.snow_line &&	c->act_food == 0)
			return;
	} else if (_opt.landscape == LT_DESERT) {
		if (GetMapExtraBits(c->xy) == 1 && (c->act_food==0 || c->act_paper==0))
			return;
	}

	c->growth_rate = m;
	if (m <= c->grow_counter)
		c->grow_counter = m;

	c->flags12 |= 1;
}

static void UpdateCityAmounts(City *c)
{
	// Using +1 here to prevent overflow and division by zero
	c->pct_pass_transported = c->new_act_pass * 256 / (c->new_max_pass + 1);

	c->max_pass = c->new_max_pass; c->new_max_pass = 0;
	c->act_pass = c->new_act_pass; c->new_act_pass = 0;
	c->act_food = c->new_act_food; c->new_act_food = 0;
	c->act_paper = c->new_act_paper; c->new_act_paper = 0;

	// Using +1 here to prevent overflow and division by zero
	c->pct_mail_transported = c->new_act_mail * 256 / (c->new_max_mail + 1);
	c->max_mail = c->new_max_mail; c->new_max_mail = 0;
	c->act_mail = c->new_act_mail; c->new_act_mail = 0;

	InvalidateWindow(WC_TOWN_VIEW, c->index);
}

void ResetTownRatingsForPlayer(int player)
{
	City *c;
	FOR_ALL_CITIES(c) {
		c->ratings[player] = 500;
	}
}

bool CheckIfAuthorityRefuses(uint tile)
{
	City *c;

	if (_current_player >= 8)
		return true;

	c = ClosestCityFromTile(tile, 20);
	if (c == NULL)
		return true;

	if (c->ratings[_current_player] > -200)
		return true;

	_error_message = STR_2009_LOCAL_AUTHORITY_REFUSES;
	SET_DPARAM16(0, c->index);

	return false;
}


City *ClosestCityFromTile(uint tile, uint threshold)
{
	City *c;
	uint dist, best = threshold;
	City *best_city = NULL;
	
	FOR_ALL_CITIES(c) {
		if (c->xy != 0) {
			dist = GetTileDist(tile, c->xy);
			if (dist < best) {
				best = dist;
				best_city = c;
			}
		}
	}

	return best_city;
}

void IncCityRating(City *city, int add, int max)
{
	city->have_ratings |= 1 << _current_player;

	add += city->ratings[_current_player];
	if (add <= max)
		city->ratings[_current_player] = add;
}

void CitiesMonthlyLoop()
{
	City *c;

	FOR_ALL_CITIES(c) if (c->xy != 0) {
		if (c->road_build_months != 0)
			c->road_build_months--;

		UpdateCityGrowRate(c);
		UpdateCityAmounts(c);
	}
}

void InitializeTowns()
{
	Subsidy *s;
	int i;

	memset(_cities, 0, sizeof(_cities));
	for(i=0; i!=lengthof(_cities); i++)
		_cities[i].index = i;

	memset(_subsidies, 0, sizeof(_subsidies));
	for (s=_subsidies; s != endof(_subsidies); s++)
		s->cargo_type = 0xFF;

	_cur_town_ctr = 0;
	_town_sort_dirty = true;
}

const TileTypeProcs _tile_type_town_procs = {
	DrawTile_Town,						/* draw_tile_proc */
	GetSlopeZ_Town,						/* get_slope_z_proc */
	ClearTile_Town,						/* clear_tile_proc */
	GetAcceptedCargo_Town,		/* get_accepted_cargo_proc */
	GetTileDesc_Town,					/* get_tile_desc_proc */
	GetTileTrackStatus_Town,	/* get_tile_track_status_proc */
	ClickTile_Town,						/* click_tile_proc */
	AnimateTile_Town,					/* animate_tile_proc */
	TileLoop_Town,						/* tile_loop_clear */
	ChangeTileOwner_Town,			/* change_tile_owner_clear */
	NULL,											/* get_produced_cargo_proc */
	NULL,											/* vehicle_enter_tile_proc */
	NULL,											/* vehicle_leave_tile_proc */
};


// Save and load of cities.
static const byte _city_desc[] = {
	SLE_VAR(City,xy,					SLE_UINT16),
	SLE_VAR(City,population,	SLE_UINT16),
	SLE_VAR(City,num_houses,	SLE_UINT16),
	SLE_VAR(City,citynametype,SLE_UINT16),
	SLE_VAR(City,citynameparts,SLE_UINT32),

	SLE_VAR(City,flags12,			SLE_UINT8),
	SLE_VAR(City,statues,			SLE_UINT8),
	SLE_VAR(City,sort_index_obsolete,	SLE_UINT8),

	SLE_VAR(City,have_ratings,SLE_UINT8),
	SLE_ARR(City,ratings,			SLE_INT16, 8),

	SLE_VAR(City,max_pass,		SLE_UINT16),
	SLE_VAR(City,max_mail,		SLE_UINT16),
	SLE_VAR(City,new_max_pass,SLE_UINT16),
	SLE_VAR(City,new_max_mail,SLE_UINT16),
	SLE_VAR(City,act_pass,		SLE_UINT16),
	SLE_VAR(City,act_mail,		SLE_UINT16),
	SLE_VAR(City,new_act_pass,SLE_UINT16),
	SLE_VAR(City,new_act_mail,SLE_UINT16),

	SLE_VAR(City,pct_pass_transported,SLE_UINT8),
	SLE_VAR(City,pct_mail_transported,SLE_UINT8),

	SLE_VAR(City,act_food,		SLE_UINT16),
	SLE_VAR(City,act_paper,		SLE_UINT16),
	SLE_VAR(City,new_act_food,SLE_UINT16),
	SLE_VAR(City,new_act_paper,SLE_UINT16),

	SLE_VAR(City,time_until_rebuild,		SLE_UINT8),
	SLE_VAR(City,grow_counter,					SLE_UINT8),
	SLE_VAR(City,growth_rate,						SLE_UINT8),
	SLE_VAR(City,fund_buildings_months,	SLE_UINT8),
	SLE_VAR(City,road_build_months,			SLE_UINT8),

	SLE_END()
};

static void Save_CITY()
{
	City *c;

	FOR_ALL_CITIES(c) if (c->xy != 0) {
		SlSetArrayIndex(c->index);
		SlObject(c, _city_desc);
	}
}

static void Load_CITY()
{
	int index;
	while ((index = SlIterateArray()) != -1) {
		City *c = DEREF_CITY(index);
		SlObject(c, _city_desc);
	}
}

void AfterLoadCity()
{
	City *c;
	FOR_ALL_CITIES(c) {
		if (c->xy != 0) {
			UpdateCityRadius(c);
			UpdateCityVirtCoord(c);
		}
	}
	_town_sort_dirty = true;
}


const ChunkHandler _city_chunk_handlers[] = {
	{ 'CITY', Save_CITY, Load_CITY, CH_ARRAY | CH_LAST},
};


