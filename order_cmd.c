#include "stdafx.h"
#include "ttd.h"
#include "vehicle.h"
#include "command.h"
#include "station.h"
#include "player.h"

/* p1 = vehicle
 * p2 & 0xFF = sel
 * p2 >> 8 = station
 */
int32 CmdInsertTrainOrder(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v = &_vehicles[p1];
	int sel = (byte)p2;
	int t;

	if ((byte)sel > v->num_orders)
		return_cmd_error(STR_EMPTY);

	if (_ptr_to_next_order == endof(_order_array))
		return_cmd_error(STR_8831_NO_MORE_SPACE_FOR_ORDERS);

	if (v->num_orders >= 40)
		return_cmd_error(STR_8832_TOO_MANY_ORDERS);

	if (v->type == VEH_Ship && IS_HUMAN_PLAYER(v->owner) &&
			sel != 0 && ((t=v->schedule_ptr[sel-1])&OT_MASK) == OT_GOTO_STATION) {
		
		int dist = GetTileDist(DEREF_STATION(t >> 8)->xy, DEREF_STATION(p2 >> 8)->xy);
		if (dist >= 130)
			return_cmd_error(STR_0210_TOO_FAR_FROM_PREVIOUS_DESTINATIO);
	}

	if (flags & DC_EXEC) {
		uint16 *s1, *s2;
		Vehicle *u;

		s1 = &v->schedule_ptr[sel];
		s2 = _ptr_to_next_order++;

		do s2[1] = s2[0]; while (--s2 >= s1);

		if (p2 & 0x20000) {
			s1[0] = (p2 & 0xFF00) | OT_GOTO_CHECKPOINT;
		} else if (p2 & 0x10000) {
			s1[0] = (p2 & 0xFF00) | OT_GOTO_DEPOT | OF_UNLOAD;
		} else {
			s1[0] = (p2 & 0xFF00) | OT_GOTO_STATION;
		}

		s1 = v->schedule_ptr;
		
		for(u=_vehicles; u != endof(_vehicles); u++) {
			if (u->type != 0 && u->schedule_ptr != NULL) {
				if (s1 < u->schedule_ptr) {
					u->schedule_ptr++;
				} else if (s1 == u->schedule_ptr) { // handle shared orders
					u->num_orders++;

					if ((byte)sel <= u->cur_order_index) {
						sel++;
						if ((byte)sel < u->num_orders)
							u->cur_order_index = sel;
					}
					InvalidateWindow(WC_VEHICLE_VIEW, u->index);
					InvalidateWindow(WC_VEHICLE_ORDERS, u->index);
				}
			}
		}
	}

	return 0;
}

static int32 DecloneOrder(Vehicle *dst, uint32 flags)
{
	if (_ptr_to_next_order == endof(_order_array))
		return_cmd_error(STR_8831_NO_MORE_SPACE_FOR_ORDERS);

	if (flags & DC_EXEC) {
		DeleteVehicleSchedule(dst);
		
		dst->num_orders = 0;
		*(dst->schedule_ptr = _ptr_to_next_order++) = 0;
		
		InvalidateWindow(WC_VEHICLE_ORDERS, dst->index);
	}
	return 0;
}

/* p1 = vehicle
 * p2 = sel
 */
int32 CmdDeleteTrainOrder(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v = &_vehicles[p1], *u;
	uint sel = (uint)p2;

	_error_message = STR_EMPTY;
	if (sel >= v->num_orders)
		return DecloneOrder(v, flags);

	if (flags & DC_EXEC) {
		uint16 *s1;
		
		s1 = &v->schedule_ptr[sel];

		// copy all orders to get rid of the hole
		do s1[0] = s1[1]; while (++s1 != _ptr_to_next_order);
		_ptr_to_next_order--;

		s1 = v->schedule_ptr;

		for(u=_vehicles; u!=endof(_vehicles); u++) {
			if (u->type != 0 && u->schedule_ptr != NULL) {
				if (s1 < u->schedule_ptr) {
					u->schedule_ptr--;
				} else if (s1 == u->schedule_ptr) {// handle shared orders
					u->num_orders--;
					if ((byte)sel < u->cur_order_index)
						u->cur_order_index--;

					if ((byte)sel == u->cur_order_index && (u->next_order&(OT_MASK|OF_NON_STOP)) == (OT_LOADING|OF_NON_STOP))
						u->next_order = OT_LOADING;

					InvalidateWindow(WC_VEHICLE_VIEW, u->index);
					InvalidateWindow(WC_VEHICLE_ORDERS, u->index);
				}
			}
		}
	}
	
	return 0;
}

/* p1 = vehicle */
int32 CmdSkipTrainOrder(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (flags & DC_EXEC) {
		Vehicle *v = &_vehicles[p1];

		{
			byte b = v->cur_order_index + 1;
			if (b >= v->num_orders) b = 0;
			v->cur_order_index = b;
			v->u.rail.days_since_order_progr = 0;
		}

		if ((v->next_order&(OT_MASK|OF_NON_STOP)) == (OT_LOADING|OF_NON_STOP))
			v->next_order = OT_LOADING;

		InvalidateWindow(WC_VEHICLE_ORDERS, v->index);
	}
	return 0;
}

/* p1 = vehicle
 * p2&0xFF = sel
 * p2>>8 = mode
 */
int32 CmdModifyTrainOrder(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *v = &_vehicles[p1];
	byte sel = (byte)p2;
	uint16 *sched;

	if (sel >= v->num_orders)
		return CMD_ERROR;

	sched = &v->schedule_ptr[sel];
	if (!((*sched & OT_MASK) == OT_GOTO_STATION || 
			((*sched & OT_MASK) == OT_GOTO_DEPOT && (p2>>8) == 2)))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		switch(p2 >> 8) {
		case 0: // full load
			*sched ^= OF_FULL_LOAD;
			*sched &= ~OF_UNLOAD;
			break;
		case 1: // unload
			*sched ^= OF_UNLOAD;
			*sched &= ~OF_FULL_LOAD;
			break;
		case 2: // non stop
			*sched ^= OF_NON_STOP;
			break;
		}
		sched = v->schedule_ptr;

		for(v=_vehicles; v!=endof(_vehicles); v++) {
			if (v->schedule_ptr == sched)
				InvalidateWindow(WC_VEHICLE_ORDERS, v->index);
		}
		
	}
	
	return 0;
}

// Clone an order
// p1 & 0xFFFF is destination vehicle
// p1 >> 16 is source vehicle

// p2 is
//   0 - clone
//   1 - copy
//   2 - unclone


int32 CmdCloneOrder(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Vehicle *dst = &_vehicles[p1 & 0xFFFF];
	
	if (!(dst->type && dst->owner == _current_player))
		return CMD_ERROR;

	switch(p2) {
	
	// share vehicle orders?
	case 0: {
		Vehicle *src = &_vehicles[p1 >> 16];

		// sanity checks
		if (!(src->owner == _current_player && dst->type == src->type && dst != src))
			return CMD_ERROR;

		if (flags & DC_EXEC) {
			DeleteVehicleSchedule(dst);
			dst->schedule_ptr = src->schedule_ptr;
			dst->num_orders = src->num_orders;

			InvalidateWindow(WC_VEHICLE_ORDERS, src->index);
			InvalidateWindow(WC_VEHICLE_ORDERS, dst->index);
		}
		break;
	}

	// copy vehicle orders?
	case 1: {
		Vehicle *src = &_vehicles[p1 >> 16];
		int delta;

		// sanity checks
		if (!(src->owner == _current_player && dst->type == src->type && dst != src))
			return CMD_ERROR;

		// make sure there's orders available
		delta = IsScheduleShared(dst) ? src->num_orders + 1 : src->num_orders - dst->num_orders;
		if (delta > endof(_order_array) - _ptr_to_next_order)
			return_cmd_error(STR_8831_NO_MORE_SPACE_FOR_ORDERS);

		if (flags & DC_EXEC) {
			DeleteVehicleSchedule(dst);
			dst->schedule_ptr = _ptr_to_next_order;
			dst->num_orders = src->num_orders;
			_ptr_to_next_order += src->num_orders + 1;
			memcpy(dst->schedule_ptr, src->schedule_ptr, (src->num_orders + 1) * sizeof(uint16));

			InvalidateWindow(WC_VEHICLE_ORDERS, dst->index);
		}
		break;
	}

	// declone vehicle orders?
	case 2: return DecloneOrder(dst, flags);
	}

	return 0;
}

void BackupVehicleOrders(Vehicle *v, uint16 *bak)
{
	Vehicle *u = IsScheduleShared(v);
	uint16 *sched, ord;

	// stored shared orders in this special way?
	if (u) {
		bak[0] = 0xFFFF;
		bak[1] = u->index;
		return;
	}
	
	sched = v->schedule_ptr;
	do {
		ord = *sched++;
		*bak++ = ord;
	} while (ord != 0);
}

void RestoreVehicleOrders(Vehicle *v, uint16 *bak)
{
	uint16 ord;
	int ind = 0;

	if (bak[0] == 0xFFFF) {
		DoCommandByTile(0, v->index | bak[1]<<16, 0, DC_EXEC, CMD_CLONE_ORDER);
		return;
	}

	while ((ord = *bak++) != 0) {
		if ((ord & OT_MASK) == OT_GOTO_STATION) {
			
			if (DoCommandByTile(0, v->index, ind | (ord & 0xFF00), DC_EXEC, CMD_INSERT_TRAIN_ORDER) == CMD_ERROR)
				break;

			if (ord & OF_NON_STOP)
				DoCommandByTile(0, v->index, ind | 0x200, DC_EXEC, CMD_MODIFY_TRAIN_ORDER);

			if (ord & OF_FULL_LOAD) 
				DoCommandByTile(0, v->index, ind | 0x000, DC_EXEC, CMD_MODIFY_TRAIN_ORDER);

			if (ord & OF_UNLOAD)
				DoCommandByTile(0, v->index, ind | 0x100, DC_EXEC, CMD_MODIFY_TRAIN_ORDER);

		}
		ind++;
	}
}
