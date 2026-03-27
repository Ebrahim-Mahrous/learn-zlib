#include "lz.h"
#include "lz_debug.h"

#define FIXED_BLOCK_SIZE 65535

static const uint8_t CL_ORDER[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

static const uint32_t BASE_LENGTH[] = {
	3, 4, 5, 6, 7, 8, 9, 10, //257 - 264
	11, 13, 15, 17,          //265 - 268
	19, 23, 27, 31,          //269 - 273 
	35, 43, 51, 59,          //274 - 276
	67, 83, 99, 115,         //278 - 280
	131, 163, 195, 227,      //281 - 284
	258                      //285
};

static const uint8_t BASE_LENGTH_EXTRA_BITS[] = {
	0, 0, 0, 0, 0, 0, 0, 0, //257 - 264
	1, 1, 1, 1, //265 - 268
	2, 2, 2, 2, //269 - 273 
	3, 3, 3, 3, //274 - 276
	4, 4, 4, 4, //278 - 280
	5, 5, 5, 5, //281 - 284
	0           //285
};

static const uint32_t BASE_DIST[] = {
	/*0*/ 1, 2, 3, 4,    //0-3
	/*1*/ 5, 7,          //4-5
	/*2*/ 9, 13,         //6-7
	/*3*/ 17, 25,        //8-9
	/*4*/ 33, 49,        //10-11
	/*5*/ 65, 97,        //12-13
	/*6*/ 129, 193,      //14-15
	/*7*/ 257, 385,      //16-17
	/*8*/ 513, 769,      //18-19
	/*9*/ 1025, 1537,    //20-21
	/*10*/ 2049, 3073,   //22-23
	/*11*/ 4097, 6145,   //24-25
	/*12*/ 8193, 12289,  //26-27
	/*13*/ 16385, 24577, //28-29
	0   , 0
};

static const uint8_t BASE_DIST_EXTRA_BITS[] = {
	/*0*/ 0, 0, 0, 0, //0-3
	/*1*/ 1, 1,       //4-5
	/*2*/ 2, 2,       //6-7
	/*3*/ 3, 3,       //8-9
	/*4*/ 4, 4,       //10-11
	/*5*/ 5, 5,       //12-13
	/*6*/ 6, 6,       //14-15
	/*7*/ 7, 7,       //16-17
	/*8*/ 8, 8,       //18-19
	/*9*/ 9, 9,       //20-21
	/*10*/ 10, 10,    //22-23
	/*11*/ 11, 11,    //24-25
	/*12*/ 12, 12,    //26-27
	/*13*/ 13, 13,     //28-29
	0 , 0
};

static const uint8_t FIXED_LITLEN_CL[] = {
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8
};

static const uint8_t FIXED_DIST_CL[] = {
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

typedef struct HuffEntry {
	uint32_t code;
	uint32_t length;
} HuffEntry;

typedef struct HuffTable {
	uint64_t size;
	HuffEntry *tree;
} HuffTable;

#ifdef _WIN32 
void* memset(
	void* dest,
	int c,
	size_t count
);
#pragma intrinsic(memset)
#else
void* memset(
	void* dest,
	int c,
	size_t count
) 
{
	for (uint64_t i = 0; i < count; ++i) {
		((uint8_t*)dest)[i] = c;
	}
	return dest;
}
#endif

static uint32_t lzComputeAdler32(const uint8_t* data, uint64_t size) {
	uint32_t s1 = 1;
	uint32_t s2 = 0;

	for (uint64_t i = 0; i < size; ++i) {
		s1 = (s1 + data[i]) % 65521;
		s2 = (s2 + s1) % 65521;
	}
	uint32_t adler32 = (s2  << 16) + s1;

	return 
		((adler32 >> 24) & 0x000000FF) |
		((adler32 >> 8)  & 0x0000FF00) |
		((adler32 << 8)  & 0x00FF0000) |
		((adler32 << 24) & 0xFF000000);
}

static uint32_t ReverseBits(uint32_t code, uint32_t len) {
	uint32_t res = 0;
	for (uint32_t i = 0; i < len; ++i) {
		res = (res << 1) | (code & 1);
		code >>= 1;
	}
	return res;
}

static void hBuild(HuffTable* table, const uint8_t* codeLens) {
	uint32_t BL_COUNT[19] = { 0 };
	uint32_t BL_NEXT[19] = { 0 };

	uint64_t size = table->size;

	uint32_t maxLen = 0;
	for (uint32_t i = 0; i < size; ++i) {
		uint32_t len = codeLens[i];
		maxLen = maxLen < len ? len : maxLen;
	}

	for (uint32_t i = 0; i < size; ++i) {
		BL_COUNT[codeLens[i]]++;
	}
	BL_COUNT[0] = 0;

	uint32_t code = 0;
	for (uint32_t i = 1; i <= maxLen; ++i) {
		code = (code + BL_COUNT[i - 1]) << 1;
		BL_NEXT[i] = code;
	}

	for (uint32_t i = 0; i <= size; ++i) {
		uint32_t len = codeLens[i];
		if (len != 0) {
			table->tree[i].length = len;
			table->tree[i].code = ReverseBits(BL_NEXT[len]++, len);
		}
	}
}

static int hDecode(HuffTable* table, BitReader* bs) {
	DEBUG(table);
	DEBUG(bs);

	int32_t error = 0;
	uint64_t size = table->size;
	HuffEntry* tree = table->tree;

	for (int i = 0; i < size; ++i) {
		uint32_t len = tree[i].length;
		if (!len) {
			continue;
		}
		uint32_t code = 0;
		CHECK(code = bsPeakBits(bs, len), error);
		if (code == tree[i].code) {
			bs->bitBuff >>= len;
			bs->bitCount -= len;
			return i;
		}
	}
	return 1;
}

int lzInflateInit(ZlibReader* z, const uint8_t* data, uint64_t size) {
	DEBUG(z);
	DEBUG(data);

	int32_t error = 0;
	CHECK(bsReaderInit(&z->stream, data, size), error);
	CHECK(z->header.cm = bsGetBits(&z->stream, 4), error);
	CHECK(z->header.cinfo = bsGetBits(&z->stream, 4), error);
	CHECK(z->header.fcheck = bsGetBits(&z->stream, 5), error);
	CHECK(z->header.fdict = bsGetBits(&z->stream, 1), error);
	CHECK(z->header.flevel = bsGetBits(&z->stream, 2), error);

	if ((z->header.cm != 8) || (z->header.cinfo != 7)) {
		return -2;
	}
	if (z->header.fdict != 0) {
		return -3; // TODO: FDICT support.
	}

	return 1;
}

int lzInflate(ZlibReader* z, uint8_t* output, uint64_t outSize)
{
	DEBUG(z);
	DEBUG(output);
	DEBUG(outSize);
	DEBUG(z->stream.bits);

	static uint8_t CL[19] = { 0 };
	static uint8_t HUFF_TREE[320] = { 0 };
	static HuffEntry CLCL_BUFF[19] = { 0 };
	static HuffEntry LITLEN_BUFF[286] = { 0 };
	static HuffEntry DIST_BUFF[32] = { 0 };

#ifdef _DEBUG
	int32_t error = 0;
#endif // _DEBUG

	uint64_t outIdx = 0;

	int32_t BFINAL = 0;
	int32_t BTYPE = 0;

	do
	{
		CHECK(BFINAL = bsGetBits(&z->stream, 1), error);
		CHECK(BTYPE = bsGetBits(&z->stream, 2), error);

		switch (BTYPE) {
		case 0:
		{
			bsReaderFlush(&z->stream);

			uint16_t LEN = 0;
			uint16_t NLEN = 0;
			CHECK(LEN = (uint16_t)bsGetBits(&z->stream, 16), error);
			CHECK(NLEN = (uint16_t)bsGetBits(&z->stream, 16), error);

			if ((uint16_t)(~LEN) != (uint16_t)(NLEN)) {
				return -4;
			}

			for (uint16_t i = 0; i < LEN; ++i) {
				if (outIdx > outSize) {
					return -3;
				}
				CHECK(output[outIdx++] = bsGetByte(&z->stream), error);
			}
			break;
		}
		case 1:
		{
			memset(LITLEN_BUFF, 0, sizeof(LITLEN_BUFF));
			memset(DIST_BUFF, 0, sizeof(DIST_BUFF));

			HuffTable litlen = { .size = sizeof(FIXED_LITLEN_CL), .tree =  LITLEN_BUFF};
			HuffTable dist = { .size = sizeof(FIXED_DIST_CL), .tree = DIST_BUFF};

			hBuild((HuffTable*)&litlen, FIXED_LITLEN_CL);
			hBuild((HuffTable*)&dist, FIXED_DIST_CL);

			while (1) {
				uint32_t code = 0;
				CHECK(code = hDecode((HuffTable*)&litlen, &z->stream), error);
				if (code == 256)
					break;
				else if (code < 256) {
					if (outIdx > outSize) {
						return -3;
					}
					output[outIdx++] = code;
				}
				else if (code > 256) {
					uint8_t baseIdx = code - 257;
					uint32_t baseLen = BASE_LENGTH[baseIdx] + bsGetBits(&z->stream, BASE_LENGTH_EXTRA_BITS[baseIdx]);

					uint8_t distanceIdx = hDecode((HuffTable*)&dist, &z->stream);
					uint32_t distanceLen = BASE_DIST[distanceIdx] + bsGetBits(&z->stream, BASE_DIST_EXTRA_BITS[distanceIdx]);

					uint64_t backIdx = outIdx - distanceLen;
					while (baseLen--) {
						if (outIdx > outSize) {
							return -3;
						}
						output[outIdx++] = output[backIdx++];
					}
				}
			}
			break;
		};
		case 2:
		{
			memset(CL, 0, sizeof(CL));
			memset(HUFF_TREE, 0, sizeof(HUFF_TREE));
			memset(CLCL_BUFF, 0, sizeof(CLCL_BUFF));
			memset(LITLEN_BUFF, 0, sizeof(LITLEN_BUFF));
			memset(DIST_BUFF, 0, sizeof(DIST_BUFF));

			uint32_t HLIT = 0;
			uint32_t HDIST = 0;
			uint32_t HCLEN = 0;

			CHECK(HLIT = (bsGetBits(&z->stream, 5) + 257), error);
			CHECK(HDIST = (bsGetBits(&z->stream, 5) + 1), error);
			CHECK(HCLEN = (bsGetBits(&z->stream, 4) + 4), error);

			if (HLIT > 286 || HDIST > 32 || HCLEN > 19) {
				return -4;
			}

			HuffTable clcl = { .size = 19, .tree = CLCL_BUFF };
			HuffTable litlen = { .size = HLIT, .tree = LITLEN_BUFF};
			HuffTable dist = { .size = HDIST, .tree = DIST_BUFF};

			for (uint32_t i = 0; i < HCLEN; ++i) {
				CHECK(CL[CL_ORDER[i]] = (uint8_t)bsGetBits(&z->stream, 3), error);
			}

			hBuild((HuffTable*)&clcl, CL);

			for (uint32_t i = 0; i < litlen.size + dist.size;) {
				uint32_t code = 0;
				CHECK(code = hDecode((HuffTable*)&clcl, &z->stream), error);
				if (code <= 15) {
					HUFF_TREE[i++] = code;
				}
				else if (code == 16) {
					DEBUG(i > 0);
					uint32_t rep = 0;
					CHECK(rep = (uint32_t)bsGetBits(&z->stream, 2) + 3, error);
					for (uint32_t r = 0; r < rep; r++) HUFF_TREE[i++] = HUFF_TREE[i - 1];
				}
				else if (code == 17) {
					uint32_t rep = 0;
					CHECK(rep = (uint32_t)bsGetBits(&z->stream, 3) + 3, error);
					for (uint32_t r = 0; r < rep; r++) HUFF_TREE[i++] = 0;
				}
				else {
					uint32_t rep = 0;
					CHECK(rep = (uint32_t)bsGetBits(&z->stream, 7) + 11, error);
					for (uint32_t r = 0; r < rep; r++) HUFF_TREE[i++] = 0;
				}
			}

			hBuild((HuffTable*)&litlen, HUFF_TREE);
			hBuild((HuffTable*)&dist, HUFF_TREE + litlen.size);

			while (1) {
				uint32_t code = 0;
				CHECK(code = hDecode((HuffTable*)&litlen, &z->stream), error);
				if (code == 256) {
					break;
				}
				else if (code <= 255) {
					if (outIdx > outSize) {
						return -3;
					}
					output[outIdx++] = code;
				}
				else if (code > 256 && code < 286) {
					uint8_t baseIdx = code - 257;
					uint32_t baseLen = 0;
					baseLen = BASE_LENGTH[baseIdx] + bsGetBits(&z->stream, BASE_LENGTH_EXTRA_BITS[baseIdx]);

					uint8_t distIdx = 0;
					CHECK(distIdx = hDecode((HuffTable*)&dist, &z->stream), error);
					uint32_t basedist = BASE_DIST[distIdx] + bsGetBits(&z->stream, BASE_DIST_EXTRA_BITS[distIdx]);

					uint64_t backIdx = outIdx - basedist;
					while (baseLen--) {
						if (outIdx > outSize) {
							return -3;
						}
						output[outIdx++] = output[backIdx++];
					}
				}
			}
			break;
		}
		default:
		{
			return -5;
			break;
		}
		}

	} while (BFINAL == 0);

	((uint8_t*)&z->adler32)[0] = bsGetByte(&z->stream);
	((uint8_t*)&z->adler32)[1] = bsGetByte(&z->stream);
	((uint8_t*)&z->adler32)[2] = bsGetByte(&z->stream);
	((uint8_t*)&z->adler32)[3] = bsGetByte(&z->stream);

	if (z->adler32 != lzComputeAdler32(output, outIdx)) {
		return -5;
	}

	return 1;
}

int lzDeflateInit(ZlibWriter* z, const uint8_t* data, uint64_t size)
{
	DEBUG(z);
	DEBUG(data);
	DEBUG(size);

	z->size = size;
	z->uncompressed = data;

	return 1;
}

int lzDeflate(ZlibWriter* z, uint8_t* output, uint64_t outSize)
{
	DEBUG(z);
	DEBUG(output);
	DEBUG(outSize);
#ifdef _DEBUG
	int32_t error = 0;
#endif // _DEBUG

	uint64_t outIdx = 0;
	uint64_t remaining = z->size;
	CHECK(bsWriterInit(&z->stream, output, outSize), error);

	z->header.cm = 0x08;
	z->header.cinfo = 0x07;
	z->header.fcheck = 0x01;
	z->header.fdict = 0x00;
	z->header.flevel = 0x00;

	CHECK(bsWriteBits(&z->stream, *(uint64_t*)&z->header, 16), error);

	uint8_t BFINAL = 0;
	do {
		BFINAL = z->size < FIXED_BLOCK_SIZE;

		DeflateBlock block = { 0 };
		block.BFINAL = BFINAL;
		block.BTYPE = 0x00;

		CHECK(bsWriteBits(&z->stream, *(uint64_t*)&block, 3), error);
		bsWriterFlush(&z->stream);

		uint16_t LEN = (uint16_t)BFINAL ? (uint16_t)z->size : FIXED_BLOCK_SIZE;
		uint16_t NLEN = ~LEN;
		remaining -= LEN;

		CHECK(bsWriteBits(&z->stream, LEN, 16), error);
		CHECK(bsWriteBits(&z->stream, NLEN, 16), error);

		CHECK(bsWriteBytes(&z->stream, z->uncompressed + outIdx, LEN), error);
		outIdx += LEN;

	} while (!BFINAL);

	uint32_t adler32 = lzComputeAdler32(z->uncompressed, z->size);

	CHECK(bsWriteBits(&z->stream, adler32, 32), error);

	return 1;
}