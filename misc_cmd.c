
#include "stdafx.h"
#include "ttd.h"
#include "command.h"
#include "player.h"
#include "gfx.h"
#include "window.h"
#include "saveload.h"
#include "economy.h"

/* p1 = player
   p2 = face
 */

int32 CmdSetPlayerFace(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (flags & DC_EXEC) {
		DEREF_PLAYER(p1)->face = p2;
		MarkWholeScreenDirty();
	}
	return 0;
}

/* p1 = player
 * p2 = color
 */
int32 CmdSetPlayerColor(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Player *p,*pp;

//	/* can only set color for itself */
//	if ( (byte)p1 != _current_player)
//		return CMD_ERROR;

	p = DEREF_PLAYER(p1);

	/* ensure no dups */
	FOR_ALL_PLAYERS(pp) {
		if (pp->name_1 != 0 && pp != p && pp->player_color == (byte)p2)
			return CMD_ERROR;
	}
	
	if (flags & DC_EXEC) {
		_player_colors[p1] = (byte)p2;
		p->player_color = (byte)p2;
		MarkWholeScreenDirty();
	}
	return 0;
}

int32 CmdIncreaseLoan(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Player *p;
	int32 size;

	if ( (byte)p1 != _current_player)
		return CMD_ERROR;

	p = DEREF_PLAYER(p1);

	if (p->current_loan >= _economy.max_loan) {
		SET_DPARAM32(0, _economy.max_loan);
		return_cmd_error(STR_702B_MAXIMUM_PERMITTED_LOAN);
	}

	if (flags & DC_EXEC) {
		if (p2)
			size = _economy.max_loan - p->current_loan;
		else
			size = IS_HUMAN_PLAYER((byte)p1) ? 10000 : 50000;

		p->money64 += size;
		p->current_loan += size;
		UpdatePlayerMoney32(p);
		InvalidatePlayerWindows(p);
	}

	return 0;
}

int32 CmdDecreaseLoan(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Player *p;
	int32 size;
	if ( (byte)p1 != _current_player)
		return CMD_ERROR;

	p = DEREF_PLAYER(p1);

	if (p->current_loan == 0)
		return_cmd_error(STR_702D_LOAN_ALREADY_REPAYED);

	size = p->current_loan;

	if (!p2)
		size = min(size, IS_HUMAN_PLAYER((byte)p1) ? 10000 : 50000);

	if (p->player_money < size) {
		SET_DPARAM32(0, size);
		return_cmd_error(STR_702E_REQUIRED);
	}	

	if (flags & DC_EXEC) {
		p->money64 -= size;
		p->current_loan -= size;
		UpdatePlayerMoney32(p);
		InvalidatePlayerWindows(p);
	}
	return 0;
}

int32 CmdChangeCompanyName(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	StringID str,old_str;
	Player *p;

	str = AllocateName((byte*)_decode_parameters, 4);
	if (str == 0)
		return CMD_ERROR;
	
	if (flags & DC_EXEC) {
		p = DEREF_PLAYER(p1);
		old_str = p->name_1;
		p->name_1 = str;
		DeleteName(old_str);
		MarkWholeScreenDirty();
	} else {
		DeleteName(str);
	}

	return 0;
}

int32 CmdChangePresidentName(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	StringID str,old_str;
	Player *p;

	str = AllocateName((byte*)_decode_parameters, 4);
	if (str == 0)
		return CMD_ERROR;
	
	if (flags & DC_EXEC) {
		p = DEREF_PLAYER(p1);
		old_str = p->president_name_1;
		p->president_name_1 = str;
		DeleteName(old_str);

		if (p->name_1 == STR_SV_UNNAMED) {
			byte *s = " Transport";
			byte *d = (byte*)_decode_parameters, b;
			d--; do d++; while (*d);
			do *d++ = b = *s++; while(d != (byte*)endof(_decode_parameters) && b != 0);
			DoCommandByTile(0, p1, 0, DC_EXEC, CMD_CHANGE_COMPANY_NAME);
		} 
		MarkWholeScreenDirty();
	} else {
		DeleteName(str);
	}

	return 0;	
}

static void UpdateSignVirtCoords(SignStruct *ss)
{
	Point pt = RemapCoords(ss->x, ss->y, ss->z);
	SET_DPARAM16(0, ss->str);
	UpdateViewportSignPos(&ss->sign, pt.x, pt.y - 6, STR_2806);
}

void UpdateAllSignVirtCoords()
{
	SignStruct *ss;
	for(ss=_sign_list; ss != endof(_sign_list); ss++)
		if (ss->str != 0)
			UpdateSignVirtCoords(ss);

}

static void MarkSignDirty(SignStruct *ss)
{
	MarkAllViewportsDirty(
		ss->sign.left-6,
		ss->sign.top-3,
		ss->sign.left+ss->sign.width_1*4+12,
		ss->sign.top + 45
	);
}


int32 CmdPlaceSign(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	SignStruct *ss;
	
	for(ss=_sign_list; ss != endof(_sign_list); ss++) {
		if (ss->str == 0) {
			if (flags & DC_EXEC) {
				ss->str = STR_280A_SIGN;
				ss->x = x;
				ss->y = y;
				ss->z = (byte)GetSlopeZ(x,y);
				UpdateSignVirtCoords(ss);
				MarkSignDirty(ss);
				_new_sign_struct = ss;
			}
			return 0;
		}	
	}

	return_cmd_error(STR_2808_TOO_MANY_SIGNS);
}

// p1 = sign index
int32 CmdRenameSign(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	StringID str,old_str;
	SignStruct *ss;

	if (_decode_parameters[0] != 0) {
		str = AllocateName((byte*)_decode_parameters, 0);
		if (str == 0)
			return CMD_ERROR;

		if (flags & DC_EXEC) {
			ss = &_sign_list[p1];
			MarkSignDirty(ss);
			DeleteName(ss->str);
			ss->str = str;
			UpdateSignVirtCoords(ss);
			MarkSignDirty(ss);
		} else {
			DeleteName(str);
		}
	}	else {
		if (flags & DC_EXEC) {
			ss = &_sign_list[p1];
			old_str = ss->str;
			ss->str = 0;
			DeleteName(old_str);
		}
	}
	return 0;
}


int32 CmdPauseOrResume(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Window *w;

	if (flags & DC_EXEC) {
		w = FindWindowById(WC_MAIN_TOOLBAR, 0);
		w->click_state ^= 1 << 0;
		if (w->click_state & (1 << 0)) {
			PauseGame();
		} else {
			UnpauseGame();
		}
		SetWindowDirty(w);
	}
	return 0;
}


static const int16 _default_game_diff[3][17] = {
	{2, 2, 1, 2, 300, 2, 0, 2, 0, 1, 2, 0, 1, 0, 0, 0, 0},
	{4, 1, 1, 1, 150, 3, 1, 3, 1, 2, 1, 1, 2, 0, 1, 1, 1},
	{7, 0, 2, 1, 100, 4, 1, 3, 2, 2, 0, 2, 3, 1, 1, 1, 1},
};

void SetDifficultyLevel(int mode)
{
	int i;

	_opt_mod_ptr->diff_level = mode;
	if (mode != 3) {
		for(i=0; i!=17; i++)
			((int*)&_opt_mod_ptr->diff)[i] = _default_game_diff[mode][i];
	}

}

int32 CmdChangeDifficultyLevel(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	if (flags & DC_EXEC) {
		if (p1 != (uint32)-1L) {
			((int*)&_opt_mod_ptr->diff)[p1] = p2;
			SetDifficultyLevel(3);		
		} else {
			SetDifficultyLevel(p2);
		}
		InvalidateWindow(WC_GAME_OPTIONS, 0);
	}
	return 0;	
}

static const byte _sign_desc[] = {
	SLE_VAR(SignStruct,str,						SLE_UINT16),
	SLE_VAR(SignStruct,x,							SLE_INT16),
	SLE_VAR(SignStruct,y,							SLE_INT16),
	SLE_VAR(SignStruct,z,							SLE_UINT8),
	SLE_END()
};

static void Save_SIGN()
{
	SignStruct *s;
	int i;
	for(i=0,s=_sign_list; i!=lengthof(_sign_list); i++,s++) {
		if (s->str != 0) {
			SlSetArrayIndex(i);
			SlObject(s, _sign_desc);
		}
	}
}

static void Load_SIGN()
{
	int index;
	while ((index = SlIterateArray()) != -1) {
		SlObject(&_sign_list[index], _sign_desc);
	}
}

const ChunkHandler _sign_chunk_handlers[] = {
	{ 'SIGN', Save_SIGN, Load_SIGN, CH_ARRAY | CH_LAST},
};


