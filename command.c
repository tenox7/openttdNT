#include "stdafx.h"
#include "ttd.h"
#include "gui.h"
#include "command.h"
#include "player.h"

int _docommand_recursive;
byte _shift_key = 1;

#define DEF_COMMAND(yyyy) int32 yyyy(int x, int y, uint32 flags, uint32 p1, uint32 p2)

DEF_COMMAND(CmdBuildRailroadTrack);
DEF_COMMAND(CmdRemoveRailroadTrack);
DEF_COMMAND(CmdBuildSingleRail);
DEF_COMMAND(CmdRemoveSingleRail);

DEF_COMMAND(CmdLandscapeClear);

DEF_COMMAND(CmdBuildBridge);
DEF_COMMAND(CmdClearBridgeTile);

DEF_COMMAND(CmdBuildRailroadStation);

DEF_COMMAND(CmdBuildSignals);
DEF_COMMAND(CmdRemoveSignals);

DEF_COMMAND(CmdTerraformLand);

DEF_COMMAND(CmdPurchaseLandArea);
DEF_COMMAND(CmdSellLandArea);

DEF_COMMAND(CmdBuildTunnel);
DEF_COMMAND(CmdClearTunnelTile);

DEF_COMMAND(CmdBuildTrainDepot);
DEF_COMMAND(CmdBuildTrainCheckpoint);
DEF_COMMAND(CmdRenameCheckpoint);
DEF_COMMAND(CmdRemoveTrainCheckpoint);

DEF_COMMAND(CmdBuildTruckStation);

DEF_COMMAND(CmdBuildBusStation);

DEF_COMMAND(CmdBuildLongRoad);
DEF_COMMAND(CmdRemoveLongRoad);
DEF_COMMAND(CmdBuildRoad);
DEF_COMMAND(CmdRemoveRoad);

DEF_COMMAND(CmdBuildRoadDepot);

DEF_COMMAND(CmdBuildAirport);

DEF_COMMAND(CmdBuildDock);

DEF_COMMAND(CmdBuildShipDepot);

DEF_COMMAND(CmdBuildBuoy);

DEF_COMMAND(CmdPlantTree);

DEF_COMMAND(CmdBuildRailVehicle);
DEF_COMMAND(CmdMoveRailVehicle);

DEF_COMMAND(CmdStartStopTrain);

DEF_COMMAND(CmdSellRailLoco);
DEF_COMMAND(CmdSellRailWagon);

DEF_COMMAND(CmdTrainGotoDepot);
DEF_COMMAND(CmdForceTrainProceed);
DEF_COMMAND(CmdReverseTrainDirection);

DEF_COMMAND(CmdModifyTrainOrder);
DEF_COMMAND(CmdSkipTrainOrder);
DEF_COMMAND(CmdDeleteTrainOrder);
DEF_COMMAND(CmdInsertTrainOrder);
DEF_COMMAND(CmdChangeTrainServiceInt);

DEF_COMMAND(CmdBuildIndustry);

DEF_COMMAND(CmdBuildCompanyHQ);
DEF_COMMAND(CmdSetPlayerFace);
DEF_COMMAND(CmdSetPlayerColor);

DEF_COMMAND(CmdIncreaseLoan);
DEF_COMMAND(CmdDecreaseLoan);

DEF_COMMAND(CmdWantEnginePreview);

DEF_COMMAND(CmdNameVehicle);
DEF_COMMAND(CmdRenameEngine);

DEF_COMMAND(CmdChangeCompanyName);
DEF_COMMAND(CmdChangePresidentName);

DEF_COMMAND(CmdRenameStation);

DEF_COMMAND(CmdSellAircraft);
DEF_COMMAND(CmdStartStopAircraft);
DEF_COMMAND(CmdBuildAircraft);
DEF_COMMAND(CmdSendAircraftToHangar);
DEF_COMMAND(CmdChangeAircraftServiceInt);
DEF_COMMAND(CmdRefitAircraft);

DEF_COMMAND(CmdPlaceSign);
DEF_COMMAND(CmdRenameSign);

DEF_COMMAND(CmdBuildRoadVeh);
DEF_COMMAND(CmdStartStopRoadVeh);
DEF_COMMAND(CmdSellRoadVeh);
DEF_COMMAND(CmdSendRoadVehToDepot);
DEF_COMMAND(CmdTurnRoadVeh);
DEF_COMMAND(CmdChangeRoadVehServiceInt);

DEF_COMMAND(CmdPauseOrResume);

DEF_COMMAND(CmdBuyShareInCompany);
DEF_COMMAND(CmdSellShareInCompany);
DEF_COMMAND(CmdBuyCompany);

DEF_COMMAND(CmdBuildTown);
DEF_COMMAND(CmdBuildTownHouse);
DEF_COMMAND(CmdClearTownHouse);
DEF_COMMAND(CmdRenameTown);
DEF_COMMAND(CmdDoTownAction);

DEF_COMMAND(CmdSetRoadDriveSide);
DEF_COMMAND(CmdSetCityNameType);
DEF_COMMAND(CmdExitToGameMenu);
DEF_COMMAND(CmdChangeDifficultyLevel);

DEF_COMMAND(CmdStartStopShip);
DEF_COMMAND(CmdSellShip);
DEF_COMMAND(CmdBuildShip);
DEF_COMMAND(CmdSendShipToDepot);
DEF_COMMAND(CmdChangeShipServiceInt);
DEF_COMMAND(CmdRefitShip);


DEF_COMMAND(CmdStartNewGame);
DEF_COMMAND(CmdLoadGame);
DEF_COMMAND(CmdCreateScenario);
DEF_COMMAND(CmdSetSinglePlayer);
DEF_COMMAND(CmdQuitGame);
DEF_COMMAND(CmdSetNewLandscapeType);

DEF_COMMAND(CmdGenRandomNewGame);
DEF_COMMAND(CmdCloneOrder);

/* The master command table */
static CommandProc * const _command_proc_table[] = {
	CmdBuildRailroadTrack,			/* 0 */
	CmdRemoveRailroadTrack,			/* 1 */
	CmdBuildSingleRail,				/* 2 */
	CmdRemoveSingleRail,			/* 3 */
	CmdLandscapeClear,				/* 4 */
	CmdBuildBridge,					/* 5 */
	CmdBuildRailroadStation,		/* 6 */
	CmdBuildTrainDepot,				/* 7 */
	CmdBuildSignals,				/* 8 */
	CmdRemoveSignals,				/* 9 */
	CmdTerraformLand,				/* 10 */
	CmdPurchaseLandArea,			/* 11 */
	CmdSellLandArea,				/* 12 */
	CmdBuildTunnel,					/* 13 */
	CmdClearTunnelTile,				/* 14 */
	CmdClearBridgeTile,				/* 15 */
	CmdBuildTrainCheckpoint,	/* 16 */
	CmdRenameCheckpoint,				/* 17 */
	CmdRemoveTrainCheckpoint,		/* 18 */
	CmdBuildTruckStation,			/* 19 */
	NULL,							/* 20 */
	CmdBuildBusStation,				/* 21 */
	NULL,							/* 22 */
	CmdBuildLongRoad,				/* 23 */
	CmdRemoveLongRoad,				/* 24 */
	CmdBuildRoad,					/* 25 */
	CmdRemoveRoad,					/* 26 */
	CmdBuildRoadDepot,				/* 27 */
	NULL,							/* 28 */
	CmdBuildAirport,				/* 29 */
	CmdBuildDock,					/* 30 */
	CmdBuildShipDepot,				/* 31 */
	CmdBuildBuoy,					/* 32 */
	CmdPlantTree,					/* 33 */
	CmdBuildRailVehicle,			/* 34 */
	CmdMoveRailVehicle,				/* 35 */
	CmdStartStopTrain,				/* 36 */
	CmdSellRailLoco,				/* 37 */
	CmdSellRailWagon,				/* 38 */
	CmdTrainGotoDepot,				/* 39 */
	CmdForceTrainProceed,			/* 40 */
	CmdReverseTrainDirection,		/* 41 */

	CmdModifyTrainOrder,			/* 42 */
	CmdSkipTrainOrder,				/* 43 */
	CmdDeleteTrainOrder,			/* 44 */
	CmdInsertTrainOrder,			/* 45 */

	CmdChangeTrainServiceInt,		/* 46 */

	CmdBuildIndustry,				/* 47 */
	CmdBuildCompanyHQ,				/* 48 */
	CmdSetPlayerFace,				/* 49 */
	CmdSetPlayerColor,				/* 50 */

	CmdIncreaseLoan,				/* 51 */
	CmdDecreaseLoan,				/* 52 */

	CmdWantEnginePreview,			/* 53 */

	CmdNameVehicle,					/* 54 */
	CmdRenameEngine,				/* 55 */

	CmdChangeCompanyName,			/* 56 */
	CmdChangePresidentName,			/* 57 */

	CmdRenameStation,				/* 58 */

	CmdSellAircraft,				/* 59 */
	CmdStartStopAircraft,			/* 60 */
	CmdBuildAircraft,				/* 61 */
	CmdSendAircraftToHangar,		/* 62 */
	CmdChangeAircraftServiceInt,	/* 63 */
	CmdRefitAircraft,				/* 64 */

	CmdPlaceSign,					/* 65 */
	CmdRenameSign,					/* 66 */

	CmdBuildRoadVeh,				/* 67 */
	CmdStartStopRoadVeh,			/* 68 */
	CmdSellRoadVeh,					/* 69 */
	CmdSendRoadVehToDepot,			/* 70 */
	CmdTurnRoadVeh,					/* 71 */
	CmdChangeRoadVehServiceInt,		/* 72 */

	CmdPauseOrResume,				/* 73 */

	CmdBuyShareInCompany,			/* 74 */
	CmdSellShareInCompany,			/* 75 */
	CmdBuyCompany,					/* 76 */

	CmdBuildTown,					/* 77 */
	CmdBuildTownHouse,				/* 78 */
	CmdClearTownHouse,				/* 79 */
	CmdRenameTown,					/* 80 */
	CmdDoTownAction,				/* 81 */

	CmdSetRoadDriveSide,			/* 82 */
	CmdSetCityNameType,				/* 83 */
	CmdExitToGameMenu,				/* 84 */
	CmdChangeDifficultyLevel,		/* 85 */

	CmdStartStopShip,				/* 86 */
	CmdSellShip,					/* 87 */	
	CmdBuildShip,					/* 88 */
	CmdSendShipToDepot,				/* 89 */
	CmdChangeShipServiceInt,		/* 90 */
	CmdRefitShip,					/* 91 */

	CmdStartNewGame,				/* 92 */
	CmdLoadGame,					/* 93 */
	CmdCreateScenario,				/* 94 */
	CmdSetSinglePlayer,				/* 95 */
	CmdQuitGame,					/* 96 */
	CmdSetNewLandscapeType,			/* 97 */

	CmdGenRandomNewGame,			/* 98 */

	CmdCloneOrder,					/* 99 */
};

int32 DoCommandByTile(TileIndex tile, uint32 p1, uint32 p2, uint32 flags, uint procc)
{
	return DoCommand(GET_TILE_X(tile)*16, GET_TILE_Y(tile)*16, p1, p2, flags, procc);
} 


//extern void _stdcall Sleep(int s);

int32 DoCommand(int x, int y, uint32 p1, uint32 p2, uint32 flags, uint procc)
{
	int32 res;
	CommandProc *proc;
	
	proc = _command_proc_table[procc];

	if (flags & DC_SET_ERR) {
		flags ^= DC_SET_ERR;
		_error_message_2 = (StringID)(flags >> 16);
	}

	if (_docommand_recursive == 0) {
		_error_message = INVALID_STRING_ID;

		if (flags & DC_EXEC && _current_player == _local_player) {
			if (_game_mode!=GM_MENU && _shift_key == 0) {
				/* handle_shift_key */
				_docommand_recursive++;
				res = proc(x, y, flags&~DC_EXEC, p1, p2);
				if (--_docommand_recursive != 0)
					return res;
				if (res != CMD_ERROR) {
					ShowEstimatedCostOrIncome(res, x, y);
				} else {
					ShowErrorMessage(_error_message, _error_message_2, x, y);
				}
				return CMD_ERROR;
			}
			NetworkSend(x, y, p1, p2, flags, procc);
//			Sleep(200);
		} else if (_game_mode == GM_NORMAL && _current_player != _local_player && !(flags & DC_QUERY_COST)) {
			// Commands can only be executed from the state game loop.
			assert(_in_state_game_loop);
		}
	}

	/* update last build coord of player */
	if ( (x|y) != 0 && _current_player < 8) {
		DEREF_PLAYER(_current_player)->last_build_coordinate = TILE_FROM_XY(x,y);
	}

	_docommand_recursive++;

	res = proc(x, y, flags&~DC_EXEC, p1, p2);
	if ((uint32)res >> 16 == 0x8000) {
		if (res & 0xFFFF)
			_error_message = res & 0xFFFF;
		goto show_error;
	}

	if (_docommand_recursive==1 && !(flags&DC_QUERY_COST) && res != 0) {
		if (!CheckPlayerHasMoney(res))
			goto show_error;
	}

	if (!(flags & DC_EXEC)) {
		_docommand_recursive--;
		return res;
	}

	_yearly_expenses_type = 0;
	proc(x, y, flags, p1, p2);

	if (--_docommand_recursive == 0) {
		SubtractMoneyFromPlayer(res);
		if (_current_player == _local_player && res != 0 && _game_mode != GM_EDITOR)
			ShowCostOrIncomeAnimation(x, y, GetSlopeZ(x, y), res);
	}

	return res;

show_error:;
	if (--_docommand_recursive == 0 && (flags & DC_EXEC) && _current_player == _local_player) {
		ShowErrorMessage(_error_message, _error_message_2, x, y);
	}
	return CMD_ERROR;
}
