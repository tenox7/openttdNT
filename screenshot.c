#include "stdafx.h"
#include "ttd.h"
#include "gfx.h"
#include "viewport.h"
#include "player.h"
#include "gui.h"

// called by the ScreenShot proc to generate screenshot lines.
typedef void ScreenshotCallback(void *userdata, byte *buf, uint y, uint pitch, uint n);

// format specification structs for windows bitmaps. these can't be taken from windows.h since that is platform dependent.

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

typedef struct BitmapFileHeader { 
	uint16 type;
	uint32 size;
	uint32 reserved;
	uint32 off_bits;
} GCC_PACK BitmapFileHeader;
assert_compile(sizeof(BitmapFileHeader) == 14);

#if defined(_MSC_VER)
#pragma pack(pop)
#endif


typedef struct BitmapInfoHeader {
	uint32 size;
	int32 width, height;
	uint16 planes, bitcount;
	uint32 compression, sizeimage, xpels, ypels, clrused, clrimp;
} BitmapInfoHeader;
assert_compile(sizeof(BitmapInfoHeader) == 40);

typedef struct RgbQuad {
	byte blue, green, red, reserved;
} RgbQuad;
assert_compile(sizeof(RgbQuad) == 4);


// generic .BMP writer
static bool MakeBmpImage(const char *name, ScreenshotCallback *callb, void *userdata, uint w, uint h, int pixelformat, const byte *palette)
{
	BitmapFileHeader bfh;
	BitmapInfoHeader bih;
	RgbQuad *rq = alloca(sizeof(RgbQuad) * 256);
	byte *buff;
	FILE *f;
	uint i, padw;
	uint n, maxlines;

	// only implemented for 8bit images so far.
	if (pixelformat != 8)
		return false;

	f = fopen(name, "wb");
	if (f == NULL) return false;

	// each scanline must be aligned on a 32bit boundary
	padw = (w + 3) & ~3;

	// setup the file header
	bfh.type = TO_LE16('MB');
	bfh.size = TO_LE32(sizeof(bfh) + sizeof(bih) + sizeof(RgbQuad) * 256 + padw * h);
	bfh.reserved = 0;
	bfh.off_bits = TO_LE32(sizeof(bfh) + sizeof(bih) + sizeof(RgbQuad) * 256);

	// setup the info header
	bih.size = TO_LE32(sizeof(BitmapInfoHeader));
	bih.width = TO_LE32(w);
	bih.height = TO_LE32(h);
	bih.planes = TO_LE16(1);
	bih.bitcount = TO_LE16(8);
	bih.compression = 0;
	bih.sizeimage = 0;
	bih.xpels = 0;
	bih.ypels = 0;
	bih.clrused = 0;
	bih.clrimp = 0;

	// convert the palette to the windows format
	for(i=0; i!=256; i++) {
		rq[i].red = *palette++;
		rq[i].green = *palette++;
		rq[i].blue = *palette++;
		rq[i].reserved = 0;
	}

	// write file header and info header and palette
	fwrite(&bfh, 1, sizeof(bfh), f);
	fwrite(&bih, 1, sizeof(bih), f);
	fwrite(rq, 1, sizeof(RgbQuad) * 256, f);

	// use by default 64k temp memory
	maxlines = clamp(65536 / padw, 16, 128);

	// now generate the bitmap bits
	buff = (byte*)alloca(padw * maxlines); // by default generate 128 lines at a time.
	memset(buff, 0, padw * maxlines); // zero the buffer to have the padding bytes set to 0

	// start at the bottom, since bitmaps are stored bottom up.
	do {
		// determine # lines
		n = min(h, maxlines);
		h -= n;
		
		// render the pixels
		callb(userdata, buff, h, padw, n);
		
		// write each line 
		while (n) 
			fwrite(buff + (--n) * padw, 1, padw, f);
	} while (h);

	fclose(f);

	return true;
}

// screenshot generator that dumps the current video buffer
void CurrentScreenCallback(void *userdata, byte *buf, uint y, uint pitch, uint n)
{
	assert(_screen.pitch == (int)pitch);
	memcpy(buf, _screen.dst_ptr + y * _screen.pitch, n * _screen.pitch);
}

void ViewportDoDraw(ViewPort *vp, int left, int top, int right, int bottom);

// generate a large piece of the world
void LargeWorldCallback(void *userdata, byte *buf, uint y, uint pitch, uint n)
{
	ViewPort *vp = (ViewPort *)userdata;
	DrawPixelInfo dpi, *old_dpi;
	int wx, left;

	old_dpi = _cur_dpi;
	_cur_dpi = &dpi;

	dpi.dst_ptr = buf;
	dpi.height = n;
	dpi.width = vp->width;
	dpi.pitch = pitch;
	dpi.zoom = 0;
	dpi.left = 0;
	dpi.top = y;

	left = 0;
	while (vp->width - left != 0) {
		wx = min(vp->width - left, 1600);
		left += wx;
		
		ViewportDoDraw(vp, 
			((left - wx - vp->left) << vp->zoom) + vp->virtual_left,
			((y - vp->top) << vp->zoom) + vp->virtual_top,
			((left - vp->left) << vp->zoom) + vp->virtual_left,
			(((y+n) - vp->top) << vp->zoom) + vp->virtual_top
		);
	}

	_cur_dpi = old_dpi;
}

static char *MakeScreenshotName()
{
	char *filename = _screenshot_name;
	Player *p = &_players[_local_player];
	char *base;
	int serial;

	SET_DPARAM16(0, p->name_1);
	SET_DPARAM32(1, p->name_2);
	SET_DPARAM16(2, _date);
	base = GetString(filename, STR_4004);
	
	strcpy(base, ".bmp");

	serial = 0;
	while (FileExists(filename)) {
		sprintf(base, " #%d.bmp", ++serial);
	}

	return filename;
}

extern byte _cur_palette[768];

bool MakeScreenshot()
{
	return MakeBmpImage(MakeScreenshotName(), CurrentScreenCallback, NULL, _screen.width, _screen.height, 8, _cur_palette); 
}

bool MakeWorldScreenshot(int left, int top, int width, int height, int zoom)
{
	ViewPort vp;

	vp.zoom = zoom;
	vp.left = 0;
	vp.top = 0;
	vp.virtual_width = width;
	vp.width = width >> zoom;
	vp.virtual_height = height;
	vp.height = height >> zoom;
	vp.virtual_left = left;
	vp.virtual_top = top;

	return MakeBmpImage(MakeScreenshotName(), LargeWorldCallback, &vp, vp.width, vp.height, 8, _cur_palette);
}
