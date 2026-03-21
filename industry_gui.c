#include "stdafx.h"
#include "ttd.h"

#include "window.h"
#include "gfx.h"
#include "command.h"
#include "viewport.h"
#include "industry.h"
#include "city.h"

static const byte _build_industry_types[4][8] = {
	{	1, 2, 4, 6, 8 },
	{ 1, 14, 4, 13, 7 },
	{ 25, 13, 4, 23, 22 },
	{ 27, 30, 31, 33 },
};

extern const byte _industry_type_costs[37];

static void BuildIndustryWndProc(Window *w, WindowEvent *e)
{
	switch(e->event) {
	case WE_PAINT:
		DrawWindowWidgets(w);
		if (_thd.place_mode == 1 && _thd.window_class == WC_BUILD_INDUSTRY) {
			int ind_type = _build_industry_types[_opt.landscape][WP(w,def_d).data_1];

			SET_DPARAM32(0, (_price.build_industry >> 8) * _industry_type_costs[ind_type]);
			DrawStringCentered(85, w->height - 21, STR_482F_COST, 0);
		}
		break;

	case WE_CLICK: {
		int wid = e->click.widget;
		if (wid >= 3) {
			if (HandlePlacePushButton(w, wid, 0xFF1, 1, NULL))
				WP(w,def_d).data_1 = wid - 3;
		}
	} break;

	case WE_PLACE_OBJ:
		if (DoCommandByTile(e->place.tile, _build_industry_types[_opt.landscape][WP(w,def_d).data_1], 0, 
			DC_EXEC | DC_MSG(STR_4830_CAN_T_CONSTRUCT_THIS_INDUSTRY), CMD_BUILD_INDUSTRY) != CMD_ERROR) {
			ResetObjectToPlace();
		}
		break;
		
	case WE_ABORT_PLACE_OBJ:
		w->click_state = 0;
		SetWindowDirty(w);
		break;
	}
}

static const Widget _build_industry_land0_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5,STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   169,     0,    13, STR_0314_FUND_NEW_INDUSTRY,STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,     7,     0,   169,    14,   115, 0x0,0},
{   WWT_CLOSEBOX,    14,     2,   167,    16,    27, STR_0241_POWER_STATION,STR_0263_CONSTRUCT_POWER_STATION},
{   WWT_CLOSEBOX,    14,     2,   167,    29,    40, STR_0242_SAWMILL,STR_0264_CONSTRUCT_SAWMILL},
{   WWT_CLOSEBOX,    14,     2,   167,    42,    53, STR_0244_OIL_REFINERY,STR_0266_CONSTRUCT_OIL_REFINERY},
{   WWT_CLOSEBOX,    14,     2,   167,    55,    66, STR_0246_FACTORY,STR_0268_CONSTRUCT_FACTORY},
{   WWT_CLOSEBOX,    14,     2,   167,    68,    79, STR_0247_STEEL_MILL,STR_0269_CONSTRUCT_STEEL_MILL},
{      WWT_LAST},
};

static const Widget _build_industry_land1_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5,STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   169,     0,    13, STR_0314_FUND_NEW_INDUSTRY,STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,     7,     0,   169,    14,   115, 0x0,0},
{   WWT_CLOSEBOX,    14,     2,   167,    16,    27, STR_0241_POWER_STATION,STR_0263_CONSTRUCT_POWER_STATION},
{   WWT_CLOSEBOX,    14,     2,   167,    29,    40, STR_024C_PAPER_MILL, STR_026E_CONSTRUCT_PAPER_MILL},
{   WWT_CLOSEBOX,    14,     2,   167,    42,    53, STR_0244_OIL_REFINERY,STR_0266_CONSTRUCT_OIL_REFINERY},
{   WWT_CLOSEBOX,    14,     2,   167,    55,    66, STR_024D_FOOD_PROCESSING_PLANT,STR_026F_CONSTRUCT_FOOD_PROCESSING},
{   WWT_CLOSEBOX,    14,     2,   167,    68,    79, STR_024E_PRINTING_WORKS,STR_0270_CONSTRUCT_PRINTING_WORKS},
{      WWT_LAST},
};

static const Widget _build_industry_land2_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5,STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   169,     0,    13, STR_0314_FUND_NEW_INDUSTRY,STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,     7,     0,   169,    14,   115, 0x0,0},
{   WWT_CLOSEBOX,    14,     2,   167,    16,    27, STR_0250_LUMBER_MILL,STR_0273_CONSTRUCT_LUMBER_MILL_TO},
{   WWT_CLOSEBOX,    14,     2,   167,    29,    40, STR_024D_FOOD_PROCESSING_PLANT,STR_026F_CONSTRUCT_FOOD_PROCESSING},
{   WWT_CLOSEBOX,    14,     2,   167,    42,    53, STR_0244_OIL_REFINERY,STR_0266_CONSTRUCT_OIL_REFINERY},
{   WWT_CLOSEBOX,    14,     2,   167,    55,    66, STR_0246_FACTORY,STR_0268_CONSTRUCT_FACTORY},
{   WWT_CLOSEBOX,    14,     2,   167,    68,    79, STR_0254_WATER_TOWER,STR_0277_CONSTRUCT_WATER_TOWER_CAN},
{      WWT_LAST},
};

static const Widget _build_industry_land3_widgets[] = {
{   WWT_CLOSEBOX,     7,     0,    10,     0,    13, STR_00C5,STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     7,    11,   169,     0,    13, STR_0314_FUND_NEW_INDUSTRY,STR_018C_WINDOW_TITLE_DRAG_THIS},
{      WWT_PANEL,     7,     0,   169,    14,   115, 0x0,0},
{   WWT_CLOSEBOX,    14,     2,   167,    16,    27, STR_0258_CANDY_FACTORY,STR_027B_CONSTRUCT_CANDY_FACTORY},
{   WWT_CLOSEBOX,    14,     2,   167,    29,    40, STR_025B_TOY_SHOP,STR_027E_CONSTRUCT_TOY_SHOP},
{   WWT_CLOSEBOX,    14,     2,   167,    42,    53, STR_025C_TOY_FACTORY,STR_027F_CONSTRUCT_TOY_FACTORY},
{   WWT_CLOSEBOX,    14,     2,   167,    55,    66, STR_025E_FIZZY_DRINK_FACTORY,STR_0281_CONSTRUCT_FIZZY_DRINK_FACTORY},
{      WWT_LAST},
};

static const WindowDesc _build_industry_land0_desc = {
	-1, -1, 0xAA, 0x74,
	WC_BUILD_INDUSTRY,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_industry_land0_widgets,
	BuildIndustryWndProc
};

static const WindowDesc _build_industry_land1_desc = {
	-1, -1, 0xAA, 0x74,
	WC_BUILD_INDUSTRY,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_industry_land1_widgets,
	BuildIndustryWndProc
};

static const WindowDesc _build_industry_land2_desc = {
	-1, -1, 0xAA, 0x74,
	WC_BUILD_INDUSTRY,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_industry_land2_widgets,
	BuildIndustryWndProc
};

static const WindowDesc _build_industry_land3_desc = {
	-1, -1, 0xAA, 0x74,
	WC_BUILD_INDUSTRY,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET,
	_build_industry_land3_widgets,
	BuildIndustryWndProc
};

static const WindowDesc * const _industry_window_desc[] = {
	&_build_industry_land0_desc,
	&_build_industry_land1_desc,
	&_build_industry_land2_desc,
	&_build_industry_land3_desc,
};

void ShowBuildIndustryWindow()
{	
	AllocateWindowDescFront(_industry_window_desc[_opt.landscape],0);
}

static void IndustryViewWndProc(Window *w, WindowEvent *e)
{
	Industry *i;
	StringID str;

	switch(e->event) {
	case WE_PAINT:
		i = &_industries[w->window_number];
		SET_DPARAM16(0, i->city->index);
		SET_DPARAM16(1, i->type + STR_4802_COAL_MINE);
		DrawWindowWidgets(w);

		if (i->accepts_cargo[0] != 0xFF) {
			SET_DPARAM16(0, _cargoc.names_s[i->accepts_cargo[0]]);
			str = STR_4827_REQUIRES;
			if (i->accepts_cargo[1] != 0xFF) {
				SET_DPARAM16(1, _cargoc.names_s[i->accepts_cargo[1]]);
				str++;
				if (i->accepts_cargo[2] != 0xFF) {
					SET_DPARAM16(2, _cargoc.names_s[i->accepts_cargo[2]]);
					str++;
				}
			}
			DrawString(2, 107, str, 0);
		}
		
		if (i->produced_cargo[0] != 0xFF) {
			DrawString(2, 117, STR_482A_PRODUCTION_LAST_MONTH, 0);

			SET_DPARAM16(1, i->total_production[0]);
			SET_DPARAM16(0, _cargoc.names_long_s[i->produced_cargo[0]] + ((i->total_production[0]!=1)<<5)); 
			SET_DPARAM16(2, i->pct_transported[0] * 100 >> 8);
			DrawString(4, 127, STR_482B_TRANSPORTED, 0);

			if (i->produced_cargo[1] != 0xFF) {
				SET_DPARAM16(1, i->total_production[1]);
				SET_DPARAM16(0, _cargoc.names_long_s[i->produced_cargo[1]] + ((i->total_production[1]!=1)<<5)); 
				SET_DPARAM16(2, i->pct_transported[1] * 100 >> 8);
				DrawString(4, 137, STR_482B_TRANSPORTED, 0);				
			}
		}

		DrawWindowViewport(w);
		break;

	case WE_CLICK:
		switch(e->click.widget) {
		case 5:
			i = &_industries[w->window_number];
			ScrollMainWindowToTile(i->xy + TILE_XY(1,1));
			break;
		}
		break;
	}
}

static const Widget _industry_view_widgets[] = {
{    WWT_TEXTBTN,     9,     0,    10,     0,    13, STR_00C5, STR_018B_CLOSE_WINDOW},
{    WWT_CAPTION,     9,    11,   259,     0,    13, STR_4801, STR_018C_WINDOW_TITLE_DRAG_THIS},
{     WWT_IMGBTN,     9,     0,   259,    14,   105, 0x0},
{          WWT_6,     9,     2,   257,    16,   103, 0x0},
{     WWT_IMGBTN,     9,     0,   259,   106,   147, 0x0},
{ WWT_PUSHTXTBTN,     9,     0,   129,   148,   159, STR_00E4_LOCATION, STR_482C_CENTER_THE_MAIN_VIEW_ON},
{     WWT_IMGBTN,     9,   130,   259,   148,   159, 0x0},
{      WWT_LAST},
};

static const WindowDesc _industry_view_desc = {
	-1, -1, 0x104, 0xA0,
	WC_INDUSTRY_VIEW,0,
	WDF_STD_TOOLTIPS | WDF_STD_BTN | WDF_DEF_WIDGET | WDF_UNCLICK_BUTTONS,
	_industry_view_widgets,
	IndustryViewWndProc
};

void ShowIndustryViewWindow(int industry)
{
	Window *w;
	Industry *i;

	w = AllocateWindowDescFront(&_industry_view_desc, industry);
	if (w) {
		w->flags4 |= WF_DISABLE_VP_SCROLL;
		i = &_industries[w->window_number];
		AssignWindowViewport(w, 3, 17, 0xFE, 0x56, i->xy + TILE_XY(1,1), 1);
	}
}