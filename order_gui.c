#include "stdafx.h"
#include "ttd.h"
#include "window.h"
#include "gui.h"
#include "gfx.h"
#include "vehicle.h"
#include "station.h"
#include "city.h"
#include "command.h"
#include "viewport.h"

static int OrderGetSel(Window *w)
{
	Vehicle *v = &_vehicles[w->window_number];
	uint16 *sched = v->schedule_ptr;
	int num = WP(w,order_d).sel;
	int count = 0;

	if (num == 0)
		return 0;

	while (*sched != 0) {
		sched++;
		count++;
		if (--num == 0)
			break;
	}

	return count;
}

static void DrawOrdersWindow(Window *w)
{
	Vehicle *v;
	int num, sel;
	uint16 *sched, ord;
	int y, i;
	StringID str;
	bool shared_schedule;

	v = &_vehicles[w->window_number];

	w->disabled_state = (v->owner == _local_player) ? 0 : 0x3F0;

	shared_schedule = IsScheduleShared(v) != NULL;

	sched = v->schedule_ptr;
	num=0;
	while (*sched != 0)
		sched++,num++;

	if ((uint)num + shared_schedule <= (uint)WP(w,order_d).sel)
		SETBIT(w->disabled_state, 5); /* delete */

	if (num == 0)
		SETBIT(w->disabled_state, 4); /* skip */

	SetVScrollCount(w, num+1);

	sel = OrderGetSel(w);
	
	switch(v->schedule_ptr[sel] & 0x1F) {
	case OT_GOTO_STATION:
		break;
	case OT_GOTO_DEPOT:
		SETBIT(w->disabled_state, 8);	/* full load */
		SETBIT(w->disabled_state, 9);	/* unload */
		break;
	default:
		SETBIT(w->disabled_state, 6); /* nonstop */
		SETBIT(w->disabled_state, 8);	/* full load */
		SETBIT(w->disabled_state, 9);	/* unload */				
	}

	


	SET_DPARAM16(0, v->string_id);
	SET_DPARAM16(1, v->unitnumber);
	DrawWindowWidgets(w);

	y = 15;

	i = 0;
	for(;;) {
		str = ((byte)v->cur_order_index == i) ? STR_8805 : STR_8804;
		
		ord = v->schedule_ptr[i];

		if ( (uint)(i - w->vscroll.pos) < 6) {
			
			if (ord == 0) {
				str = shared_schedule ? STR_END_OF_SHARED_ORDERS : STR_882A_END_OF_ORDERS;
			} else {
				SET_DPARAM16(1, 6);

				if ( (ord & OT_MASK) == OT_GOTO_STATION) {
					SET_DPARAM16(1, STR_8806_GO_TO + ((ord >> 5) & 7));
					SET_DPARAM16(2, ord >> 8);
				} else if ((ord & OT_MASK) == OT_GOTO_DEPOT) {
					StringID s;
					SET_DPARAM16(2, _depots[ord >> 8].city_index);
					s = (v->type == VEH_Train) ? STR_880E_GO_TO_TRAIN_DEPOT : STR_9038_GO_TO_ROADVEH_DEPOT ;
					SET_DPARAM16(1, s + ((ord>>7)&1) );
				} else if ((ord & OT_MASK) == OT_GOTO_CHECKPOINT) {
					SET_DPARAM16(2, ord >> 8);
					SET_DPARAM16(1, STR_GO_TO_CHECKPOINT);
				}
			}
			{
				byte color = (i == WP(w,order_d).sel) ? 0xC : 0x10;
				SET_DPARAM(0, i+1);
				DrawString(2, y, str, color);
			}
			y += 10;
		}

		i++;

		if (ord == 0)
			break;
	}
}


// lookup a vehicle on a tile
typedef struct {
	TileIndex tile;
	byte owner;
	byte type;
} FindVehS;

static void *FindVehicleCallb(Vehicle *v, FindVehS *f)
{
	if (v->tile != f->tile || v->owner != f->owner || v->vehstatus & VS_HIDDEN ) return NULL;
	return v;
} 

Vehicle *GetVehicleOnTile(TileIndex tile, byte owner)
{
	FindVehS fs = {tile, owner};
	return VehicleFromPos(tile, &fs, (VehicleFromPosProc*)FindVehicleCallb);
}

static uint GetOrderCmdFromTile(Vehicle *v, uint tile)
{
	Station *st;
	int st_index;
	Vehicle *u;

	if (IS_TILETYPE(tile, MP_STATION) || IS_TILETYPE(tile+=TILE_XY(1,0), MP_STATION) ||
			IS_TILETYPE(tile+=TILE_XY(-1,1), MP_STATION) || 	IS_TILETYPE(tile+=TILE_XY(1,0), MP_STATION)) {
		st = DEREF_STATION(st_index = _map2[tile]);

		if (st->owner == _current_player) {
			byte facil;
			(facil=FACIL_DOCK, v->type == VEH_Ship) ||
			(facil=FACIL_TRAIN, v->type == VEH_Train) ||
			(facil=FACIL_AIRPORT, v->type == VEH_Aircraft) ||
			(facil=FACIL_BUS_STOP, v->type == VEH_Road && v->cargo_type == CT_PASSENGERS) ||
			(facil=FACIL_TRUCK_STOP, 1);
			if (st->facilities & facil)
				return st_index;
		}
	} else {
		tile += TILE_XY(-1,-1);

		if (_patches.gotodepot) {
			if (IS_TILETYPE(tile, MP_RAILWAY)) {
				if (v->type == VEH_Train && _map_owner[tile] == _local_player) {
					if ((_map5[tile]&0xFC)==0xC0)
						return GetDepotByTile(tile) | 0x100;

					if ((_map5[tile]&0xFE)==0xC4)
						return GetCheckpointByTile(tile) | 0x200;

				}
			} else if (IS_TILETYPE(tile, MP_STREET)) {
				if ((_map5[tile] & 0xF0) == 0x20 && v->type == VEH_Road && _map_owner[tile] == _local_player)
					return GetDepotByTile(tile) | 0x100;
			}
		}
	}

	u = GetVehicleOnTile(tile, v->owner);
	if (u && v->type == u->type) {
		// find the first vehicle in a chain
		if (v->type == VEH_Train && v->subtype != 0)
			u = GetFirstVehicleInChain(u);
		// share orders with another vehicle
		return u->index | 0x80000000;
	}

	// not found
	return (uint)-1;
}

static void OrdersPlaceObj(Vehicle *v, uint tile, Window *w)
{
	uint cmd = GetOrderCmdFromTile(v, tile);
	if ( cmd == (uint)-1) return;

	if (cmd & 0x80000000) {
		// clone orders
		if (DoCommandByTile(v->tile, v->index | (cmd << 16), _ctrl_pressed ? 0 : 1, 
			_ctrl_pressed ? DC_EXEC | DC_MSG(STR_CANT_SHARE_ORDER_LIST) : DC_EXEC | DC_MSG(STR_CANT_COPY_ORDER_LIST), CMD_CLONE_ORDER) == CMD_ERROR)
			return;
		WP(w,order_d).sel = -1;
		ResetObjectToPlace();
	}  else {
		if (DoCommandByTile(v->tile, v->index, (cmd << 8) | OrderGetSel(w), DC_EXEC | DC_MSG(STR_8833_CAN_T_INSERT_NEW_ORDER), CMD_INSERT_TRAIN_ORDER)== CMD_ERROR)
			return;

		if (WP(w,order_d).sel != -1)
			WP(w,order_d).sel++;

		ResetObjectToPlace();
	}

}


static void OrdersWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT:
		DrawOrdersWindow(w);
		break;

	case WE_CLICK: {
		Vehicle *v = &_vehicles[w->window_number];
		int mode;
		switch(e->click.widget) {
		case 2:	{/* orders list */	
			int sel;
			sel = (e->click.pt.y - 15) / 10;

			if ( (uint) sel >= 6)
				return;

			sel += w->vscroll.pos;

			if (sel == WP(w,order_d).sel) sel = -1;
			WP(w,order_d).sel = sel;
			SetWindowDirty(w);
		}	break;

		case 4: /* skip button */
			DoCommandByTile(v->tile,v->index, 0, DC_EXEC, CMD_SKIP_TRAIN_ORDER);
			break;

		case 5: /* delete button */
			DoCommandByTile(v->tile,v->index, OrderGetSel(w), 
				DC_EXEC | DC_MSG(STR_8834_CAN_T_DELETE_THIS_ORDER), CMD_DELETE_TRAIN_ORDER);
			break;

		case 7: /* goto button */
			InvalidateWidget(w, 7);
			w->click_state ^= 1<<7;
			if (HASBIT(w->click_state, 7)) {
				SetObjectToPlaceWnd(ANIMCURSOR_PICKSTATION, 1, w);
			} else {
				ResetObjectToPlace();
			}
			break;

		case 8: /* full load button */
			mode = 0;
			DoCommandByTile(v->tile, v->index, OrderGetSel(w) | (mode << 8), 
				DC_EXEC | DC_MSG(STR_8835_CAN_T_MODIFY_THIS_ORDER), CMD_MODIFY_TRAIN_ORDER);
			break;

		case 9: /* unload button */
			mode = 1;
			DoCommandByTile(v->tile, v->index, OrderGetSel(w) | (mode << 8),
				DC_EXEC | DC_MSG(STR_8835_CAN_T_MODIFY_THIS_ORDER), CMD_MODIFY_TRAIN_ORDER);
			break;

		case 6: /* non stop button */
			mode = 2;
			DoCommandByTile(v->tile, v->index, OrderGetSel(w) | (mode << 8),
				DC_EXEC | DC_MSG(STR_8835_CAN_T_MODIFY_THIS_ORDER), CMD_MODIFY_TRAIN_ORDER);
			break;
		}
	} break;

	case WE_4: {
		if (FindWindowById(WC_VEHICLE_VIEW, w->window_number) == NULL)
			DeleteWindow(w);
	} break;

	case WE_PLACE_OBJ: {
		OrdersPlaceObj(&_vehicles[w->window_number], e->place.tile, w);
	} break;

	case WE_ABORT_PLACE_OBJ: {
		w->click_state &= ~(1<<7);
		InvalidateWidget(w, 7);
	} break;
	}
}

static const Widget _train_orders_widgets[] = {
{   WWT_CLOSEBOX,    14,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,    14,    11,   319,     0,    13, STR_8829_ORDERS, STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,    14,     0,   308,    14,    75, 0x0, STR_8852_ORDERS_LIST_CLICK_ON_ORDER},
{  WWT_SCROLLBAR,    14,   309,   319,    14,    75, 0x0, STR_0190_SCROLL_BAR_SCROLLS_LIST},
{ WWT_PUSHTXTBTN,    14,     0,    52,    76,    87, STR_8823_SKIP, STR_8853_SKIP_THE_CURRENT_ORDER},
{ WWT_PUSHTXTBTN,    14,    53,   105,    76,    87, STR_8824_DELETE, STR_8854_DELETE_THE_HIGHLIGHTED},
{ WWT_PUSHTXTBTN,    14,   106,   158,    76,    87, STR_8825_NON_STOP, STR_8855_MAKE_THE_HIGHLIGHTED_ORDER},
{WWT_NODISTXTBTN,    14,   159,   211,    76,    87, STR_8826_GO_TO, STR_8856_INSERT_A_NEW_ORDER_BEFORE},
{ WWT_PUSHTXTBTN,    14,   212,   264,    76,    87, STR_8827_FULL_LOAD, STR_8857_MAKE_THE_HIGHLIGHTED_ORDER},
{ WWT_PUSHTXTBTN,    14,   265,   319,    76,    87, STR_8828_UNLOAD, STR_8858_MAKE_THE_HIGHLIGHTED_ORDER},
{      WWT_LAST},
};

static const WindowDesc _train_orders_desc = {
	-1,-1, 0x140, 0x58,
	WC_VEHICLE_ORDERS,WC_VEHICLE_VIEW,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_train_orders_widgets,
	OrdersWndProc
};

static const Widget _other_train_orders_widgets[] = {
{   WWT_CLOSEBOX,    14,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,    14,    11,   319,     0,    13, STR_8829_ORDERS, STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,    14,     0,   308,    14,    75, 0x0, STR_8852_ORDERS_LIST_CLICK_ON_ORDER},
{  WWT_SCROLLBAR,    14,   309,   319,    14,    75, 0x0, STR_0190_SCROLL_BAR_SCROLLS_LIST},
{      WWT_LAST},
};

static const WindowDesc _other_train_orders_desc = {
	-1,-1, 0x140, 0x4C,
	WC_VEHICLE_ORDERS,WC_VEHICLE_VIEW,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_other_train_orders_widgets,
	OrdersWndProc
};


static const Widget _roadveh_orders_widgets[] = {
{    WWT_TEXTBTN,    14,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,    14,    11,   319,     0,    13, STR_900B_ORDERS, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,    14,     0,   308,    14,    75, 0x0,STR_8852_ORDERS_LIST_CLICK_ON_ORDER},
{  WWT_SCROLLBAR,    14,   309,   319,    14,    75, 0x0, STR_0190_SCROLL_BAR_SCROLLS_LIST},
{ WWT_PUSHTXTBTN,    14,     0,    63,    76,    87, STR_8823_SKIP, STR_8853_SKIP_THE_CURRENT_ORDER},
{ WWT_PUSHTXTBTN,    14,    64,   127,    76,    87, STR_8824_DELETE, STR_8854_DELETE_THE_HIGHLIGHTED},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{WWT_NODISTXTBTN,    14,   128,   191,    76,    87, STR_8826_GO_TO, STR_8856_INSERT_A_NEW_ORDER_BEFORE},
{ WWT_PUSHTXTBTN,    14,   192,   255,    76,    87, STR_8827_FULL_LOAD, STR_8857_MAKE_THE_HIGHLIGHTED_ORDER},
{ WWT_PUSHTXTBTN,    14,   256,   319,    76,    87, STR_8828_UNLOAD, STR_8858_MAKE_THE_HIGHLIGHTED_ORDER},
{      WWT_LAST},
};

static const WindowDesc _roadveh_orders_desc = {
	-1,-1, 0x140, 0x58,
	WC_VEHICLE_ORDERS,WC_VEHICLE_VIEW,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_roadveh_orders_widgets,
	OrdersWndProc
};

static const Widget _other_roadveh_orders_widgets[] = {
{    WWT_TEXTBTN,    14,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,    14,    11,   319,     0,    13, STR_900B_ORDERS, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,    14,     0,   308,    14,    75, 0x0,STR_8852_ORDERS_LIST_CLICK_ON_ORDER},
{  WWT_SCROLLBAR,    14,   309,   319,    14,    75, 0x0, STR_0190_SCROLL_BAR_SCROLLS_LIST},
{      WWT_LAST},
};

static const WindowDesc _other_roadveh_orders_desc = {
	-1,-1, 0x140, 0x4C,
	WC_VEHICLE_ORDERS,WC_VEHICLE_VIEW,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_other_roadveh_orders_widgets,
	OrdersWndProc
};

static const Widget _ship_orders_widgets[] = {
{    WWT_TEXTBTN,    14,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,    14,    11,   319,     0,    13, STR_9810_ORDERS, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,    14,     0,   308,    14,    75, 0x0,STR_8852_ORDERS_LIST_CLICK_ON_ORDER},
{  WWT_SCROLLBAR,    14,   309,   319,    14,    75, 0x0, STR_0190_SCROLL_BAR_SCROLLS_LIST},
{ WWT_PUSHTXTBTN,    14,     0,    63,    76,    87, STR_8823_SKIP, STR_8853_SKIP_THE_CURRENT_ORDER},
{ WWT_PUSHTXTBTN,    14,    64,   127,    76,    87, STR_8824_DELETE, STR_8854_DELETE_THE_HIGHLIGHTED},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{WWT_NODISTXTBTN,    14,   128,   191,    76,    87, STR_8826_GO_TO, STR_8856_INSERT_A_NEW_ORDER_BEFORE},
{ WWT_PUSHTXTBTN,    14,   192,   255,    76,    87, STR_8827_FULL_LOAD, STR_8857_MAKE_THE_HIGHLIGHTED_ORDER},
{ WWT_PUSHTXTBTN,    14,   256,   319,    76,    87, STR_8828_UNLOAD, STR_8858_MAKE_THE_HIGHLIGHTED_ORDER},
{      WWT_LAST},
};

static const WindowDesc _ship_orders_desc = {
	-1,-1, 0x140, 0x58,
	WC_VEHICLE_ORDERS,WC_VEHICLE_VIEW,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_ship_orders_widgets,
	OrdersWndProc
};

static const Widget _other_ship_orders_widgets[] = {
{    WWT_TEXTBTN,    14,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,    14,    11,   319,     0,    13, STR_9810_ORDERS, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,    14,     0,   308,    14,    75, 0x0,STR_8852_ORDERS_LIST_CLICK_ON_ORDER},
{  WWT_SCROLLBAR,    14,   309,   319,    14,    75, 0x0, STR_0190_SCROLL_BAR_SCROLLS_LIST},
{      WWT_LAST},
};

static const WindowDesc _other_ship_orders_desc = {
	-1,-1, 0x140, 0x4C,
	WC_VEHICLE_ORDERS,WC_VEHICLE_VIEW,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_other_ship_orders_widgets,
	OrdersWndProc
};


static const Widget _aircraft_orders_widgets[] = {
{    WWT_TEXTBTN,    14,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,    14,    11,   319,     0,    13, STR_A00B_ORDERS, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,    14,     0,   308,    14,    75, 0x0, STR_8852_ORDERS_LIST_CLICK_ON_ORDER},
{  WWT_SCROLLBAR,    14,   309,   319,    14,    75, 0x0, STR_0190_SCROLL_BAR_SCROLLS_LIST},
{ WWT_PUSHTXTBTN,    14,     0,    63,    76,    87, STR_8823_SKIP, STR_8853_SKIP_THE_CURRENT_ORDER},
{ WWT_PUSHTXTBTN,    14,    64,   127,    76,    87, STR_8824_DELETE, STR_8854_DELETE_THE_HIGHLIGHTED},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{WWT_NODISTXTBTN,    14,   128,   191,    76,    87, STR_8826_GO_TO, STR_8856_INSERT_A_NEW_ORDER_BEFORE},
{ WWT_PUSHTXTBTN,    14,   192,   255,    76,    87, STR_8827_FULL_LOAD, STR_8857_MAKE_THE_HIGHLIGHTED_ORDER},
{ WWT_PUSHTXTBTN,    14,   256,   319,    76,    87, STR_8828_UNLOAD, STR_8858_MAKE_THE_HIGHLIGHTED_ORDER},
{      WWT_LAST},
};

static const WindowDesc _aircraft_orders_desc = {
	-1,-1, 0x140, 0x58,
	WC_VEHICLE_ORDERS,WC_VEHICLE_VIEW,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_aircraft_orders_widgets,
	OrdersWndProc
};

static const Widget _other_aircraft_orders_widgets[] = {
{    WWT_TEXTBTN,    14,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,    14,    11,   319,     0,    13, STR_A00B_ORDERS, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,    14,     0,   308,    14,    75, 0x0, STR_8852_ORDERS_LIST_CLICK_ON_ORDER},
{  WWT_SCROLLBAR,    14,   309,   319,    14,    75, 0x0, STR_0190_SCROLL_BAR_SCROLLS_LIST},
{      WWT_LAST},
};

static const WindowDesc _other_aircraft_orders_desc = {
	-1,-1, 0x140, 0x4C,
	WC_VEHICLE_ORDERS,WC_VEHICLE_VIEW,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_other_aircraft_orders_widgets,
	OrdersWndProc
};

static const WindowDesc * const _order_window_desc[8] = {
	&_train_orders_desc, 	&_other_train_orders_desc,
	&_roadveh_orders_desc,   &_other_roadveh_orders_desc,
	&_ship_orders_desc,   &_other_ship_orders_desc,
	&_aircraft_orders_desc, &_other_aircraft_orders_desc,
};


void ShowOrdersWindow(Vehicle *v)
{
	Window *w;
	VehicleID veh = v->index;

	DeleteWindowById(WC_VEHICLE_ORDERS, veh);
	DeleteWindowById(WC_VEHICLE_DETAILS, veh);
	
	_alloc_wnd_parent_num = veh;
	w = AllocateWindowDesc(
		_order_window_desc[(v->type - VEH_Train)*2 + (v->owner != _local_player)]);

	w->window_number = veh;
	w->caption_color = v->owner;
	w->vscroll.cap = 6;
	WP(w,order_d).sel = -1;
}
