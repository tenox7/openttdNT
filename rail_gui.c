#include "stdafx.h"
#include "ttd.h"

#include "window.h"
#include "gui.h"
#include "viewport.h"
#include "gfx.h"
#include "command.h"
#include "vehicle.h"

static uint _cur_train_build_type;
static bool _remove_button_clicked;
static byte _build_depot_direction;

struct {
	byte orientation;
	byte numtracks;
	byte platlength;
} _railstation;



static void ShowBuildTrainDepotPicker();
static void ShowStationBuilder();

typedef void OnButtonClick(Window *w);

static void GenericPlaceRail(uint tile, int cmd)
{
	int32 res;

	if (!_remove_button_clicked) {
		res = DoCommandByTile(tile, _cur_train_build_type, cmd, 
			DC_MSG(STR_1011_CAN_T_BUILD_RAILROAD_TRACK) | DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_BUILD_SINGLE_RAIL);		
		if (res != CMD_ERROR)
			SndPlayTileFx(0x1E, tile);
		
	} else {
		res = DoCommandByTile(tile, _cur_train_build_type, cmd, 
			DC_MSG(STR_1012_CAN_T_REMOVE_RAILROAD_TRACK) | DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_REMOVE_SINGLE_RAIL);
		if (res != CMD_ERROR)
			SndPlayTileFx(0x1E, tile);
	}
}

static void PlaceRail_N(uint tile)
{
	int cmd = _tile_fract_coords.x > _tile_fract_coords.y ? 4 : 5;
	GenericPlaceRail(tile, cmd);
}

static void PlaceRail_NE(uint tile)
{
	VpStartPlaceSizing(tile, VPM_FIX_Y);
}

static void PlaceRail_E(uint tile)
{
	int cmd = _tile_fract_coords.x + _tile_fract_coords.y <= 15 ? 2 : 3;
	GenericPlaceRail(tile, cmd);
}

static void PlaceRail_NW(uint tile)
{
	VpStartPlaceSizing(tile, VPM_FIX_X);
}

static void PlaceRail_AutoRail(uint tile)
{
	VpStartPlaceSizing(tile, VPM_RAILDIRS);
}

void PlaceProc_Demolish(uint tile)
{
	int32 ret;

	ret = DoCommandByTile(tile, 0, 0, 
		DC_EXEC | DC_MSG(STR_00B5_CAN_T_CLEAR_THIS_AREA), CMD_LANDSCAPE_CLEAR);

	if (ret == CMD_ERROR)
		return;

	SndPlayTileFx(0x10, tile);
	CreateEffectVehicleAbove(GET_TILE_X(tile)*16 + 8,GET_TILE_Y(tile)*16 + 8, 2, EV_DEMOLISH);
}

static void GenericRaiseLowerLand(uint tile, int mode)
{
	int32 ret;

	tile = TILE_FROM_XY(GET_TILE_X(tile)*16 + _tile_fract_coords.x + 8, GET_TILE_Y(tile)*16 + _tile_fract_coords.y + 8);

	if (mode) {
		ret = DoCommandByTile(tile, 8, (uint32)mode, 
					DC_EXEC | DC_AUTO | DC_MSG(STR_0808_CAN_T_RAISE_LAND_HERE), CMD_TERRAFORM_LAND);
	} else {
		ret = DoCommandByTile(tile, 8, (uint32)mode, 
					 DC_EXEC | DC_AUTO | DC_MSG(STR_0809_CAN_T_LOWER_LAND_HERE), CMD_TERRAFORM_LAND);
	}

	if (ret == CMD_ERROR) {
		SetRedErrorSquare(_terraform_err_tile);
		return;
	}

	SndPlayTileFx(0x1D, tile);
}

void PlaceProc_LowerLand(uint tile)
{
	GenericRaiseLowerLand(tile, 0);
}

void PlaceProc_RaiseLand(uint tile)
{
	GenericRaiseLowerLand(tile, 1);
}

static int16 _place_depot_offs_xy[4] = { -1,	0x100,	1,	-0x100};

static void PlaceExtraDepotRail(uint tile, uint16 extra)
{
	byte b = _map5[tile];

	if (b & 0xC0 || !(b & (extra >> 8)))
		return;

	DoCommandByTile(tile, _cur_train_build_type, extra & 0xFF, 
		DC_MSG(STR_EMPTY) | DC_AUTO | DC_NO_WATER | DC_EXEC, CMD_BUILD_SINGLE_RAIL);
}

static const uint16 _place_depot_extra[12] = {
	0x604,		0x2102,		0x1202,		0x505,
	0x2400,		0x2801,		0x1800,		0x1401,
	0x2203,		0x904,		0x0A05,		0x1103,
};

static void PlaceRail_Depot(uint tile)
{
	int32 ret;
	int dir = _build_depot_direction;
	
	ret = DoCommandByTile(tile, _cur_train_build_type, dir,
		DC_MSG(STR_100E_CAN_T_BUILD_TRAIN_DEPOT) | DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_BUILD_TRAIN_DEPOT);

	if (ret == CMD_ERROR)
		return;

	SndPlayTileFx(0x1E, tile);
	ResetObjectToPlace();

	tile += _place_depot_offs_xy[dir];

	if (IS_TILETYPE(tile, MP_RAILWAY)) {
		PlaceExtraDepotRail(tile, _place_depot_extra[dir]);
		PlaceExtraDepotRail(tile, _place_depot_extra[dir + 4]);
		PlaceExtraDepotRail(tile, _place_depot_extra[dir + 8]);
	}
}

static void PlaceRail_Checkpoint(uint tile)
{
	int32 ret;

	if (!_remove_button_clicked) {
		ret = DoCommandByTile(tile, 0, 0, DC_MSG(STR_CANT_BUILD_TRAIN_CHECKPOINT) | DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_BUILD_TRAIN_CHECKPOINT);
	} else {
		ret = DoCommandByTile(tile, 0, 0, DC_MSG(STR_CANT_REMOVE_TRAIN_CHECKPOINT) | DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_REMOVE_TRAIN_CHECKPOINT);
	}
	if (ret == CMD_ERROR)
		return;
	SndPlayTileFx(0x1E, tile);
	ResetObjectToPlace();
}

static void PlaceRail_Station(uint tile)
{
	int32 ret;

	/* pass orientation, numtracks, platlength and railtype */

	ret = DoCommandByTile(tile, _railstation.orientation | (_railstation.numtracks<<8) | (_railstation.platlength<<16),_cur_train_build_type,
			DC_MSG(STR_100F_CAN_T_BUILD_RAILROAD_STATION) | DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_BUILD_RAILROAD_STATION);

	if (ret == CMD_ERROR)
		return;

	SndPlayTileFx(0x1E, tile);
	ResetObjectToPlace();
}

static void PlaceRail_Signals(uint tile)
{
	uint trackstat;
	int i;
	int32 ret;

	trackstat = (byte)GetTileTrackStatus(tile,0);

	if ((trackstat & 0x30) == 0x30) {
		trackstat = (_tile_fract_coords.x <= _tile_fract_coords.y) ? 0x20 : 0x10;
	}

	if ((trackstat & 0x0C) == 0x0C) {
		trackstat = (_tile_fract_coords.x + _tile_fract_coords.y <= 15) ? 4 : 8;
	}

	// Lookup the bit index
	i = 0;
	if (trackstat != 0) {	while (!(trackstat & 1)) { i++; trackstat >>= 1; }}

	if (!_remove_button_clicked) {
		ret = DoCommandByTile(tile, i + (_ctrl_pressed ? 8 : 0), 0, 
			DC_EXEC | DC_AUTO | DC_MSG(STR_1010_CAN_T_BUILD_SIGNALS_HERE), CMD_BUILD_SIGNALS);

		if (ret != CMD_ERROR)
			SndPlayTileFx(0x1E, tile);
	} else {
		ret = DoCommandByTile(tile, i, 0,
			DC_EXEC | DC_AUTO | DC_MSG(STR_1013_CAN_T_REMOVE_SIGNALS_FROM), CMD_REMOVE_SIGNALS);

		if (ret != CMD_ERROR)
			SndPlayTileFx(0x1E, tile);
	}
}

static void PlaceRail_Bridge(uint tile)
{
	VpStartPlaceSizing(tile, VPM_X_OR_Y);
}

static void PlaceRail_Tunnel(uint tile)
{
	int32 ret;
	uint endtile;

	ret = DoCommandByTile(tile, _cur_train_build_type, 0,
		DC_MSG(STR_5016_CAN_T_BUILD_TUNNEL_HERE) | DC_EXEC | DC_AUTO, CMD_BUILD_TUNNEL);

	endtile = _build_tunnel_endtile;

	if (ret == CMD_ERROR) {
		if (endtile != 0) SetRedErrorSquare(endtile);
		return;
	}

	SndPlayTileFx(0x1E, tile);
	ResetObjectToPlace();
}

void PlaceProc_BuyLand(uint tile)
{
	int32 ret;

	ret = DoCommandByTile(tile, 0,0, 
			DC_MSG(STR_5806_CAN_T_PURCHASE_THIS_LAND) | DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_PURCHASE_LAND_AREA);

	if (ret == CMD_ERROR)
		return;

	SndPlayTileFx(0x1E, tile);
}


static void BuildRailClick_AutoRail(Window *w)
{
	HandlePlacePushButton(w, 3, _cur_train_build_type + SPR_OPENTTD_BASE + 4, 1, PlaceRail_AutoRail);
}

static void BuildRailClick_N(Window *w)
{
	HandlePlacePushButton(w, 4, _cur_train_build_type*4 + 0x4EF, 1, PlaceRail_N);
}

static void BuildRailClick_NE(Window *w)
{
	HandlePlacePushButton(w, 5, _cur_train_build_type*4 + 0x4F0, 1, PlaceRail_NE);
}

static void BuildRailClick_E(Window *w)
{
	HandlePlacePushButton(w, 6, _cur_train_build_type*4 + 0x4F1, 1, PlaceRail_E);
}


static void BuildRailClick_NW(Window *w)
{
	HandlePlacePushButton(w, 7, _cur_train_build_type*4 + 0x4F2, 1, PlaceRail_NW);
}

static void BuildRailClick_Demolish(Window *w)
{
	HandlePlacePushButton(w, 8, ANIMCURSOR_DEMOLISH, 1, PlaceProc_Demolish);
}

static void BuildRailClick_Lower(Window *w)
{
	HandlePlacePushButton(w, 9, ANIMCURSOR_LOWERLAND, 2, PlaceProc_LowerLand);
}

static void BuildRailClick_Raise(Window *w)
{
	HandlePlacePushButton(w, 10, ANIMCURSOR_RAISELAND, 2, PlaceProc_RaiseLand);
}

static void BuildRailClick_Depot(Window *w)
{
	if (HandlePlacePushButton(w, 11, 0x510, 1, PlaceRail_Depot)) ShowBuildTrainDepotPicker();	
}

static void BuildRailClick_Station(Window *w)
{
	if (HandlePlacePushButton(w, 12, 0x514, 1, PlaceRail_Station)) ShowStationBuilder();	
}

static void BuildRailClick_Signals(Window *w)
{
	HandlePlacePushButton(w, 13, ANIMCURSOR_BUILDSIGNALS, 1, PlaceRail_Signals);
}

static void BuildRailClick_Bridge(Window *w)
{
	HandlePlacePushButton(w, 14, 0xA21, 1, PlaceRail_Bridge);
}

static void BuildRailClick_Tunnel(Window *w)
{
	HandlePlacePushButton(w, 15, 0x982 + _cur_train_build_type, 3, PlaceRail_Tunnel);
}

static void BuildRailClick_Remove(Window *w)
{
	if (w->disabled_state & (1<<16))
		return;
	SetWindowDirty(w);
	SndPlayFx(0x13);
	_thd.make_square_red = !!((w->click_state ^= (1 << 16)) & (1<<16));
}

static void BuildRailClick_Sign(Window *w)
{
	HandlePlacePushButton(w, 17, 0x12B8, 1, PlaceProc_BuyLand);
}

static void BuildRailClick_Checkpoint(Window *w)
{
	HandlePlacePushButton(w, 18, SPR_OPENTTD_BASE + 7, 1, PlaceRail_Checkpoint);
}

static void DoRailroadTrack(int mode)
{
	if (_remove_button_clicked) {
		DoCommand(_thd.selstart.x, _thd.selstart.y, PACK_POINT(_thd.selend.x, _thd.selend.y),(mode << 4) | _cur_train_build_type,
			DC_MSG(STR_1012_CAN_T_REMOVE_RAILROAD_TRACK) | DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_REMOVE_RAILROAD_TRACK);
	} else {
		DoCommand(_thd.selstart.x, _thd.selstart.y, PACK_POINT(_thd.selend.x, _thd.selend.y),(mode << 4) | _cur_train_build_type,
			DC_MSG(STR_1011_CAN_T_BUILD_RAILROAD_TRACK) | DC_EXEC | DC_AUTO | DC_NO_WATER, CMD_BUILD_RAILROAD_TRACK);
	}
}

typedef struct {
	byte bit, a,b, mouse;
} BestFitStruct;

#define M(d,m) (d << 6) | m
static const BestFitStruct _bestfit[] = {
	// both edges have rail
	{2, M(0, 1+8), M(3, 2+8), 0}, // upper track
	{3, M(2, 1+4), M(1, 2+4), 0}, // lower track

	{4, M(2, 1+32), M(3, 2+32), 1<<2}, // left track
	{5, M(0, 1+16), M(1, 2+16), 1<<3}, // right track

	{0, M(0,1+8+16), M(2,1+4+32), 0}, // diag1 track
	{1, M(3,2+8+32), M(1,2+4+16), 0}, // diag2 track
	
	// one edge with rail
	{0, M(0,1), 0, 0}, // diag1 track
	{0, M(2,1), 0, 0}, // diag1 track
	
	{1, M(1,2), 0, 0}, // diag2 track
	{1, M(3,2), 0, 0}, // diag2 track

	{2, M(0,8), 0, 1<<0}, // upper track
	{2, M(3,8), 0, 1<<0}, // upper track

	{3, M(1,4), 0, 1<<1}, // lower track
	{3, M(2,4), 0, 1<<1}, // lower track

	{4, M(2,32), 0, 1<<2}, // left track
	{4, M(3,32), 0, 1<<2}, // left track

	{5, M(0,16), 0, 1<<3}, // right track
	{5, M(1,16), 0, 1<<3}, // right track

	{0xff,0,0},
};
#undef M

static int GetBestArea(int x, int y)
{
	int r = 0;
	if (x + y > 0x10) r += 2;
	else if (x + y == 0x10) return -1;
	if (y - x > 0) r += 1;
	else if (y - x == 0) return -1;
	return r;
}

int GetBestFit1x1(int x, int y)
{
	byte m[5];
	const BestFitStruct *bfs;
	byte mouse;
	uint tile;
	int best;
	int i;

	// determine the mouse regions
	mouse = ((x & 0xF) + (y & 0xF) < 0x10 ? 1 << 0 : 1 << 1) +
					((x & 0xF) > (y & 0xF) ? 1 << 2 : 1 << 3);

	// get the rail in each direction
	tile = TILE_FROM_XY(x,y);
	for(i=0; i!=5; i++) {
		static int16 _tile_inc[5] = {
			TILE_XY(-1, 0),
			TILE_XY(0, 1) - TILE_XY(-1, 0),
			TILE_XY(1, 0) - TILE_XY(0, 1),
			TILE_XY(0, -1) - TILE_XY(1, 0),
			TILE_XY(0, 0) - TILE_XY(0, -1),
		};
		tile += _tile_inc[i];
		m[i] = 0;
		if (IS_TILETYPE(tile, MP_RAILWAY) && _map5[tile] < 0x80) 
			m[i] = _map5[tile]&0x3F;	
		if (_remove_button_clicked) m[i] ^= 0x3F;	
	}

	// check "mouse gesture"?
	{
		int a1,a2;
		if ((a1 = GetBestArea(x & 0xF, y & 0xF)) != -1 && (a2 = GetBestArea(_tile_fract_coords.x, _tile_fract_coords.y)) != -1 && a1 != a2) {
			static const byte _get_dir_by_areas[4][4] = {
				{0,2,4,1},
				{2,0,0,5},
				{4,0,0,3},
				{1,5,3,0},
			};
			i = _get_dir_by_areas[a2][a1];
			if (!HASBIT(m[4], i))
				return i;
		}
	}

	// check each bestfit struct
	for(bfs = _bestfit, best=-1; bfs->bit != 0xFF; bfs++) {
		if ((bfs->a & m[bfs->a >> 6]) && (!bfs->b || bfs->b & m[bfs->b >> 6]) && !HASBIT(m[4], bfs->bit)) {
			if ( (byte)(~mouse & bfs->mouse) == 0)
				return bfs->bit;
			if (best != -1)
				return best;
			best = bfs->bit;
		}
	}

	return best;
}

static void SwapSelection()
{
	TileHighlightData *thd = &_thd;
	Point pt = thd->selstart;
	thd->selstart.x = thd->selend.x & ~0xF;
	thd->selstart.y = thd->selend.y & ~0xF;
	thd->selend = pt;
}

static bool Check2x1AutoRail(int mode)
{
	TileHighlightData *thd = &_thd;
	int fxpy = _tile_fract_coords.x + _tile_fract_coords.y;
	int sxpy = (thd->selend.x & 0xF) + (thd->selend.y & 0xF);
	int fxmy = _tile_fract_coords.x - _tile_fract_coords.y;
	int sxmy = (thd->selend.x & 0xF) - (thd->selend.y & 0xF);
	
	switch(mode) {
	case 0:
		if (fxpy >= 20 && sxpy <= 12) { SwapSelection(); DoRailroadTrack(0); return true; }
		if (fxmy < -3 && sxmy > 3) { DoRailroadTrack(0); return true; }
		break;

	case 1:
		if (fxmy > 3 && sxmy < -3) { SwapSelection(); DoRailroadTrack(0); return true; }
		if (fxpy <= 12 && sxpy >= 20) { DoRailroadTrack(0); return true; }
		break;	

	case 2:
		if (fxmy > 3 && sxmy < -3) { DoRailroadTrack(3); return true; }
		if (fxpy >= 20 && sxpy <= 12) { SwapSelection(); DoRailroadTrack(0); return true; }
		break;

	case 3:
		if (fxmy < -3 && sxmy > 3) { SwapSelection(); DoRailroadTrack(3); return true; }
		if (fxpy <= 12 && sxpy >= 20) { DoRailroadTrack(0); return true; }
		break;
	}

	return false;
}


static void HandleAutodirPlacement()
{
	TileHighlightData *thd = &_thd;
	int bit;

	if (thd->drawstyle == HT_RECT) {
		int dx = thd->selstart.x - (thd->selend.x&~0xF);
		int dy = thd->selstart.y - (thd->selend.y&~0xF);
		
		if (dx == 0 && dy == 0 ) {
			// 1x1 tile
			bit = GetBestFit1x1(thd->selend.x, thd->selend.y);
			if (bit == -1) return;
			GenericPlaceRail(TILE_FROM_XY(thd->selend.x, thd->selend.y), bit);
		} else if (dx == 0) {
			if (dy == -16) {
				if (Check2x1AutoRail(0)) return;
			} else if (dy == 16) {
				if (Check2x1AutoRail(1)) return;
			}
			// same x coordinate
			DoRailroadTrack(VPM_FIX_X);
		} else {
			// same y coordinate
			// check it's it -16 or 16, then we must check if it should be normal tiles or special tiles.
			if (dx == -16) {
				if (Check2x1AutoRail(2)) return;
			} else if (dx == 16) {
				if (Check2x1AutoRail(3)) return;
			}
			DoRailroadTrack(VPM_FIX_Y);
		}
	} else {
		DoRailroadTrack(thd->drawstyle & 1 ? 0 : 3);
	}
}

static OnButtonClick * const _build_railroad_button_proc[] = {
	BuildRailClick_AutoRail,
	BuildRailClick_N,
	BuildRailClick_NE,
	BuildRailClick_E,
	BuildRailClick_NW,
	BuildRailClick_Demolish,
	BuildRailClick_Lower,
	BuildRailClick_Raise,
	BuildRailClick_Depot,
	BuildRailClick_Station,
	BuildRailClick_Signals,
	BuildRailClick_Bridge,
	BuildRailClick_Tunnel,
	BuildRailClick_Remove,
	BuildRailClick_Sign,
	BuildRailClick_Checkpoint,
};

static const uint16 _rail_keycodes[] = {
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	0,	// depot
	0,	// station
	'S',// signals
	'B',// bridge
	'T',// tunnel
	'R',// remove
	0,	// sign
	'C',// checkpoint
};




static void BuildRailToolbWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT:
		w->disabled_state &= ~(1 << 16);
		if (!(w->click_state & ((1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<13)|(1<<18)))) {
			w->disabled_state |= (1 << 16);
			w->click_state &= ~(1<<16);
		}
		DrawWindowWidgets(w);
		break;
	case WE_CLICK: {
		if (e->click.widget >= 3)
			_build_railroad_button_proc[e->click.widget - 3](w);
	} break;

	case WE_KEYPRESS: {
		int i;
		for(i=0; i!=lengthof(_rail_keycodes); i++)
			if (e->keypress.keycode == _rail_keycodes[i]) {
				e->keypress.cont = false;
				_build_railroad_button_proc[i](w);
				break;
			}
		break;
	}

	case WE_PLACE_OBJ:
		_remove_button_clicked = (w->click_state & (1 << 16)) != 0;
		_place_proc(e->place.tile);
		return;

	case WE_PLACE_DRAG: {
		VpSelectTilesWithMethod(e->place.pt.x, e->place.pt.y, e->place.userdata);
		return;
	}

	case WE_PLACE_MOUSEUP:
		if (e->click.pt.x != -1) {
			if (e->place.userdata == VPM_X_OR_Y) {
				ShowBuildBridgeWindow(e->place.pt.x, e->place.pt.y, _cur_train_build_type);
			} else if (e->place.userdata == VPM_RAILDIRS) {
				bool old = _remove_button_clicked;
				if (_ctrl_pressed) _remove_button_clicked = true;
				HandleAutodirPlacement();
				_remove_button_clicked = old;
			} else {
				DoRailroadTrack(e->place.userdata);
			}
		}
		_no_button_sound = true;
		if (e->place.userdata == VPM_FIX_X) {
			BuildRailClick_NW(w);
		} else if (e->place.userdata == VPM_FIX_Y) {
			BuildRailClick_NE(w);
		} else if (e->place.userdata == VPM_RAILDIRS) {
			BuildRailClick_AutoRail(w);
		}
		if (_remove_button_clicked) {
			BuildRailClick_Remove(w);
		}
		_no_button_sound = false;
		break;

	case WE_ABORT_PLACE_OBJ:
		w->click_state = 0;
		SetWindowDirty(w);

		w = FindWindowById(WC_BUILD_STATION, 0);
		if (w != NULL) WP(w,def_d).close=true;
		w = FindWindowById(WC_BUILD_DEPOT, 0);
		if (w != NULL) WP(w,def_d).close=true;
		break;

	case WE_PLACE_PRESIZE: {
		uint tile = e->place.tile;
		DoCommandByTile(tile, 0, 0, DC_AUTO, CMD_BUILD_TUNNEL);
		VpSetPresizeRange(tile, _build_tunnel_endtile==0?tile:_build_tunnel_endtile);
	} break;
	}
}

static const Widget _build_railroad_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   395,     0,    13, STR_100A_RAILROAD_CONSTRUCTION, STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,     7,    22,    25,    14,    35, 0x0, 0x0},
{      WWT_PANEL,     7,     0,    21,    14,    35, SPR_OPENTTD_BASE + 0, STR_BUILD_AUTORAIL_TIP},
{      WWT_PANEL,     7,    26,    47,    14,    35, 0x4E3, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,    48,    69,    14,    35, 0x4E4, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,    70,    91,    14,    35, 0x4E5, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,    92,   113,    14,    35, 0x4E6, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,   114,   135,    14,    35, 0x2BF, STR_018D_DEMOLISH_BUILDINGS_ETC},
{      WWT_PANEL,     7,   136,   157,    14,    35, 0x2B7, STR_018E_LOWER_A_CORNER_OF_LAND},
{      WWT_PANEL,     7,   158,   179,    14,    35, 0x2B6, STR_018F_RAISE_A_CORNER_OF_LAND},
{      WWT_PANEL,     7,   180,   201,    14,    35, 0x50E, STR_1019_BUILD_TRAIN_DEPOT_FOR_BUILDING},

{      WWT_PANEL,     7,   224,   264,    14,    35, 0x512, STR_101A_BUILD_RAILROAD_STATION},
{      WWT_PANEL,     7,   266,   287,    14,    35, 0x50B, STR_101B_BUILD_RAILROAD_SIGNALS},
{      WWT_PANEL,     7,   288,   329,    14,    35, 0xA22, STR_101C_BUILD_RAILROAD_BRIDGE},
{      WWT_PANEL,     7,   330,   351,    14,    35, 0x97E, STR_101D_BUILD_RAILROAD_TUNNEL},
{      WWT_PANEL,     7,   352,   373,    14,    35, 0x2CA, STR_101E_TOGGLE_BUILD_REMOVE_FOR},
{      WWT_PANEL,     7,   374,   395,    14,    35, 0x12B7, STR_0329_PURCHASE_LAND_FOR_FUTURE},

{      WWT_PANEL,     7,   202,   223,    14,    35, SPR_OPENTTD_BASE + 3, STR_CONVERT_RAIL_TO_CHECKPOINT_TIP},
{      WWT_LAST},
};

static const WindowDesc _build_railroad_desc = {
	244, 22, 396, 36,
	WC_BUILD_TOOLBAR,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_railroad_widgets,
	BuildRailToolbWndProc
};

static const Widget _build_monorail_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   395,     0,    13, STR_100B_MONORAIL_CONSTRUCTION, STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,     7,    22,    25,    14,    35, 0x0, 0x0},
{      WWT_PANEL,     7,     0,    21,    14,    35, SPR_OPENTTD_BASE + 1, STR_BUILD_AUTORAIL_TIP},
{      WWT_PANEL,     7,    26,    47,    14,    35, 0x4E7, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,    48,    69,    14,    35, 0x4E8, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,    70,    91,    14,    35, 0x4E9, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,    92,   113,    14,    35, 0x4EA, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,   114,   135,    14,    35, 0x2BF, STR_018D_DEMOLISH_BUILDINGS_ETC},
{      WWT_PANEL,     7,   136,   157,    14,    35, 0x2B7, STR_018E_LOWER_A_CORNER_OF_LAND},
{      WWT_PANEL,     7,   158,   179,    14,    35, 0x2B6, STR_018F_RAISE_A_CORNER_OF_LAND},
{      WWT_PANEL,     7,   180,   201,    14,    35, 0x50E, STR_1019_BUILD_TRAIN_DEPOT_FOR_BUILDING},

{      WWT_PANEL,     7,   224,   265,    14,    35, 0x512, STR_101A_BUILD_RAILROAD_STATION},
{      WWT_PANEL,     7,   266,   287,    14,    35, 0x50B, STR_101B_BUILD_RAILROAD_SIGNALS},
{      WWT_PANEL,     7,   288,   329,    14,    35, 0xA22, STR_101C_BUILD_RAILROAD_BRIDGE},
{      WWT_PANEL,     7,   330,   351,    14,    35, 0x97E, STR_101D_BUILD_RAILROAD_TUNNEL},
{      WWT_PANEL,     7,   352,   373,    14,    35, 0x2CA, STR_101E_TOGGLE_BUILD_REMOVE_FOR},
{      WWT_PANEL,     7,   374,   395,    14,    35, 0x12B7, STR_0329_PURCHASE_LAND_FOR_FUTURE},

{      WWT_PANEL,     7,   202,   223,    14,    35, SPR_OPENTTD_BASE + 3, STR_CONVERT_RAIL_TO_CHECKPOINT_TIP},
{      WWT_LAST},
};

static const WindowDesc _build_monorail_desc = {
	244, 22, 396, 36,
	WC_BUILD_TOOLBAR,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_monorail_widgets,
	BuildRailToolbWndProc
};

static const Widget _build_maglev_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   395,     0,    13, STR_100C_MAGLEV_CONSTRUCTION, STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,     7,    22,    25,    14,    35, 0x0, 0x0},
{      WWT_PANEL,     7,     0,    21,    14,    35, SPR_OPENTTD_BASE + 2, STR_BUILD_AUTORAIL_TIP},
{      WWT_PANEL,     7,    26,    47,    14,    35, 0x4EB, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,    48,    69,    14,    35, 0x4EC, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,    70,    91,    14,    35, 0x4EE, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,    92,   113,    14,    35, 0x4ED, STR_1018_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,     7,   114,   135,    14,    35, 0x2BF, STR_018D_DEMOLISH_BUILDINGS_ETC},
{      WWT_PANEL,     7,   136,   157,    14,    35, 0x2B7, STR_018E_LOWER_A_CORNER_OF_LAND},
{      WWT_PANEL,     7,   158,   179,    14,    35, 0x2B6, STR_018F_RAISE_A_CORNER_OF_LAND},
{      WWT_PANEL,     7,   180,   201,    14,    35, 0x50E, STR_1019_BUILD_TRAIN_DEPOT_FOR_BUILDING},

{      WWT_PANEL,     7,   224,   265,    14,    35, 0x512, STR_101A_BUILD_RAILROAD_STATION},
{      WWT_PANEL,     7,   266,   287,    14,    35, 0x50B, STR_101B_BUILD_RAILROAD_SIGNALS},
{      WWT_PANEL,     7,   288,   329,    14,    35, 0xA22, STR_101C_BUILD_RAILROAD_BRIDGE},
{      WWT_PANEL,     7,   330,   351,    14,    35, 0x97E, STR_101D_BUILD_RAILROAD_TUNNEL},
{      WWT_PANEL,     7,   352,   373,    14,    35, 0x2CA, STR_101E_TOGGLE_BUILD_REMOVE_FOR},
{      WWT_PANEL,     7,   374,   395,    14,    35, 0x12B7, STR_0329_PURCHASE_LAND_FOR_FUTURE},

{      WWT_PANEL,     7,   202,   223,    14,    35, SPR_OPENTTD_BASE + 3, STR_CONVERT_RAIL_TO_CHECKPOINT_TIP},
{      WWT_LAST},
};

static const WindowDesc _build_maglev_desc = {
	244, 22, 396, 36,
	WC_BUILD_TOOLBAR,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_maglev_widgets,
	BuildRailToolbWndProc
};


static const WindowDesc * const _build_rr_desc[] = {
	&_build_railroad_desc,
	&_build_monorail_desc,
	&_build_maglev_desc,
};

void ShowBuildRailToolbar(int index)
{
	assert_array(index, _build_rr_desc);
	DeleteWindowById(WC_BUILD_TOOLBAR, 0);	
	_cur_train_build_type = (byte)index;
	AllocateWindowDesc(_build_rr_desc[index]);
}

static void StationBuildWndProc(Window *w, WindowEvent *e) {
	switch(e->event) {
	case WE_PAINT: {
		uint bits;
		int x,y;

		if (WP(w,def_d).close)
			return;

		bits = (1<<3) << ( _railstation.orientation);
		bits |= (1<<(5-1)) << (_railstation.numtracks);
		bits |= (1<<(12-1)) << (_railstation.platlength);
		bits |= (1<<19) << (_station_show_coverage);
		w->click_state = bits;
		
		x = _railstation.numtracks;
		y = _railstation.platlength;
		if (_railstation.orientation == 0) intswap(x,y);
		SetTileSelectSize(x, y);
		if (_station_show_coverage)
			SetTileSelectBigSize(-4, -4, 8, 8);

		DrawWindowWidgets(w);
		
		StationPickerDrawSprite(39, 42, _cur_train_build_type, 2);
		StationPickerDrawSprite(107, 42, _cur_train_build_type, 3);

		DrawStringCentered(0x4A, 0x0F, STR_3002_ORIENTATION, 0);
		DrawStringCentered(0x4A, 0x4C, STR_3003_NUMBER_OF_TRACKS, 0);
		DrawStringCentered(0x4A, 0x65, STR_3004_PLATFORM_LENGTH, 0);
		DrawStringCentered(0x4A, 0x7E, STR_3066_COVERAGE_AREA_HIGHLIGHT, 0);

		DrawStationCoverageAreaText(2, 151, (uint)-1);
	} break;
		
	case WE_CLICK: {
		switch(e->click.widget) {
		case 0:
			ResetObjectToPlace();
			break;
		case 3:
		case 4:
			_railstation.orientation = e->click.widget - 3;
			SndPlayFx(0x13);
			SetWindowDirty(w);
			break;

		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			_railstation.numtracks = (e->click.widget - 5) + 1;
			SndPlayFx(0x13);
			SetWindowDirty(w);
			break;

		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
			_railstation.platlength = (e->click.widget - 12) + 1;
			SndPlayFx(0x13);
			SetWindowDirty(w);
			break;

		case 19:
		case 20:
			_station_show_coverage = e->click.widget - 19;
			SndPlayFx(0x13);
			SetWindowDirty(w);
			break;
		}
	} break;
	
	case WE_MOUSELOOP: {
		if (WP(w,def_d).close) {
			DeleteWindow(w);
			return;
		}
		CheckRedrawStationCoverage(w);
		} break;
	}
}

static const Widget _station_builder_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   147,     0,    13, STR_3000_RAIL_STATION_SELECTION, STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,     7,     0,   147,    14,   184, 0x0, 0},
{      WWT_PANEL,    14,     7,    72,    26,    73, 0x0, STR_304E_SELECT_RAILROAD_STATION},
{      WWT_PANEL,    14,    75,   140,    26,    73, 0x0, STR_304E_SELECT_RAILROAD_STATION},

{   WWT_CLOSEBOX,    14,    22,    36,    87,    98, STR_00CB_1, STR_304F_SELECT_NUMBER_OF_PLATFORMS},
{   WWT_CLOSEBOX,    14,    37,    51,    87,    98, STR_00CC_2, STR_304F_SELECT_NUMBER_OF_PLATFORMS},
{   WWT_CLOSEBOX,    14,    52,    66,    87,    98, STR_00CD_3, STR_304F_SELECT_NUMBER_OF_PLATFORMS},
{   WWT_CLOSEBOX,    14,    67,    81,    87,    98, STR_00CE_4, STR_304F_SELECT_NUMBER_OF_PLATFORMS},
{   WWT_CLOSEBOX,    14,    82,    96,    87,    98, STR_00CF_5, STR_304F_SELECT_NUMBER_OF_PLATFORMS},
{   WWT_CLOSEBOX,    14,    97,   111,    87,    98, STR_0335_6, STR_304F_SELECT_NUMBER_OF_PLATFORMS},
{   WWT_CLOSEBOX,    14,   112,   126,    87,    98, STR_0336_7, STR_304F_SELECT_NUMBER_OF_PLATFORMS},

{   WWT_CLOSEBOX,    14,    22,    36,   112,   123, STR_00CB_1, STR_3050_SELECT_LENGTH_OF_RAILROAD},
{   WWT_CLOSEBOX,    14,    37,    51,   112,   123, STR_00CC_2, STR_3050_SELECT_LENGTH_OF_RAILROAD},
{   WWT_CLOSEBOX,    14,    52,    66,   112,   123, STR_00CD_3, STR_3050_SELECT_LENGTH_OF_RAILROAD},
{   WWT_CLOSEBOX,    14,    67,    81,   112,   123, STR_00CE_4, STR_3050_SELECT_LENGTH_OF_RAILROAD},
{   WWT_CLOSEBOX,    14,    82,    96,   112,   123, STR_00CF_5, STR_3050_SELECT_LENGTH_OF_RAILROAD},
{   WWT_CLOSEBOX,    14,    97,   111,   112,   123, STR_0335_6, STR_3050_SELECT_LENGTH_OF_RAILROAD},
{   WWT_CLOSEBOX,    14,   112,   126,   112,   123, STR_0336_7, STR_3050_SELECT_LENGTH_OF_RAILROAD},

{   WWT_CLOSEBOX,    14,    14,    73,   137,   148, STR_02DB_OFF, STR_3065_DON_T_HIGHLIGHT_COVERAGE},
{   WWT_CLOSEBOX,    14,    74,   133,   137,   148, STR_02DA_ON, STR_3064_HIGHLIGHT_COVERAGE_AREA},
{      WWT_LAST},
};

static const WindowDesc _station_builder_desc = {
	-1, -1, 0x94, 0xB9,
	WC_BUILD_STATION,WC_BUILD_TOOLBAR,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_station_builder_widgets,
	StationBuildWndProc
};

static void ShowStationBuilder()
{
	AllocateWindowDesc(&_station_builder_desc);
}

static void BuildTrainDepotWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT: {
		int r;

		w->click_state = (1 << 3) << _build_depot_direction;
		DrawWindowWidgets(w);

		r = _cur_train_build_type;
		DrawTrainDepotSprite(70, 17, 0, r);
		DrawTrainDepotSprite(70, 69, 1, r);
		DrawTrainDepotSprite(2, 69, 2, r);
		DrawTrainDepotSprite(2, 17, 3, r);
		break;
		}
	case WE_CLICK: {
		switch(e->click.widget) {
		case 0:
			ResetObjectToPlace();
			break;
		case 3:
		case 4:
		case 5:
		case 6:
			_build_depot_direction = e->click.widget - 3;
			SndPlayFx(0x13);
			SetWindowDirty(w);
			break;
		}
	} break;
	
	case WE_MOUSELOOP:
		if (WP(w,def_d).close)
			DeleteWindow(w);
		return;
	}
}

static const Widget _build_depot_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   139,     0,    13, STR_1014_TRAIN_DEPOT_ORIENTATION,STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,     7,     0,   139,    14,   121, 0x0, 0},
{      WWT_PANEL,    14,    71,   136,    17,    66, 0x0, STR_1020_SELECT_RAILROAD_DEPOT_ORIENTATIO},
{      WWT_PANEL,    14,    71,   136,    69,   118, 0x0, STR_1020_SELECT_RAILROAD_DEPOT_ORIENTATIO},
{      WWT_PANEL,    14,     3,    68,    69,   118, 0x0, STR_1020_SELECT_RAILROAD_DEPOT_ORIENTATIO},
{      WWT_PANEL,    14,     3,    68,    17,    66, 0x0, STR_1020_SELECT_RAILROAD_DEPOT_ORIENTATIO},
{      WWT_LAST},
};

static const WindowDesc _build_depot_desc = {
	-1,-1, 0x8C, 0x7A,
	WC_BUILD_DEPOT,WC_BUILD_TOOLBAR,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_depot_widgets,
	BuildTrainDepotWndProc
};

static void ShowBuildTrainDepotPicker()
{
	AllocateWindowDesc(&_build_depot_desc);
}

void InitializeRailGui()
{
	_build_depot_direction = 3;	
	_railstation.numtracks = 1;
	_railstation.platlength = 1;
}
