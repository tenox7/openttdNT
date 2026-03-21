#include "stdafx.h"
#include "ttd.h"
#include "pathfind.h"

static bool TPFSetTileBit(TrackPathFinder *tpf, uint tile, int dir)
{
	uint hash, val, offs;
	TrackPathFinderLink *link, *new_link;
	uint bits = 1 << dir;

	if (tpf->hasbit_12)
		return true;

	hash = PATHFIND_HASH_TILE(tile);

	val = tpf->hash_head[hash];

	if (val == 0) {
		/* unused hash entry, set the appropriate bit in it and return true
		 * to indicate that a bit was set. */
		tpf->hash_head[hash] = bits;
		tpf->hash_tile[hash] = (TileIndex)tile;
		return true;
	} else if (!(val & 0x8000)) {
		/* single tile */

		if ( (TileIndex)tile == tpf->hash_tile[hash] ) {
			/* found another bit for the same tile,
			 * check if this bit is already set, if so, return false */
			if (val & bits)
				return false;

			/* otherwise set the bit and return true to indicate that the bit
			 * was set */
			tpf->hash_head[hash] = val | bits;
			return true;
		} else {
			/* two tiles with the same hash, need to make a link */
			
			/* allocate a link. if out of links, handle this by returning
			 * that a tile was already visisted. */
			if (tpf->num_links_left == 0)
				return false;
			tpf->num_links_left--;
			link = tpf->new_link++;

			/* move the data that was previously in the hash_??? variables
			 * to the link struct, and let the hash variables point to the link */
			link->tile = tpf->hash_tile[hash];
			tpf->hash_tile[hash] = PATHFIND_GET_LINK_OFFS(tpf, link);

			link->flags = tpf->hash_head[hash];
			tpf->hash_head[hash] = 0xFFFF; /* multi link */ 

			link->next = 0xFFFF;
		}
	} else {
		/* a linked list of many tiles,
		 * find the one corresponding to the tile, if it exists.
		 * otherwise make a new link */
		
		offs = tpf->hash_tile[hash];
		do {
			link = PATHFIND_GET_LINK_PTR(tpf, offs);
			if ( (TileIndex)tile == link->tile) {
				/* found the tile in the link list,
				 * check if the bit was alrady set, if so return false to indicate that the
				 * bit was already set */
				if (link->flags & bits)
					return false;
				link->flags |= bits;
				return true;
			}
		} while ((offs=link->next) != 0xFFFF);
	}
	
	/* get here if we need to add a new link to link,
	 * first, allocate a new link, in the same way as before */
	if (tpf->num_links_left == 0)
			return false;
	tpf->num_links_left--;
	new_link = tpf->new_link++;

	/* then fill the link with the new info, and establish a ptr from the old
	 * link to the new one */
	new_link->tile = (TileIndex)tile;
	new_link->flags = bits;
	new_link->next = 0xFFFF;

	link->next = PATHFIND_GET_LINK_OFFS(tpf, new_link);
	return true;
}

static const byte _bits_mask[4] = {
	0x19,
	0x16,
	0x25,
	0x2A,
};

static const byte _tpf_new_direction[14] = {
	0,1,0,1,2,1, 0,0,
	2,3,3,2,3,0,
};

static const byte _otherdir_mask[4] = {
	0x10,
	0,
	0x5,
	0x2A,
};

#ifdef DEBUG_TILE_PUSH
extern void dbg_push_tile(uint tile, int track);
extern void dbg_pop_tile();
#endif

void TPFMode2(TrackPathFinder *tpf, uint tile, int direction)
{
	uint bits;
	int i;
	RememberData rd;

	// This addition will sometimes overflow by a single tile.
	// The use of TILE_MASK here makes sure that we still point at a valid
	// tile, and then this tile will be in the sentinel row/col, so GetTileTrackStatus will fail. 
	tile = TILE_MASK(tile + _tileoffs_by_dir[direction]);

	if (++tpf->rd.cur_length > 50)
		return;
	
	bits = GetTileTrackStatus(tile, tpf->tracktype);
	bits = (byte)((bits | (bits >> 8)) & _bits_mask[direction]);
	if (bits == 0)
		return;

	assert(GET_TILE_X(tile) != 255 && GET_TILE_Y(tile) != 255);

	if ( (bits & (bits - 1)) == 0 ) {
		/* only one direction */
		i = 0;
		while (!(bits&1))
			i++, bits>>=1;

		rd = tpf->rd;
		goto continue_here;
	}
	/* several directions */
	i=0;
	do {
		if (!(bits & 1)) continue;
		rd = tpf->rd;

		// Change direction 4 times only
		if ((byte)i != tpf->rd.pft_var6) {
			if(++tpf->rd.num_tries > 4) {
				tpf->rd = rd;		
				return;
			}
			tpf->rd.pft_var6 = (byte)i;
		}

continue_here:;
		tpf->the_dir = HASBIT(_otherdir_mask[direction],i) ? (i+8) : i;
		
#ifdef DEBUG_TILE_PUSH
		dbg_push_tile(tile, tpf->the_dir);
#endif
		if (!tpf->enum_proc(tile, tpf->userdata, tpf->the_dir, tpf->rd.cur_length, NULL)) {
			TPFMode2(tpf, tile, _tpf_new_direction[tpf->the_dir]);
		}
#ifdef DEBUG_TILE_PUSH
		dbg_pop_tile();
#endif

		tpf->rd = rd;
	} while (++i, bits>>=1);

}

static const int8 _get_tunlen_inc[5] = { -16, 0, 16, 0, -16 };

FindLengthOfTunnelResult FindLengthOfTunnel(uint tile, int direction, byte type)
{
	FindLengthOfTunnelResult flotr;
	int x,y;
	byte z;

	flotr.length = 0;

	x = GET_TILE_X(tile) * 16;
	y = GET_TILE_Y(tile) * 16;

	z = (byte)GetSlopeZ(x+8, y+8);

	for(;;) {
		flotr.length++;

		x += _get_tunlen_inc[direction];
		y += _get_tunlen_inc[direction+1];

		tile = TILE_FROM_XY(x,y);

		if (IS_TILETYPE(tile, MP_TUNNELBRIDGE) &&
				(_map5[tile] & 0xF0) == 0 &&
				((_map5[tile]>>1)&6) == type &&
				((_map5[tile] & 3)^2) == direction &&
				(byte)GetSlopeZ(x+8, y+8) == z)
					break;
	}

	flotr.tile = tile;
	return flotr;
}

static const uint16 _tpfmode1_and[4] = { 0x1009, 0x16, 0x520, 0x2A00 };

uint SkipToEndOfTunnel(TrackPathFinder *tpf, uint tile, int direction) {
	FindLengthOfTunnelResult flotr;
	TPFSetTileBit(tpf, tile, 14);
	flotr = FindLengthOfTunnel(tile, direction, tpf->tracktype);
	tpf->rd.cur_length += flotr.length;
	TPFSetTileBit(tpf, flotr.tile, 14);
	return flotr.tile;
}

const byte _ffb_64[128] = {
0,0,1,0,2,0,1,0,
3,0,1,0,2,0,1,0,
4,0,1,0,2,0,1,0,
3,0,1,0,2,0,1,0,
5,0,1,0,2,0,1,0,
3,0,1,0,2,0,1,0,
4,0,1,0,2,0,1,0,
3,0,1,0,2,0,1,0,

0,0,0,2,0,4,4,6,
0,8,8,10,8,12,12,14,
0,16,16,18,16,20,20,22,
16,24,24,26,24,28,28,30,
0,32,32,34,32,36,36,38,
32,40,40,42,40,44,44,46,
32,48,48,50,48,52,52,54,
48,56,56,58,56,60,60,62,
};

void TPFMode1(TrackPathFinder *tpf, uint tile, int direction)
{
	uint bits;
	int i;
	RememberData rd;
	uint tile_org = tile;

	if (IS_TILETYPE(tile, MP_TUNNELBRIDGE) &&
			(_map5[tile] & 0xF0)==0 && 
			(_map5[tile] & 3) == direction &&
			((_map5[tile]>>1)&6) == tpf->tracktype) {
		tile = SkipToEndOfTunnel(tpf, tile, direction);		
	} else {
		tile += _tileoffs_by_dir[direction];
	}

	tpf->rd.cur_length++;

	bits = GetTileTrackStatus(tile, tpf->tracktype);

	if ((byte)bits != tpf->var2) {
		bits &= _tpfmode1_and[direction];
		bits = bits | (bits>>8);
	}
	bits &= 0xBF;

	if (bits != 0) {
		if (!tpf->hasbit_12 || (tpf->rd.cur_length <= 64 && (KILL_FIRST_BIT(bits) == 0 || ++tpf->rd.num_tries <= 7))) {
			do {
				i = FIND_FIRST_BIT(bits);
				bits = KILL_FIRST_BIT(bits);

				tpf->the_dir = (_otherdir_mask[direction] & (byte)(1 << i)) ? (i+8) : i;
				rd = tpf->rd;
#ifdef DEBUG_TILE_PUSH
		dbg_push_tile(tile, tpf->the_dir);
#endif
				if (TPFSetTileBit(tpf, tile, tpf->the_dir) &&
						!tpf->enum_proc(tile, tpf->userdata, tpf->the_dir, tpf->rd.cur_length, &tpf->rd.pft_var6) ) {
					TPFMode1(tpf, tile, _tpf_new_direction[tpf->the_dir]);
				}
#ifdef DEBUG_TILE_PUSH
		dbg_pop_tile();
#endif
				tpf->rd = rd;
			} while (bits != 0);
		}
	}

	/* the next is only used when signals are checked.
	 * seems to go in 2 directions simultaneously */
	 
	/* if i can get rid of this, tail end recursion can be used to minimize
	 * stack space dramatically. */	
	if (tpf->hasbit_13)
		return;
	
	tile = tile_org;
	direction ^= 2;

	bits = GetTileTrackStatus(tile, tpf->tracktype);
	bits |= (bits >> 8);

	if ( (byte)bits != tpf->var2) {
		bits &= _bits_mask[direction];
	}

	bits &= 0xBF;
	if (bits == 0)
		return;

	do {
		i = FIND_FIRST_BIT(bits);
		bits = KILL_FIRST_BIT(bits);
		
		tpf->the_dir = (_otherdir_mask[direction] & (byte)(1 << i)) ? (i+8) : i;
		rd = tpf->rd;
		if (TPFSetTileBit(tpf, tile, tpf->the_dir) &&
				!tpf->enum_proc(tile, tpf->userdata, tpf->the_dir, tpf->rd.cur_length, &tpf->rd.pft_var6) ) {
			TPFMode1(tpf, tile, _tpf_new_direction[tpf->the_dir]);
		}
		tpf->rd = rd;
	} while (bits != 0);
}

void FollowTrack(uint tile, uint16 flags, byte direction, TPFEnumProc *enum_proc, TPFAfterProc *after_proc, void *data)
{
	TrackPathFinder *tpf = alloca(sizeof(TrackPathFinder));

	assert(direction < 4);

	/* initialize path finder variables */	
	tpf->userdata = data;
	tpf->enum_proc = enum_proc;	
	tpf->new_link = tpf->start_link = tpf->links;
	tpf->num_links_left = 0x400;

	tpf->rd.cur_length = 0;
	tpf->rd.num_tries = 0;
	tpf->rd.pft_var6 = 0;
	
	tpf->var2 = HASBIT(flags, 15) ? 0x43 : 0xFF; /* 0x8000 */

	tpf->hasbit_12 = HASBIT(flags, 12) != 0;     /* 0x1000 */
	tpf->hasbit_13 = HASBIT(flags, 13) != 0;		 /* 0x2000 */


	tpf->tracktype = (byte)flags;

	if (HASBIT(flags, 11)) {	
		tpf->rd.pft_var6 = 0xFF;
		tpf->enum_proc(tile, data, 0, 0, 0);
		TPFMode2(tpf, tile, direction);	
	} else {
		/* clear the hash_heads */
		memset(tpf->hash_head, 0, sizeof(tpf->hash_head));
		TPFMode1(tpf, tile, direction);
	}

	if (after_proc != NULL)
		after_proc(tpf);	
}

