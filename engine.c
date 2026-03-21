#include "stdafx.h"
#include "ttd.h"
#include "engine.h"
#include "table/engines.h"
#include "player.h"
#include "command.h"
#include "vehicle.h"
#include "news.h"
#include "saveload.h"

#define UPDATE_PLAYER_RAILTYPE(e,p) if ((byte)(e->railtype + 1) > p->max_railtype) p->max_railtype = e->railtype + 1;

enum {
	ENGINE_AVAILABLE = 1,
	ENGINE_INTRODUCING = 2,
};

void ShowEnginePreviewWindow(int engine);

void DeleteCustomEngineNames()
{
	uint i;
	StringID old;

	for(i=0; i!=TOTAL_NUM_ENGINES; i++) {
		old = _engine_name_strings[i];
		_engine_name_strings[i] = i + STR_8000_KIRBY_PAUL_TANK_STEAM;
		DeleteName(old);
	}

	_vehicle_design_names &= ~1;
}

void LoadCustomEngineNames()
{
	// XXX: not done */
	DEBUG(misc, 1) ("LoadCustomEngineNames: not done");
}

static void SetupEngineNames()
{
	uint i;

	for(i=0; i!=TOTAL_NUM_ENGINES; i++)
		_engine_name_strings[i] = STR_SV_EMPTY;

	DeleteCustomEngineNames();
	LoadCustomEngineNames();
}

static void AdjustAvailAircraft()
{
	uint16 date = _date;
	byte avail = _avail_aircraft;
	byte avail_org = avail;

	if (date >= 12784) avail |= 2; /* enable big airport */
	if (date >= 14610) avail &= ~1;/* disable small airport */
	if (date >= 15706) avail |= 4; /* enable heliport */

	if (avail != avail_org) {
		_avail_aircraft = avail;
		InvalidateWindow(WC_BUILD_STATION, 0);
	}
}

static void CalcEngineReliability(Engine *e)
{
	uint age = e->age;

	if (age < e->duration_phase_1) {
		uint start = e->reliability_start;
		e->reliability = age * (e->reliability_max - start) / e->duration_phase_1 + start;
	} else if ((age -= e->duration_phase_1) < e->duration_phase_2) {
		e->reliability = e->reliability_max;
	} else if ((age -= e->duration_phase_2) < e->duration_phase_3) {
		uint max = e->reliability_max;
		e->reliability = age * (e->reliability_final - max) / e->duration_phase_3 + max;
	} else {
		e->player_avail = 0;
		e->reliability = e->reliability_final;
	}
}

void StartupEngines()
{
	Engine *e;
	const EngineInfo *ei;
	uint32 r;

	SetupEngineNames();


	for(e=_engines, ei=_engine_info; e != endof(_engines); e++,ei++) {
		
		e->age = 0;

		e->railtype = ei->railtype_climates >> 4;
		e->flags = 0;
		e->player_avail = 0;
		
		r = Random();
		e->intro_date = (uint16)((r & 0x1FF) + ei->base_intro);
		if (e->intro_date <= _date) {
			e->age = (_date - e->intro_date) >> 5;
			e->player_avail = (byte)-1;
			e->flags |= 1;
		}

		e->reliability_start = (uint16)(((r >> 16) & 0x3fff) + 0x7AE0);
		r = Random();
		e->reliability_max = (uint16)((r & 0x3fff) + 0xbfff);
		e->reliability_final = (uint16)(((r>>16) & 0x3fff) + 0x3fff);

		r = Random();
		e->duration_phase_1 = (uint16)((r & 0x1F) + 7);
		e->duration_phase_2 = (uint16)(((r >> 5) & 0xF) + ei->base_life * 12 - 96);
		e->duration_phase_3 = (uint16)(((r >> 9) & 0x7F) + 120);

		e->reliability_spd_dec = (ei->unk2&0x7F) << 2;

		/* my invented flag for something that is a wagon */
		if (ei->unk2 & 0x80) {
			e->age = 0xFFFF;
		} else {
			CalcEngineReliability(e);
		}

		e->lifelength = ei->lifelength;

		if (!HASBIT(ei->railtype_climates, _opt.landscape)) {
			e->flags |= 1;
			e->player_avail = 0;
		}
	}

	_avail_aircraft = 7;
	AdjustAvailAircraft();
}

void AcceptEnginePreview(Engine *e, int player)
{
	Player *p;

	SETBIT(e->player_avail, player);

	p = DEREF_PLAYER(player);
	
	UPDATE_PLAYER_RAILTYPE(e,p);

	e->preview_player = 0xFF;
	InvalidateWindowClasses(WC_BUILD_VEHICLE);
}

void EnginesDailyLoop()
{
	Engine *e;
	int i,num;
	Player *p;
	uint mask;
	int32 best_hist;
	int best_player;

	if (_cur_year >= 130)
		return;

	for(e=_engines,i=0; i!=TOTAL_NUM_ENGINES; e++,i++) {
		if (e->flags & 2) {
			if (e->flags & 4) {
				if (!--e->preview_wait) {
					e->flags &= ~4;
					DeleteWindowById(WC_ENGINE_PREVIEW, i);
					e->preview_player++;
				}	
			} else if (e->preview_player != 0xFF) {
				num = e->preview_player;
				mask = 0;
				do {
					best_hist = -1;
					best_player = -1;
					FOR_ALL_PLAYERS(p) {
						if (p->name_1 != 0 && p->block_preview == 0 && !HASBIT(mask,p->index) && 
								p->old_economy[0].performance_history > best_hist) {
							best_hist = p->old_economy[0].performance_history;
							best_player = p->index;
						}
					}
					if (best_player == -1) {
						e->preview_player = 0xFF;
						goto next_engine;
					}
					mask |= (1 << best_player);
				} while (--num != 0);
				
				if (!IS_HUMAN_PLAYER(best_player)) {
					/* TTDBUG: TTD has a bug here */
					AcceptEnginePreview(e, best_player);
				} else {
					e->flags |= 4;
					e->preview_wait = 20;
					if (IS_INTERACTIVE_PLAYER(best_player)) {
						ShowEnginePreviewWindow(i);					
					}
				}
			}
		}
		next_engine:;
	}
}

int32 CmdWantEnginePreview(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (flags & DC_EXEC) {
		AcceptEnginePreview(&_engines[p1], _current_player);
	}
	return 0;
}

void NewVehicleAvailable(Engine *e)
{
	Vehicle *v;
	Player *p;
	int index = e - _engines;

	// In case the player didn't build the vehicle during the intro period,
	// prevent that player from getting future intro periods for a while.
	if (e->flags&ENGINE_INTRODUCING)
	{
		FOR_ALL_PLAYERS(p) {
			if (!HASBIT(e->player_avail,p->index))
				continue;
		
			for(v=_vehicles;;) {
				if (v->type == VEH_Train || v->type == VEH_Road || v->type == VEH_Ship ||
						(v->type == VEH_Aircraft && v->subtype <= 2)) {
					if (v->owner == p->index && v->engine_type == index) break;
				}
				if (++v == endof(_vehicles)) {
					p->block_preview = 20;
					break;
				}
			}
		}
	}

	// Now available for all players
	e->player_avail = (byte)-1;
	FOR_ALL_PLAYERS(p) {
		if (p->name_1 != 0)
			UPDATE_PLAYER_RAILTYPE(e,p);
	}

	e->flags = (e->flags & ~ENGINE_INTRODUCING) | ENGINE_AVAILABLE;
	
	if ((byte)index < NUM_TRAIN_ENGINES) {
		AddNewsItem(index, NEWS_FLAGS(NM_CALLBACK, 0, NT_NEW_VEHICLES, DNC_TRAINAVAIL), 0, 0);
	} else if ((byte)index < NUM_TRAIN_ENGINES + NUM_ROAD_ENGINES) {
		AddNewsItem(index, NEWS_FLAGS(NM_CALLBACK, 0, NT_NEW_VEHICLES, DNC_ROADAVAIL), 0, 0);
	} else if ((byte)index < NUM_TRAIN_ENGINES + NUM_ROAD_ENGINES + NUM_SHIP_ENGINES) {
		AddNewsItem(index, NEWS_FLAGS(NM_CALLBACK, 0, NT_NEW_VEHICLES, DNC_SHIPAVAIL), 0, 0);
	} else {
		AddNewsItem(index, NEWS_FLAGS(NM_CALLBACK, 0, NT_NEW_VEHICLES, DNC_AIRCRAFTAVAIL), 0, 0);
	}

	InvalidateWindowClasses(WC_BUILD_VEHICLE);
}

void EnginesMonthlyLoop()
{
	Engine *e;

	if (_cur_year >= 130)
		return;

	for(e=_engines; e != endof(_engines); e++) {
		// Age the vehicle
		if (e->flags&ENGINE_AVAILABLE && e->age != 0xFFFF) {
			e->age++;
			CalcEngineReliability(e);
		}

		// Introduction date has passed?
		if (!(e->flags & (ENGINE_AVAILABLE|ENGINE_INTRODUCING)) && _date >= e->intro_date) {
			e->flags |= ENGINE_INTRODUCING;
			e->preview_player = 1; // Give to the player with the highest rating.
		}

		// Introduce it to all players
		if (!(e->flags & ENGINE_AVAILABLE) && (uint16)(_date - 365) >= e->intro_date) {
			NewVehicleAvailable(e);
		}
	}
	AdjustAvailAircraft();
}

int32 CmdRenameEngine(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	StringID str;

	str = AllocateName((byte*)_decode_parameters, 0);
	if (str == 0)
		return CMD_ERROR;
	
	if (flags & DC_EXEC) {
		StringID old_str = _engine_name_strings[p1];
		_engine_name_strings[p1] = str;
		DeleteName(old_str);
		_vehicle_design_names |= 3;
		MarkWholeScreenDirty();
	} else {
		DeleteName(str);
	}

	return 0;
}

int GetPlayerMaxRailtype(int p)
{
	Engine *e;
	int rt = 0;
	int i;

	for(e=_engines,i=0; i!=lengthof(_engines); e++,i++) {
		if (!HASBIT(e->player_avail, p))
			continue;

		if (i >= 27 && i < 54)
			continue;

		if (i >= 57 && i < 84)
			continue;

		if (i >= 89 && i < 116)
			continue;

		if (rt < e->railtype)
			rt = e->railtype;
	}

	return rt + 1;
}


static const byte _engine_desc[] = {
	SLE_VAR(Engine,intro_date,						SLE_UINT16),
	SLE_VAR(Engine,age,										SLE_UINT16),
	SLE_VAR(Engine,reliability,						SLE_UINT16),
	SLE_VAR(Engine,reliability_spd_dec,		SLE_UINT16),
	SLE_VAR(Engine,reliability_start,			SLE_UINT16),
	SLE_VAR(Engine,reliability_max,				SLE_UINT16),
	SLE_VAR(Engine,reliability_final,			SLE_UINT16),
	SLE_VAR(Engine,duration_phase_1,			SLE_UINT16),
	SLE_VAR(Engine,duration_phase_2,			SLE_UINT16),
	SLE_VAR(Engine,duration_phase_3,			SLE_UINT16),

	SLE_VAR(Engine,lifelength,						SLE_UINT8),
	SLE_VAR(Engine,flags,									SLE_UINT8),
	SLE_VAR(Engine,preview_player,				SLE_UINT8),
	SLE_VAR(Engine,preview_wait,					SLE_UINT8),
	SLE_VAR(Engine,railtype,							SLE_UINT8),
	SLE_VAR(Engine,player_avail,					SLE_UINT8),

	SLE_END()
};

static void Save_ENGN()
{
	Engine *e;
	int i;
	for(i=0,e=_engines; i != lengthof(_engines); i++,e++) {
		SlSetArrayIndex(i);
		SlObject(e, _engine_desc);
	}
}

static void Load_ENGN()
{
	int index;
	while ((index = SlIterateArray()) != -1) {
		SlObject(&_engines[index], _engine_desc);
	}
}

static void LoadSave_ENGS()
{
	SlArray(_engine_name_strings, lengthof(_engine_name_strings), SLE_STRINGID);
}

const ChunkHandler _engine_chunk_handlers[] = {
	{ 'ENGN', Save_ENGN, Load_ENGN, CH_ARRAY},
	{ 'ENGS', LoadSave_ENGS, LoadSave_ENGS, CH_RIFF | CH_LAST},
};



