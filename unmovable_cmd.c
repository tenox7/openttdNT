#include "stdafx.h"
#include "ttd.h"
#include "command.h"
#include "viewport.h"
#include "player.h"
#include "gui.h"

typedef struct DrawTileUnmovableStruct {
	uint16 image;
	byte subcoord_x;
	byte subcoord_y;
	byte width;
	byte height;
	byte z_size;
	byte unused;
} DrawTileUnmovableStruct;

typedef struct DrawTileSeqStruct {
	int8 delta_x;
	int8 delta_y;
	int8 delta_z;
	byte width,height;
	byte unk;
	SpriteID image;
} DrawTileSeqStruct;

#include "table/unmovable_land.h"

static void DrawTile_Unmovable(TileInfo *ti)
{
	uint32 image, ormod;
	
	if (!(ti->map5 & 0x80)) {
		if (ti->map5 == 2) {
			
			// statue
			DrawGroundSprite(0x58C);

			image = PLAYER_SPRITE_COLOR(_map_owner[ti->tile]);
			image += 0x8A48;
			if (!(_display_opt & DO_TRANS_BUILDINGS))
				image = (image & 0x3FFF) | 0x3224000;
			AddSortableSpriteToDraw(image, ti->x, ti->y, 16, 16, 25, ti->z);
		} else if (ti->map5 == 3) {
			
			// "owned by" sign
			DrawClearLandTile(ti, 0);
			
			AddSortableSpriteToDraw(
				PLAYER_SPRITE_COLOR(_map_owner[ti->tile]) + 0x92B6,
				ti->x+8, ti->y+8,
				1, 1, 
				10, 
				(byte)GetSlopeZ(ti->x+8, ti->y+8)
			);
		} else {
			// lighthouse or transmitter
			
			const DrawTileUnmovableStruct *dtus;

			if (ti->tileh) DrawFoundation(ti, ti->tileh);
			DrawClearLandTile(ti, 2);
			OffsetGroundSpriteEnd();
			
			dtus = &_draw_tile_unmovable_data[ti->map5];		

			image = dtus->image;
			if (!(_display_opt & DO_TRANS_BUILDINGS))
				image = (image & 0x3FFF) | 0x3224000;
			
			AddSortableSpriteToDraw(image, 
				ti->x | dtus->subcoord_x,
				ti->y | dtus->subcoord_y,
				dtus->width, dtus->height,
				dtus->z_size, ti->z);
		}
	} else {
		const DrawTileSeqStruct *dtss;
		const byte *t;

		if (ti->tileh) DrawFoundation(ti, ti->tileh);

		ormod = PLAYER_SPRITE_COLOR(_map_owner[ti->tile]);

		t = _unmovable_display_datas[ti->map5 & 0x7F];
		DrawGroundSprite(READ_LE_UINT16(t) | ormod);
		OffsetGroundSpriteEnd();

		t += sizeof(uint16);
			
		for(dtss = (DrawTileSeqStruct *)t; (byte)dtss->delta_x != 0x80; dtss++) {
			image =	dtss->image;
			if (_display_opt & DO_TRANS_BUILDINGS) {
				image |= ormod;
			} else {
				image = (image & 0x3FFF) | 0x03224000;
			}
			AddSortableSpriteToDraw(image, ti->x + dtss->delta_x, ti->y + dtss->delta_y,
				dtss->width, dtss->height, dtss->unk, ti->z + dtss->delta_z);
		}
	}
}

static uint16 GetSlopeZ_Unmovable(TileInfo *ti) {
	return GetPartialZ(ti->x&0xF, ti->y&0xF, ti->tileh) + ti->z;
}


static int32 ClearTile_Unmovable(uint tile, byte flags)
{
	byte m5 = _map5[tile];

	if (m5 & 0x80)
		return_cmd_error(STR_5804_COMPANY_HEADQUARTERS_IN);

	if (m5 == 3) {
		return DoCommandByTile(tile, 0, 0, flags, CMD_SELL_LAND_AREA);
	}

	if (_game_mode != GM_EDITOR)
		return_cmd_error(STR_5800_OBJECT_IN_THE_WAY);
	
	if (flags & DC_EXEC)
		DoClearSquare(tile);
	return 0;
}

static void GetAcceptedCargo_Unmovable(uint tile, AcceptedCargo *ac)
{
	/* not used */
}

static const StringID _unmovable_tile_str[] = {
	STR_5803_COMPANY_HEADQUARTERS,
	STR_5801_TRANSMITTER,
	STR_5802_LIGHTHOUSE,
	STR_2016_STATUE,
	STR_5805_COMPANY_OWNED_LAND,
};	

static void GetTileDesc_Unmovable(uint tile, TileDesc *td)
{
	int i = _map5[tile];
	if (i & 0x80) i = -1;
	td->str = _unmovable_tile_str[i + 1];
	td->owner = _map_owner[tile];
}

static void AnimateTile_Unmovable(uint tile)
{
	/* not used */
}

static void TileLoop_Unmovable(uint tile)
{
	/* not used */
}


static uint32 GetTileTrackStatus_Unmovable(uint tile, int mode)
{
	return 0;
}

static void ClickTile_Unmovable(uint tile)
{
	if (_map5[tile] & 0x80) {
		ShowPlayerCompany(_map_owner[tile]);
	}
}

static const uint16 _tile_andor[4+4] = {
	0xFF00, 0xFF, 0xFF00, 0xFF,
	0, 0, 0xFF, 0xFF00
};

static const int16 _tile_add[4] = {
	TILE_XY(1,0),
	TILE_XY(0,1),
	TILE_XY(-1,0),
	TILE_XY(0,-1),
};

void GenerateUnmovables()
{
	int i,j;
	uint tile;
	uint32 r;
	int dir;
	int h;

	if (_opt.landscape == LT_CANDY)
		return;

	/* add radio tower */
	i = 1000;
	do {
		tile = TILE_MASK(Random());
		if (IS_TILETYPE(tile, MP_CLEAR) && GetTileSlope(tile, &h) == 0 && h >= 0x30) {
			_map_type_and_height[tile] |= MP_UNMOVABLE << 4;
			_map5[tile] = 0;
			_map_owner[tile] = 0x10;
		}
	} while (--i);

	if (_opt.landscape == LT_DESERT)
		return;

	/* add lighthouses */
	i = (Random()&3) + 7;
	do {
restart:
		r = Random();
		dir = r >> 30;
		tile = (uint16) ((TILE_MASK(r) & _tile_andor[dir]) | _tile_andor[dir+4]);
		j = 20;
		do {
			if (--j == 0)
				goto restart;
			tile += _tile_add[dir];
		} while (!(IS_TILETYPE(tile, MP_CLEAR) && GetTileSlope(tile, &h) == 0 && h <= 16));
		
		assert(tile == TILE_MASK(tile));

		_map_type_and_height[tile] |= MP_UNMOVABLE << 4;
		_map5[tile] = 1;
		_map_owner[tile] = 0x10;
	} while (--i);
}

extern int32 CheckFlatLandBelow(uint tile, uint w, uint h, uint flags, uint invalid_dirs);

int32 CmdBuildCompanyHQ(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile = TILE_FROM_XY(x,y);

	if (CheckFlatLandBelow(tile, 2, 2, flags, 0) == CMD_ERROR)
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		DEREF_PLAYER(_current_player)->location_of_house = tile;

		ModifyTile(tile + TILE_XY(0,0),
			MP_SETTYPE(MP_UNMOVABLE) | MP_MAPOWNER_CURRENT | MP_MAP5,
			0x80
		);

		ModifyTile(tile + TILE_XY(0,1),
			MP_SETTYPE(MP_UNMOVABLE) | MP_MAPOWNER_CURRENT | MP_MAP5,
			0x81
		);

		ModifyTile(tile + TILE_XY(1,0),
			MP_SETTYPE(MP_UNMOVABLE) | MP_MAPOWNER_CURRENT | MP_MAP5,
			0x82
		);

		ModifyTile(tile + TILE_XY(1,1),
			MP_SETTYPE(MP_UNMOVABLE) | MP_MAPOWNER_CURRENT | MP_MAP5,
			0x83
		);
	}

	return 0;
}

static void ChangeTileOwner_Unmovable(uint tile, byte old_player, byte new_player)
{
	if (_map_owner[tile] != old_player)
		return;
	
	if (_map5[tile]==3 && new_player != 255) {
		_map_owner[tile] = new_player;
	}	else {
		DoClearSquare(tile);
	}
}

const TileTypeProcs _tile_type_unmovable_procs = {
	DrawTile_Unmovable,						/* draw_tile_proc */
	GetSlopeZ_Unmovable,					/* get_slope_z_proc */
	ClearTile_Unmovable,					/* clear_tile_proc */
	GetAcceptedCargo_Unmovable,		/* get_accepted_cargo_proc */
	GetTileDesc_Unmovable,				/* get_tile_desc_proc */
	GetTileTrackStatus_Unmovable,	/* get_tile_track_status_proc */
	ClickTile_Unmovable,					/* click_tile_proc */
	AnimateTile_Unmovable,				/* animate_tile_proc */
	TileLoop_Unmovable,						/* tile_loop_clear */
	ChangeTileOwner_Unmovable,		/* change_tile_owner_clear */
	NULL,													/* get_produced_cargo_proc */
	NULL,													/* vehicle_enter_tile_proc */
	NULL,													/* vehicle_leave_tile_proc */
};
