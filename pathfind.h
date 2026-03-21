
typedef struct TrackPathFinder TrackPathFinder;
typedef bool TPFEnumProc(uint tile, void *data, int track, uint length, byte *state);
typedef void TPFAfterProc(TrackPathFinder *tpf);


#define PATHFIND_GET_LINK_OFFS(tpf, link) ((byte*)(link) - (byte*)tpf->start_link)
#define PATHFIND_GET_LINK_PTR(tpf, link_offs) (TrackPathFinderLink*)((byte*)tpf->start_link + (link_offs))

/* y7 y6 y5 y4 y3 y2 y1 y0 x7 x6 x5 x4 x3 x2 x1 x0
 * y7 y6 y5 y4 y3 y2 y1 y0 x4 x3 x2 x1 x0  0  0  0
 *  0  0 y7 y6 y5 y4 y3 y2 y1 y0 x4 x3 x2 x1 x0  0
 *  0  0  0  0 y5 y4 y3 y2 y1 y0 x4 x3 x2 x1 x0  0
 */
#define PATHFIND_HASH_TILE(tile) (GET_TILE_X(tile) & 0x1F) + ((GET_TILE_Y(tile)&0x1F)<<5)

typedef struct TrackPathFinderLink {
	TileIndex tile;
	uint16 flags;
	uint16 next;
} TrackPathFinderLink;

typedef struct RememberData {
	uint16 cur_length;
	byte num_tries;
	byte pft_var6;
} RememberData;

struct TrackPathFinder {

	int num_links_left;
	TrackPathFinderLink *new_link, *start_link;

	TPFEnumProc *enum_proc;

	void *userdata;
	
	RememberData rd;

	int the_dir;

	byte tracktype;
	byte var2;
	bool hasbit_12;
	bool hasbit_13;

	uint16 hash_head[0x400];
	TileIndex hash_tile[0x400]; /* stores the link index when multi link. */

	TrackPathFinderLink links[0x400]; /* hopefully, this is enough. */
	
};

void FollowTrack(uint tile, uint16 flags, byte direction, TPFEnumProc *enum_proc, TPFAfterProc *after_proc, void *data);

typedef struct {
	uint tile;
	int length;
} FindLengthOfTunnelResult;
FindLengthOfTunnelResult FindLengthOfTunnel(uint tile, int direction, byte type);
