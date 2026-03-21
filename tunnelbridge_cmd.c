#include "stdafx.h"
#include "ttd.h"
#include "vehicle.h"
#include "viewport.h"
#include "command.h"
#include "player.h"

static const byte _bridge_available_year[11] = {
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 75
};

static const byte _bridge_minlen[11] = {
	0, 0, 0, 2, 3, 3, 3, 3, 3, 0, 2
};

static const byte _bridge_maxlen[11] = {
	16, 2, 5, 10, 16, 16, 7, 8, 9, 2, 16
};

static const byte _bridge_type_price_mod[11] = {
	80, 112, 144, 168, 185, 192, 224, 232, 248, 240, 255
};

const uint16 _bridge_speeds[11] = {
	0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x0A0, 0x0D0, 0x0F0, 0x100, 0x140
};

const PalSpriteID _bridge_sprites[11] = {
	0x0A24, 0x31E8A26, 0x0A25, 0x3208A22,
	0x0A22, 0x3218A22, 0x0A23, 0x31C8A23,
	0x31E8A23, 0x0A27, 0x0A28
};

// calculate the price factor for building a long bridge.
// basically the cost delta is 1,1, 1, 2,2, 3,3,3, 4,4,4,4, 5,5,5,5,5, 6,6,6,6,6,6,  7,7,7,7,7,7,7,  8,8,8,8,8,8,8,8,
static int CalcBridgeLenCostFactor(int x)
{
	int n,r;
	if (x < 2) return x;
	x -= 2;
	for(n=0,r=2;;n++) {
		if (x <= n) return r + x * n;
		r += n * n;
		x -= n;
	}
}

const StringID _bridge_material[11] = {
	STR_5012_WOODEN,
	STR_5013_CONCRETE,
	STR_500F_GIRDER_STEEL,
	STR_5011_SUSPENSION_CONCRETE,
	STR_500E_SUSPENSION_STEEL,
	STR_500E_SUSPENSION_STEEL,
	STR_5010_CANTILEVER_STEEL,
	STR_5010_CANTILEVER_STEEL,
	STR_5010_CANTILEVER_STEEL,
	STR_500F_GIRDER_STEEL,
	STR_5014_TUBULAR_STEEL,
};


/* Build a Bridge 
 * x,y - end tile coord
 * p1  - packed start tile coords (~ dx)
 * p2&0xFF - bridge type (hi bh)
 * p2>>8 - rail type. &0x80 means road bridge.
 */
int32 CmdBuildBridge(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	int bridge_type;
	byte bh_low, railtype, m5, lookflag;
	int sx,sy;
	TileInfo ti_start, ti_end, ti; /* OPT: only 2 of those are ever used */
	int bridge_len, bridge_odd_len;
	uint direction;
	int i;
	int32 cost, ret;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);
	
	/* unpack parameters */
	bridge_type = p2 & 0xFF;
	railtype = (byte)(p2 >> 8);
	
	if (railtype & 0x80) {
		railtype = 0;
		bh_low = 2;
	} else {
		bh_low = 0;
	}

	sx = GET_TILE_X(p1) * 16;
	sy = GET_TILE_Y(p1) * 16;

	direction = 0;

	/* check if valid, and make sure that (x,y) are smaller than (sx,sy) */
	if (x == sx) {
		if (y == sy)
			return_cmd_error(STR_5008_CANNOT_START_AND_END_ON);
		direction = 1;
		if (y > sy) {
			intswap(y,sy);
			intswap(x,sx);
		}
	} else if (y == sy) {
		if (x > sx) {
			intswap(y,sy);
			intswap(x,sx);
		}
	} else
		return_cmd_error(STR_500A_START_AND_END_MUST_BE_IN);

	/* retrieve landscape height and ensure it's on land */
	if (
		((FindLandscapeHeight(&ti_end, sx, sy),
			ti_end.type == MP_WATER) && ti_end.map5 == 0) ||
		((FindLandscapeHeight(&ti_start, x, y),
			ti_start.type == MP_WATER) && ti_start.map5 == 0))
		return_cmd_error(STR_02A0_ENDS_OF_BRIDGE_MUST_BOTH);

	_error_message = STR_500C;

	/* check various params */
	{
		int max;

		if (_bridge_available_year[bridge_type] > _cur_year)
			return CMD_ERROR;

		bridge_len = ((sx + sy - x - y) >> 4) - 1;

		max = _bridge_maxlen[bridge_type];
		if (max == 16 && _patches.longbridges) max = 100;
		if (bridge_len < _bridge_minlen[bridge_type] || bridge_len > max)
			return CMD_ERROR;

		bridge_odd_len = bridge_len&1 ? (bridge_len>>1) : -1;
	}

	/* try and clear the end landscape */
	if ((ret=DoCommandByTile(ti_end.tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR)) == CMD_ERROR)
		return CMD_ERROR;
	cost = ret;
	
	/* check slope at end tile */
	if (!((direction?0x41:0x9) & (1 << ti_end.tileh)))
		return_cmd_error(STR_1000_LAND_SLOPED_IN_WRONG_DIRECTION);

	/* do the drill? */
	if (flags & DC_EXEC) {
		ModifyTile(ti_end.tile,
			MP_SETTYPE(MP_TUNNELBRIDGE) |
			MP_MAP2 | MP_MAP3LO | MP_MAPOWNER_CURRENT | MP_MAP5,
			(bridge_type << 4), /* map2 */
			railtype, /* map3_lo */
			0x80 | 0x20 | direction | bh_low /* map5 */
		);
	}
	
	/* try and clear the start landscape */
	if ((ret=DoCommandByTile(ti_start.tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR)) == CMD_ERROR)
		return CMD_ERROR;
	cost += ret;

	/* check slope at start tile */
	if (!((direction?0x201:0x1001) & (1 << ti_start.tileh)))
		return_cmd_error(STR_1000_LAND_SLOPED_IN_WRONG_DIRECTION);

	/* build a piece of start tile? */
	if (flags & DC_EXEC) {
		ModifyTile(ti_start.tile,
			MP_SETTYPE(MP_TUNNELBRIDGE) |
			MP_MAP2 | MP_MAP3LO | MP_MAPOWNER_CURRENT | MP_MAP5,
			(bridge_type << 4), /* map2 */
			railtype, /* map3_lo */
			0x80 | direction | bh_low /* map5 */
		);
	}

	lookflag = 0;
	if ((bridge_len&3) == 3)
		lookflag |= 2;

	for(i=0; i!=bridge_len; i++) {
		if (direction != 0)
			y+=16;
		else
			x+=16;

		FindLandscapeHeight(&ti, x, y);

		_error_message = STR_5009_LEVEL_LAND_OR_WATER_REQUIRED;
		if (ti.tileh != 0)
			return CMD_ERROR;

		if (ti.type == MP_WATER) {
			if (ti.map5 != 0) goto not_valid_below;
			m5 = 0xC8;
		} else if (ti.type == MP_RAILWAY) {
			if (direction == 0) {
				if (ti.map5 != 2) goto not_valid_below;
			} else {
				if (ti.map5 != 1) goto not_valid_below;
			}
			m5 = 0xE0;
		} else if (ti.type == MP_STREET) {
			if (direction == 0) {
				if (ti.map5 != 5) goto not_valid_below;
			} else {
				if (ti.map5 != 10) goto not_valid_below;
			}
			m5 = 0xE8;
		} else {
not_valid_below:;
			/* try and clear the middle landscape */
			if ((ret=DoCommandByTile(ti.tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR)) == CMD_ERROR)
				return CMD_ERROR;
			cost += ret;
			m5 = 0xC0;
		}

		/* doit */

		if (flags & DC_EXEC) {
			_map5[ti.tile] = (byte)(m5 | direction | bh_low);
			_map_type_and_height[ti.tile] &= ~0xF0;
			_map_type_and_height[ti.tile] |= MP_TUNNELBRIDGE << 4;

			if ( (m5=0, i == 0) || (m5=1, i+1 == bridge_len) ) {

			} else if (lookflag & 2) {
				if ( (m5=3,lookflag&1) ||
				     (m5=2, (uint)i < (uint)bridge_odd_len) ) {
					lookflag ^= 1;
				} else {
					m5 = 4;
					bridge_odd_len = -1;
				}
			} else {
				if ( (m5=2,!(lookflag&1)) ||
				     (m5=3,(uint)i < (uint)bridge_odd_len) ) {
					lookflag ^= 1;
				} else {
					m5 = 5;
					bridge_odd_len = -1;
				}
			}

			_map2[ti.tile] = (bridge_type << 4) | m5;
			_map3_lo[ti.tile] &= 0xF;
			_map3_lo[ti.tile] |= (byte)(railtype << 4);

			MarkTileDirtyByTile(ti.tile);
		}
	}

	SetSignalsOnBothDir(ti_start.tile, (direction&1) ? 1 : 0);

	bridge_len += 2;		

	if (IS_HUMAN_PLAYER(_current_player)) { bridge_len = CalcBridgeLenCostFactor(bridge_len); }

	cost += ((bridge_len * _price.build_bridge) * _bridge_type_price_mod[bridge_type]) >> 8;

	return cost;
}

static bool DoCheckTunnelInWay(uint tile, uint z, uint dir)
{
	TileInfo ti;
	int delta;

	delta = _tileoffs_by_dir[dir];

	do {
		tile -= delta;
		FindLandscapeHeightByTile(&ti, tile);
	} while (z < ti.z);

	if (z == ti.z && ti.type == MP_TUNNELBRIDGE && (ti.map5&0xF0) == 0 && (ti.map5&3) == dir) {
		_error_message = STR_5003_ANOTHER_TUNNEL_IN_THE_WAY;
		return false;
	}

	return true;
}

bool CheckTunnelInWay(uint tile, int z)
{
	return DoCheckTunnelInWay(tile,z,0) &&
		DoCheckTunnelInWay(tile,z,1) &&
		DoCheckTunnelInWay(tile,z,2) &&
		DoCheckTunnelInWay(tile,z,3);
}

static byte _build_tunnel_bh;
static byte _build_tunnel_railtype;

static int32 DoBuildTunnel(int x, int y, int x2, int y2, uint32 flags, uint exc_tile)
{
	uint end_tile;
	int direction;
	int32 cost, ret;
	TileInfo ti;
	uint z;

	if ( (uint) x > 0xFEF ||  (uint) y > 0xFEF)
		return CMD_ERROR;

	/* check if valid, and make sure that (x,y) is smaller than (x2,y2) */
	direction = 0;
	if (x == x2) {
		if (y == y2)
			return_cmd_error(STR_5008_CANNOT_START_AND_END_ON);
		direction++;
		if (y > y2) {
			intswap(y,y2);
			intswap(x,x2);
			exc_tile|=2;
		}
	} else if (y == y2) {
		if (x > x2) {
			intswap(y,y2);
			intswap(x,x2);
			exc_tile|=2;
		}
	} else
		return_cmd_error(STR_500A_START_AND_END_MUST_BE_IN);

	cost = 0;

	FindLandscapeHeight(&ti, x2, y2);
	end_tile = ti.tile;
	z = ti.z;

	if (exc_tile != 3) {
		if ( (direction ? 9U : 12U) != ti.tileh)
			return_cmd_error(STR_1000_LAND_SLOPED_IN_WRONG_DIRECTION);
		ret = DoCommandByTile(ti.tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		if (ret == CMD_ERROR)
			return CMD_ERROR;
		cost += ret;
	}
	cost += _price.build_tunnel;

	for(;;) {
		if (direction) y2-=16; else x2-=16;

		if (x2 == x && y2 == y)
			break;

		FindLandscapeHeight(&ti, x2, y2);
		if (ti.z <= z)
			return_cmd_error(STR_5002);

		if (!_patches.crossing_tunnels && !CheckTunnelInWay(ti.tile, z))
			return CMD_ERROR;

		cost += _price.build_tunnel;
		cost += (cost >> 3);

		if (cost >= 400000000)
			cost = 400000000;
	}

	FindLandscapeHeight(&ti, x2, y2);
	if (ti.z != z)
		return_cmd_error(STR_5004);

	if (exc_tile != 1) {
		if ( (direction ? 6U : 3U) != ti.tileh)
			return_cmd_error(STR_1000_LAND_SLOPED_IN_WRONG_DIRECTION);

		ret = DoCommandByTile(ti.tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		if (ret == CMD_ERROR)
			return CMD_ERROR;
		cost += ret;
	}

	if (flags & DC_EXEC) {
		ModifyTile(ti.tile,
			MP_SETTYPE(MP_TUNNELBRIDGE) |
			MP_MAP3LO | MP_MAPOWNER_CURRENT | MP_MAP5,
			_build_tunnel_railtype, /* map3lo */
			((_build_tunnel_bh << 1) | 2) - direction /* map5 */
		);

		ModifyTile(end_tile,
			MP_SETTYPE(MP_TUNNELBRIDGE) |
			MP_MAP3LO | MP_MAPOWNER_CURRENT | MP_MAP5,
			_build_tunnel_railtype, /* map3lo */
			(_build_tunnel_bh << 1) | (direction ? 3:0)/* map5 */
		);

		UpdateSignalsOnSegment(end_tile, direction?7:1);
	}

	return cost + _price.build_tunnel;
}

/* Build Tunnel
 * x,y - start tile coord
 * p1 - railtype
 * p2 - ptr to uint that recieves end tile
 */
int32 CmdBuildTunnel(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	TileInfo ti, tiorg;
	int direction;
	uint z;
	static const int8 _build_tunnel_coord_mod[4+1] = { -16, 0, 16, 0, -16 };
	static const byte _build_tunnel_tileh[4] = {3, 9, 12, 6};
	uint excavated_tile;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	_build_tunnel_railtype = (byte)(p1 & 0xFF);
	_build_tunnel_bh = (byte)(p1 >> 8);

	_build_tunnel_endtile = 0;
	excavated_tile = 0;

	FindLandscapeHeight(&tiorg, x, y);

	if (!EnsureNoVehicle(tiorg.tile))
		return CMD_ERROR;

	if (!(direction=0, tiorg.tileh==12) &&
			!(direction++, tiorg.tileh==6) &&
			!(direction++, tiorg.tileh==3) &&
			!(direction++, tiorg.tileh==9) )
		return_cmd_error(STR_500B_SITE_UNSUITABLE_FOR_TUNNEL);

	z = tiorg.z;
	do {
		x += _build_tunnel_coord_mod[direction];
		y += _build_tunnel_coord_mod[direction+1];
		FindLandscapeHeight(&ti, x, y);
	} while (z != ti.z);
	_build_tunnel_endtile = ti.tile;

	if (ti.tileh != _build_tunnel_tileh[direction]) {
		if (DoCommandByTile(ti.tile, ti.tileh & ~_build_tunnel_tileh[direction], 0,
			 flags, CMD_TERRAFORM_LAND) == CMD_ERROR)
			return_cmd_error(STR_5005_UNABLE_TO_EXCAVATE_LAND);
		excavated_tile = 1;
	}

	if (!EnsureNoVehicle(ti.tile)) {
		return CMD_ERROR;
	}

	if (flags & DC_EXEC && DoBuildTunnel(x,y,tiorg.x,tiorg.y,flags&~DC_EXEC,excavated_tile) == CMD_ERROR)
		return CMD_ERROR;
	
	return DoBuildTunnel(x,y,tiorg.x, tiorg.y,flags,excavated_tile);
}

static const byte _updsignals_tunnel_dir[4] = { 5, 7, 1, 3};

int32 CmdClearTunnelTile(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile;
	int32 cost;
	TileInfo ti, ti2;
	Vehicle *v;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);
	
	FindLandscapeHeight(&ti, x, y);

	tile = ti.tile;
	if (_game_mode != GM_EDITOR) {
		if (!CheckOwnership(_map_owner[tile]))
			return CMD_ERROR;
	}

	cost = _price.clear_tunnel;
	do {
		cost += _price.clear_tunnel;
		tile += _tileoffs_by_dir[ti.map5 & 3];
		FindLandscapeHeightByTile(&ti2, tile);
	} while (ti2.type != MP_TUNNELBRIDGE || ti2.map5 & 0xF0 || ti2.z != ti.z || (ti2.map5^2) != ti.map5);

	_build_tunnel_endtile = tile;
	
	if ((v=FindVehicleBetween(ti.tile, tile, (byte)ti.z)) != NULL)
		return_cmd_error(v->type == VEH_Train ? STR_5000_TRAIN_IN_TUNNEL : STR_5001_ROAD_VEHICLE_IN_TUNNEL);

	if (flags & DC_EXEC) {
		DoClearSquare(ti.tile);
		DoClearSquare(ti2.tile);
		UpdateSignalsOnSegment(ti.tile, _updsignals_tunnel_dir[ti.map5&3]);
		UpdateSignalsOnSegment(ti2.tile, _updsignals_tunnel_dir[ti2.map5&3]);
	}
	return cost;
}

int32 CmdClearBridgeTile(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	int direction;
	TileInfo ti, ti2;
	Vehicle *v;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	FindLandscapeHeight(&ti, x, y);
	direction = ti.map5&1;

	/* find start of bridge */
	for(;;) {
		FindLandscapeHeight(&ti, x, y);
		if (ti.type == MP_TUNNELBRIDGE && (ti.map5 & 0xE0) == 0x80)
					break;
		if (direction)
			y-=16;
		else
			x-=16;
	}

	if (_current_player != 0x11 &&
			_game_mode != GM_EDITOR && 
			!CheckOwnership(_map_owner[ti.tile]))
				return CMD_ERROR;

	/* find other end of bridge */
	for(;;) {
		FindLandscapeHeight(&ti2, x, y);
		if (ti2.type == MP_TUNNELBRIDGE && (ti2.map5 & 0xE0) == 0xA0)
					break;
		if (direction)
			y+=16;
		else
			x+=16;
	}

	/* Make sure there's no vehicle on the bridge */
	if ((v=FindVehicleBetween(ti.tile, ti2.tile, 0xff)) != NULL) {
		VehicleInTheWayErrMsg(v);		
		return CMD_ERROR;
	}

	if (flags & DC_EXEC) {
		uint tile = ti.tile;
		byte m5;
		uint16 new_data;

		do {
			m5 = _map5[tile];
		
			if (m5 & 0x40) {
				if (m5 & 0x20) {
					static const uint16 _new_data_table[] = {0x1002, 0x1001, 0x2005, 0x200A, 0, 0, 0, 0};
					new_data = _new_data_table[((m5 & 0x18) >> 2) | (m5&1)];
				}	else {
					if (!(m5 & 0x18)) goto clear_it;
					new_data = 0x6000;
				}

				_map_type_and_height[tile] &= 0x0F;
				_map_type_and_height[tile] |= new_data >> 8;
				_map5[tile] = (byte)new_data;
				_map2[tile] = 0;

				MarkTileDirtyByTile(tile);
			} else {
clear_it:;
				DoClearSquare(tile);
			}
			tile += direction ? TILE_XY(0,1) : TILE_XY(1,0);
		} while (tile <= ti2.tile);

		SetSignalsOnBothDir(ti.tile, direction&1);
		SetSignalsOnBothDir(ti2.tile, direction&1);

	}

	return ((((ti2.tile - ti.tile) >> (direction?8:0))&0xFF)+1) * _price.clear_bridge;
}

static int32 ClearTile_TunnelBridge(uint tile, byte flags) {
	int32 ret;
	byte m5 = _map5[tile];

	if ((m5 & 0xF0) == 0) {
		if (flags & DC_AUTO)
			return_cmd_error(STR_5006_MUST_DEMOLISH_TUNNEL_FIRST);
		return DoCommandByTile(tile, 0, 0, flags, CMD_CLEAR_TUNNEL_TILE);
	} else if (m5 & 0x80) {
		if (flags & DC_AUTO)
			return_cmd_error(STR_5007_MUST_DEMOLISH_BRIDGE_FIRST);

		ret = DoCommandByTile(tile, 0, 0, flags, CMD_CLEAR_BRIDGE_TILE);
		if (ret == CMD_ERROR)
			return CMD_ERROR;

		if (flags & DC_EXEC) {
			ret += DoCommandByTile(tile, 0, 0,flags, CMD_LANDSCAPE_CLEAR);
		}
		return ret;
	} else {
		return CMD_ERROR;
	}
}


#include "table/tunnel_land.h"

static void DrawTile_TunnelBridge(TileInfo *ti)
{
	uint32 image, trackbase;
	uint tmp;
	const uint32 *b;
	bool ice = _map3_hi[ti->tile] & 0x80;
	
	/* draw tunnel? */
	if ( (byte)(ti->map5&0xF0) == 0) {
		/* railway type */
		image = (_map3_lo[ti->tile] & 0xF) * 8;

		/* ice? */
		if (ice)
			image += 32;

		image += _draw_tunnel_table_1[(ti->map5 >> 2) & 0x3];
		image += (ti->map5 & 3) << 1;
		DrawGroundSprite(image);

		AddSortableSpriteToDraw(image+1, ti->x + 15, ti->y + 15, 1, 1, 8, (byte)ti->z);
	} else if ((byte)ti->map5 & 0x80) {
		trackbase = (_map3_lo[ti->tile] & 0xF) * TRACKTYPE_SPRITE_PITCH;

		tmp = _map3_lo[ti->tile];

		if (ti->map5 & 0x40)
			tmp >>= 4;

		tmp &= 0xF;
		if (tmp != 0) tmp++;

		tmp = (ti->map5&7)*2 + (tmp*4);

		if (!(ti->map5 & 0x40)) {
			b = _bridge_sprite_table[_map2[ti->tile]>>4][6];

			if (ti->tileh == 0) b += 4;
			if (ti->map5 & 0x20) b += 2;
			if (ti->map5 & 1) b++;

			image = b[(tmp&(3<<2))*2]; /* actually ((tmp>>2)&3)*8 */

			if (!ice) {
				DrawClearLandTile(ti, 3);
			} else {
				DrawGroundSprite(0x11C6 + _tileh_to_sprite[ti->tileh]);
			}

			if (image & 0x40000000) {
				image = (image & ~0x40000000) + trackbase;
			}
			if (image & 0x80000000) {
				image = (image & 0xFFFF) + SPRITE_PALETTE(PLAYER_SPRITE_COLOR(_map_owner[ti->tile]));
			}
			DrawGroundSprite(image);
		} else {
			byte z;
			int x,y;

			image = (ti->map5 >> 3) & 3; /* water below? */

			if (!(ti->map5 & 0x20)) {
				DrawGroundSprite((ice?_draw_tunnel_table_4:_draw_tunnel_table_3)[image]);
			} else {

				/* something is below. */
				image *= 2;
				if (image != 0)
					trackbase = 0;

				if (ti->map5&1) image++;
				DrawGroundSprite((ice?_draw_tunnel_table_6:_draw_tunnel_table_5)[image] + trackbase);

				image = (ice?_draw_tunnel_table_8:_draw_tunnel_table_7)[image];
				if (image != 0) {
					DrawGroundSprite(image + PLAYER_SPRITE_COLOR(_map_owner[ti->tile]));
				}
			}
			/* middle */
			b = _bridge_sprite_table[_map2[ti->tile]>>4][_map2[ti->tile]&0xF];

			z = ti->z + 5;
			
			image = b[tmp*2];
			if (image & 0x40000000) image = (image&~0x40000000) + trackbase;
			AddSortableSpriteToDraw(image, ti->x, ti->y, (ti->map5&1)?11:16, (ti->map5&1)?16:11, 1, z);

			image = b[tmp*2+1];
			if (image & 0x40000000) image = (image&~0x40000000) + trackbase;	

			x = ti->x;
			y = ti->y;

			if (ti->map5&1) {
				x += 12;
				if (image != 0)
					AddSortableSpriteToDraw(image, x,y, 1, 16, 0x28, z);
			} else {
				y += 12;
				if (image != 0)
					AddSortableSpriteToDraw(image, x,y, 16, 1, 0x28, z);
			}

			image = b[tmp*2+2];
			if (image != 0) {
				DrawGroundSpriteAt(image, x, y, z);
			}
		}
	}
}

static uint16 GetSlopeZ_TunnelBridge(TileInfo *ti) {
	uint z = GetPartialZ(ti->x&0xF, ti->y&0xF, ti->tileh) + ti->z;
	uint x = ti->x & 0xF;
	uint y = ti->y & 0xF;
	uint t;

	if ( (ti->map5 & 0xF0) == 0) {
		t = (ti->map5&1) ? x:y;
		
		if (!IS_INT_INSIDE(t, 5, 10+1))
			return (uint16) z;

		if (ti->tileh != 3 && ti->tileh != 6) {
			return (uint16) (z & ~7);
		}

//		assert(z > 0);
		
		return (uint16) ( (z-1) & ~7 );
	} else if ( ti->map5 & 0x80 ) {
		if (!(ti->map5 & 0x40)) {
			if (ti->tileh != 0) {
				t = (ti->map5&1) ? x:y;
				if (!IS_INT_INSIDE(t, 5, 10+1))
					return (uint16) z;

//				assert(z > 0);
	
				if (ti->tileh & 2)
					z--;
				return (uint16) ((z & ~7) + 8);
			} else if (!(ti->map5 & 0x20)) {
				if (!(ti->map5&1)) {
					t = x;
					if (!IS_INT_INSIDE(y, 5, 10+1))
						return (uint16) z;
				} else {
					t = y;
					if (!IS_INT_INSIDE(x, 5, 10+1))
						return (uint16) z;
				}
				return (uint16)(z + (t>>1) + 1);
			} else {
				if (!(ti->map5&1)) {
					t = x;
					if (!IS_INT_INSIDE(y, 5, 10+1))
						return (uint16) z;
				} else {
					t = y;
					if (!IS_INT_INSIDE(x, 5, 10+1))
						return (uint16) z;
				}

				return (uint16)(z + ((t^0xF)>>1));
			}
		} else {
			if (!(ti->map5&1)) {
				t = x;
				if (!IS_INT_INSIDE(y, 5, 10+1))
					return (uint16) z;
			} else {
				t = y;
				if (!IS_INT_INSIDE(x, 5, 10+1))
					return (uint16) z;
			}

			if (!IS_INT_INSIDE(t, 5, 10+1))
				return (uint16) (z + 8);

			return z | 256;
		}
	}	else {
		return (uint16) z;
	}
}

static void GetAcceptedCargo_TunnelBridge(uint tile, AcceptedCargo *ac)
{
	/* not used */
}

static const StringID _bridge_tile_str[16+16] = {
	STR_501F_WOODEN_RAIL_BRIDGE,
	STR_5020_CONCRETE_RAIL_BRIDGE,
	STR_501C_STEEL_GIRDER_RAIL_BRIDGE,
	STR_501E_REINFORCED_CONCRETE_SUSPENSION,
	STR_501B_STEEL_SUSPENSION_RAIL_BRIDGE,
	STR_501B_STEEL_SUSPENSION_RAIL_BRIDGE,
	STR_501D_STEEL_CANTILEVER_RAIL_BRIDGE,
	STR_501D_STEEL_CANTILEVER_RAIL_BRIDGE,
	STR_501D_STEEL_CANTILEVER_RAIL_BRIDGE,
	STR_501C_STEEL_GIRDER_RAIL_BRIDGE,
	STR_5027_TUBULAR_RAIL_BRIDGE,
	0,0,0,0,0,

	STR_5025_WOODEN_ROAD_BRIDGE,
	STR_5026_CONCRETE_ROAD_BRIDGE,
	STR_5022_STEEL_GIRDER_ROAD_BRIDGE,
	STR_5024_REINFORCED_CONCRETE_SUSPENSION,
	STR_5021_STEEL_SUSPENSION_ROAD_BRIDGE,
	STR_5021_STEEL_SUSPENSION_ROAD_BRIDGE,
	STR_5023_STEEL_CANTILEVER_ROAD_BRIDGE,
	STR_5023_STEEL_CANTILEVER_ROAD_BRIDGE,
	STR_5023_STEEL_CANTILEVER_ROAD_BRIDGE,
	STR_5022_STEEL_GIRDER_ROAD_BRIDGE,
	STR_5028_TUBULAR_ROAD_BRIDGE,
	0,0,0,0,0,
};

static void GetTileDesc_TunnelBridge(uint tile, TileDesc *td)
{
	int delta;

	if ((_map5[tile] & 0x80) == 0) {
		td->str = STR_5017_RAILROAD_TUNNEL + ((_map5[tile] >> 2) & 3);
	} else {
		td->str = _bridge_tile_str[ (_map2[tile] >> 4) + (((_map5[tile]>>1)&3)<<4) ];

		/* scan to the end of the bridge, that's where the owner is stored */
		if (_map5[tile] & 0x40) {
			delta = _map5[tile] & 1 ? TILE_XY(0,-1) : TILE_XY(-1,0);
			do tile += delta; while (_map5[tile] & 0x40);
		}
	}
	td->owner = _map_owner[tile];
}


static void AnimateTile_TunnelBridge(uint tile)
{
	/* not used */
}

static void TileLoop_TunnelBridge(uint tile)
{
	byte m5;
	
	if (_opt.landscape == LT_HILLY) {
		if ( GetTileZ(tile) > _opt.snow_line) {
			if (!(_map3_hi[tile] & 0x80)) {
				_map3_hi[tile] |= 0x80;
				MarkTileDirtyByTile(tile);
			}
		} else {
			if (_map3_hi[tile] & 0x80) {
				_map3_hi[tile] &= ~0x80;
				MarkTileDirtyByTile(tile);
			}
		}
	} else if (_opt.landscape == LT_DESERT) {
		if (GetMapExtraBits(tile) == 1 && !(_map3_hi[tile]&0x80)) {
			_map3_hi[tile] |= 0x80;
			MarkTileDirtyByTile(tile);			
		}
	}

	m5 = _map5[tile] & 0xF8;
	if (m5 == 0xC8 || m5 == 0xF0)
		TileLoop_Water(tile);
}

static void ClickTile_TunnelBridge(uint tile)
{
	/* not used */
}


static uint32 GetTileTrackStatus_TunnelBridge(uint tile, int mode)
{
	uint32 result;
	byte t, m5 = _map5[tile];

	if ((m5 & 0xF0) == 0) {
		if (((m5 & 0xC) >> 1) == mode) { /* XXX: possible problem when switching mode constants */
			return m5&1 ? 0x202 : 0x101;
		}
	} else if (m5 & 0x80) {
		result = 0;
		if ((m5 & 6) == mode) {
			result = m5&1 ? 0x202 : 0x101;
		}
		if (m5 & 0x40) {
			t = m5;
			if (!(t & 0x20)) {
				if ((t &= 0x18) != 8)
					return result;
				t = (t&0xE7) | 0x30;
			}

			if ((t & 0x18) >> 2 != mode)
				return result;

			result ^= m5&1 ? 0x101 : 0x202;
		}
		return result;
	}
	return 0;
}

static void ChangeTileOwner_TunnelBridge(uint tile, byte old_player, byte new_player)
{
	if (_map_owner[tile] != old_player)
		return;
	
	if (new_player != 255) {
		_map_owner[tile] = new_player;
	}	else {
		DoCommandByTile(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
	}
}


static const byte _tunnel_fractcoord_1[4] = {0x8E,0x18,0x81,0xE8};
static const byte _tunnel_fractcoord_2[4] = {0x81,0x98,0x87,0x38};
static const byte _tunnel_fractcoord_3[4] = {0x82,0x88,0x86,0x48};
static const byte _exit_tunnel_track[4] = {1,2,1,2};

static const byte _road_exit_tunnel_state[4] = {8, 9, 0, 1};
static const byte _road_exit_tunnel_frame[4] = {2, 7, 9, 4};

static const byte _tunnel_fractcoord_4[4] = {0x52, 0x85, 0x98, 0x29};
static const byte _tunnel_fractcoord_5[4] = {0x92, 0x89, 0x58, 0x25};
static const byte _tunnel_fractcoord_6[4] = {0x92, 0x89, 0x56, 0x45};
static const byte _tunnel_fractcoord_7[4] = {0x52, 0x85, 0x96, 0x49};

static uint32 VehicleEnter_TunnelBridge(Vehicle *v, uint tile, int x, int y)
{
	int z;
	int dir, vdir;
	byte fc;
	int h;

	if ((_map5[tile] & 0xF0) == 0) {
		z = (byte)GetSlopeZ(x, y) - v->z_pos;
		if (myabs(z) > 2)
			return 8;

		if (v->type == VEH_Train) {
			fc = (x&0xF)+(y<<4);
			
			dir = _map5[tile] & 3;
			vdir = v->direction >> 1;

			if (v->u.rail.track != 0x40 && dir == vdir) {
				if (v->subtype == 0 && fc == _tunnel_fractcoord_1[dir]) {
					if (v->spritenum < 4)
						SndPlayVehicleFx(3, v);
					return 0;
				}
				if (fc == _tunnel_fractcoord_2[dir]) {
					v->tile = tile;
					v->u.rail.track = 0x40;
					v->vehstatus |= VS_HIDDEN;
					return 4;
				}
			}

			if (dir == (vdir^2) && fc == _tunnel_fractcoord_3[dir] && z == 0) {
				v->tile = tile;
				v->u.rail.track = _exit_tunnel_track[dir];
				v->vehstatus &= ~VS_HIDDEN;
				return 4;
			}
		} else if (v->type == VEH_Road) {
			fc = (x&0xF)+(y<<4);
			dir = _map5[tile] & 3;
			vdir = v->direction >> 1;

			// Enter tunnel?
			if (v->u.road.state != 0xFF && dir == vdir) {
				if (fc == _tunnel_fractcoord_4[dir] ||
						fc == _tunnel_fractcoord_5[dir]) {
					
					v->tile = tile;
					v->u.road.state = 0xFF;
					v->vehstatus |= VS_HIDDEN;	
					return 4;
				} else {
					return 0;
				}
			}

			if (dir == (vdir^2) && (
					fc == _tunnel_fractcoord_6[dir] ||
					fc == _tunnel_fractcoord_7[dir]) &&
					z == 0) {
				v->tile = tile;
				v->u.road.state = _road_exit_tunnel_state[dir];
				v->u.road.frame = _road_exit_tunnel_frame[dir];
				v->vehstatus &= ~VS_HIDDEN;
				return 4;
			}
		}
	} else if (_map5[tile] & 0x80) {
		if (v->type == VEH_Road || (v->type == VEH_Train && v->subtype == 0)) {
			if (GetTileSlope(tile, &h) != 0 || myabs(h - v->z_pos) > 2) {
				/* modify speed of vehicle */
				uint16 spd = _bridge_speeds[_map2[tile] >> 4];
				if (v->type == VEH_Road) spd<<=1;
				if (spd < v->cur_speed)
					v->cur_speed = spd;
			}
		}
	}
	return 0;
}

uint GetVehicleOutOfTunnelTile(Vehicle *v)
{
	uint tile = v->tile;
	int delta_tile;
	byte z;

	/* locate either ending of the tunnel */
	delta_tile = (v->direction&2) ? TILE_XY(0,1) : TILE_XY(1,0);
	z = v->z_pos;
	for(;;) {
		TileInfo ti;
		FindLandscapeHeightByTile(&ti, tile);

		if (ti.type == MP_TUNNELBRIDGE && (ti.map5 & 0xF0)==0 && (byte)ti.z == z)
			break;

		tile += delta_tile;
	}
	return tile;
}

const TileTypeProcs _tile_type_tunnelbridge_procs = {
	DrawTile_TunnelBridge,					/* draw_tile_proc */
	GetSlopeZ_TunnelBridge,					/* get_slope_z_proc */
	ClearTile_TunnelBridge,					/* clear_tile_proc */
	GetAcceptedCargo_TunnelBridge,	/* get_accepted_cargo_proc */
	GetTileDesc_TunnelBridge,				/* get_tile_desc_proc */
	GetTileTrackStatus_TunnelBridge,/* get_tile_track_status_proc */
	ClickTile_TunnelBridge,					/* click_tile_proc */
	AnimateTile_TunnelBridge,				/* animate_tile_proc */
	TileLoop_TunnelBridge,					/* tile_loop_clear */
	ChangeTileOwner_TunnelBridge,		/* change_tile_owner_clear */
	NULL,														/* get_produced_cargo_proc */
	VehicleEnter_TunnelBridge,			/* vehicle_enter_tile_proc */
	NULL,														/* vehicle_leave_tile_proc */
};

