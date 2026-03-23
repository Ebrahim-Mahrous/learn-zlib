#ifndef _ZLIB_H
#define _ZLIB_H
#include "lz_bitstream.h"

typedef struct ZlibHeader {
	u8 cm : 4;
	u8 cinfo : 4;
	u8 fcheck : 5;
	u8 fdict : 1;
	u8 flevel : 2;
} ZlibHeader;

typedef struct DeflateBlock {
	u8 BFINAL : 1;
	u8 BTYPE : 2;
} DeflateBlock;

typedef struct {
	ZlibHeader header;
	BitReader stream;
	u32 adler32;
} ZlibReader;

typedef struct ZlibWriter {
	ZlibHeader header;
	BitWriter stream;
	u64 size;
	const byte* uncompressed;
} ZlibWriter;


int lzInflateInit(ZlibReader* z, const byte* data, u64 size);
int lzInflate(ZlibReader* z, byte* output, u64 size);

int lzDeflateInit(ZlibWriter* z, const byte* data, u64 size);
int lzDeflate(ZlibWriter* z, byte* output, u64 size);

#endif // _ZLIB_H
