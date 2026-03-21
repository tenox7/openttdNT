#include "stdafx.h"
#include "ttd.h"

#include "window.h"
#include "gui.h"
#include "viewport.h"
#include "gfx.h"
#include "player.h"
#include "command.h"

static void ShowSelectTutorialWindow()
{
}

static const Widget _select_game_widgets[] = {
{    WWT_CAPTION,    13,     0,   335,     0,    13, STR_0307_TRANSPORT_TYCOON_DELUXE},
{     WWT_IMGBTN,    13,     0,   335,    14,   196, 0x0},
{ WWT_PUSHTXTBTN,    12,    10,   167,    22,    33, STR_0140_NEW_GAME, STR_02FB_START_A_NEW_GAME},
{ WWT_PUSHTXTBTN,    12,   168,   325,    22,    33, STR_0141_LOAD_GAME, STR_02FC_LOAD_A_SAVED_GAME_FROM},
//{ WWT_PUSHTXTBTN,    12,    10,   167,   177,   188, STR_0142_TUTORIAL_DEMONSTRATION, STR_02FD_VIEW_DEMONSTRATIONS_TUTORIALS},
{ WWT_PUSHTXTBTN,    12,    10,   167,   177,   188, STR_CONFIG_PATCHES, STR_CONFIG_PATCHES_TIP},
{ WWT_PUSHTXTBTN,    12,    10,   167,    40,    51, STR_0220_CREATE_SCENARIO, STR_02FE_CREATE_A_CUSTOMIZED_GAME},
{ WWT_PUSHTXTBTN,    12,    10,   167,   136,   147, STR_0143_1_PLAYER, STR_02FF_SELECT_SINGLE_PLAYER_GAME},
{ WWT_PUSHTXTBTN,    12,   168,   325,   136,   147, STR_0144_2_PLAYERS, STR_0300_SELECT_TWO_PLAYER_GAME},
{ WWT_PUSHTXTBTN,    12,    10,   167,   159,   170, STR_0148_GAME_OPTIONS, STR_0301_DISPLAY_GAME_OPTIONS},
{ WWT_PUSHTXTBTN,    12,   168,   325,   159,   170, STR_01FE_DIFFICULTY,STR_0302_DISPLAY_DIFFICULTY_OPTIONS},
{ WWT_PUSHTXTBTN,    12,   168,   325,    40,    51, STR_029A_PLAY_SCENARIO, STR_0303_START_A_NEW_GAME_USING},
{ WWT_PUSHTXTBTN,    12,   168,   325,   177,   188, STR_0304_QUIT, STR_0305_LEAVE_TRANSPORT_TYCOON},
{    WWT_PANEL_2,    12,    10,    85,    69,   122, 0x1312, STR_030E_SELECT_TEMPERATE_LANDSCAPE},
{    WWT_PANEL_2,    12,    90,   165,    69,   122, 0x1314, STR_030F_SELECT_SUB_ARCTIC_LANDSCAPE},
{    WWT_PANEL_2,    12,   170,   245,    69,   122, 0x1316, STR_0310_SELECT_SUB_TROPICAL_LANDSCAPE},
{    WWT_PANEL_2,    12,   250,   325,    69,   122, 0x1318, STR_0311_SELECT_TOYLAND_LANDSCAPE},
{      WWT_LAST},
};

static void SelectGameWndProc(Window *w, WindowEvent *e) {
	switch(e->event) {
	case WE_PAINT:
		w->click_state = (w->click_state & ~(0xC0) & ~(0xF << 12)) | (1 << (_new_opt.landscape+12)) | (_num_human_players==1?(1<<6):(1<<7));
		w->disabled_state = _num_human_players==2 ? 0x30 : 0;
		SET_DPARAM16(0, STR_6801_EASY + _new_opt.diff_level);
		DrawWindowWidgets(w);
		break;

	case WE_CLICK:
		switch(e->click.widget) {
		case 2: DoCommandByTile(0, 0, 0, DC_EXEC, CMD_START_NEW_GAME); break;
		case 3: ShowSaveLoadDialog(SLD_LOAD_GAME); break;
		case 4: ShowPatchesSelection(); break;
		case 5: DoCommandByTile(0, InteractiveRandom(), 0, DC_EXEC, CMD_CREATE_SCENARIO); break;
		case 6: 
			if (_num_human_players != 1)
				DoCommandByTile(0, 0, 0, DC_EXEC, CMD_SET_SINGLE_PLAYER);
			break;
		case 7:
			if (_num_human_players != 2)
				ShowInfo("Multiplayer is not implemented in OpenTTD yet\n");
			break;
		case 8: ShowGameOptions(); break;
		case 9: ShowGameDifficulty(); break;
		case 10:ShowSaveLoadDialog(SLD_LOAD_SCENARIO); break;
		case 11:AskExitGame(); break;
		case 12: case 13: case 14: case 15:
			DoCommandByTile(0, e->click.widget - 12, 0, DC_EXEC, CMD_SET_NEW_LANDSCAPE_TYPE);
			break;
		}
		break;
	}
}

static const WindowDesc _select_game_desc = {
	WDP_CENTER, WDP_CENTER, 0x150, 0xC5,
	WC_SELECT_GAME,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_select_game_widgets,
	SelectGameWndProc
};

void ShowSelectGameWindow()
{
	AllocateWindowDesc(&_select_game_desc);
}


static const Widget _select_scenario_widgets[] = {
{    WWT_TEXTBTN,     7,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   256,     0,    13, STR_400E_SELECT_NEW_GAME_TYPE, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,     7,     0,   245,    14,   319, 0x0},
{          WWT_6,     7,     2,   243,    16,   317, 0x0, STR_400F_SELECT_SCENARIO_GREEN_PRE},
{  WWT_SCROLLBAR,     7,   246,   256,    14,   319, 0x0, STR_0190_SCROLL_BAR_SCROLLS_LIST},
{      WWT_LAST},
};


static void SelectScenarioWndProc(Window *w, WindowEvent *e) {
	switch(e->event) {
	case WE_PAINT:
		DrawWindowWidgets(w);
		DrawString(4, 0x11, STR_4010_GENERATE_RANDOM_NEW_GAME, 9);
		break;

	case WE_CLICK:
		switch(e->click.widget) {
		case 0: // Close button
			
			// Close the choose game dialog.
			DoCommandByTile(0, 1, 0, DC_EXEC, CMD_START_NEW_GAME);
			break;
		
		case 1: // Drag the window
			StartWindowDrag(w);
			break;

		case 3: {// Click the listbox
//			int y = (e->click.pt.y - 0x11) / 10;
			DoCommandByTile(0, 0, 0, DC_EXEC, CMD_GEN_RANDOM_NEW_GAME);
			break;
		}			
		}
	case WE_DESTROY:
		break;
	}
}

static const WindowDesc _select_scenario_desc = {
	WDP_CENTER, WDP_CENTER, 0x101, 0x140,
	WC_SAVELOAD,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_select_scenario_widgets,
	SelectScenarioWndProc
};

static void AskForNewGameToStart()
{
	Window *w;

	DeleteWindowById(WC_QUERY_STRING, 0);
	DeleteWindowById(WC_SAVELOAD, 0);

	w = AllocateWindowDesc(&_select_scenario_desc);
	w->vscroll.cap = 30;

}

// p1 = mode
//    0 - start new game
//    1 - close new game dialog

int32 CmdStartNewGame(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (!(flags & DC_EXEC))
		return 0;

	switch(p1) {
	case 0:
		AskForNewGameToStart();
		break;
	case 1:
		DeleteWindowById(WC_SAVELOAD, 0);
		break;
	}
	
	return 0;
}

int32 CmdGenRandomNewGame(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (!(flags & DC_EXEC))
		return 0;

	// Setup difficulty level..
 	_switch_mode = SM_NEWGAME;
	return 0;
}

int32 CmdLoadGame(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (!(flags & DC_EXEC))
		return 0;

//	ShowSaveLoadDialog(0);
	return 0;
}

int32 CmdCreateScenario(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (!(flags & DC_EXEC))
		return 0;

	_switch_mode = SM_EDITOR;
	return 0;
}

int32 CmdSetSinglePlayer(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	printf("CmdSetSinglePlayer\n");
	return 0;
}

static const Widget _ask_abandon_game_widgets[] = {
{    WWT_TEXTBTN,     4,     0,    10,     0,    13, STR_00C5},
{    WWT_CAPTION,     4,    11,   179,     0,    13, STR_00C7_QUIT},
{     WWT_IMGBTN,     4,     0,   179,    14,    91, 0x0},
{    WWT_TEXTBTN,    12,    25,    84,    72,    83, STR_00C9_NO},
{    WWT_TEXTBTN,    12,    95,   154,    72,    83, STR_00C8_YES},
{      WWT_LAST},
};

static void AskAbandonGameWndProc(Window *w, WindowEvent *e) {
	switch(e->event) {
	case WE_PAINT:
		DrawWindowWidgets(w);
#if defined(_WIN32)
		SET_DPARAM16(0, STR_0133_WINDOWS);
#elif defined(__APPLE__)
		SET_DPARAM16(0, STR_0135_OSX);
#else
		SET_DPARAM16(0, STR_0134_UNIX);
#endif
		DrawStringMultiCenter(0x5A, 0x26, STR_00CA_ARE_YOU_SURE_YOU_WANT_TO, 178);
		return;

	case WE_CLICK:
		switch(e->click.widget) {
		case 3:
			DoCommandByTile(0, 1, 0, DC_EXEC, CMD_QUIT_GAME);
			break;
		case 4:
			DoCommandByTile(0, 2, 0, DC_EXEC, CMD_QUIT_GAME);
			break;
		}
		break;

	case WE_DESTROY:
		UnpauseGame();
		break;
	}
}

void AskExitGame()
{
	DoCommandByTile(0, 0, 0, DC_EXEC, CMD_QUIT_GAME);
}

static const WindowDesc _ask_abandon_game_desc = {
	WDP_CENTER, WDP_CENTER, 0xB4, 0x5C,
	WC_ASK_ABANDON_GAME,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET | WDF_STD_BTN | WDF_UNCLICK_BUTTONS,
	_ask_abandon_game_widgets,
	AskAbandonGameWndProc
};


int32 CmdQuitGame(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (!(flags & DC_EXEC))
		return 0;

	switch(p1) {
	case 0:
		if (AllocateWindowDescFront(&_ask_abandon_game_desc, 0))
			PauseGame();
		break;

	case 1:
		DeleteWindowById(WC_ASK_ABANDON_GAME, 0);
		break;

	case 2:
		_exit_game = true;
		break;
	}
	return 0;
}

static const Widget _ask_quit_game_widgets[] = {
{    WWT_TEXTBTN,     4,     0,    10,     0,    13, STR_00C5},
{    WWT_CAPTION,     4,    11,   179,     0,    13, STR_0161_QUIT_GAME},
{     WWT_IMGBTN,     4,     0,   179,    14,    91, 0x0},
{    WWT_TEXTBTN,    12,    25,    84,    72,    83, STR_00C9_NO},
{    WWT_TEXTBTN,    12,    95,   154,    72,    83, STR_00C8_YES},
{      WWT_LAST},
};

static void AskQuitGameWndProc(Window *w, WindowEvent *e) {
	switch(e->event) {
	case WE_PAINT:
		DrawWindowWidgets(w);
		DrawStringMultiCenter(0x5A, 0x26, 
			_game_mode != GM_EDITOR ? STR_0160_ARE_YOU_SURE_YOU_WANT_TO : 
				STR_029B_ARE_YOU_SURE_YOU_WANT_TO, 
			178);
		return;

	case WE_CLICK:
		switch(e->click.widget) {
		case 3:
			DoCommandByTile(0, 1, 0, DC_EXEC, CMD_EXIT_TO_GAME_MENU);
			break;
		case 4:
			DoCommandByTile(0, 2, 0, DC_EXEC, CMD_EXIT_TO_GAME_MENU);
			break;
		}
		break;

	case WE_DESTROY:
		UnpauseGame();
		break;
	}
}


static const WindowDesc _ask_quit_game_desc = {
	WDP_CENTER, WDP_CENTER, 0xB4, 0x5C,
	WC_QUIT_GAME,0,
	WDF_STD_TOOLTIPS | WDF_DEF_WIDGET | WDF_STD_BTN | WDF_UNCLICK_BUTTONS,
	_ask_quit_game_widgets,
	AskQuitGameWndProc
};


int32 CmdExitToGameMenu(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (!(flags & DC_EXEC))
		return 0;

	switch(p1) {
	case 0:
		if (AllocateWindowDescFront(&_ask_quit_game_desc, 0))
			PauseGame();
		break;
	case 1:
		DeleteWindowById(WC_QUIT_GAME, 0);
		break;
	case 2:
		_switch_mode = SM_MENU;
		break;
	}
	return 0;
}

int32 CmdSetNewLandscapeType(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (flags & DC_EXEC) {
		// XXX: some stuff
		_new_opt.landscape = p1;
		InvalidateWindowClasses(WC_SELECT_GAME);
	}
	return 0;
}
