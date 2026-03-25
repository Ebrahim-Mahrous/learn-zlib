#ifndef _ZLIB_H
#define _ZLIB_H
#include "lz_bitstream.h"

typedef struct ZlibHeader {
	uint8_t cm : 4;
	uint8_t cinfo : 4;
	uint8_t fcheck : 5;
	uint8_t fdict : 1;
	uint8_t flevel : 2;
} ZlibHeader;

typedef struct DeflateBlock {
	uint8_t BFINAL : 1;
	uint8_t BTYPE : 2;
} DeflateBlock;

typedef struct {
	ZlibHeader header;
	BitReader stream;
	uint32_t adler32;
} ZlibReader;

typedef struct ZlibWriter {
	ZlibHeader header;
	BitWriter stream;
	uint64_t size;
	const uint8_t* uncompressed;
} ZlibWriter;


int lzInflateInit(ZlibReader* z, const uint8_t* data, uint64_t size);
int lzInflate(ZlibReader* z, uint8_t* output, uint64_t size);

int lzDeflateInit(ZlibWriter* z, const uint8_t* data, uint64_t size);
int lzDeflate(ZlibWriter* z, uint8_t* output, uint64_t size);

#endif // _ZLIB_H
