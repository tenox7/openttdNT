#include "stdafx.h"
#include "ttd.h"
#include "vehicle.h"
#include "viewport.h"
#include "command.h"
#include "city.h"

bool IsShipDepotTile(TileIndex tile)
{
	return IS_TILETYPE(tile, MP_WATER) &&	(_map5[tile]&~3) == 0x80;
}

bool IsClearWaterTile(uint tile)
{
	TileInfo ti;
	FindLandscapeHeightByTile(&ti, tile);
	return (ti.type == MP_WATER && ti.tileh == 0 && ti.map5 == 0);
}

/* Build a ship depot
 * p1 - direction
 */

int32 CmdBuildShipDepot(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile, tile2;
	
	int32 cost, ret;
	Depot *dep;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	tile = TILE_FROM_XY(x,y);
	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	tile2 = tile + (p1 ? TILE_XY(0,1) : TILE_XY(1,0));
	if (!EnsureNoVehicle(tile2))
		return CMD_ERROR;

	if (!IsClearWaterTile(tile) || !IsClearWaterTile(tile2))
		return_cmd_error(STR_3801_MUST_BE_BUILT_ON_WATER);

	ret = DoCommandByTile(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (ret == CMD_ERROR) return CMD_ERROR;
	ret = DoCommandByTile(tile2, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (ret == CMD_ERROR)
		return CMD_ERROR;
	
	// pretend that we're not making land from the water even though we actually are.
	cost = 0;

	dep = AllocateDepot();
	if (dep == NULL)
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		dep->xy = tile;
		_last_built_ship_depot_tile = tile;
		dep->city_index = ClosestCityFromTile(tile, (uint)-1)->index;

		ModifyTile(tile, 
			MP_SETTYPE(MP_WATER) | MP_MAPOWNER_CURRENT | MP_MAP5,
			(0x80 + p1*2)
		);
	
		ModifyTile(tile2, 
			MP_SETTYPE(MP_WATER) | MP_MAPOWNER_CURRENT | MP_MAP5,
			(0x81 + p1*2)
		);
	}

	return cost + _price.build_ship_depot;
}

static int32 RemoveShipDepot(uint tile, uint32 flags)
{
	uint tile2;

	if (!CheckOwnership(_map_owner[tile]))
		return CMD_ERROR;

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	tile2 = tile + ((_map5[tile] & 2) ? TILE_XY(0,1) : TILE_XY(1,0));

	if (!EnsureNoVehicle(tile2))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		Depot *d;

		DoClearSquare(tile);
		DoClearSquare(tile2);

		// Kill the entry from the depot table
		for(d=_depots; d->xy != tile; d++) {}
		d->xy = 0;
				
		DeleteWindowById(WC_VEHICLE_DEPOT, tile);		
	}

	return _price.remove_ship_depot;
}


static int32 ClearTile_Water(uint tile, byte flags) {
	byte m5 = _map5[tile];
	uint slope;

	if (!(m5 & 0x80)) {
		
		// Allow building on water? It's ok to build on shores.
		if (flags & DC_NO_WATER && m5 != 1)
			return_cmd_error(STR_3807_CAN_T_BUILD_ON_WATER);

		// Make sure no vehicle is on the tile
		if (!EnsureNoVehicle(tile))
			return CMD_ERROR;

		// Make sure it's not an edge tile.
		if (!(IS_INT_INSIDE(GET_TILE_X(tile),1,253+1) &&
				IS_INT_INSIDE(GET_TILE_Y(tile),1,253+1)))
			return_cmd_error(STR_0002_TOO_CLOSE_TO_EDGE_OF_MAP);

		if (m5 == 0) {
			if (flags & DC_EXEC)
				DoClearSquare(tile);
			return _price.clear_water;
		} else if (m5 == 1) {
			slope = GetTileSlope(tile,NULL);
			if (slope == 8 || slope == 4 || slope == 2 || slope == 1) {
				if (flags & DC_EXEC)
					DoClearSquare(tile);
				return _price.clear_water;
			}
			if (flags & DC_EXEC)			
				DoClearSquare(tile);
			return _price.purchase_land;
		} else
			return CMD_ERROR;
	} else {
		if (flags & DC_AUTO)
			return_cmd_error(STR_2004_BUILDING_MUST_BE_DEMOLISHED);

		if (m5 == 0x80 || m5 == 0x82) {}
		else if (m5 == 0x81) { tile -= TILE_XY(1,0); }
		else if (m5 == 0x83) { tile -= TILE_XY(0,1); }
		else
			return CMD_ERROR;

		return RemoveShipDepot(tile,flags);
	}
}

typedef struct WaterDrawTileStruct {
	int8 delta_x;
	int8 delta_y;
	int8 delta_z;
	byte width;
	byte height;
	byte unk;
	SpriteID image;
} WaterDrawTileStruct;

#include "table/water_land.h"

static void DrawTile_Water(TileInfo *ti)
{
	const byte *t;
	const WaterDrawTileStruct *wdts;
	uint32 image, image_or_modificator;

	if (ti->map5 == 0) {
		DrawGroundSprite(0xFDD);
		return;
	}

	if (! ((byte)ti->map5 & 0x80)) {
		/* shore edge */
		assert(ti->tileh < 16);
		DrawGroundSprite(_water_shore_sprites[ti->tileh]);
		return;
	}

	image_or_modificator = PLAYER_SPRITE_COLOR(_map_owner[ti->tile]);

	t = _water_display_datas[ti->map5 & 0x7F];
	DrawGroundSprite(READ_LE_UINT16(t));	
	t += sizeof(uint16);
		
	for(wdts = (WaterDrawTileStruct *)t; (byte)wdts->delta_x != 0x80; wdts++) {
		image =	wdts->image;
		if (_display_opt & DO_TRANS_BUILDINGS) {
			image |= image_or_modificator;	
		} else {
			image = (image & 0x3FFF) | 0x03224000;
		}

		AddSortableSpriteToDraw(image, ti->x + wdts->delta_x, ti->y + wdts->delta_y, wdts->width, wdts->height, wdts->unk, ti->z + wdts->delta_z);
	}
}

void DrawShipDepotSprite(int x, int y, int image)
{
	const byte *t;
	const WaterDrawTileStruct *wdts;

	t = _water_display_datas[image];
	DrawSprite(READ_LE_UINT16(t), x, y);
	t += sizeof(uint16);
		
	for(wdts = (WaterDrawTileStruct *)t; (byte)wdts->delta_x != 0x80; wdts++) {
		Point pt = RemapCoords(wdts->delta_x, wdts->delta_y, wdts->delta_z);
		DrawSprite(wdts->image + PLAYER_SPRITE_COLOR(_local_player), x + pt.x, y + pt.y);
	}
	
}


uint16 GetSlopeZ_Water(TileInfo *ti)
{ 
	return GetPartialZ(ti->x&0xF, ti->y&0xF, ti->tileh) + ti->z;
}

static void GetAcceptedCargo_Water(uint tile, AcceptedCargo *ac)
{
	/* not used */
}

static void GetTileDesc_Water(uint tile, TileDesc *td)
{
	if (_map5[tile] == 0)
		td->str = STR_3804_WATER;
	else if (_map5[tile] == 1)
		td->str = STR_3805_COAST_OR_RIVERBANK;
	else
		td->str = STR_3806_SHIP_DEPOT;

	td->owner = _map_owner[tile];
}

static void AnimateTile_Water(uint tile)
{
	/* not used */
}

static void TileLoopWaterHelper(uint tile, const int16 *offs)
{
	byte *p;

	p = &_map_type_and_height[tile];
	tile += offs[0];

	if (p[offs[0]] >> 4 == MP_WATER) /* edi */
		return;

	if ( (p[offs[1]] | p[offs[2]]) & 0xF ) /* esi & ebp */
		return;

	if ( (p[offs[3]] | p[offs[4]]) & 0xF ) {
		_current_player = 0x11;
		
		if (DoCommandByTile(tile,0,0,DC_EXEC | DC_AUTO, CMD_LANDSCAPE_CLEAR) == CMD_ERROR)
			return;

		ModifyTile(tile, MP_SETTYPE(MP_WATER) | MP_MAPOWNER | MP_MAP5,0x11,1);
	} else {
		if (IS_TILETYPE(tile, MP_TUNNELBRIDGE)) {
			byte m5 = _map5[tile];
			if ( (m5&0xF8) == 0xC8 || (m5&0xF8) == 0xF0)
				return;

			if ( (m5&0xC0) == 0xC0) {
				ModifyTile(tile, MP_MAPOWNER | MP_MAP5,0x11,(m5 & ~0x38)|0x8);
//				_map5[tile] = ;
//				_map_owner[tile] = 0x11;
//				MarkTileDirtyByTile(tile);
				return;
			}
		}

		_current_player = 0x11;
		if (DoCommandByTile(tile,0,0,
			DC_EXEC, CMD_LANDSCAPE_CLEAR) == CMD_ERROR)
				return;

		ModifyTile(tile, MP_SETTYPE(MP_WATER) | MP_MAPOWNER | MP_MAP5,0x11,0);
	}
}

// called from tunnelbridge_cmd
void TileLoop_Water(uint tile)
{
	int i;
	
	// XXX: convert into TILE_XY
	static const int16 _tile_loop_offs_array[4][5] = {
		{-1, 0, 0x100, -1, 255},
		{0x100, 0x100, 0x101, 0x200, 0x201},
		{1, 1, 0x101, 2, 0x102},
		{-0x100, 0, 1, -0x100, -0xff},
	};

	if ( IS_INT_INSIDE(GET_TILE_X(tile),1,253+1) &&
	     IS_INT_INSIDE(GET_TILE_Y(tile),1,253+1)) {
		for(i=0; i!=4; i++)
			TileLoopWaterHelper(tile, _tile_loop_offs_array[i]);
	}
}


static const byte _water_tileh[16] = {0, 32, 4, 0, 16, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0};
static const byte _water_m5[4] = {1,1,2,2};

static uint32 GetTileTrackStatus_Water(uint tile, int mode)
{
	uint m5;
	uint b;

	if (mode != 4)
		return 0;

	m5 = _map5[tile];
	if (m5 == 0)
		return 0x3F3F;

	if (m5 == 1) {
		b = _water_tileh[GetTileSlope(tile, NULL)&0xF];
		return b + (b<<8);
	}

	if (!(m5 & 0x80))
		return 0;

	b = _water_m5[m5 & 0x7F];
	return b + (b<<8);
}

extern void ShowShipDepotWindow(uint tile);

static void ClickTile_Water(uint tile)
{
	byte m5 = _map5[tile] - 0x80;
	
	if (IS_BYTE_INSIDE(m5, 0, 3+1)) {
		if (m5 & 1)
			tile += (m5==1) ? TILE_XY(-1,0) : TILE_XY(0,-1);
		ShowShipDepotWindow(tile);
	}
}

static void ChangeTileOwner_Water(uint tile, byte old_player, byte new_player)
{
	if (_map_owner[tile] != old_player)
		return;

	if (new_player != 255) {
		_map_owner[tile] = new_player;
	} else {
		DoCommandByTile(tile, 0, 0, DC_EXEC, CMD_LANDSCAPE_CLEAR);
	}
}

static uint32 VehicleEnter_Water(Vehicle *v, uint tile, int x, int y)
{
	return 0;
}

void InitializeDock()
{
	_last_built_ship_depot_tile = 0;
}

const TileTypeProcs _tile_type_water_procs = {
	DrawTile_Water,						/* draw_tile_proc */
	GetSlopeZ_Water,					/* get_slope_z_proc */
	ClearTile_Water,					/* clear_tile_proc */
	GetAcceptedCargo_Water,		/* get_accepted_cargo_proc */
	GetTileDesc_Water,				/* get_tile_desc_proc */
	GetTileTrackStatus_Water,	/* get_tile_track_status_proc */
	ClickTile_Water,					/* click_tile_proc */
	AnimateTile_Water,				/* animate_tile_proc */
	TileLoop_Water,						/* tile_loop_clear */
	ChangeTileOwner_Water,		/* change_tile_owner_clear */
	NULL,											/* get_produced_cargo_proc */
	VehicleEnter_Water,				/* vehicle_enter_tile_proc */
	NULL,											/* vehicle_leave_tile_proc */
};

