#include "stdafx.h"
#include "ttd.h"
#include "vehicle.h"
#include "viewport.h"
#include "command.h"
#include "player.h"
#include "city.h"
#include "gfx.h"

static bool _road_special_gettrackstatus;

void RoadVehEnterDepot(Vehicle *v);


static bool HasTileRoadAt(uint tile, int i)
{
	int mask;
	byte b;

	switch(GET_TILETYPE(tile)) {
	case MP_STREET:
		b = _map5[tile];
		
		if ((b & 0xF0) == 0) {
		} else if ((b & 0xF0) == 0x10) {
			b = (b&8)?5:10;
		} else if ((b & 0xF0) == 0x20) {
			return (~b & 3) == i;
		} else
			return false;
		break;
						
	case MP_STATION:
		b = _map5[tile];
		if (!IS_BYTE_INSIDE(b, 0x43, 0x43+8))
			return false;
		return ((~(b - 0x43) & 3) == i);

	case MP_TUNNELBRIDGE:
		mask = GetRoadBitsByTile(tile);
		b = 10; if (mask & 1) break;
		b = 5;  if (mask & 2) break;
		return false;

	default:
		return false;
	}

	return HASBIT(b, i);
}

static bool CheckAllowRemoveRoad(uint tile, uint br)
{
	int blocks;
	byte owner;
	uint n;

	if (_game_mode == GM_EDITOR)
		return true;

	blocks = GetRoadBitsByTile(tile);
	if (blocks == 0)
		return true;

	// Only do the special processing when the player
	// is not a city
	if (_current_player != 0x10 && _current_player&0xF0)
		return true;

	// A railway crossing has the road owner in the map3_lo byte.
	if (IS_TILETYPE(tile, MP_STREET) && (_map5[tile] & 0xF0) == 0x10) {
		owner = _map3_lo[tile];
	} else {
		owner = _map_owner[tile];
	}
	
	// Only do the special processing if the road is owned
	// by a city
	if (!(owner & 0x80)) {
		return owner >= 8 || CheckOwnership(owner);
	}

	// Get a bitmask of which neighbouring roads has a tile
	n = 0;
	if (blocks&0x25 && HasTileRoadAt(TILE_ADDXY(tile,-1, 0), 1)) n |= 8;
	if (blocks&0x2A && HasTileRoadAt(TILE_ADDXY(tile, 0, 1), 0)) n |= 4;
	if (blocks&0x19 && HasTileRoadAt(TILE_ADDXY(tile, 1, 0), 3)) n |= 2;
	if (blocks&0x16 && HasTileRoadAt(TILE_ADDXY(tile, 0,-1), 2)) n |= 1;
	
	// If 0 or 1 bits are set in n, or if no bits that match the bits to remove,
	// then allow it
	if ((n & (n-1)) != 0 && (n & br) != 0) {
		SET_DPARAM16(0, owner - 0x80);
		_error_message = STR_2009_LOCAL_AUTHORITY_REFUSES;
		return false;
	}

	return true;
}

bool IsRoadDepotTile(TileIndex tile)
{
	return IS_TILETYPE(tile, MP_STREET) &&
					(_map5[tile] & 0xF0) == 0x20;
}

uint GetRoadBitsByTile(TileIndex tile)
{
	uint32 r = GetTileTrackStatus(tile, 2);
	return (byte)(r | (r >> 8));
}


/* Delete a piece of road
 * p1 = piece type
 */

int32 CmdRemoveRoad(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	TileInfo ti;
	int32 cost;
	uint tile;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	FindLandscapeHeight(&ti, x, y);
	tile = ti.tile;

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	{
		bool b;
		_road_special_gettrackstatus = true;
		b = CheckAllowRemoveRoad(tile, p1);
		_road_special_gettrackstatus = false;
		if (!b)
			return CMD_ERROR;
	}

	if (ti.type == MP_TUNNELBRIDGE) {
		if ((ti.map5 & 0xE9) == 0xE8) {
			if (p1 & 10) goto return_error;
		} else if ((ti.map5 & 0xE9) == 0xE9) {
			if (p1 & 5) goto return_error;
		} else
			goto return_error;

		cost = _price.remove_road * 2;

		if (flags & DC_EXEC) {
			_map5[tile] = ti.map5 & 0xC7;
			_map_owner[tile] = 0x10;
			MarkTileDirtyByTile(tile);
		}
		return cost;
	} else if (ti.type == MP_STREET) {
		if (!(ti.map5 & 0xF0)) {
			if (ti.tileh != 0  && (ti.map5 == 5 || ti.map5 == 10)) {
				p1 |= (p1 & 0xC) >> 2;
				p1 |= (p1 & 0x3) << 2;
			}

			{
				uint t;

				if ((t=(p1 & ti.map5)) != p1)
					goto return_error;

				cost = 0;
				do {
					if (t&1) cost += _price.remove_road;			
				} while(t>>=1);
			}

			if (flags & DC_EXEC) {
				_map5[tile] ^= p1;
				if ((_map5[tile]&0xF) == 0)
					DoClearSquare(tile);
				else
					MarkTileDirtyByTile(tile);
			}
			return cost;			
		} else if (!(ti.map5 & 0xE0)) {
			byte t;

			if (!(ti.map5 & 8)) {
				t = 2;	
				if (p1 & 5) goto return_error;
			} else {
				t = 1;
				if (p1 & 10) goto return_error;
			}

			cost = _price.remove_road * 2;
			if (flags & DC_EXEC) {
				ModifyTile(tile, 
					MP_SETTYPE(MP_RAILWAY) |
					MP_MAP2_CLEAR | MP_MAP3LO | MP_MAP3HI_CLEAR | MP_MAP5,
					_map3_hi[tile] & 0xF, /* map3_lo */
					t											/* map5 */
				);
			}
			return cost;
		} else
			goto return_error;

	} else {
return_error:;
		return_cmd_error(INVALID_STRING_ID);
	}
}


enum {
	ROAD_NW = 1, // NW road track
	ROAD_SW = 2, // SW road track
	ROAD_SE = 4, // SE road track
	ROAD_NE = 8, // NE road track
	ROAD_ALL = (ROAD_NW | ROAD_SW | ROAD_SE | ROAD_NE)
};
 
static const byte _valid_tileh_slopes_road[2][15] = {
	// set of normal ones
	{
		ROAD_ALL, 0, 0,
		ROAD_SW | ROAD_NE, 0, 0,  // 3, 4, 5
		ROAD_NW | ROAD_SE, 0, 0,
		ROAD_NW | ROAD_SE, 0, 0,  // 9, 10, 11
		ROAD_SW | ROAD_NE, 0, 0
	},  
	// allowed road for an evenly raised platform
	{ 
		0,
		ROAD_SW | ROAD_NW,
		ROAD_SW | ROAD_SE,
		ROAD_NW | ROAD_SE | ROAD_SW,

		ROAD_SE | ROAD_NE, // 4
		ROAD_ALL,
		ROAD_SW | ROAD_NE | ROAD_SE,
		ROAD_ALL,

		ROAD_NW | ROAD_NE, // 8
		ROAD_SW | ROAD_NE | ROAD_NW,
		ROAD_ALL,
		ROAD_ALL,

		ROAD_NW | ROAD_SE | ROAD_NE, // 12
		ROAD_ALL,
		ROAD_ALL
	}
};


static uint32 CheckRoadSlope(int tileh, byte *pieces, byte existing)	
{
	if (!(tileh & 0x10)) {
		byte road_bits = *pieces | existing;

		// no special foundation
		if ((~_valid_tileh_slopes_road[0][tileh] & road_bits) == 0) {
			// force that all bits are set when we have slopes
			if (tileh != 0) *pieces |= _valid_tileh_slopes_road[0][tileh];
			return 0; // no extra cost
		}
		
		// foundation is used. Whole tile is leveled up
		if ((~_valid_tileh_slopes_road[1][tileh] & road_bits) == 0) {
			return existing ? 0 : _price.terraform;	
		}

		// partly leveled up tile, only if there's no road on that tile
		if ( !existing && (tileh == 1 || tileh == 2 || tileh == 4 || tileh == 8) ) {
			// force full pieces.
			*pieces |= (*pieces & 0xC) >> 2;
			*pieces |= (*pieces & 0x3) << 2;
			return existing ? 0 : _price.terraform;	
		}
	}
	return CMD_ERROR;
}

/* Build a piece of road
 * p1 = piece flags
 */

int32 CmdBuildRoad(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	TileInfo ti;
	int32 cost;
	byte pieces = (byte)p1, existing = 0;
	uint tile;
	
	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	FindLandscapeHeight(&ti, x, y);
	tile = ti.tile;

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	if (ti.type == MP_STREET) {
		if (!(ti.map5 & 0xF0)) {
			if ( ((pieces) & (byte)(ti.map5)) == (pieces))
				return_cmd_error(STR_1007_ALREADY_BUILT);
			existing = ti.map5;
		} else {
			if (!(ti.map5 & 0xE0) && pieces != ((ti.map5 & 8) ? 5 : 10))
				return_cmd_error(STR_1007_ALREADY_BUILT);
			goto do_clear;
		}
	} else if (ti.type == MP_RAILWAY) {
		byte m5;

		if (ti.tileh != 0) goto do_clear;
		
		if (ti.map5 == 2) {
			if (pieces & 5) goto do_clear;
			m5 = 0x10;
		} else if (ti.map5 == 1) {
			if (pieces & 10) goto do_clear;
			m5 = 0x18;
		} else
			goto do_clear;

		if (flags & DC_EXEC) {
			ModifyTile(tile,
				MP_SETTYPE(MP_STREET) |
				MP_MAP2_CLEAR | MP_MAP3LO | MP_MAP3HI | MP_MAP5,
				_current_player, /* map3_lo */
				_map3_lo[tile] & 0xF, /* map3_hi */
				m5 /* map5 */
			);
		}
		return _price.build_road * 2;
	} else if (ti.type == MP_TUNNELBRIDGE) {
		byte t;
		
		t = ti.map5 & 0xF9;

		if (t == 0xE8) {
			if (!(pieces & 10))
				return_cmd_error(STR_1007_ALREADY_BUILT);
		} else if (t == 0xE9) {
			if (!(pieces & 5))
				return_cmd_error(STR_1007_ALREADY_BUILT);
		} else if (t == 0xC0) {
			if (pieces & 10) goto do_clear;
		} else if (t == 0xC1) {
			if (pieces & 5) goto do_clear;
		} else
			goto do_clear;

		cost = _price.build_road * 2;
		if (flags & DC_EXEC) {
			ModifyTile(tile,
				MP_MAPOWNER_CURRENT | MP_MAP5,
				(ti.map5 & 0xC7) | 0x28 /*map5 */
			);
		}
		return cost;
	} else {
do_clear:;
		if (DoCommandByTile(tile, 0, 0, flags & ~DC_EXEC, CMD_LANDSCAPE_CLEAR) == CMD_ERROR)
			return CMD_ERROR;
	}

	cost = CheckRoadSlope(ti.tileh, &pieces, existing);
	if (cost == CMD_ERROR) return_cmd_error(STR_1800_LAND_SLOPED_IN_WRONG_DIRECTION);

	// the AI is not allowed to used foundationed tiles.
	if (cost && (_is_ai_player || !_patches.build_on_slopes))
		return CMD_ERROR;

	if (!(ti.type == MP_STREET && (ti.map5 & 0xF0) == 0)) {
		cost += DoCommandByTile(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	} else {
		// Don't put the pieces that already exist
		pieces &= ~ti.map5;
	}

	{
		byte t = pieces;
		while (t) {
			if (t & 1) cost += _price.build_road;
			t >>= 1;
		}
	}

	if (flags & DC_EXEC) {
		if (ti.type != MP_STREET) {
			_map_type_and_height[tile] &= 0xF;
			_map_type_and_height[tile] |= MP_STREET << 4;
			_map5[tile] = 0;
			_map_owner[tile] = _current_player;
		}

		_map5[tile] |= (byte)pieces;

		MarkTileDirtyByTile(tile);
	}
	return cost;
}

// Build a long piece of road.
// x,y = end tile
// p1 = start tile
// p2&1 = start tile starts in the 2nd half
// p2&2 = end tile starts in the 2nd half
// p2&4 = direction (0 = along x, 1=along y)
int32 CmdBuildLongRoad(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint start_tile, end_tile, tile;
	int mode;
	int32 cost,ret;

	start_tile = p1;
	end_tile = TILE_FROM_XY(x, y);

	if (start_tile > end_tile || (start_tile == end_tile && (p2&1))) {
		uint t = start_tile; start_tile = end_tile; end_tile = t;
		p2 ^= IS_INT_INSIDE(p2&3, 1, 3) ? 3 : 0;
	}

	cost = 0;
	tile = start_tile;
	// Start tile is the small number.
	for(;;) {
		mode = (p2&4) ? 5 : 10;

		if (tile == start_tile && (p2&1))
			mode &= (4+2);
		else if (tile == end_tile && !(p2&2))
			mode &= (1+8);

		ret = DoCommandByTile(tile, mode, 0, flags, CMD_BUILD_ROAD);
		if (ret == CMD_ERROR) {
			if (_error_message != STR_1007_ALREADY_BUILT)
				return CMD_ERROR;
		} else {
			cost += ret;
		}

		if (tile == end_tile)
			break;

		tile += (p2&4)?TILE_XY(0,1):TILE_XY(1,0);
	}

	// already built?
	if (cost == 0)
		return CMD_ERROR;

	return cost;
}

// Remove a long piece of road.
// x,y = end tile
// p1 = start tile
// p2&1 = start tile starts in the 2nd half
// p2&2 = end tile starts in the 2nd half
// p2&4 = direction (0 = along x, 1=along y)
int32 CmdRemoveLongRoad(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint start_tile, end_tile, tile;
	int32 cost,ret;

	start_tile = p1;
	end_tile = TILE_FROM_XY(x, y);

	if (start_tile > end_tile || (start_tile == end_tile && (p2&1))) {
		uint t = start_tile; start_tile = end_tile; end_tile = t;
		p2 ^= IS_INT_INSIDE(p2&3, 1, 3) ? 3 : 0;
	}

	cost = 0;
	tile = start_tile;
	// Start tile is the small number.
	for(;;) {
		// try to remove the first half
		if (tile != end_tile || (p2&2)) {
			ret = DoCommandByTile(tile, (p2 & 4) ? ROAD_SE : ROAD_SW, 0, flags, CMD_REMOVE_ROAD);
			if (ret != CMD_ERROR)
				cost += ret;
		}

		// try to remove the second half
		if (tile != start_tile || !(p2&1)) {
			ret = DoCommandByTile(tile, (p2 & 4) ? ROAD_NW : ROAD_NE, 0, flags, CMD_REMOVE_ROAD);
			if (ret != CMD_ERROR)
				cost += ret;
		}

		if (tile == end_tile)
			break;

		tile += (p2&4)?TILE_XY(0,1):TILE_XY(1,0);
	}

	// already built?
	if (cost == 0)
		return CMD_ERROR;

	return cost;
}

/* Build a road depot 
 * p1 - direction (0-3)
 * p2 - unused
 */

int32 CmdBuildRoadDepot(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	TileInfo ti;
	int32 cost;
	Depot *dep;
	uint tile;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	FindLandscapeHeight(&ti, x, y);

	tile = ti.tile;

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	if (ti.tileh != 0) {
		if (_is_ai_player || !_patches.build_on_slopes || (ti.tileh & 0x10 || !((0x4C >> p1) & ti.tileh) ))
			return_cmd_error(STR_0007_FLAT_LAND_REQUIRED);
	}

	cost = DoCommandByTile(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (cost == CMD_ERROR)
		return CMD_ERROR;

	dep = AllocateDepot();
	if (dep == NULL)
		return CMD_ERROR;

	if (flags & DC_EXEC) {	
		if (_current_player == _local_player)
			_last_built_road_depot_tile = (TileIndex)tile;

		dep->xy = tile;
		dep->city_index = ClosestCityFromTile(tile, (uint)-1)->index;

		ModifyTile(tile,
			MP_SETTYPE(MP_STREET) |
			MP_MAPOWNER_CURRENT | MP_MAP5,
			(p1 | 0x20) /* map5 */
		);

	}
	return cost + _price.build_road_depot;
}

static int32 RemoveRoadDepot(uint tile, uint32 flags)
{
	if (!CheckOwnership(_map_owner[tile]))
		return CMD_ERROR;

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		DoDeleteDepot(tile);
	}

	return _price.remove_road_depot;
}

#define M(x) (1<<(x))

static int32 ClearTile_Road(uint tile, byte flags) {
	int32 ret;
	byte m5 = _map5[tile];

	if ( (m5 & 0xF0) == 0) {
		byte b = m5 & 0xF;

		if (! ((1 << b) & (M(1)|M(2)|M(4)|M(8))) ) {
			if ( (!(flags & DC_AI_BUILDING) ||	!(_map_owner[tile]&0x80)) && flags&DC_AUTO)
				return_cmd_error(STR_1801_MUST_REMOVE_ROAD_FIRST);
		}
		return DoCommandByTile(tile, b, 0, flags, CMD_REMOVE_ROAD);
	} else if ( (m5 & 0xE0) == 0) {
		if (flags & DC_AUTO)
			return_cmd_error(STR_1801_MUST_REMOVE_ROAD_FIRST);

		ret = DoCommandByTile(tile, (m5&8)?5:10, 0, flags, CMD_REMOVE_ROAD);
		if (ret == CMD_ERROR)
			return CMD_ERROR;
		
		if (flags & DC_EXEC) {
			DoCommandByTile(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		}	

		return ret;
	} else {
		if (flags & DC_AUTO)
			return_cmd_error(STR_2004_BUILDING_MUST_BE_DEMOLISHED);
		return RemoveRoadDepot(tile,flags);
	}
}


typedef struct DrawRoadTileStruct {
	uint16 image;
	byte subcoord_x;
	byte subcoord_y;
} DrawRoadTileStruct;

typedef struct DrawRoadSeqStruct {
	uint32 image;
	byte subcoord_x;
	byte subcoord_y;
	byte width;
	byte height;
} DrawRoadSeqStruct;

#include "table/road_land.h"


static uint GetRoadFoundation(uint tileh, uint bits) {
	int i;
	// normal level sloped building
	if ((~_valid_tileh_slopes_road[1][tileh] & bits) == 0)
		return tileh;

	// inclined sloped building
	if ( ((i=0, tileh == 1) || (i+=2, tileh == 2) || (i+=2, tileh == 4) || (i+=2, tileh == 8)) && 
		((bits == (ROAD_SW | ROAD_NE)) || (i++, bits == (ROAD_NW | ROAD_SE))))
		return i + 15;

	return 0;
}

static const byte _road_sloped_sprites[14] = {
	0,  0,  2,  0,
	0,  1,  0,  0,
	3,  0,  0,  0,
	0,  0
};

static void DrawTile_Road(TileInfo *ti)
{
	uint32 image;	
	byte m2;
	const byte *s;

	if ( (ti->map5 & 0xF0) == 0) { // if it is a road the upper 4 bits are 0
		const DrawRoadTileStruct *drts;

		if (ti->tileh != 0) {
			int f = GetRoadFoundation(ti->tileh, ti->map5 & 0xF);
			if (f) DrawFoundation(ti, f);
			
			// default sloped sprites..
			if (ti->tileh != 0) {
				image = _road_sloped_sprites[ti->tileh - 1] + 0x53F;
			} else  {
				image = _road_tile_sprites_1[ti->map5 & 0xF];
			}
		} else {
			image = _road_tile_sprites_1[ti->map5 & 0xF];
		}

		m2 = _map2[ti->tile] & 7;

		if (m2 == 0) image |= 0x3178000;
		
		if (_map3_hi[ti->tile] & 0x80) {
			image += 19;
		} else if (m2 > 1 && m2 != 6) {
			image -= 19; /* pavement along the road? */
		}

		DrawGroundSprite(image);
		OffsetGroundSpriteEnd();
		// XXX: bug, if roadwork, offsetting will not work right

		if (!(_display_opt & DO_FULL_DETAIL) || _cur_dpi->zoom == 2)
			return;

		if (m2 >= 6) {
			// roadwork
			DrawGroundSprite(0x586 + ((ti->map5&8)!=0 ? 0 : 1));
			return;
		}

		drts = (const DrawRoadTileStruct*)_road_display_table[m2][ti->map5 & 0xF];
		
		while ((image = drts->image) != 0) {
			int x = ti->x | drts->subcoord_x;
			int y = ti->y | drts->subcoord_y;
			byte z = ti->z;
			if (ti->tileh != 0)	z = (byte)GetSlopeZ(x, y);
			AddSortableSpriteToDraw(image, x, y, 2, 2, 0x10, z);
			drts++;
		}
	} else if ( (ti->map5 & 0xE0) == 0) {
		image = 0x55B;

		if ( (ti->map5 & 8) != 0)
			image--;

		if ( (ti->map5 & 4) != 0)
			image += 2;

		if ( _map3_hi[ti->tile] & 0x80) {
			image += 8;
		} else {
			m2 = _map2[ti->tile] & 7;
			if (m2 == 0) image |= 0x3178000;
			if (m2 > 1) image += 4;
		}

		DrawGroundSprite(image + (_map3_hi[ti->tile] & 0xF) * 12);
	} else {
		uint32 ormod;
		int player;
		const DrawRoadSeqStruct *drss;
		
		if (ti->tileh != 0) { DrawFoundation(ti, ti->tileh); }

		ormod = 0x315;
		player = _map_owner[ti->tile];
		if (player < 8)
			ormod = PLAYER_SPRITE_COLOR(player);

		s = _road_display_datas[ti->map5 & 0xF];
		
		DrawGroundSprite(READ_LE_UINT32(s));
		s += sizeof(uint32);
		drss = (DrawRoadSeqStruct*)s;

		while ((image=drss->image) != 0) {
			if (image & 0x8000)
				image |= ormod;

			AddSortableSpriteToDraw(image, ti->x | drss->subcoord_x, 
				ti->y | drss->subcoord_y, drss->width, drss->height, 0x14, ti->z);
			drss++;
		}

		OffsetGroundSpriteEnd();
	}
}

void DrawRoadDepotSprite(int x, int y, int image)
{
	uint32 ormod;
	const DrawRoadSeqStruct *dtss;
	const byte *t;

	ormod = PLAYER_SPRITE_COLOR(_local_player);

	t = _road_display_datas[image];

	x+=33;
	y+=17;

	DrawSprite(READ_LE_UINT32(t), x, y);
	t += sizeof(uint32);

	for(dtss = (DrawRoadSeqStruct *)t; dtss->image != 0; dtss++) {
		Point pt = RemapCoords(dtss->subcoord_x, dtss->subcoord_y, 0);
		
		image = dtss->image;
		if (image & 0x8000)
			image |= ormod;

		DrawSprite(image, x + pt.x, y + pt.y);
	}
}

static const byte _road_inclined_tileh[] = {
	3,9,3,6,12,6,12,9,
};

uint16 GetSlopeZ_Road(TileInfo *ti)
{
	uint z = ti->z;
	int th = ti->tileh;

	// check if it's a foundation
	if (ti->tileh != 0) {
		if ((ti->map5 & 0xF0) == 0) {
			uint f = GetRoadFoundation(ti->tileh, ti->map5 & 0x3F);
			if (f != 0) {
				if (f < 15) {
					// leveled foundation
					return z + 8;
				}
				// inclined foundation
				th = _road_inclined_tileh[f - 15];
			}
		} else if ((ti->map5 & 0xF0) == 0x20) {
			// depot
			return z + 8;
		}
		return GetPartialZ(ti->x&0xF, ti->y&0xF, th) + z;
	}
	return z;
}

static void GetAcceptedCargo_Road(uint tile, AcceptedCargo *ac)
{
	/* not used */
}

static void AnimateTile_Road(uint tile)
{
	if ((_map5[tile] & 0xF0) == 0x10) {
		MarkTileDirtyByTile(tile);
	}
}

static const byte _city_road_types[5][2] = {
	{1,1},
	{2,2},
	{2,2},
	{5,5},
	{3,2},
};

static const byte _city_road_types_2[5][2] = {
	{1,1},
	{2,2},
	{3,2},
	{3,2},
	{3,2},
};


static void TileLoop_Road(uint tile)
{
	City *c;
	int grp;
	
	if (_opt.landscape == LT_HILLY) {
		// Fix snow style if the road is above the snowline
		if ((_map3_hi[tile] & 0x80) != ((GetTileZ(tile) > _opt.snow_line) ? 0x80 : 0x00)) {
			_map3_hi[tile] ^= 0x80;
			MarkTileDirtyByTile(tile);
		}
	} else if (_opt.landscape == LT_DESERT) {
		// Fix desert style
		if (GetMapExtraBits(tile) == 1 && !(_map3_hi[tile] & 0x80)) {
			_map3_hi[tile] |= 0x80;
			MarkTileDirtyByTile(tile);
		}
	}

	if (_map5[tile] & 0xE0)
		return;

	if ((_map2[tile] & 7) < 6) {
		c = ClosestCityFromTile(tile, (uint)-1);
		grp = 0;
		if (c != NULL) {
			// If in the scenario editor, set the owner to the city that is closest.
			if (_game_mode == GM_EDITOR) {
				_map_owner[tile] = c->index + 0x80;
			}

			grp = GetCityRadiusGroup(c, tile);
			
			// Show an animation to indicate road work
			if (c->road_build_months != 0 && 
					!(GetTileDist(c->xy, tile) >= 8 && grp==0) &&
					(_map5[tile]==5 || _map5[tile]==10)) {
				if (GetTileSlope(tile, NULL) == 0 && EnsureNoVehicle(tile) && CHANCE16(1,20)) {
					_map2[tile] = ((_map2[tile]&7) <= 1) ? 6 : 7;
					
					SndPlayTileFx(0x1F,tile);
					CreateEffectVehicleAbove(
						GET_TILE_X(tile) * 16 + 7,
						GET_TILE_Y(tile) * 16 + 7,
						0,
						EV_ROADWORK);
					MarkTileDirtyByTile(tile);
					return;
				}
			}
		}

		{
			const byte *p = (_opt.landscape == LT_CANDY) ? _city_road_types_2[grp] : _city_road_types[grp];
			byte b = _map2[tile] & 7;

			if (b == p[0])
				return;
			
			if (b == p[1]) {
				b = p[0];
			} else if (b == 0) {
				b = p[1];
			} else {
				b = 0;
			}
			_map2[tile] = (_map2[tile] & ~7) | b;
			MarkTileDirtyByTile(tile);
		}
	} else {
		// Handle road work
		
		byte b = _map2[tile];
		if (b < 0x80) {
			_map2[tile] = b + 8;
			return;
		}
		_map2[tile] = ((b&7) == 6) ? 1 : 2;
		MarkTileDirtyByTile(tile);
	}
}

void ShowRoadDepotWindow(uint tile);

static void ClickTile_Road(uint tile)
{
	if ((_map5[tile] & 0xF0) == 0x20) {
		ShowRoadDepotWindow(tile);
	}
}

static const byte _road_trackbits[16] = {
	0x0, 0x0, 0x0, 0x10, 0x0, 0x2, 0x8, 0x1A, 0x0, 0x4, 0x1, 0x15, 0x20, 0x26, 0x29, 0x3F,
};

static uint32 GetTileTrackStatus_Road(uint tile, int mode)	{
	if (mode == 0) {
		if ((_map5[tile] & 0xF0) != 0x10)
			return 0;
		return _map5[tile] & 8 ? 0x101 : 0x202;
	} else if  (mode == 2) {
		byte b = _map5[tile];
		if ((b & 0xF0) == 0) {
			if (!_road_special_gettrackstatus && (_map2[tile]&7) >= 6)
				return 0;
			return _road_trackbits[b&0xF] * 0x101;
		} else if ((b&0xE0) == 0) {
			uint32 r = 0x101;
			if (b&8) r <<= 1;
				
			if (b&4) {
				r *= 0x10001;
			}			
			return r;
		}
	}
	return 0;
}

static const StringID _road_tile_strings[] = {
	STR_1818_ROAD_RAIL_LEVEL_CROSSING,
	STR_1817_ROAD_VEHICLE_DEPOT,

	STR_1814_ROAD,
	STR_1814_ROAD,
	STR_1814_ROAD,
	STR_1815_ROAD_WITH_STREETLIGHTS,
	STR_1814_ROAD,
	STR_1816_TREE_LINED_ROAD,
	STR_1814_ROAD,
	STR_1814_ROAD,
};

static void GetTileDesc_Road(uint tile, TileDesc *td)
{
	int i = (_map5[tile] >> 4);
	if (i == 0)
		i = (_map2[tile] & 7) + 3;
	td->str = _road_tile_strings[i - 1];
	td->owner = _map_owner[tile];
}

static const byte _roadveh_enter_depot_unk0[4] = {
	8, 9, 0, 1
};

static uint32 VehicleEnter_Road(Vehicle *v, uint tile, int x, int y)
{
	if ((_map5[tile] & 0xF0) == 0x10)	{
		if (v->type == VEH_Train && (_map5[tile] & 4) == 0) {
			/* train crossing a road */
			SndPlayVehicleFx(12, v);
			_map5[tile] |= 4;
			MarkTileDirtyByTile(tile);
		}
	} else if ((_map5[tile]&0xF0) ==  0x20){
		if (v->type == VEH_Road && v->u.road.frame == 11) {
			if (_roadveh_enter_depot_unk0[_map5[tile]&3] == v->u.road.state) {
				RoadVehEnterDepot(v);
				return 4;
			}
		}
	}
	return 0;
}

static void VehicleLeave_Road(Vehicle *v, uint tile, int x, int y)
{
	if ((_map5[tile] & 0xF0) == 0x10 && v->type == VEH_Train && v->next_in_chain == INVALID_VEHICLE) {
		_map5[tile] &= ~4;
		MarkTileDirtyByTile(tile);
	}
}

static void ChangeTileOwner_Road(uint tile, byte old_player, byte new_player)
{
	byte b;

	if (old_player == _map3_lo[tile] &&
			(_map5[tile]&0xF0) == 0x10) {
		_map3_lo[tile] = new_player == 0xff ? 0x10 : new_player;
		return;
	}

	if (_map_owner[tile] != old_player)
		return;

	if (new_player != 255) {
		_map_owner[tile] = new_player;
	}	else {
		b = _map5[tile]&0xF0;
		if (b == 0) {
			_map_owner[tile] = 0x10;
		} else if (b == 0x10) {
			_map5[tile] = (_map5[tile]&8) ? 0x14 : 0xA;
			_map_owner[tile] = _map3_lo[tile];
			_map3_lo[tile] = 0;
			_map3_hi[tile] &= 0x80;
		} else {
			DoCommandByTile(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
		}
	}
}

void InitializeRoad()
{
	_last_built_road_depot_tile = 0;
}

const TileTypeProcs _tile_type_road_procs = {
	DrawTile_Road,						/* draw_tile_proc */
	GetSlopeZ_Road,						/* get_slope_z_proc */
	ClearTile_Road,						/* clear_tile_proc */
	GetAcceptedCargo_Road,		/* get_accepted_cargo_proc */
	GetTileDesc_Road,					/* get_tile_desc_proc */
	GetTileTrackStatus_Road,	/* get_tile_track_status_proc */
	ClickTile_Road,						/* click_tile_proc */
	AnimateTile_Road,					/* animate_tile_proc */
	TileLoop_Road,						/* tile_loop_clear */
	ChangeTileOwner_Road,			/* change_tile_owner_clear */
	NULL,											/* get_produced_cargo_proc */
	VehicleEnter_Road,				/* vehicle_enter_tile_proc */
	VehicleLeave_Road,				/* vehicle_leave_tile_proc */
};
