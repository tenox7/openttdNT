#include "stdafx.h"
#include "ttd.h"

#include "window.h"
#include "gui.h"
#include "viewport.h"
#include "gfx.h"
#include "player.h"
#include "command.h"
#include "news.h"
#include "city.h"
#include "vehicle.h"

#include "table/animcursors.h"


extern void DoTestSave();
extern void DoTestLoad();

extern bool disable_computer;

static int _rename_id;
static int _rename_what;

static byte _terraform_size = 1;
static byte _last_built_railtype;
extern void GenerateWorld(bool empty);

static void HandleOnEditText(WindowEvent *e) {
	byte *b = e->edittext.str;
	int id;
	memcpy(_decode_parameters, b, 32);

	id = _rename_id;
	
	switch(_rename_what) {
	case 0:
		DoCommandByTile(0, id, 0, DC_EXEC | DC_MSG(STR_280C_CAN_T_CHANGE_SIGN_NAME), CMD_RENAME_SIGN);
		break;
	case 1:
		DoCommandByTile(0, id, 0, DC_EXEC | DC_MSG(STR_CANT_CHANGE_CHECKPOINT_NAME), CMD_RENAME_CHECKPOINT);
		break;
	}
}

// this code is shared for the majority of the pushbuttons
bool HandlePlacePushButton(Window *w, int widget, uint32 cursor, int mode, PlaceProc *placeproc)
{
	uint32 mask = 1 << widget;

	if (w->disabled_state & mask)
		return false;

	if (!_no_button_sound) SndPlayFx(0x13);
	SetWindowDirty(w);

	if (w->click_state & mask) {
		ResetObjectToPlace();
		return false;
	}

	SetObjectToPlace(cursor, mode, w->window_class, w->window_number);
	w->click_state |= mask;
	_place_proc = placeproc;
	return true;
}


typedef void ToolbarButtonProc(Window *w);

static void ToolbarPauseClick(Window *w)
{
	DoCommandByTile(0, 0, 0, DC_EXEC, CMD_PAUSE_OR_RESUME);
	SndPlayFx(0x13);
}


typedef void MenuClickedProc(int index);


void MenuClickSettings(int index)
{
	switch(index) {
	case 0: ShowGameOptions(); return;
	case 1: ShowGameDifficulty(); return;
	case 2: ShowPatchesSelection(); return;
	case 4: _display_opt ^= DO_SHOW_TOWN_NAMES; MarkWholeScreenDirty(); return;
	case 5: _display_opt ^= DO_SHOW_STATION_NAMES; MarkWholeScreenDirty(); return;
	case 6: _display_opt ^= DO_SHOW_SIGNS; MarkWholeScreenDirty(); return;
	case 7: _display_opt ^= DO_CHECKPOINTS; MarkWholeScreenDirty(); return;
	case 8: _display_opt ^= DO_FULL_ANIMATION; MarkWholeScreenDirty(); return;
	case 9: _display_opt ^= DO_FULL_DETAIL; MarkWholeScreenDirty(); return;
	case 10: _display_opt ^= DO_TRANS_BUILDINGS; MarkWholeScreenDirty(); return;
	}
}

void MenuClickSaveLoad(int index)
{
	if (_game_mode == GM_EDITOR) {
		switch(index) {
		case 0:
			ShowSaveLoadDialog(SLD_SAVE_SCENARIO);
			break;
		case 1:
			ShowSaveLoadDialog(SLD_LOAD_SCENARIO);
			break;
		case 2:
			DoCommandByTile(0, 0, 0, DC_EXEC, CMD_EXIT_TO_GAME_MENU);
			break;
		case 4:
			AskExitGame();
			break;
		}
	} else {
		switch(index) {
		case 0:
			ShowSaveLoadDialog(SLD_SAVE_GAME);
			break;
		case 1:
			ShowSaveLoadDialog(SLD_LOAD_GAME);
			break;
		case 2:
			DoCommandByTile(0, 0, 0, DC_EXEC, CMD_EXIT_TO_GAME_MENU);
			break;
		case 3:
			AskExitGame();
			break;
		}
	}
}

void MenuClickMap(int index)
{
	switch(index) {
	case 0: ShowSmallMap(); break;
	}
}

void MenuClickTown(int index)
{
	ShowTownDirectory();
}

void MenuClickScenMap(int index)
{
	switch(index) {
	case 0: ShowSmallMap(); break;
	case 1: ShowTownDirectory(); break;
	}
}

void MenuClickSubsidies(int index)
{
	ShowSubsidiesList();
}

void MenuClickStations(int index)
{
	ShowPlayerStations(index);
}

void MenuClickFinances(int index)
{
	ShowPlayerFinances(index);
}

void MenuClickCompany(int index)
{
	ShowPlayerCompany(index);
}
	

void MenuClickGraphs(int index)
{
	switch(index) {
	case 0: ShowOperatingProfitGraph(); return;
	case 1: ShowIncomeGraph(); return;
	case 2: ShowDeliveredCargoGraph(); return;
	case 3: ShowPerformanceHistoryGraph(); return;
	case 4: ShowCompanyValueGraph(); return;
	case 5: ShowCargoPaymentRates(); return;
	}
}

void MenuClickLeague(int index)
{
	ShowCompanyLeagueTable();
}

void MenuClickIndustry(int index)
{
	ShowBuildIndustryWindow();
}

void MenuClickShowTrains(int index)
{
	ShowPlayerTrains(index);
}

void MenuClickShowRoad(int index)
{
	ShowPlayerRoadVehicles(index);
}

void MenuClickShowShips(int index)
{
	ShowPlayerShips(index);
}

void MenuClickShowAir(int index)
{
	ShowPlayerAircraft(index);
}

void MenuClickBuildRail(int index)
{
	_last_built_railtype = index;
	ShowBuildRailToolbar(index);
}

void MenuClickBuildRoad(int index)
{
	ShowBuildRoadToolbar();
}

void MenuClickBuildWater(int index)
{
	ShowBuildDocksToolbar();
}

void MenuClickBuildAir(int index)
{
	ShowBuildAirToolbar();
}

void ShowRenameSignWindow(SignStruct *ss)
{
	_rename_id = ss - _sign_list;
	_rename_what = 0;
	ShowQueryString(ss->str, STR_280B_EDIT_SIGN_TEXT, 30, 180, 1, 0);
}

void ShowRenameCheckpointWindow(Checkpoint *cp)
{
	int id = cp - _checkpoints;
	_rename_id = id;
	_rename_what = 1;
	SET_DPARAM16(0, id);
	ShowQueryString(STR_CHECKPOINT_RAW, STR_EDIT_CHECKPOINT_NAME, 30, 180, 1, 0);
}


static void PlaceProc_Sign(uint tile)
{
	int32 ret = DoCommandByTile(tile, 0, 0, DC_EXEC | DC_MSG(STR_2809_CAN_T_PLACE_SIGN_HERE), CMD_PLACE_SIGN);
	if (ret != CMD_ERROR) {
		ShowRenameSignWindow(_new_sign_struct);
		ResetObjectToPlace();
	}
}

static void SelectSignTool()
{
	if (_cursor.sprite == 0x2D2)
		ResetObjectToPlace();
	else {
		SetObjectToPlace(0x2D2, 1, 1, 0);
		_place_proc = PlaceProc_Sign;
	}
}

void MenuClickForest(int index)
{
	if (index == 0) {
		ShowBuildTreesToolbar();
	} else {
		SelectSignTool();
	}
}

void MenuClickMusicWindow(int index)
{
	ShowMusicWindow();
}

void MenuClickNewspaper(int index)
{
	switch(index) {
	case 0: ShowLastNewsMessage(); break;
	case 1: ShowMessageOptions(); break;
	case 2: ; /* XXX: chat not done */
	}
}

void MenuClickHelp(int index)
{
	switch(index) {
	case 0: PlaceLandBlockInfo(); break;
	case 2: _switch_mode = SM_SCREENSHOT; break;
	case 3: _switch_mode = SM_BIG_SCREENSHOT; break;
	case 4: ShowAboutWindow(); break;
	}
}

static MenuClickedProc * const _menu_clicked_procs[] = {
	NULL, /* 0 */
	MenuClickSettings, /* 1 */
	MenuClickSaveLoad, /* 2 */
	MenuClickMap, /* 3 */
	MenuClickTown, /* 4 */
	MenuClickSubsidies, /* 5 */
	MenuClickStations, /* 6 */
	MenuClickFinances, /* 7 */
	MenuClickCompany, /* 8 */
	MenuClickGraphs, /* 9 */
	MenuClickLeague, /* 10 */
	MenuClickIndustry, /* 11 */
	MenuClickShowTrains, /* 12 */
	MenuClickShowRoad, /* 13 */
	MenuClickShowShips, /* 14 */
	MenuClickShowAir, /* 15 */
	MenuClickScenMap,  /* 16 */
	NULL, /* 17 */
	MenuClickBuildRail, /* 18 */
	MenuClickBuildRoad, /* 19 */
	MenuClickBuildWater, /* 20 */
	MenuClickBuildAir, /* 21 */
	MenuClickForest, /* 22 */
	MenuClickMusicWindow, /* 23 */
	MenuClickNewspaper, /* 24 */
	MenuClickHelp, /* 25 */
};

static void MenuWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT: {
		int count,sel;
		int x,y;
		uint16 chk;
		StringID string;
		int eo;
		int inc;

		DrawWindowWidgets(w);

		count = WP(w,menu_d).item_count;
		sel = WP(w,menu_d).sel_index;
		chk = WP(w,menu_d).checked_items;
		string = WP(w,menu_d).string_id;

		x = 1;
		y = 1;

		eo = 157;

		inc = (chk != 0) ? 2 : 1;

		do {
			if (sel== 0) GfxFillRect(x, y, x + eo, y+9, 0);
			DrawString(x + 2, y, (StringID)(string + (chk&1)), (byte)(sel==0?(byte)0xC:(byte)0x10));
			y += 10;
			string += inc;
			chk >>= 1;
		} while (--sel,--count);
	} break;

	case WE_DESTROY: {
			Window *v = FindWindowById(WC_MAIN_TOOLBAR, 0);
			v->click_state &= ~(1 << WP(w,menu_d).main_button);
			SetWindowDirty(v);
			return;
		}
		
	case WE_POPUPMENU_SELECT: {
		int index = GetMenuItemIndex(w, e->popupmenu.pt.x, e->popupmenu.pt.y);
		int action_id;
		
		if (index < 0) {
			Window *w2 = FindWindowById(WC_MAIN_TOOLBAR,0);
			if (GetWidgetFromPos(w2, e->popupmenu.pt.x - w2->left, e->popupmenu.pt.y - w2->top) == WP(w,menu_d).main_button)
				index = WP(w,menu_d).sel_index;
		}

		action_id = WP(w,menu_d).action_id;
		DeleteWindow(w);
		
		if (index >= 0)
			_menu_clicked_procs[action_id](index);
	
		break;
		}
	case WE_POPUPMENU_OVER: {
		int index = GetMenuItemIndex(w, e->popupmenu.pt.x, e->popupmenu.pt.y);

		if (index == -1 || index == WP(w,menu_d).sel_index)
			return;

		WP(w,menu_d).sel_index = index;
		SetWindowDirty(w);
		return;
		}
	}
}

static Widget _menu_widgets[] = {
{      WWT_PANEL,    14,     0,   159,     0, 65535,     0},
{      WWT_LAST},
};


static Widget _player_menu_widgets[] = {
{      WWT_PANEL,    14,     0,   240,     0,    81,     0},
{      WWT_LAST},
};


static int GetPlayerIndexFromMenu(int index)
{
	Player *p;

	if (index >= 0) {
		FOR_ALL_PLAYERS(p) {
			if (p->name_1 != 0) {
				if (--index < 0)
					return p->index;
			}
		} 
	}
	return -1;
}

static void UpdatePlayerMenuHeight(Window *w)
{
	int num = 0;
	Player *p;

	FOR_ALL_PLAYERS(p) {
		if (p->name_1)
			num++;
	}
	
	if (WP(w,menu_d).item_count != num) {
		WP(w,menu_d).item_count = num;
		SetWindowDirty(w);
		num = num * 10 + 2;
		w->height = num;
		_player_menu_widgets[0].bottom = _player_menu_widgets[0].top + num - 1;
		SetWindowDirty(w);
	}
}

static void PlayerMenuWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT: {
		int x,y;
		byte sel;
		Player *p;

		UpdatePlayerMenuHeight(w);
		DrawWindowWidgets(w);

		x = 1;
		y = 1;
		sel = WP(w,menu_d).sel_index;
		
		FOR_ALL_PLAYERS(p) {
			if (p->name_1 == 0)
				continue;

			if (p->index == sel) {
				GfxFillRect(x, y, x + 0xEE, y + 9, 0);
			}
			DrawSprite( ((p->player_color + 0x307)<<16)+0x82EB, x+2, y+1);
		
			SET_DPARAM16(0, p->name_1);
			SET_DPARAM32(1, p->name_2);
			SET_DPARAM16(2, GetPlayerNameString(p->index));
						
			DrawString(x+0x13, y, STR_7021, (byte)((p->index==sel) ? 0xC : 0x10));
			y += 10;
		}
		break;
		}

	case WE_DESTROY: {
		Window *v = FindWindowById(WC_MAIN_TOOLBAR, 0);
		v->click_state &= ~(1 << WP(w,menu_d).main_button);
		SetWindowDirty(v);
		return;
		}
		
	case WE_POPUPMENU_SELECT: {
		int index = GetPlayerIndexFromMenu(GetMenuItemIndex(w, e->popupmenu.pt.x, e->popupmenu.pt.y));
		int action_id = WP(w,menu_d).action_id;

		if (index < 0) {
			Window *w2 = FindWindowById(WC_MAIN_TOOLBAR,0);
			if (GetWidgetFromPos(w2, e->popupmenu.pt.x - w2->left, e->popupmenu.pt.y - w2->top) == WP(w,menu_d).main_button)
				index = WP(w,menu_d).sel_index;
		}

		DeleteWindow(w);
		
		if (index >= 0) {
			assert(index >= 0 && index < 30);
			_menu_clicked_procs[action_id](index);
		}
		break;
		}
	case WE_POPUPMENU_OVER: {
		int index;
		UpdatePlayerMenuHeight(w);
		index = GetPlayerIndexFromMenu(GetMenuItemIndex(w, e->popupmenu.pt.x, e->popupmenu.pt.y));

		if (index == -1 || index == WP(w,menu_d).sel_index)
			return;

		WP(w,menu_d).sel_index = index;
		SetWindowDirty(w);
		return;
		}
	}
}

static Window *PopupMainToolbMenu(Window *w, int x, int main_button, StringID base_string, int item_count)
{
	int h;

	SETBIT(w->click_state, (byte)main_button);
	InvalidateWidget(w, (byte)main_button);

	DeleteWindowById(WC_TOOLBAR_MENU, 0);

	_menu_widgets[0].bottom = h = item_count * 10 + 1;
	w = AllocateWindow(x, 0x16, 0xA0, h+1, MenuWndProc, WC_TOOLBAR_MENU, _menu_widgets);
	w->flags4 &= ~WF_WHITE_BORDER_MASK;
	
	WP(w,menu_d).item_count = item_count;
	WP(w,menu_d).sel_index = 0;
	WP(w,menu_d).main_button = main_button;
	WP(w,menu_d).action_id = (main_button >> 8) ? (main_button >> 8) : main_button;
	WP(w,menu_d).string_id = base_string;
	WP(w,menu_d).checked_items = 0;

	_popup_menu_active = true;
	
	SndPlayFx(0x13);

	return w;
}

static Window *PopupMainPlayerToolbMenu(Window *w, int x, int main_button)
{
	SETBIT(w->click_state, main_button);
	InvalidateWidget(w, main_button);

	DeleteWindowById(WC_TOOLBAR_MENU, 0);
	w = AllocateWindow(x, 0x16, 0xF1, 0x52, PlayerMenuWndProc, WC_TOOLBAR_MENU, _player_menu_widgets);
	w->flags4 &= ~WF_WHITE_BORDER_MASK;
	WP(w,menu_d).item_count = 0;
	WP(w,menu_d).sel_index = _local_player;
	WP(w,menu_d).action_id = main_button;
	WP(w,menu_d).main_button = main_button;
	_popup_menu_active = true;
	SndPlayFx(0x13);
	return w;
}

static void ToolbarSaveClick(Window *w)
{
	PopupMainToolbMenu(w, 0x2C, 2, STR_015C_SAVE_GAME, 4);
}

static void ToolbarMapClick(Window *w)
{
	PopupMainToolbMenu(w, 0x4E, 3, STR_02DE_MAP_OF_WORLD, 1);
}

static void ToolbarTownClick(Window *w)
{
	PopupMainToolbMenu(w, 0x64, 4, STR_02BB_TOWN_DIRECTORY, 1);
}

static void ToolbarSubsidiesClick(Window *w)
{
	PopupMainToolbMenu(w, 0x7A, 5, STR_02DD_SUBSIDIES, 1);
}

static void ToolbarStationsClick(Window *w)
{
	PopupMainPlayerToolbMenu(w, 0x90, 6);
}

static void ToolbarMoneyClick(Window *w)
{
	PopupMainPlayerToolbMenu(w, 0xB1, 7);
}

static void ToolbarPlayersClick(Window *w)
{
	PopupMainPlayerToolbMenu(w, 0xC7, 8);
}

static void ToolbarGraphsClick(Window *w)
{
	PopupMainToolbMenu(w, 0xDD, 9, STR_0154_OPERATING_PROFIT_GRAPH, 6);
}

static void ToolbarLeagueClick(Window *w)
{
	PopupMainToolbMenu(w, 0xF3, 10, STR_015A_COMPANY_LEAGUE_TABLE, 1);
}

static void ToolbarIndustryClick(Window *w)
{
	PopupMainToolbMenu(w, 0x109, 11, STR_0313_FUND_NEW_INDUSTRY, 1);
}

static void ToolbarTrainClick(Window *w)
{
	PopupMainPlayerToolbMenu(w, 0x12A, 12);
}

static void ToolbarRoadClick(Window *w)
{
	PopupMainPlayerToolbMenu(w, 0x140, 13);
}

static void ToolbarShipClick(Window *w)
{
	PopupMainPlayerToolbMenu(w, 0x156, 14);
}

static void ToolbarAirClick(Window *w)
{
	PopupMainPlayerToolbMenu(w, 0x16C, 15);
}

bool DoZoomInOut(int how)
{
	ViewPort *vp;
	Window *w, *wt;
	int button;
	
	switch(_game_mode) {
	case GM_EDITOR: button = 8; break;
	case GM_NORMAL: button = 16; break;
	default: return false;
	}

	w = FindWindowById(WC_MAIN_WINDOW, 0);
	assert(w);
	vp = w->viewport;

	wt = FindWindowById(WC_MAIN_TOOLBAR, 0);
	assert(wt);

	if (how == ZOOM_IN) {
		if (vp->zoom == 0) return false;
		vp->zoom--;
		vp->virtual_width >>= 1;
		vp->virtual_height >>= 1;

		WP(w,vp_d).scrollpos_x += vp->virtual_width >> 1;
		WP(w,vp_d).scrollpos_y += vp->virtual_height >> 1;

		SetWindowDirty(w);
	} else if (how == ZOOM_OUT) {
		if (vp->zoom == 2) return false;
		vp->zoom++;

		WP(w,vp_d).scrollpos_x -= vp->virtual_width >> 1;
		WP(w,vp_d).scrollpos_y -= vp->virtual_height >> 1;

		vp->virtual_width <<= 1;
		vp->virtual_height <<= 1;

		SetWindowDirty(w);
	}

	// update the toolbar button too
	CLRBIT(wt->disabled_state, button);
	CLRBIT(wt->disabled_state, button + 1);
	if (vp->zoom == 0) SETBIT(wt->disabled_state, button);
	else if (vp->zoom == 2) SETBIT(wt->disabled_state, button + 1);
	SetWindowDirty(wt);

	return true;
}

static void MaxZoomIn()
{
	while (DoZoomInOut(ZOOM_IN)) {}
}

static void ToolbarZoomInClick(Window *w)
{
	if (DoZoomInOut(ZOOM_IN)) {
		HandleButtonClick(w, 16);
		SndPlayFx(0x13);
	}
}

static void ToolbarZoomOutClick(Window *w)
{
	if (DoZoomInOut(ZOOM_OUT)) {
		HandleButtonClick(w, 17);
		SndPlayFx(0x13);
	}
}

static void ToolbarBuildRailClick(Window *w)
{
	Player *p = DEREF_PLAYER(_local_player);
	Window *w2;
	w2 = PopupMainToolbMenu(w, 0x1C4, 18, STR_1015_RAILROAD_CONSTRUCTION, p->max_railtype);
	WP(w2,menu_d).sel_index = _last_built_railtype;
}

static void ToolbarBuildRoadClick(Window *w)
{
	PopupMainToolbMenu(w, 0x1DA, 19, STR_180A_ROAD_CONSTRUCTION, 1);
}

static void ToolbarBuildWaterClick(Window *w)
{
	PopupMainToolbMenu(w, 0x1ED, 20, STR_9800_DOCK_CONSTRUCTION, 1);
}

static void ToolbarBuildAirClick(Window *w)
{
	PopupMainToolbMenu(w, 0x1E0, 21, STR_A01D_AIRPORT_CONSTRUCTION, 1);
}

static void ToolbarForestClick(Window *w)
{
	PopupMainToolbMenu(w, 0x1E0, 22, STR_2800_PLANT_TREES, 2);
}

static void ToolbarMusicClick(Window *w)
{
	PopupMainToolbMenu(w, 0x1E0, 23, STR_01D3_SOUND_MUSIC, 1);
}

static void ToolbarNewspaperClick(Window *w)
{
	PopupMainToolbMenu(w, 0x1E0, 24, STR_0200_LAST_MESSAGE_NEWS_REPORT, _newspaper_flag != 2 ? 2 : 3);
}

static void ToolbarHelpClick(Window *w)
{
	PopupMainToolbMenu(w, 0x1E0, 25, STR_02D5_LAND_BLOCK_INFO, 5);
}

static void ToolbarOptionsClick(Window *w)
{
	uint16 x;

	w = PopupMainToolbMenu(w, 0x16, 1, STR_02C3_GAME_OPTIONS, 11);

	x = (uint16)-1;
	if (_display_opt & DO_SHOW_TOWN_NAMES) x &= ~(1<<4);
	if (_display_opt & DO_SHOW_STATION_NAMES) x &= ~(1<<5);
	if (_display_opt & DO_SHOW_SIGNS) x &= ~(1<<6);
	if (_display_opt & DO_CHECKPOINTS) x &= ~(1<<7);
	if (_display_opt & DO_FULL_ANIMATION) x &= ~(1<<8);
	if (_display_opt & DO_FULL_DETAIL) x &= ~(1<<9);
	if (!(_display_opt & DO_TRANS_BUILDINGS)) x &= ~(1<<10);
	WP(w,menu_d).checked_items = x;
}


static void ToolbarScenSaveOrLoad(Window *w)
{
	PopupMainToolbMenu(w, 0x2C, 2, STR_0292_SAVE_SCENARIO, 5);
}

static void ToolbarScenDateBackward(Window *w)
{
	YearMonthDay ymd;

	// don't allow too fast scrolling
	if ((w->flags4 & WF_TIMEOUT_MASK) <= 2 << WF_TIMEOUT_SHL) {
		HandleButtonClick(w, 5);
		InvalidateWidget(w, 4);
		
		if (_date > 0x2ACE) {
			do {
				ConvertDayToYMD(&ymd, --_date);
			} while (ymd.day != 1 || ymd.month != 0);
			_cur_year = ymd.year;
			_cur_month = ymd.month;
		}
	}
	_left_button_clicked = false;
}

static void ToolbarScenDateForward(Window *w)
{
	YearMonthDay ymd;

	// don't allow too fast scrolling
	if ((w->flags4 & WF_TIMEOUT_MASK) <= 2 << WF_TIMEOUT_SHL) {
		HandleButtonClick(w, 6);
		InvalidateWidget(w, 4);

		if (_date < 0x4E79) {
			do {
				ConvertDayToYMD(&ymd, ++_date);
			} while (ymd.day != 1 || ymd.month != 0);
			_cur_year = ymd.year;
			_cur_month = ymd.month;
		}
	}
	_left_button_clicked = false;
}

static void ToolbarScenMapTownDir(Window *w)
{
	PopupMainToolbMenu(w, 0x16A, 7 | (16<<8), STR_02DE_MAP_OF_WORLD, 2);
}

static void ToolbarScenZoomIn(Window *w)
{
	if (DoZoomInOut(ZOOM_IN)) {
		HandleButtonClick(w, 8);
		SndPlayFx(0x13);
	}
}

static void ToolbarScenZoomOut(Window *w)
{
	if (DoZoomInOut(ZOOM_OUT)) {
		HandleButtonClick(w, 17);
		SndPlayFx(0x13);
	}
}

void ZoomInOrOutToCursor(bool in)
{
	Point pt;
	Window* w;
	ViewPort* vp;

	w = FindWindowById(WC_MAIN_WINDOW, 0);
	assert(w != 0);

	vp = w->viewport;

	if (_game_mode != GM_MENU) {
		if ((in && vp->zoom == 0) || (!in && vp->zoom == 2))
			return;

		pt = GetTileZoomCenter(in);
		if (pt.x != -1) {
			ScrollMainWindowTo(pt.x, pt.y);

			DoZoomInOut(in ? ZOOM_IN : ZOOM_OUT);
		}
	}
}

void ResetLandscape()
{
	_random_seed_1 = InteractiveRandom();
	_random_seed_2 = InteractiveRandom();
	
	GenerateWorld(true);
	MarkWholeScreenDirty();
} 

static const Widget _ask_reset_landscape_widgets[] = {
{    WWT_TEXTBTN,     4,     0,    10,     0,    13, STR_00C5},
{    WWT_CAPTION,     4,    11,   179,     0,    13, STR_022C_RESET_LANDSCAPE},
{     WWT_IMGBTN,     4,     0,   179,    14,    91, 0x0},
{    WWT_TEXTBTN,    12,    25,    84,    72,    83, STR_00C9_NO},
{    WWT_TEXTBTN,    12,    95,   154,    72,    83, STR_00C8_YES},
{      WWT_LAST},
};

static void AskResetLandscapeWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT:
		DrawWindowWidgets(w);
		DrawStringMultiCenter(90, 38,STR_022D_ARE_YOU_SURE_YOU_WANT_TO , 178);
	case WE_CLICK:
		switch(e->click.widget) {
		case 3:
			DeleteWindow(w);
			break;
		case 4:
			DeleteWindow(w);
			ResetLandscape();
			break;
		}
	}
}

static const WindowDesc _ask_reset_landscape_desc = {
	0xe6,0xcd, 0xb4, 0x5c,
	WC_ASK_RESET_LANDSCAPE,0,
	WDF_STD_BTN | WDF_DEF_WIDGET,
	_ask_reset_landscape_widgets,
	AskResetLandscapeWndProc,
};

static void AskResetLandscape()
{
	AllocateWindowDesc(&_ask_reset_landscape_desc);
}

static void CommonRaiseLowerBigLand(uint tile, int mode)
{
	int32 ret;
	int size;
	uint h;

	_error_message_2 = mode ? STR_0808_CAN_T_RAISE_LAND_HERE : STR_0809_CAN_T_LOWER_LAND_HERE;

	_generating_world = true;

	tile = TILE_FROM_XY(GET_TILE_X(tile)*16+_tile_fract_coords.x + 8,GET_TILE_Y(tile)*16+_tile_fract_coords.y + 8);

	if (_terraform_size == 1) {
		ret = DoCommandByTile(tile, 8, (uint32)mode, DC_EXEC | DC_AUTO, CMD_TERRAFORM_LAND);
		if (ret == CMD_ERROR) {
			SetRedErrorSquare(_terraform_err_tile);
			_generating_world = false;
		} else {
			SndPlayTileFx(0x1D, tile);
		}
	} else {
		SndPlayTileFx(0x1D, tile);

		size = _terraform_size;
		assert(size != 0);

		if (mode != 0) {
			/* Raise land */
			h = 15;
			BEGIN_TILE_LOOP(tile2, size, size, tile)
				h = min(h, _map_type_and_height[tile2]&0xF);
			END_TILE_LOOP(tile2, size, size, tile)
		} else {
			/* Lower land */
			h = 0;
			BEGIN_TILE_LOOP(tile2, size, size, tile)
				h = max(h, _map_type_and_height[tile2]&0xF);
			END_TILE_LOOP(tile2, size, size, tile)
		}

		BEGIN_TILE_LOOP(tile2, size, size, tile)
			if ((uint)(_map_type_and_height[tile2]&0xF) == h) {
				DoCommandByTile(tile2, 8, (uint32)mode, DC_EXEC | DC_AUTO, CMD_TERRAFORM_LAND);
			}
		END_TILE_LOOP(tile2, size, size, tile)
	}

	_generating_world = false;
}

void PlaceProc_RaiseBigLand(uint tile)
{
	CommonRaiseLowerBigLand(tile, 1);
}

void PlaceProc_LowerBigLand(uint tile)
{
	CommonRaiseLowerBigLand(tile, 0);
}

void PlaceProc_RockyArea(uint tile)
{
	if (!IS_TILETYPE(tile, MP_CLEAR))
		return;

	_map5[tile] = (_map5[tile] & ~0x1C) | 8;
	MarkTileDirtyByTile(tile);
	SndPlayTileFx(0x1D, tile);
}

void PlaceProc_LightHouse(uint tile)
{
	TileInfo ti;

	FindLandscapeHeightByTile(&ti, tile);
	if (ti.type != MP_CLEAR || (ti.tileh & 0x10))
		return;

	ModifyTile(tile, MP_SETTYPE(MP_UNMOVABLE) | MP_MAP5, 1);
	SndPlayTileFx(0x1D, tile);
}

void PlaceProc_Transmitter(uint tile)
{
	TileInfo ti;

	FindLandscapeHeightByTile(&ti, tile);
	if (ti.type != MP_CLEAR || (ti.tileh & 0x10))
		return;

	ModifyTile(tile, MP_SETTYPE(MP_UNMOVABLE) | MP_MAP5, 0);
	SndPlayTileFx(0x1D, tile);
}

void PlaceProc_Desert(uint tile)
{
	SetMapExtraBits(tile, GetMapExtraBits(tile) == 1 ? 0 : 1);
}

static const Widget _scen_edit_land_gen_widgets[] = {
{    WWT_TEXTBTN,     7,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   165,     0,    13, STR_0223_LAND_GENERATION, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,     7,     0,   165,    14,    85, 0x0, 0},
{     WWT_IMGBTN,    14,     2,    23,    16,    37, 0x2B6, STR_018F_RAISE_A_CORNER_OF_LAND},
{     WWT_IMGBTN,    14,     2,    23,    38,    59, 0x2B7, STR_018E_LOWER_A_CORNER_OF_LAND},
{     WWT_IMGBTN,    14,     2,    23,    62,    83, 0x2BF, STR_018D_DEMOLISH_BUILDINGS_ETC},
{    WWT_TEXTBTN,    14,   130,   140,    21,    32, STR_0224, STR_0228_INCREASE_SIZE_OF_LAND_AREA},
{    WWT_TEXTBTN,    14,   130,   140,    33,    44, STR_0225, STR_0229_DECREASE_SIZE_OF_LAND_AREA},
{    WWT_TEXTBTN,    14,    25,   140,    56,    67, STR_0226_RANDOM_LAND, STR_022A_GENERATE_RANDOM_LAND},
{    WWT_TEXTBTN,    14,    25,   140,    71,    82, STR_0227_RESET_LAND, STR_022B_RESET_LANDSCAPE},
{     WWT_IMGBTN,    14,   142,   163,    16,    37, 0xFF4, STR_028C_PLACE_ROCKY_AREAS_ON_LANDSCAPE},
{     WWT_IMGBTN,    14,   142,   163,    39,    60, 0xFF5, STR_028D_PLACE_LIGHTHOUSE},
{     WWT_IMGBTN,    14,   142,   163,    62,    83, 0xFF6, STR_028E_PLACE_TRANSMITTER},
{      WWT_LAST},
};

static const int8 _multi_terraform_coords[][2] = {
	{  0, -2},
	{  4,  0},{ -4,  0},{  0,  2},
	{ -8,  2},{ -4,  4},{  0,  6},{  4,  4},{  8,  2},
	{-12,  0},{ -8, -2},{ -4, -4},{  0, -6},{  4, -4},{  8, -2},{ 12,  0},
	{-16,  2},{-12,  4},{ -8,  6},{ -4,  8},{  0, 10},{  4,  8},{  8,  6},{ 12,  4},{ 16,  2},
	{-20,  0},{-16, -2},{-12, -4},{ -8, -6},{ -4, -8},{  0,-10},{  4, -8},{  8, -6},{ 12, -4},{ 16, -2},{ 20,  0},
	{-24,  2},{-20,  4},{-16,  6},{-12,  8},{ -8, 10},{ -4, 12},{  0, 14},{  4, 12},{  8, 10},{ 12,  8},{ 16,  6},{ 20,  4},{ 24,  2},
	{-28,  0},{-24, -2},{-20, -4},{-16, -6},{-12, -8},{ -8,-10},{ -4,-12},{  0,-14},{  4,-12},{  8,-10},{ 12, -8},{ 16, -6},{ 20, -4},{ 24, -2},{ 28,  0},
};

static void ScenEditLandGenWndProc(Window *w, WindowEvent *e)
{
	// XXX: show different tooltips in desert mode
	switch(e->event) {
	case WE_PAINT:
		// XXX: only show reset button when nothing was built
		DrawWindowWidgets(w);
		
		{
			int n = _terraform_size * _terraform_size;
			const int8 *coords = &_multi_terraform_coords[0][0];

			assert(n != 0);
			do {
				DrawSprite(0xFEF, 79 + coords[0], 33 + coords[1]);
				coords += 2;
			} while (--n);
		}

		if (_thd.window_class == WC_SCEN_LAND_GEN && (w->click_state&(1<<3|1<<4))) {
			SetTileSelectSize(_terraform_size, _terraform_size);
		}
		break;
	case WE_CLICK:
		switch(e->click.widget) {
		case 3: /* raise corner */
			HandlePlacePushButton(w, 3, ANIMCURSOR_RAISELAND, 2, PlaceProc_RaiseBigLand);
			break;
		case 4: /* lower corner */
			HandlePlacePushButton(w, 4, ANIMCURSOR_LOWERLAND, 2, PlaceProc_LowerBigLand);
			break;
		case 5: /* demolish */
			HandlePlacePushButton(w, 5, ANIMCURSOR_DEMOLISH, 1, PlaceProc_Demolish);
			break;
		{
			int size;
		case 6: /* increase terraform size */
			HandleButtonClick(w, 6);
			size = 1;
			goto terraform_size_common;
		case 7: /* decrease terraform size */
			HandleButtonClick(w, 7);
			size = -1;
terraform_size_common:;
			size += _terraform_size;
			if (!IS_INT_INSIDE(size, 1, 8+1))
				return;
			_terraform_size = size;
			SndPlayFx(0x13);
			SetWindowDirty(w);
			break;
		}

		case 8: /* gen random land */
			HandleButtonClick(w, 8);
			SndPlayFx(0x13);
			_switch_mode = SM_GENRANDLAND;
			break;

		case 9: /* reset landscape */
			HandleButtonClick(w,9);
			AskResetLandscape();
			break;

		case 10: /* place rocky areas */
			HandlePlacePushButton(w, 10, 0xFF7, 1, PlaceProc_RockyArea);
			break;
			
		case 11: /* place lighthouse */
			HandlePlacePushButton(w, 11, 0xFF8, 1, _opt.landscape == LT_DESERT ? PlaceProc_Desert : PlaceProc_LightHouse);
			break;

		case 12: /* place transmitter */
			HandlePlacePushButton(w, 12, 0xFF9, 1, PlaceProc_Transmitter);
			break;
		}
		break;
	case WE_TIMEOUT:
		UnclickSomeWindowButtons(w, ~(1<<3 | 1<<4 | 1<<5 | 1<<10|1<<11|1<<12));
		break;
	case WE_PLACE_OBJ:
		_place_proc(e->place.tile);
		break;
	case WE_ABORT_PLACE_OBJ:
		w->click_state = 0;
		SetWindowDirty(w);
		break;
	}
}

static const WindowDesc _scen_edit_land_gen_desc = {
	-1,-1, 0xA6, 0x56,
	WC_SCEN_LAND_GEN,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_scen_edit_land_gen_widgets,
	ScenEditLandGenWndProc,
};

static void ToolbarScenGenLand(Window *w)
{
	HandleButtonClick(w, 10);
	SndPlayFx(0x13);

	AllocateWindowDescFront(&_scen_edit_land_gen_desc, 0);
}

void PlaceProc_Town(uint tile)
{
	if (DoCommandByTile(tile, 0, 0, 
		DC_EXEC | DC_MSG(STR_0236_CAN_T_BUILD_TOWN_HERE), CMD_BUILD_TOWN) == CMD_ERROR)
			return;
	SndPlayTileFx(0x1D, tile);
	ResetObjectToPlace();
}


static const Widget _scen_edit_town_gen_widgets[] = {
{    WWT_TEXTBTN,     7,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   159,     0,    13, STR_0233_TOWN_GENERATION, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,     7,     0,   159,    14,    68, 0x0},
{    WWT_TEXTBTN,    14,     2,   157,    16,    27, STR_0234_NEW_TOWN, STR_0235_CONSTRUCT_NEW_TOWN},
{    WWT_TEXTBTN,    14,     2,   157,    29,    40, STR_023D_RANDOM_TOWN, STR_023E_BUILD_TOWN_IN_RANDOM_LOCATION},
{    WWT_TEXTBTN,    14,     2,    53,    55,    66, STR_02A1_SMALL, STR_02A4_SELECT_TOWN_SIZE},
{    WWT_TEXTBTN,    14,    54,   105,    55,    66, STR_02A2_MEDIUM, STR_02A4_SELECT_TOWN_SIZE},
{    WWT_TEXTBTN,    14,   106,   157,    55,    66, STR_02A3_LARGE, STR_02A4_SELECT_TOWN_SIZE},
{      WWT_LAST},
};

static void ScenEditTownGenWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT:
		w->click_state = (w->click_state & ~0xE0) | (1 << (_new_town_size + 5));
		DrawWindowWidgets(w);
		DrawStringCentered(80, 41, STR_02A5_TOWN_SIZE, 0);
		break;

	case WE_CLICK:
		switch(e->click.widget) {
		case 3: /* new town */
			HandlePlacePushButton(w, 3, 0xFF0, 1, PlaceProc_Town);
			break;
		case 4: {/* random town */
			City *c;

			HandleButtonClick(w, 4);
			_generating_world = true;
			c = CreateRandomTown();
			_generating_world = false;
			if (c != NULL)
				ScrollMainWindowToTile(c->xy);
			break;
		}

		case 5: case 6: case 7:
			_new_town_size = e->click.widget - 5;
			SetWindowDirty(w);
			break;
		}
		break;

	case WE_TIMEOUT:
		UnclickSomeWindowButtons(w, 1<<4);
		break;
	case WE_PLACE_OBJ:
		_place_proc(e->place.tile);
		break;
	case WE_ABORT_PLACE_OBJ:
		w->click_state = 0;
		SetWindowDirty(w);
		break;
	}
}

static const WindowDesc _scen_edit_town_gen_desc = {
	-1,-1, 0xA0, 0x45,
	WC_SCEN_TOWN_GEN,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_scen_edit_town_gen_widgets,
	ScenEditTownGenWndProc,
};

static void ToolbarScenGenTown(Window *w)
{
	HandleButtonClick(w, 11);
	SndPlayFx(0x13);

	AllocateWindowDescFront(&_scen_edit_town_gen_desc, 0);
}


static const Widget _scenedit_industry_normal_widgets[] = {
{    WWT_TEXTBTN,     7,     0,    10,     0,    13, STR_00C5},
{    WWT_CAPTION,     7,    11,   169,     0,    13, STR_023F_INDUSTRY_GENERATION},
{     WWT_IMGBTN,     7,     0,   169,    14,   198, 0x0},
{    WWT_TEXTBTN,    14,     2,   167,    16,    27, STR_0240_COAL_MINE},
{    WWT_TEXTBTN,    14,     2,   167,    29,    40, STR_0241_POWER_STATION},
{    WWT_TEXTBTN,    14,     2,   167,    42,    53, STR_0242_SAWMILL},
{    WWT_TEXTBTN,    14,     2,   167,    55,    66, STR_0243_FOREST},
{    WWT_TEXTBTN,    14,     2,   167,    68,    79, STR_0244_OIL_REFINERY},
{    WWT_TEXTBTN,    14,     2,   167,    81,    92, STR_0245_OIL_RIG},
{    WWT_TEXTBTN,    14,     2,   167,    94,   105, STR_0246_FACTORY},
{    WWT_TEXTBTN,    14,     2,   167,   107,   118, STR_0247_STEEL_MILL},
{    WWT_TEXTBTN,    14,     2,   167,   120,   131, STR_0248_FARM},
{    WWT_TEXTBTN,    14,     2,   167,   133,   144, STR_0249_IRON_ORE_MINE},
{    WWT_TEXTBTN,    14,     2,   167,   146,   157, STR_024A_OIL_WELLS},
{    WWT_TEXTBTN,    14,     2,   167,   159,   170, STR_024B_BANK},
{      WWT_LAST},
};

static const Widget _scenedit_industry_hilly_widgets[] = {
{    WWT_TEXTBTN,     7,     0,    10,     0,    13, STR_00C5},
{    WWT_CAPTION,     7,    11,   169,     0,    13, STR_023F_INDUSTRY_GENERATION},
{     WWT_IMGBTN,     7,     0,   169,    14,   198, 0x0},
{    WWT_TEXTBTN,    14,     2,   167,    16,    27, STR_0240_COAL_MINE},
{    WWT_TEXTBTN,    14,     2,   167,    29,    40, STR_0241_POWER_STATION},
{    WWT_TEXTBTN,    14,     2,   167,    42,    53, STR_024C_PAPER_MILL},
{    WWT_TEXTBTN,    14,     2,   167,    55,    66, STR_0243_FOREST},
{    WWT_TEXTBTN,    14,     2,   167,    68,    79, STR_0244_OIL_REFINERY},
{    WWT_TEXTBTN,    14,     2,   167,    81,    92, STR_024D_FOOD_PROCESSING_PLANT},
{    WWT_TEXTBTN,    14,     2,   167,    94,   105, STR_024E_PRINTING_WORKS},
{    WWT_TEXTBTN,    14,     2,   167,   107,   118, STR_024F_GOLD_MINE},
{    WWT_TEXTBTN,    14,     2,   167,   120,   131, STR_0248_FARM},
{    WWT_TEXTBTN,    14,     2,   167,   133,   144, STR_024B_BANK},
{    WWT_TEXTBTN,    14,     2,   167,   146,   157, STR_024A_OIL_WELLS},
{      WWT_LAST},
};

static const Widget _scenedit_industry_desert_widgets[] = {
{    WWT_TEXTBTN,     7,     0,    10,     0,    13, STR_00C5},
{    WWT_CAPTION,     7,    11,   169,     0,    13, STR_023F_INDUSTRY_GENERATION},
{     WWT_IMGBTN,     7,     0,   169,    14,   198, 0x0},
{    WWT_TEXTBTN,    14,     2,   167,    16,    27, STR_0250_LUMBER_MILL},
{    WWT_TEXTBTN,    14,     2,   167,    29,    40, STR_0251_FRUIT_PLANTATION},
{    WWT_TEXTBTN,    14,     2,   167,    42,    53, STR_0252_RUBBER_PLANTATION},
{    WWT_TEXTBTN,    14,     2,   167,    55,    66, STR_0244_OIL_REFINERY},
{    WWT_TEXTBTN,    14,     2,   167,    68,    79, STR_024D_FOOD_PROCESSING_PLANT},
{    WWT_TEXTBTN,    14,     2,   167,    81,    92, STR_0246_FACTORY},
{    WWT_TEXTBTN,    14,     2,   167,    94,   105, STR_0253_WATER_SUPPLY},
{    WWT_TEXTBTN,    14,     2,   167,   107,   118, STR_0248_FARM},
{    WWT_TEXTBTN,    14,     2,   167,   120,   131, STR_0254_WATER_TOWER},
{    WWT_TEXTBTN,    14,     2,   167,   133,   144, STR_024A_OIL_WELLS},
{    WWT_TEXTBTN,    14,     2,   167,   146,   157, STR_024B_BANK},
{    WWT_TEXTBTN,    14,     2,   167,   159,   170, STR_0255_DIAMOND_MINE},
{    WWT_TEXTBTN,    14,     2,   167,   172,   183, STR_0256_COPPER_ORE_MINE},
{      WWT_LAST},
};

static const Widget _scenedit_industry_candy_widgets[] = {
{    WWT_TEXTBTN,     7,     0,    10,     0,    13, STR_00C5},
{    WWT_CAPTION,     7,    11,   169,     0,    13, STR_023F_INDUSTRY_GENERATION},
{     WWT_IMGBTN,     7,     0,   169,    14,   198, 0x0},
{    WWT_TEXTBTN,    14,     2,   167,    16,    27, STR_0257_COTTON_CANDY_FOREST},
{    WWT_TEXTBTN,    14,     2,   167,    29,    40, STR_0258_CANDY_FACTORY},
{    WWT_TEXTBTN,    14,     2,   167,    42,    53, STR_0259_BATTERY_FARM},
{    WWT_TEXTBTN,    14,     2,   167,    55,    66, STR_025A_COLA_WELLS},
{    WWT_TEXTBTN,    14,     2,   167,    68,    79, STR_025B_TOY_SHOP},
{    WWT_TEXTBTN,    14,     2,   167,    81,    92, STR_025C_TOY_FACTORY},
{    WWT_TEXTBTN,    14,     2,   167,    94,   105, STR_025D_PLASTIC_FOUNTAINS},
{    WWT_TEXTBTN,    14,     2,   167,   107,   118, STR_025E_FIZZY_DRINK_FACTORY},
{    WWT_TEXTBTN,    14,     2,   167,   120,   131, STR_025F_BUBBLE_GENERATOR},
{    WWT_TEXTBTN,    14,     2,   167,   133,   144, STR_0260_TOFFEE_QUARRY},
{    WWT_TEXTBTN,    14,     2,   167,   146,   157, STR_0261_SUGAR_MINE},
{      WWT_LAST},
};

int _industry_type_to_place;

static bool AnyCityExists() {
	City *c;
	FOR_ALL_CITIES(c) {
		if (c->xy)
			return true;
	}
	return false;
}

extern Industry *CreateNewIndustry(uint tile, int type);

static bool TryBuildIndustry(TileIndex tile, int type)
{
	int n;

	n = 100;
	do {
		if (CreateNewIndustry(AdjustTileCoordRandomly(tile, 1), type)) return true;
	} while (--n);

	n = 200;
	do {
		if (CreateNewIndustry(AdjustTileCoordRandomly(tile, 2), type)) return true;
	} while (--n);

	n = 700;
	do {
		if (CreateNewIndustry(AdjustTileCoordRandomly(tile, 4), type)) return true;
	} while (--n);

	return false;
}


static const byte _industry_type_list[4][16] = {
	{0, 1, 2, 3, 4, 5, 6, 8, 9, 18, 11, 12},
	{0, 1, 14, 3, 4, 13, 7, 15, 9, 16, 11, 12},
	{25, 19, 20, 4, 13, 23, 21, 24, 22, 11, 16, 17, 10},
	{26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36},
};

static void ScenEditIndustryWndProc(Window *w, WindowEvent *e)
{
	int button;

	switch(e->event) {
	case WE_PAINT:
		DrawWindowWidgets(w);
		break;

	case WE_CLICK:
		if ((button=e->click.widget) >= 3) {
			if (HandlePlacePushButton(w, button, 0xFF1, 1, NULL))
				_industry_type_to_place = _industry_type_list[_opt.landscape][button - 3];
		}
		break;
	case WE_PLACE_OBJ: {
		int type;

		// Show error if no city exists at all
		type = _industry_type_to_place;
		if (!AnyCityExists()) {
			SET_DPARAM16(0, type + STR_4802_COAL_MINE);
			ShowErrorMessage(STR_0286_MUST_BUILD_TOWN_FIRST,STR_0285_CAN_T_BUILD_HERE,e->place.pt.x, e->place.pt.y);
			return;
		}

		_current_player = 0x10;
		_generating_world = true;
		if (!TryBuildIndustry(e->place.tile,type)) {
			SET_DPARAM16(0, type + STR_4802_COAL_MINE);
			ShowErrorMessage(_error_message, STR_0285_CAN_T_BUILD_HERE,e->place.pt.x, e->place.pt.y);
		}
		_generating_world = false;
		break;
	}
	case WE_ABORT_PLACE_OBJ:
		w->click_state = 0;
		SetWindowDirty(w);
		break;

	}
}

static const WindowDesc _scenedit_industry_normal_desc = {
	-1,-1, 0xAA, 0xC7,
	WC_SCEN_INDUSTRY,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_scenedit_industry_normal_widgets,
	ScenEditIndustryWndProc,
};

static const WindowDesc _scenedit_industry_hilly_desc = {
	-1,-1, 0xAA, 0xC7,
	WC_SCEN_INDUSTRY,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_scenedit_industry_hilly_widgets,
	ScenEditIndustryWndProc,
};

static const WindowDesc _scenedit_industry_desert_desc = {
	-1,-1, 0xAA, 0xC7,
	WC_SCEN_INDUSTRY,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_scenedit_industry_desert_widgets,
	ScenEditIndustryWndProc,
};

static const WindowDesc _scenedit_industry_candy_desc = {
	-1,-1, 0xAA, 0xC7,
	WC_SCEN_INDUSTRY,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_scenedit_industry_candy_widgets,
	ScenEditIndustryWndProc,
};

static const WindowDesc * const _scenedit_industry_descs[] = {
	&_scenedit_industry_normal_desc,
	&_scenedit_industry_hilly_desc,
	&_scenedit_industry_desert_desc,
	&_scenedit_industry_candy_desc,
};


static void ToolbarScenGenIndustry(Window *w)
{
	HandleButtonClick(w, 12);
	SndPlayFx(0x13);
	AllocateWindowDescFront(_scenedit_industry_descs[_opt.landscape],0);
}

static void ToolbarScenBuildRoad(Window *w)
{
	HandleButtonClick(w, 13);
	SndPlayFx(0x13);
	ShowBuildRoadScenToolbar();
}

static void ToolbarScenPlantTrees(Window *w)
{
	HandleButtonClick(w, 14);
	SndPlayFx(0x13);
	ShowBuildTreesScenToolbar();
}

static void ToolbarScenPlaceSign(Window *w)
{
	HandleButtonClick(w, 15);
	SndPlayFx(0x13);
	SelectSignTool();
}

static void ToolbarBtn_NULL(Window *w)
{
}

static ToolbarButtonProc* const _toolbar_button_procs[] = {
	ToolbarPauseClick,
	ToolbarOptionsClick,
	ToolbarSaveClick,
	ToolbarMapClick,
	ToolbarTownClick,
	ToolbarSubsidiesClick,
	ToolbarStationsClick,
	ToolbarMoneyClick,
	ToolbarPlayersClick,
	ToolbarGraphsClick,
	ToolbarLeagueClick,
	ToolbarIndustryClick,
	ToolbarTrainClick,
	ToolbarRoadClick,
	ToolbarShipClick,
	ToolbarAirClick,
	ToolbarZoomInClick,
	ToolbarZoomOutClick,
	ToolbarBuildRailClick,
	ToolbarBuildRoadClick,
	ToolbarBuildWaterClick,
	ToolbarBuildAirClick,
	ToolbarForestClick,
	ToolbarMusicClick,
	ToolbarNewspaperClick,
	ToolbarHelpClick,
};

static void MainToolbarWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT:
		// Draw brown-red toolbar bg.
		GfxFillRect(0, 0, w->width-1, w->height-1, 0xB2);
		GfxFillRect(0, 0, w->width-1, w->height-1, 0x80B4);

		DrawWindowWidgets(w);
		break;

	case WE_CLICK: {
		if (_game_mode == GM_MENU)
			return;

		_toolbar_button_procs[e->click.widget](w);
	} break;

	case WE_KEYPRESS: {
		switch(e->keypress.keycode) {
		case WKC_F1: ToolbarPauseClick(w); break;
		case WKC_F2: ShowGameOptions(); break;
		case WKC_F3: MenuClickSaveLoad(0); break;
		case WKC_F4: ShowSmallMap(); break;
		case WKC_F5: ShowTownDirectory(); break;
		case WKC_F6: ShowSubsidiesList(); break;
		case WKC_F7: ShowPlayerStations(_local_player); break;
		case WKC_F8: ShowPlayerFinances(_local_player); break;
		case WKC_F9: ShowPlayerCompany(_local_player); break;
		case WKC_F10:ShowOperatingProfitGraph(); break;
		case WKC_F11: ShowCompanyLeagueTable(); break;
		case WKC_F12: ShowBuildIndustryWindow(); break;
		case WKC_SHIFT | WKC_F1: ShowPlayerTrains(_local_player); break;
		case WKC_SHIFT | WKC_F2: ShowPlayerRoadVehicles(_local_player); break;
		case WKC_SHIFT | WKC_F3: ShowPlayerShips(_local_player); break;
		case WKC_SHIFT | WKC_F4: ShowPlayerAircraft(_local_player); break;
		case WKC_SHIFT | WKC_F5: ToolbarZoomInClick(w); break;
		case WKC_SHIFT | WKC_F6: ToolbarZoomOutClick(w); break;
		case WKC_SHIFT | WKC_F7: ShowBuildRailToolbar(0); break;
		case WKC_SHIFT | WKC_F8: ShowBuildRoadToolbar(); break;
		case WKC_SHIFT | WKC_F9: ShowBuildDocksToolbar(); break;
		case WKC_SHIFT | WKC_F10:ShowBuildAirToolbar(); break;
		case WKC_SHIFT | WKC_F11: ShowBuildTreesToolbar(); break;
		case WKC_SHIFT | WKC_F12: ShowMusicWindow(); break;
		}
	} break;
	
	case WE_PLACE_OBJ: {
		_place_proc(e->place.tile);
	} break;

	case WE_ABORT_PLACE_OBJ: {
		w->click_state &= ~(1<<25);
		SetWindowDirty(w);
	} break;

	case WE_ON_EDIT_TEXT: HandleOnEditText(e); break;

	case WE_TIMEOUT:
		UnclickSomeWindowButtons(w, ~1);
		break;
	}
}

static const Widget _toolb_normal_widgets[] = {
{      WWT_PANEL,    14,     0,    21,     0,    21, 0x2D6, STR_0171_PAUSE_GAME},
{      WWT_PANEL,    14,    22,    43,     0,    21, 0x2EF, STR_0187_OPTIONS},
{    WWT_PANEL_2,    14,    44,    65,     0,    21, 0x2D4, STR_0172_SAVE_GAME_ABANDON_GAME},
{      WWT_PANEL,    14,    78,    99,     0,    21, 0x2C4, STR_0174_DISPLAY_MAP},
{      WWT_PANEL,    14,   100,   121,     0,    21, 0xFED, STR_0176_DISPLAY_TOWN_DIRECTORY},
{      WWT_PANEL,    14,   122,   143,     0,    21, 0x2A7, STR_02DC_DISPLAY_SUBSIDIES},
{      WWT_PANEL,    14,   144,   165,     0,    21, 0x513, STR_0173_DISPLAY_LIST_OF_COMPANY},
{      WWT_PANEL,    14,   177,   198,     0,    21, 0x2E1, STR_0177_DISPLAY_COMPANY_FINANCES},
{      WWT_PANEL,    14,   199,   220,     0,    21, 0x2E7, STR_0178_DISPLAY_COMPANY_GENERAL},
{      WWT_PANEL,    14,   221,   242,     0,    21, 0x2E9, STR_0179_DISPLAY_GRAPHS},
{      WWT_PANEL,    14,   243,   264,     0,    21, 0x2AC, STR_017A_DISPLAY_COMPANY_LEAGUE},
{      WWT_PANEL,    14,   265,   286,     0,    21, 0x2E5, STR_0312_FUND_CONSTRUCTION_OF_NEW},
{      WWT_PANEL,    14,   298,   319,     0,    21, 0x2DB, STR_017B_DISPLAY_LIST_OF_COMPANY},
{      WWT_PANEL,    14,   320,   341,     0,    21, 0x2DC, STR_017C_DISPLAY_LIST_OF_COMPANY},
{      WWT_PANEL,    14,   342,   363,     0,    21, 0x2DD, STR_017D_DISPLAY_LIST_OF_COMPANY},
{      WWT_PANEL,    14,   364,   385,     0,    21, 0x2DE, STR_017E_DISPLAY_LIST_OF_COMPANY},
{      WWT_PANEL,    14,   397,   418,     0,    21, 0x2DF, STR_017F_ZOOM_THE_VIEW_IN},
{      WWT_PANEL,    14,   419,   440,     0,    21, 0x2E0, STR_0180_ZOOM_THE_VIEW_OUT},
{      WWT_PANEL,    14,   452,   473,     0,    21, 0x2D7, STR_0181_BUILD_RAILROAD_TRACK},
{      WWT_PANEL,    14,   474,   495,     0,    21, 0x2D8, STR_0182_BUILD_ROADS},
{      WWT_PANEL,    14,   496,   517,     0,    21, 0x2D9, STR_0183_BUILD_SHIP_DOCKS},
{      WWT_PANEL,    14,   518,   539,     0,    21, 0x2DA, STR_0184_BUILD_AIRPORTS},
{      WWT_PANEL,    14,   540,   561,     0,    21, 0x2E6, STR_0185_PLANT_TREES_PLACE_SIGNS},
{      WWT_PANEL,    14,   574,   595,     0,    21, 0x2C9, STR_01D4_SHOW_SOUND_MUSIC_WINDOW},
{      WWT_PANEL,    14,   596,   617,     0,    21, 0x2A8, STR_0203_SHOW_LAST_MESSAGE_NEWS},
{      WWT_PANEL,    14,   618,   639,     0,    21, 0x2D3, STR_0186_LAND_BLOCK_INFORMATION},
{      WWT_LAST},
};

static const WindowDesc _toolb_normal_desc = {
	0, 0, 0x280, 0x16,
	WC_MAIN_TOOLBAR,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET,
	_toolb_normal_widgets,
	MainToolbarWndProc
};

static const WindowDesc _toolb_intro_desc = {
	0, -22, 0x280, 0x16,
	WC_MAIN_TOOLBAR,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET,
	_toolb_normal_widgets,
	MainToolbarWndProc
};


static const Widget _toolb_scen_widgets[] = {
{      WWT_PANEL,    14,     0,    21,     0,    21, 0x2D6, STR_0171_PAUSE_GAME},
{      WWT_PANEL,    14,    22,    43,     0,    21, 0x2EF, STR_0187_OPTIONS},
{    WWT_PANEL_2,    14,    44,    65,     0,    21, 0x2D4, STR_0297_SAVE_SCENARIO_LOAD_SCENARIO	},
{      WWT_PANEL,    14,    78,   207,     0,    21, 0x0},
{      WWT_PANEL,    14,   220,   349,     0,    21, 0x0},
{   WWT_CLOSEBOX,    14,   223,   233,     5,    16, STR_0225, STR_029E_MOVE_THE_STARTING_DATE},
{   WWT_CLOSEBOX,    14,   334,   344,     5,    16, STR_0224, STR_029F_MOVE_THE_STARTING_DATE},
{      WWT_PANEL,    14,   362,   383,     0,    21, 0x2C4, STR_0175_DISPLAY_MAP_TOWN_DIRECTORY},
{      WWT_PANEL,    14,   396,   417,     0,    21, 0x2DF, STR_017F_ZOOM_THE_VIEW_IN},
{      WWT_PANEL,    14,   418,   439,     0,    21, 0x2E0, STR_0180_ZOOM_THE_VIEW_OUT},
{      WWT_PANEL,    14,   452,   473,     0,    21, 0xFF3, STR_022E_LANDSCAPE_GENERATION},
{      WWT_PANEL,    14,   474,   495,     0,    21, 0xFED, STR_022F_TOWN_GENERATION},
{      WWT_PANEL,    14,   496,   517,     0,    21, 0x2E5, STR_0230_INDUSTRY_GENERATION},
{      WWT_PANEL,    14,   518,   539,     0,    21, 0x2D8, STR_0231_ROAD_CONSTRUCTION},
{      WWT_PANEL,    14,   540,   561,     0,    21, 0x2E6, STR_0288_PLANT_TREES},
{      WWT_PANEL,    14,   562,   583,     0,    21, 0xFF2, STR_0289_PLACE_SIGN},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{      WWT_PANEL,    14,   596,   617,     0,    21, 0x2C9,STR_01D4_SHOW_SOUND_MUSIC_WINDOW},
{      WWT_EMPTY,     0,     0,     0,     0,     0, 0x0},
{      WWT_PANEL,    14,   618,   639,     0,    21, 0x2D3, STR_0186_LAND_BLOCK_INFORMATION},
{      WWT_LAST},
};

static ToolbarButtonProc* const _scen_toolbar_button_procs[] = {
	ToolbarPauseClick,
	ToolbarOptionsClick,
	ToolbarScenSaveOrLoad,
	ToolbarBtn_NULL,
	ToolbarBtn_NULL,
	ToolbarScenDateBackward,
	ToolbarScenDateForward,
	ToolbarScenMapTownDir,
	ToolbarScenZoomIn,
	ToolbarScenZoomOut,
	ToolbarScenGenLand,
	ToolbarScenGenTown,
	ToolbarScenGenIndustry,
	ToolbarScenBuildRoad,
	ToolbarScenPlantTrees,
	ToolbarScenPlaceSign,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ToolbarMusicClick,
	NULL,
	ToolbarHelpClick,
};

static void ScenEditToolbarWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT:
		// Draw brown-red toolbar bg.
		GfxFillRect(0, 0, w->width-1, w->height-1, 0xB2);
		GfxFillRect(0, 0, w->width-1, w->height-1, 0x80B4);

		DrawWindowWidgets(w);

		SET_DPARAM16(0, _date);
		DrawStringCentered(285, 6, STR_00AF, 0);

		SET_DPARAM16(0, _date);
		DrawStringCentered(143, 1, STR_0221_TRANSPORT_TYCOON, 0);
		DrawStringCentered(143, 11,STR_0222_SCENARIO_EDITOR, 0);

		break;

	case WE_CLICK: {
		if (_game_mode == GM_MENU)
			return;
		_scen_toolbar_button_procs[e->click.widget](w);
	} break;
	
	case WE_PLACE_OBJ: {
		_place_proc(e->place.tile);
	} break;

	case WE_ABORT_PLACE_OBJ: {
		w->click_state &= ~(1<<25);
		SetWindowDirty(w);
	} break;

	case WE_ON_EDIT_TEXT: HandleOnEditText(e); break;
	}
}

static const WindowDesc _toolb_scen_desc = {
	0, 0, 0x280, 0x16,
	WC_MAIN_TOOLBAR,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_toolb_scen_widgets,
	ScenEditToolbarWndProc
};

extern GetNewsStringCallbackProc * const _get_news_string_callback[];


static bool DrawScrollingStatusText(NewsItem *ni, int pos)
{
	StringID str;
	byte *s, *d;
	DrawPixelInfo tmp_dpi, *old_dpi;
	int x;
	byte buffer[256];

	if (ni->display_mode == 3) {
		str = _get_news_string_callback[ni->callback](ni);
	} else {
		COPY_IN_DPARAM(0, ni->params, lengthof(ni->params));
		str = ni->string_id;	
	}

	GetString(str_buffr, str);

	s = str_buffr;
	d = buffer;

	for(;;s++) {
		if (*s == 0) {
			*d = 0;
			break;
		} else if (*s == 0x0D) {
			d[0] = d[1] = d[2] = d[3] = ' ';
			d+=4;
		} else if (*s >= ' ' && (*s < 0x88 || *s >= 0x99)) {
			*d++ = *s;
		}
	}

	if (!FillDrawPixelInfo(&tmp_dpi, NULL, 141, 1, 358, 11))
		return true;

	old_dpi = _cur_dpi;
	_cur_dpi = &tmp_dpi;

	x = DoDrawString(buffer, pos, 0, 13);
	_cur_dpi = old_dpi;

	return x > 0;
}

void StatusBarWndProc(Window *w, WindowEvent *e)
{
	Player *p;

	switch(e->event) {
	case WE_PAINT:
		DrawWindowWidgets(w);
		SET_DPARAM16(0, _date);
		DrawStringCentered(70, 1, ((_game_paused||_patches.status_long_date)?STR_00AF:STR_00AE), 0);
		
		p = DEREF_PLAYER(_local_player);

		// Draw player money
		SET_DPARAM64(0, p->money64);
		DrawStringCentered(570, 1, p->player_money >= 0 ? STR_0004 : STR_0005, 0);

		// Draw status bar
		if (_do_autosave) {
			DrawStringCentered(320, 1,	STR_032F_AUTOSAVE, 0);
		} else if (_game_paused) {
			DrawStringCentered(320, 1,	STR_0319_PAUSED, 0);
		} else if (WP(w,def_d).data_1 > -1280 && FindWindowById(WC_NEWS_WINDOW,0) == NULL && _statusbar_news_item.string_id != 0) {
			// Draw the scrolling news text
			if (!DrawScrollingStatusText(&_statusbar_news_item, WP(w,def_d).data_1))
				WP(w,def_d).data_1 = -1280;
		} else {
			// This is the default text
			SET_DPARAM16(0, p->name_1);
			SET_DPARAM32(1, p->name_2);
			DrawStringCentered(320, 1,	STR_02BA, 0);
		}
		break;

	case WE_CLICK:
		if (e->click.widget == 1) {
			ShowLastNewsMessage();
		} else {
			ResetObjectToPlace();
		}
		break;

	case WE_TICK: {
		if (_game_paused || WP(w,def_d).data_1 <= -1280)
			return;
		WP(w,def_d).data_1 -= 2;
		InvalidateWidget(w, 1);
		break;
	}
	}
}

static void ScrollMainViewport(int x, int y)
{
	if (_game_mode != GM_MENU) {
		Window *w = FindWindowById(WC_MAIN_WINDOW, 0);
		assert(w);

		WP(w,vp_d).scrollpos_x += x << w->viewport->zoom;
		WP(w,vp_d).scrollpos_y += y << w->viewport->zoom;
	}
}


static const Widget _main_status_widgets[] = {
{     WWT_IMGBTN,    14,     0,   139,     0,    11, 0x0, 0},
{ WWT_PUSHIMGBTN,    14,   140,   499,     0,    11, 0x0, STR_02B7_SHOW_LAST_MESSAGE_OR_NEWS},
{     WWT_IMGBTN,    14,   500,   639,     0,    11, 0x0, 0},
{      WWT_LAST},
};

static WindowDesc _main_status_desc = {
	WDP_CENTER, 0, 640, 0xC,
	WC_STATUS_BAR,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_main_status_widgets,
	StatusBarWndProc
};

extern void DebugProc(int i);

static void MainWindowWndProc(Window *w, WindowEvent *e) {
	int off_x;

	switch(e->event) {
	case WE_PAINT:
		DrawWindowViewport(w);
		if (_game_mode == GM_MENU) {
			DrawSprite(0x12E2, 30, _screen.height - 41);
			DrawSprite(0x12E3, _screen.width - 182, _screen.height - 37);

			off_x = ((_screen.width - 500) >> 1) - 70;
			DrawSprite(0x12E4, off_x+70, 50);
			DrawSprite(0x12E5, off_x+99, 50);
			DrawSprite(0x12E6, off_x+128,50);
			DrawSprite(0x12E7, off_x+161,50);
			DrawSprite(0x12E8, off_x+195,50);
			DrawSprite(0x12E9, off_x+226,50);
			DrawSprite(0x12EA, off_x+255,50);
			DrawSprite(0x12E5, off_x+287,50);
			DrawSprite(0x12E4, off_x+317,50);
			DrawSprite(0x12E4, off_x+390,50);
			DrawSprite(0x12EB, off_x+417,50);
			DrawSprite(0x12EC, off_x+447,50);
			DrawSprite(0x12EA, off_x+478,50);
			DrawSprite(0x12EA, off_x+509,50);
			DrawSprite(0x12E7, off_x+541,50);
		}
		break;

	case WE_KEYPRESS:
		switch(e->keypress.keycode) {
		case 'C':
		case 'Z': {
			Point pt;
			if (_game_mode != GM_MENU) {
				pt = GetTileBelowCursor();
				if (pt.x != -1) {
					ScrollMainWindowTo(pt.x, pt.y);
					if (e->keypress.keycode == 'Z')
						MaxZoomIn();
				}
			}
			break;
		}
		case WKC_ESC: ResetObjectToPlace(); break;
		case WKC_DELETE: DeleteNonVitalWindows(); break;
		case 'Q' | WKC_CTRL: AskExitGame(); break;
		case 'R' | WKC_CTRL: MarkWholeScreenDirty(); break;
		case 'I' | WKC_CTRL: DoTestLoad(); break;
		case 'O' | WKC_CTRL: DoTestSave(); break;
		case '0' | WKC_ALT:
		case '1' | WKC_ALT:
		case '2' | WKC_ALT:
		case '3' | WKC_ALT:
		case '4' | WKC_ALT:
			DebugProc(e->keypress.keycode - ('0' | WKC_ALT));
			break;
		case 'A' | WKC_CTRL:
			disable_computer^=1;
			ShowInfoF("Disable Computer: %s", disable_computer?"ON":"OFF");
			break;
		case 'B' | WKC_CTRL:
			_local_player^=1;
			ShowInfoF("Play as computer: %s", _local_player?"ON":"OFF");
			break;

		case WKC_UP: ScrollMainViewport(0, -10); break;
		case WKC_DOWN: ScrollMainViewport(0, 10); break;
		case WKC_LEFT: ScrollMainViewport(-10, 0); break;
		case WKC_RIGHT: ScrollMainViewport(10, 0); break;

		default:
			return;
		}
		e->keypress.cont = false;
		break;
	}
}


void ShowSelectGameWindow();

void SetupColorsAndInitialWindow()
{
	int i;
	byte *b;
	Window *w;
	int width,height;

	for(i=0; i!=16; i++) {
		b = GetSpritePtr(0x307 + i);
		assert(b);
		_color_list[i] = *(ColorList*)(b + 0xC6);
	}

	width = _screen.width;
	height = _screen.height;

	// XXX: these are not done
	switch(_game_mode) {
	case GM_MENU:
		w = AllocateWindow(0, 0, width, height, MainWindowWndProc, 0, NULL);
		AssignWindowViewport(w, 0, 0, width, height, 0, 0);
//		w = AllocateWindowDesc(&_toolb_intro_desc);
//		w->flags4 &= ~WF_WHITE_BORDER_MASK;
		ShowSelectGameWindow();
		break;
	case GM_NORMAL:
		w = AllocateWindow(0, 0, width, height, MainWindowWndProc, 0, NULL);
		AssignWindowViewport(w, 0, 0, width, height, 0, 0);

		w = AllocateWindowDesc(&_toolb_normal_desc);
		w->disabled_state = 1 << 16;
		w->flags4 &= ~WF_WHITE_BORDER_MASK;

		_main_status_desc.top = height - 12;
		w = AllocateWindowDesc(&_main_status_desc);
		w->flags4 &= ~WF_WHITE_BORDER_MASK;

		WP(w,def_d).data_1 = -1280;

		break;
	case GM_EDITOR:
		w = AllocateWindow(0, 0, width, height, MainWindowWndProc, 0, NULL);
		AssignWindowViewport(w, 0, 0, width, height, 0, 0);

		w = AllocateWindowDesc(&_toolb_scen_desc);
		w->disabled_state = 1 << 8;
		w->flags4 &= ~WF_WHITE_BORDER_MASK;

		break;
	default:
		NOT_REACHED();
	}
}

void GameSizeChanged()
{
	RelocateAllWindows(_screen.width, _screen.height);
	ScreenSizeChanged();
	MarkWholeScreenDirty();
}
