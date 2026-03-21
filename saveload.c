#include "stdafx.h"
#include "ttd.h"
#include "vehicle.h"
#include "station.h"
#include "city.h"
#include "player.h"
#include "saveload.h"

enum {
	SAVEGAME_IDENTIFIER = 'OTTD',
	SAVEGAME_MAJOR_VERSION = 1,
	SAVEGAME_MINOR_VERSION = 0,
};

typedef struct LoadSavegameState {
	int8 mode;
	byte rep_char;

	size_t count;
	size_t buffer_count;
	FILE *fin;

	byte *buffer_cur;

	byte buffer[4880];

} LoadSavegameState;

static LoadSavegameState *_cur_state;

static byte GetSavegameByteFromBuffer()
{
	LoadSavegameState *lss = _cur_state;

	if (lss->buffer_count == 0) {
		fread(lss->buffer, 1, 4880, lss->fin);

		lss->buffer_count = 4880;
		lss->buffer_cur = lss->buffer;
	}

	lss->buffer_count--;
	return *lss->buffer_cur++;
}

static byte DecodeSavegameByte()
{
	LoadSavegameState *lss = _cur_state;
	int8 x;

	if (lss->mode < 0) {
		if (lss->count != 0) {
			lss->count--;
			return lss->rep_char;
		}
	} else if (lss->mode > 0) {
		if (lss->count != 0) {
			lss->count--;
			return GetSavegameByteFromBuffer();
		}
	}
	
	x = GetSavegameByteFromBuffer();
	if (x >= 0) {
		lss->count = x;
		lss->mode = 1;
		return GetSavegameByteFromBuffer();
	} else {
		lss->mode = -1;
		lss->count = -x;
		lss->rep_char = GetSavegameByteFromBuffer();
		return lss->rep_char;
	}	
}

static void LoadSavegameBytes(void *p, size_t count)
{
	byte *ptr = (byte*)p;
	assert(count > 0);
	do {
		*ptr++ = DecodeSavegameByte();
	} while (--count);
}


static void SkipSavegameBytes(size_t count) {
	assert(count > 0);
	do {
		DecodeSavegameByte();
	} while (--count);
}


/* dummy function that just loads the map bytes */
bool LoadSavegame(const char *filename)
{
	LoadSavegameState lss;
	int i;

	_cur_state = &lss;
	
	lss.mode = 0;
	lss.buffer_count = 0;

	lss.fin = fopen(filename, "rb");

	if (lss.fin == NULL)
		return false;
	
	fseek(lss.fin, 49, SEEK_SET);

	/* skip to the MapOwner */
	SkipSavegameBytes(0x0045DBAA - 0x00458EF0);

	LoadSavegameBytes(_map_owner, 256*256);
	LoadSavegameBytes(_map2, 256*256);

	for(i=0; i!=256*256; i++) {
		_map3_lo[i] = DecodeSavegameByte();
		_map3_hi[i] = DecodeSavegameByte();
	}

	LoadSavegameBytes(_map_extra_bits, 256*256/4);

	/* skip to the end of the persistent block */
	SkipSavegameBytes(0x004D0069 - 0x004A1BAA);

	LoadSavegameBytes(_map_type_and_height, 256*256);
	LoadSavegameBytes(_map5, 256*256);


	fclose(lss.fin);

	return true;
}


/******************************************************/
/******************************************************/
/******************************************************/

typedef void ByteWriterProc(byte v);
typedef byte *ByteReaderProc(uint *len);

typedef uint ReferenceToIntProc(void *v, uint t);
typedef void *IntToReferenceProc(uint r, uint t);

typedef struct {
	bool save;
	byte need_length;
	byte block_mode;
	bool error;
	byte version;

	int obj_len;
	int array_index, last_array_index;

	uint32 offs_base;

	ByteWriterProc *write_byte;
	ByteReaderProc *read_bytes;

	ReferenceToIntProc *ref_to_int_proc;
	IntToReferenceProc *int_to_ref_proc;

	const ChunkHandler * const * chs;
	const byte * const *includes;

	byte *bufp, *bufe;
	int tmp;
} SaverLoader;

enum NeedLengthValues { NL_NONE = 0,NL_WANTLENGTH = 1,NL_CALCLENGTH = 2};

SaverLoader _sl;

static void SlReadFill()
{
	uint len;
	_sl.bufp = _sl.read_bytes(&len);
	assert(len != 0);
	_sl.bufe = _sl.bufp + len;
	_sl.offs_base += len;
}

int SlReadByte()
{
	if (_sl.bufp == _sl.bufe) SlReadFill();
	return *_sl.bufp++;
}

static uint32 SlGetOffs()
{
	return _sl.offs_base - (_sl.bufe - _sl.bufp);
}

void SlWriteByte(byte v)
{
	_sl.write_byte(v);
}

int SlReadUint16()
{
	int x = SlReadByte() << 8;
	return x | SlReadByte();
}

uint32 SlReadUint32()
{
	uint32 x = SlReadUint16() << 16;
	return x | SlReadUint16();
}

uint64 SlReadUint64()
{
	uint32 x = SlReadUint32();
	uint32 y = SlReadUint32();
	return (uint64)x << 32 | y;
}

void SlWriteUint16(uint16 v)
{
	SlWriteByte((byte)(v >> 8));
	SlWriteByte((byte)v);
}

void SlWriteUint32(uint32 v)
{
	SlWriteUint16((uint16)(v >> 16));
	SlWriteUint16((uint16)v);
}

void SlWriteUint64(uint64 x)
{
	SlWriteUint32((uint32)(x >> 32));
	SlWriteUint32((uint32)x);
}

static int SlReadSimpleGamma()
{
	int x = SlReadByte();
	if (x & 0x80)
		x = ((x&0x7F) << 8) + SlReadByte();
	return x;
}

static void SlWriteSimpleGamma(uint i)
{
	assert(i < (1 << 14));
	if (i >= 0x80) {
		SlWriteByte((byte)(0x80|(i >> 8)));
		SlWriteByte((byte)i);
	} else {
		SlWriteByte(i);
	}
}

static uint SlGetGammaLength(uint i) {
	return (i>=0x80) ? 2 : 1;
}

int INLINE SlReadSparseIndex()
{
	return SlReadSimpleGamma();
}

void INLINE SlWriteSparseIndex(uint index)
{
	SlWriteSimpleGamma(index);
}

int INLINE SlReadArrayLength()
{
	return SlReadSimpleGamma();
}

void INLINE SlWriteArrayLength(uint length)
{
	SlWriteSimpleGamma(length);
}

void SlSetArrayIndex(uint index)
{
	_sl.need_length = NL_WANTLENGTH;
	_sl.array_index = index;
}

int SlIterateArray()
{
	int ind;
	static uint32 next_offs;
	
	// Must be at end of current block.
	assert(next_offs == 0 || SlGetOffs() == next_offs);

	while(true) {
		uint len = SlReadArrayLength();
		if (len == 0) {
			next_offs = 0;
			return -1;
		}

		_sl.obj_len = --len;
		next_offs = SlGetOffs() + len;

		switch(_sl.block_mode) {
		case CH_SPARSE_ARRAY:	ind = SlReadSparseIndex(); break;
		case CH_ARRAY:        ind = _sl.array_index++; break;
		default:
			DEBUG(misc, 0) ("SlIterateArray: error\n");
			return -1; // error
		}
		
		if (len != 0)
			return ind;
	}

}

// Sets the length of either a RIFF object or the number of items in an array.
void SlSetLength(uint length)
{
	switch(_sl.need_length) {
	case NL_WANTLENGTH:
		_sl.need_length = NL_NONE;
		switch(_sl.block_mode) {
		case CH_RIFF:
			// Really simple to write a RIFF length :)
			SlWriteUint32(length);
		break;
		case CH_ARRAY:
			assert(_sl.array_index >= _sl.last_array_index);
			while (++_sl.last_array_index <= _sl.array_index)
				SlWriteArrayLength(1);
			SlWriteArrayLength(length + 1);
			break;
		case CH_SPARSE_ARRAY:
			SlWriteArrayLength(length + 1 + SlGetGammaLength(_sl.array_index)); // Also include length of sparse index.
			SlWriteSparseIndex(_sl.array_index);
			break;
		default: NOT_REACHED();
		}
	case NL_CALCLENGTH:
		_sl.obj_len += length;
		break;
	}
}

void SlCopyBytes(void *ptr, size_t length)
{
	byte *p = (byte*)ptr;

	if (_sl.save) {
		while(length) {
			SlWriteByte(*p++);
			length--;
		}
	} else {
		while(length) {
			// INLINED SlReadByte
			if (_sl.bufp == _sl.bufe) SlReadFill();
			*p++ = *_sl.bufp++;
			length--;
		}
	}
}

void SlSkipBytes(size_t length)
{
	while (length) {
		SlReadByte();
		length--;
	}
}

uint SlGetFieldLength()
{
	return _sl.obj_len;
}


static void SlSaveLoadConv(void *ptr, uint conv)
{
	int64 x;
	
	if (_sl.save) {
		// Read a value from the struct. These ARE endian safe.
		switch((conv >> 4)&7) {
		case SLE_VAR_I8>>4: x = *(int8*)ptr; break;
		case SLE_VAR_U8>>4: x = *(byte*)ptr; break;
		case SLE_VAR_I16>>4: x = *(int16*)ptr; break;
		case SLE_VAR_U16>>4: x = *(uint16*)ptr; break;
		case SLE_VAR_I32>>4: x = *(int32*)ptr; break;
		case SLE_VAR_U32>>4: x = *(uint32*)ptr; break;
		case SLE_VAR_I64>>4: x = *(int64*)ptr; break; 
		case SLE_VAR_U64>>4: x = *(uint64*)ptr; break;
//			assert(0);
		default:
			NOT_REACHED();
		}

		// Write it to the file
		switch(conv & 0xF) {
		case SLE_FILE_I8: assert(x >= -128 && x <= 127); SlWriteByte(x);break;
		case SLE_FILE_U8:	assert(x >= 0 && x <= 255); SlWriteByte(x);break;
		case SLE_FILE_I16:assert(x >= -32768 && x <= 32767); SlWriteUint16(x);break;
		case SLE_FILE_STRINGID:
		case SLE_FILE_U16:assert(x >= 0 && x <= 65535); SlWriteUint16(x);break;
		case SLE_FILE_I32:
		case SLE_FILE_U32:SlWriteUint32((uint32)x);break;
		case SLE_FILE_I64:
		case SLE_FILE_U64:SlWriteUint64(x);break;
		default:
			assert(0);
			NOT_REACHED();
		}
	} else {
		
		// Read a value from the file
		switch(conv & 0xF) {
		case SLE_FILE_I8: x = (int8)SlReadByte();	break;
		case SLE_FILE_U8: x = (byte)SlReadByte(); break;
		case SLE_FILE_I16: x = (int16)SlReadUint16();	break;
		case SLE_FILE_U16: x = (uint16)SlReadUint16(); break;
		case SLE_FILE_I32: x = (int32)SlReadUint32();	break;
		case SLE_FILE_U32: x = (uint32)SlReadUint32(); break;
		case SLE_FILE_I64: x = (int64)SlReadUint64();	break;
		case SLE_FILE_U64: x = (uint64)SlReadUint64(); break;
		case SLE_FILE_STRINGID: x = RemapOldStringID((uint16)SlReadUint16()); break;
		default:
			assert(0);
			NOT_REACHED();
		}

		// Write it to the struct, these ARE endian safe.
		switch((conv >> 4)&7) {
		case SLE_VAR_I8>>4: *(int8*)ptr = x; break;
		case SLE_VAR_U8>>4: *(byte*)ptr = x; break;
		case SLE_VAR_I16>>4: *(int16*)ptr = x; break;
		case SLE_VAR_U16>>4: *(uint16*)ptr = x; break;
		case SLE_VAR_I32>>4: *(int32*)ptr = x; break;
		case SLE_VAR_U32>>4: *(uint32*)ptr = x; break;
		case SLE_VAR_I64>>4: *(int64*)ptr = x; break;
		case SLE_VAR_U64>>4: *(uint64*)ptr = x; break;
//			printf("64-bit save not implemented\n");
//			assert(0);
		default:
			NOT_REACHED();
		}
	}
}

static const byte _conv_lengths[] = {1,1,2,2,4,4,8,8,2};

static uint SlCalcConvLen(uint conv, void *p)
{
	return _conv_lengths[conv & 0xF];
}

static uint SlCalcArrayLen(void *array, uint length, uint conv)
{
	return _conv_lengths[conv & 0xF] * length;
}

static const byte _conv_mem_size[8] = {1,1,2,2,4,4,8,8};
void SlArray(void *array, uint length, uint conv)
{
	// Automatically calculate the length?
	if (_sl.need_length != NL_NONE) {
		SlSetLength(SlCalcArrayLen(array, length, conv));
		// Determine length only?
		if (_sl.need_length == NL_CALCLENGTH)
			return;
	}

	// handle buggy stuff
	if (!_sl.save && _sl.version == 0) {
		if (conv == SLE_INT16 || conv == SLE_UINT16 || conv == SLE_STRINGID) {
			length *= 2;
			conv = SLE_INT8;
		} else if (conv == SLE_INT32 || conv == SLE_UINT32) {
			length *= 4;
			conv = SLE_INT8;
		}
	}

	// Optimized cases when input equals output.
	switch(conv) {
	case SLE_INT8:
	case SLE_UINT8:SlCopyBytes(array, length);break;
	default: {
		// Default "slow" case.
		byte *a = (byte*)array;
		while (length) {
			SlSaveLoadConv(a, conv);
			a += _conv_mem_size[(conv >> 4)&7];
			length--;
		}
		}
	}
}

// Calculate the size of an object.
static size_t SlCalcObjLength(void *object, const void *desc)
{
	size_t length = 0;
	uint cmd,conv;
	byte *d = (byte*)desc;

	// Need to determine the length and write a length tag.
	while (true) {
		cmd = (d[0] >> 4);
		if (cmd < 8) {
			conv = d[2];
			d += 3;
			if (cmd&4) {
				d += 2;
				// check if the field is of the right version
				if (_sl.version < d[-2] || _sl.version > d[-1])
					continue;
			}

			switch(cmd&3) {
			// Normal variable
			case 0: length += SlCalcConvLen(conv, NULL);break;			
			// Reference
			case 1: length += 2; break;
			// Array
			case 2:	length += SlCalcArrayLen(NULL, *d++, conv); break;
			default:NOT_REACHED();
			}
		} else if (cmd == 8) {
			length++;
			d += 4;
		} else if (cmd == 9) {
			length += SlCalcObjLength(NULL, _sl.includes[d[2]]);
			d += 3;
		} else if (cmd == 15)
			break;
		else
			assert(0);
	}
	return length;
}

void SlObject(void *object, const void *desc)
{
	byte *d = (byte*)desc;
	void *ptr;
	uint cmd,conv;

	// Automatically calculate the length?
	if (_sl.need_length != NL_NONE) {
		SlSetLength(SlCalcObjLength(object, d));
		if (_sl.need_length == NL_CALCLENGTH)
			return;
	}

	while (true) {
		// Currently it only supports up to 4096 byte big objects
		ptr = (byte*)object + (d[0] & 0xF) + (d[1] << 4);

		cmd = d[0] >> 4;

		if (cmd < 8) {
			conv = d[2];
			d += 3;

			if (cmd&4) {
				d += 2;
				// check if the field is of the right version
				if (_sl.version < d[-2] || _sl.version > d[-1])
					continue;
			}

			switch(cmd&3) {
			// Normal variable
			case 0:	SlSaveLoadConv(ptr, conv); break;
			// Reference
			case 1:
				if (_sl.save) {
					SlWriteUint16(_sl.ref_to_int_proc(*(void**)ptr, conv));
				} else {
					*(void**)ptr = _sl.int_to_ref_proc(SlReadUint16(), conv);
				}
				break;
			// Array
			case 2:	SlArray(ptr, *d++, conv); break;
			default:NOT_REACHED();
			}
	
		// Write byte.
		} else if (cmd == 8) {
			if (_sl.save) {
				SlWriteByte(d[3]);
			} else {
				*(byte*)ptr = d[2];
			}
			d += 4;

		// Include
		} else if (cmd == 9) {
			SlObject(ptr, _sl.includes[d[2]]);
			d += 3;
		}	else if (cmd == 15)
			break;
		else
			assert(0);
	}
}


static size_t SlCalcGlobListLength(const SaveLoadGlobVarList *desc)
{
	size_t length = 0;

	while (desc->address) {
		length += SlCalcConvLen(desc->conv, NULL);
		desc++;
	}
	return length;
}

// Save/Load a list of global variables
void SlGlobList(const SaveLoadGlobVarList *desc)
{
	if (_sl.need_length != NL_NONE) {
		SlSetLength(SlCalcGlobListLength(desc));
		if (_sl.need_length == NL_CALCLENGTH)
			return;
	}
	while (true) {
		void *ptr = desc->address;
		if (ptr == NULL)
			break;
		SlSaveLoadConv(ptr, desc->conv);
		desc++;
	}
}


void SlAutolength(AutolengthProc *proc, void *arg)
{
	if (_sl.save) {
		// Tell it to calculate the length
		_sl.need_length = NL_CALCLENGTH;
		_sl.obj_len = 0;
		proc(arg);

		// Setup length
		_sl.need_length = NL_WANTLENGTH;
		SlSetLength(_sl.obj_len);
	}
	// And write the stuff
	proc(arg);
}

static void SlLoadChunk(const ChunkHandler *ch)
{
	byte m = SlReadByte();
	size_t len;
	uint32 endoffs;

	_sl.block_mode = m;
	_sl.obj_len = 0;
	
	switch(m) {
	case CH_ARRAY:
		_sl.array_index = 0;
		ch->load_proc();
		break;

	case CH_SPARSE_ARRAY:
		ch->load_proc();
		break;

	case CH_RIFF:
		// Read length
		len = SlReadByte() << 16;
		len += SlReadUint16();
		_sl.obj_len = len;
		endoffs = SlGetOffs() + len;
		ch->load_proc();
		assert(SlGetOffs() == endoffs);
		break;
	default:
		assert(0);
	}
}

#if 0
static void SlSkipChunk(const ChunkHandler *ch)
{
	byte m = SlReadByte();
	size_t len;
	uint ind;

	switch(m) {
	case CH_SPARSE_ARRAY:
	case CH_ARRAY:
		for(ind=0;;ind++) {
			len = SlReadArrayLength();
			if (len == 0)
				break;
			SlSkipBytes(len - 1);
		}
		break;

	case CH_RIFF:
		// Read length
		len = SlReadByte() << 16;
		len += SlReadUint16();
		SlSkipBytes(len);
		break;
	default:
		assert(0);
	}
}
#endif

static ChunkSaveLoadProc *_tmp_proc_1;

static void SlStubSaveProc2(void *arg)
{
	_tmp_proc_1();
}

static void SlStubSaveProc()
{
	SlAutolength(SlStubSaveProc2, NULL);
}

static void SlSaveChunk(const ChunkHandler *ch)
{
	ChunkSaveLoadProc *proc;

	SlWriteUint32(ch->id);

	proc = ch->save_proc;
	if (ch->flags & CH_AUTO_LENGTH) {
		// Need to calculate the length. Solve that by calling SlAutoLength in the save_proc.
		_tmp_proc_1 = proc;
		proc = SlStubSaveProc;
	}
	
	_sl.block_mode = ch->flags & CH_TYPE_MASK;
	switch(ch->flags & CH_TYPE_MASK) {
	case CH_RIFF:
		_sl.need_length = NL_WANTLENGTH;
		proc();
		break;
	case CH_ARRAY:
		_sl.last_array_index = 0;
		SlWriteByte(CH_ARRAY);
		proc();
		SlWriteArrayLength(0); // Terminate arrays
		break;
	case CH_SPARSE_ARRAY:
		SlWriteByte(CH_SPARSE_ARRAY);
		proc();
		SlWriteArrayLength(0); // Terminate arrays
		break;
	default:
		NOT_REACHED();
	}
}

void SlSaveChunks()
{
	const ChunkHandler *ch;
	const ChunkHandler * const * chsc;
	uint p;

	for(p=0; p!=CH_NUM_PRI_LEVELS; p++) {
		for(chsc=_sl.chs;(ch=*chsc++) != NULL;) {
			while(true) {
				if (((ch->flags >> CH_PRI_SHL) & (CH_NUM_PRI_LEVELS - 1)) == p)
					SlSaveChunk(ch);	
				if (ch->flags & CH_LAST)
					break;
				ch++;
			}
		}
	}

	// Terminator
	SlWriteUint32(0);
}

static const ChunkHandler *SlFindChunkHandler(uint32 id)
{
	const ChunkHandler *ch;
	const ChunkHandler * const * chsc;
	for(chsc=_sl.chs;(ch=*chsc++) != NULL;) {
		while(true) {
			if (ch->id == id)
				return ch;
			if (ch->flags & CH_LAST)
				break;
			ch++;
		}
	}
	return NULL;
}

void SlLoadChunks()
{
	uint32 id;
	const ChunkHandler *ch;

	while(true) {
		id = SlReadUint32();
		if (id == 0)
			return;
		ch = SlFindChunkHandler(id);
		if (ch == NULL) {
			error("Found unknown tag %c%c%c%c.\n", id>>24,id>>16,id>>8,id);
			return;
		}
		SlLoadChunk(ch);
	}
}

/******************************************************/
/******************************************************/
/******************************************************/

typedef struct {
	void *base;
	size_t size;
} ReferenceSetup;

static const ReferenceSetup _ref_setup[] = {
	{_order_array,sizeof(_order_array[0])},
	{_vehicles,sizeof(_vehicles[0])},
	{_stations,sizeof(_stations[0])},
	{_cities,sizeof(_cities[0])},
};

uint ReferenceToInt(void *v, uint t)
{
	if (v == NULL) return 0;
	return ((byte*)v - (byte*)_ref_setup[t].base) / _ref_setup[t].size + 1;
}

void *IntToReference(uint r, uint t)
{
	if (r == 0) return NULL;
	return (byte*)_ref_setup[t].base + (r-1) * _ref_setup[t].size;
}

static void SaveLoad_MAPT() {
  SlArray(_map_type_and_height, lengthof(_map_type_and_height), SLE_UINT8);
}

static void SaveLoad_MAP2() {
  SlArray(_map2, lengthof(_map2), SLE_UINT8);
}

static void SaveLoad_M3LO() {
  SlArray(_map3_lo, lengthof(_map3_lo), SLE_UINT8);
}

static void SaveLoad_M3HI() {
  SlArray(_map3_hi, lengthof(_map3_hi), SLE_UINT8);
}

static void SaveLoad_MAPO() {
  SlArray(_map_owner, lengthof(_map_owner), SLE_UINT8);
}

static void SaveLoad_MAP5() {
  SlArray(_map5, lengthof(_map5), SLE_UINT8);
}

static void SaveLoad_MAPE() {
  SlArray(_map_extra_bits, lengthof(_map_extra_bits), SLE_UINT8);
}

const ChunkHandler _map_chunk_handlers[] = {
	{ 'MAPT', SaveLoad_MAPT, SaveLoad_MAPT, CH_RIFF },
	{ 'MAP2', SaveLoad_MAP2, SaveLoad_MAP2, CH_RIFF },
	{ 'M3LO', SaveLoad_M3LO, SaveLoad_M3LO, CH_RIFF },
	{ 'M3HI', SaveLoad_M3HI, SaveLoad_M3HI, CH_RIFF },
	{ 'MAPO', SaveLoad_MAPO, SaveLoad_MAPO, CH_RIFF },
	{ 'MAP5', SaveLoad_MAP5, SaveLoad_MAP5, CH_RIFF },
	{ 'MAPE', SaveLoad_MAPE, SaveLoad_MAPE, CH_RIFF | CH_LAST},
};

__int64 rdtsc();

extern const byte _common_veh_desc[];
static const byte * const _desc_includes[] = {
	_common_veh_desc
};

#define LZO_SIZE 8192
static byte *_lzo_buffr;
static FILE *_lzo_fh;
static uint _lzo_pos;

int CDECL lzo1x_1_compress( const byte *src, uint src_len,byte *dst, uint *dst_len,void *wrkmem );
uint32 CDECL lzo_adler32(uint32 adler, const byte *buf, uint len);
int CDECL lzo1x_decompress( const byte *src, uint  src_len,byte *dst, uint *dst_len,void *wrkmem /* NOT USED */ );

static void FlushLZO()
{
	byte out[LZO_SIZE + LZO_SIZE / 64 + 16 + 3 + 8];
	byte wrkmem[sizeof(byte*)*4096];
	uint size = _lzo_pos;
	uint outlen;
	
	if (size != 0) {
		lzo1x_1_compress(_lzo_buffr, size, out + sizeof(uint32)*2, &outlen, wrkmem);
		((uint32*)out)[1] = TO_BE32(outlen);
		((uint32*)out)[0] = TO_BE32(lzo_adler32(0, out + sizeof(uint32), outlen + sizeof(uint32)));
		fwrite(out, outlen + sizeof(uint32)*2, 1, _lzo_fh);

#ifndef NDEBUG
		{
			lzo1x_decompress(out + sizeof(uint32)*2, outlen, wrkmem, &outlen, NULL);
			if (memcmp(wrkmem, _lzo_buffr, size))
				printf("compress failure\n");
		}
#endif
		_lzo_pos = 0;
	}
}

static void WriteByteLZO(byte b)
{
	_lzo_buffr[_lzo_pos] = b;
	if (++_lzo_pos == LZO_SIZE) {
		FlushLZO();		
	}
}

static byte *FillLZO(uint *len)
{
	byte out[LZO_SIZE + LZO_SIZE / 64 + 16 + 3 + 8];
	uint32 tmp[2];
	uint32 size;

	// Read header
	fread(tmp, sizeof(tmp), 1, _lzo_fh);
	// Check if size is bad
	((uint32*)out)[0] = size = tmp[1];
	
	if (_sl.version != 0) {
		tmp[0] = TO_BE32(tmp[0]);
		size = TO_BE32(size);
	}

	if (size >= sizeof(out)) goto error;
	// Read block
	fread(out + sizeof(uint32), size, 1, _lzo_fh);
	// Check checksum
	if (tmp[0] != lzo_adler32(0, out, size + sizeof(uint32)))
		goto error;
	
	// decompress
	lzo1x_decompress(out + sizeof(uint32)*1, size, _lzo_buffr, len, NULL);
	return _lzo_buffr;

error:
	printf("Read savegame fault!\n");
	memset(_lzo_buffr, 0, LZO_SIZE);
	*len = LZO_SIZE;
	return _lzo_buffr;
}


extern const ChunkHandler _name_chunk_handlers[];
extern const ChunkHandler _player_chunk_handlers[];
extern const ChunkHandler _veh_chunk_handlers[];
extern const ChunkHandler _city_chunk_handlers[];
extern const ChunkHandler _sign_chunk_handlers[];
extern const ChunkHandler _station_chunk_handlers[];
extern const ChunkHandler _industry_chunk_handlers[];
extern const ChunkHandler _engine_chunk_handlers[];
extern const ChunkHandler _economy_chunk_handlers[];
extern const ChunkHandler _animated_tile_chunk_handlers[];

static const ChunkHandler * const _chunk_handlers[] = {
	_name_chunk_handlers,
	_veh_chunk_handlers,
	_industry_chunk_handlers,
	_economy_chunk_handlers,
	_engine_chunk_handlers,
	_city_chunk_handlers,
	_sign_chunk_handlers,
	_station_chunk_handlers,
	_player_chunk_handlers,
	_map_chunk_handlers,
	_animated_tile_chunk_handlers,
	NULL,
};


extern void InitializeGame();
extern void AfterLoadGame();
extern void BeforeSaveGame();
extern bool LoadOldSaveGame(const char *file);

bool SaveOrLoad(const char *filename, int mode)
{
	byte buffer[LZO_SIZE];
	uint32 hdr[2];

	// old style load
	if (mode == 2) {
		bool b;

		InitializeGame();
		b = LoadOldSaveGame(filename);
		AfterLoadGame();
		return b;
	}
	
	_lzo_buffr = buffer;
	_lzo_pos = 0;
	_lzo_fh = fopen(filename, mode?"wb":"rb");
	if (_lzo_fh == NULL)
		return false;
	
	_sl.bufe = _sl.bufp = NULL;
	_sl.offs_base = 0;
	_sl.read_bytes = FillLZO;
	_sl.write_byte = WriteByteLZO;
	_sl.int_to_ref_proc = IntToReference;
	_sl.ref_to_int_proc = ReferenceToInt;
	_sl.save = mode;
	_sl.includes = _desc_includes;
	_sl.chs = _chunk_handlers;

//	time = rdtsc();

	if (mode != 0) {
		hdr[0] = TO_BE32(SAVEGAME_IDENTIFIER);
		hdr[1] = TO_BE32((SAVEGAME_MAJOR_VERSION<<16) + (SAVEGAME_MINOR_VERSION << 8));
		fwrite(hdr, sizeof(hdr), 1, _lzo_fh);
		
		_sl.version = SAVEGAME_MAJOR_VERSION;

		BeforeSaveGame();
		SlSaveChunks();
		FlushLZO();
	} else {
		if (fread(hdr, sizeof(hdr), 1, _lzo_fh) != 1) {
read_err:
			printf("Savegame is obsolete or invalid format.\n");
			fclose(_lzo_fh);
			return false;
		}

		if (hdr[0] == TO_BE32(SAVEGAME_IDENTIFIER)) {
			int ver = TO_BE32(hdr[1]) >> 16;
			// new file format
			if (ver != SAVEGAME_MAJOR_VERSION) goto read_err;
			_sl.version = ver;
		} else {
			printf("unknown savegame type, trying to load it as the buggy format.\n");
			rewind(_lzo_fh);
			_sl.version = 0;
		}
	
		// Clear everything
		InitializeGame();
		SlLoadChunks();
	}

//	time = rdtsc() - time;
//	printf("Time to load/save game: %I64d\n", time);

	fclose(_lzo_fh);

	if (mode == 0) {
		AfterLoadGame();
	}

	return true;
}

void DoTestSave() {
	SaveOrLoad("test.sav", true);
}

void DoTestLoad() {
	SaveOrLoad("test.sav", false);
}

bool EmergencySave()
{
	SaveOrLoad("crash.sav", true);
	return true;
}
