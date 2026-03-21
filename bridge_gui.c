#include "stdafx.h"
#include "ttd.h"

#include "window.h"
#include "gui.h"
#include "viewport.h"
#include "gfx.h"
#include "command.h"

static struct BridgeData {
	int count;
	TileIndex start_tile;
	Point end;
	byte type;
	byte indexes[11];
	int32 costs[11];
} _bridge;

extern const PalSpriteID _bridge_sprites[11];
extern const uint16 _bridge_speeds[11];
extern const StringID _bridge_material[11];

static void BuildBridgeWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT: {
		int i;

		DrawWindowWidgets(w);

		for(i=0; i < 4 && i + w->vscroll.pos < _bridge.count; i++) {
			int ind = _bridge.indexes[i + w->vscroll.pos];

			SET_DPARAM32(2, _bridge.costs[i + w->vscroll.pos]);
			SET_DPARAM16(1, (_bridge_speeds[ind] >> 4) * 10);
			SET_DPARAM16(0, _bridge_material[ind]);
			DrawSprite(_bridge_sprites[ind], 3, 15 + i * 22);

			DrawString(44, 15 + i*22 , STR_500D, 0);
		}
	} break;

	case WE_CLICK: {
		 if (e->click.widget == 2) {
			uint ind = ((int)e->click.pt.y - 14) / 22;
			int32 ret;

			if (ind >= 4)
				return;

			ind += w->vscroll.pos;

			if (ind >= (uint)_bridge.count)
				return;

			DeleteWindow(w);

			ret = DoCommand(_bridge.end.x, _bridge.end.y, 
				_bridge.start_tile, _bridge.indexes[ind] | (_bridge.type << 8),
				DC_MSG(STR_5015_CAN_T_BUILD_BRIDGE_HERE) | DC_EXEC | DC_AUTO, CMD_BUILD_BRIDGE);

			if (ret != CMD_ERROR)
				SndPlayTileFx(0x25, TILE_FROM_XY(_bridge.end.x, _bridge.end.y));
		}
	} break;
	}
}

static const Widget _build_bridge_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5,										STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   199,     0,    13, STR_100D_SELECT_RAIL_BRIDGE,	STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_MATRIX,     7,     0,   188,    14,   101, 0x401,												STR_101F_BRIDGE_SELECTION_CLICK},
{  WWT_SCROLLBAR,     7,   189,   199,    14,   101, 0x0,													STR_0190_SCROLL_BAR_SCROLLS_LIST},
{      WWT_LAST},
};

static const WindowDesc _build_bridge_desc = {
	-1, -1, 0xc8, 0x66,
	WC_BUILD_BRIDGE,WC_BUILD_TOOLBAR,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_bridge_widgets,
	BuildBridgeWndProc
};


static const Widget _build_road_bridge_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5,										STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   199,     0,    13, STR_1803_SELECT_ROAD_BRIDGE,	STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_MATRIX,     7,     0,   188,    14,   101, 0x401,												STR_101F_BRIDGE_SELECTION_CLICK},
{  WWT_SCROLLBAR,     7,   189,   199,    14,   101, 0x0,													STR_0190_SCROLL_BAR_SCROLLS_LIST},
{      WWT_LAST},
};

static const WindowDesc _build_road_bridge_desc = {
	-1, -1, 0xc8, 0x66,
	WC_BUILD_BRIDGE,WC_BUILD_TOOLBAR,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_road_bridge_widgets,
	BuildBridgeWndProc
};


void ShowBuildBridgeWindow(int x, int y, byte type)
{
	int i, j;
	int32 ret;
	uint16 errmsg;

	DeleteWindowById(WC_BUILD_BRIDGE, 0);

	x &= ~0xF;
	y &= ~0xF;

	_bridge.type = type;
	_bridge.end.x = x;
	_bridge.end.y = y;
	_bridge.start_tile = TILE_FROM_XY(_thd.selstart.x, _thd.selstart.y);

	errmsg = 0xFFFF;

	for(i=j=0; i!=11; i++) {
		ret = DoCommand(x, y, _bridge.start_tile, i | (type << 8), DC_AUTO | DC_QUERY_COST, CMD_BUILD_BRIDGE);

		if (ret == CMD_ERROR) {
			if (_error_message != STR_500C)
				errmsg = _error_message;
		} else {
			_bridge.costs[j] = ret;
			_bridge.indexes[j] = i;
			j++;
		}
	}

	_bridge.count = j;

	if (j != 0) {
		Window *w = AllocateWindowDesc(type&0x80 ? &_build_road_bridge_desc : &_build_bridge_desc);	
		w->vscroll.cap = 4;
		w->vscroll.count = (byte)j;
	} else {
		ShowErrorMessage(errmsg, STR_5015_CAN_T_BUILD_BRIDGE_HERE, x, y);
	}
}
