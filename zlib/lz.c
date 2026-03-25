#include "lz.h"
#include "lz_debug.h"

#define FIXED_BLOCK_SIZE 65535

static const u8 CL_ORDER[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

static const u32 BASE_LENGTH[] = {
	3, 4, 5, 6, 7, 8, 9, 10, //257 - 264
	11, 13, 15, 17,          //265 - 268
	19, 23, 27, 31,          //269 - 273 
	35, 43, 51, 59,          //274 - 276
	67, 83, 99, 115,         //278 - 280
	131, 163, 195, 227,      //281 - 284
	258                      //285
};

static const u8 BASE_LENGTH_EXTRA_BITS[] = {
	0, 0, 0, 0, 0, 0, 0, 0, //257 - 264
	1, 1, 1, 1, //265 - 268
	2, 2, 2, 2, //269 - 273 
	3, 3, 3, 3, //274 - 276
	4, 4, 4, 4, //278 - 280
	5, 5, 5, 5, //281 - 284
	0           //285
};

static const u32 BASE_DIST[] = {
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

static const u8 BASE_DIST_EXTRA_BITS[] = {
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
	u32 code;
	u32 length;
} HuffEntry;

#define _HuffTable(n) struct { u64 size; HuffEntry tree[n]; }

typedef struct HuffTable {
	u64 size;
	HuffEntry tree[];
} HuffTable;

static u32 lzComputeAdler32(const byte* data, u64 size) {
	u16 s1 = 1;
	u16 s2 = 0;

	for (u64 i = 0; i < size; ++i) {
		s1 = (s1 + data[i]) % 65521;
		s2 = (s2 + s1) % 65521;
	}
	u32 adler32 = (s2 * 65536) + s1;

	return 
		((adler32 >> 24) & 0x000000FF) |
		((adler32 >> 8)  & 0x0000FF00) |
		((adler32 << 8)  & 0x00FF0000) |
		((adler32 << 24) & 0xFF000000);
}

static u32 ReverseBits(u32 code, u32 len) {
	u32 res = 0;
	for (u32 i = 0; i < len; ++i) {
		res = (res << 1) | (code & 1);
		code >>= 1;
	}
	return res;
}

static void hBuild(HuffTable* table, const u8* codeLens) {
	u32 BL_COUNT[19] = { 0 };
	u32 BL_NEXT[19] = { 0 };

	u64 size = table->size;

	u32 maxLen = 0;
	for (u32 i = 0; i < size; ++i) {
		u32 len = codeLens[i];
		maxLen = maxLen < len ? len : maxLen;
	}

	for (u32 i = 0; i < size; ++i) {
		BL_COUNT[codeLens[i]]++;
	}
	BL_COUNT[0] = 0;

	u32 code = 0;
	for (u32 i = 1; i <= maxLen; ++i) {
		code = (code + BL_COUNT[i - 1]) << 1;
		BL_NEXT[i] = code;
	}

	for (u32 i = 0; i <= size; ++i) {
		u32 len = codeLens[i];
		if (len != 0) {
			table->tree[i].length = len;
			table->tree[i].code = ReverseBits(BL_NEXT[len]++, len);
		}
	}
}

static int hDecode(HuffTable* table, BitReader* bs) {
	DEBUG(table);
	DEBUG(bs);

	i32 error = 0;
	u64 size = table->size;
	HuffEntry* tree = table->tree;

	for (int i = 0; i < size; ++i) {
		u32 len = tree[i].length;
		if (!len) {
			continue;
		}
		u32 code = 0;
		CHECK(code = bsPeakBits(bs, len), error);
		if (code == tree[i].code) {
			bs->bitBuff >>= len;
			bs->bitCount -= len;
			return i;
		}
	}
	return 1;
}

int lzInflateInit(ZlibReader* z, const byte* data, u64 size) {
	DEBUG(z);
	DEBUG(data);

	i32 error = 0;
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

int lzInflate(ZlibReader* z, byte* output, u64 outSize)
{
	DEBUG(z);
	DEBUG(output);
	DEBUG(outSize);
	DEBUG(z->stream.bits);

#ifdef _DEBUG
	i32 error = 0;
#endif // _DEBUG

	u64 outIdx = 0;

	i32 BFINAL = 0;
	i32 BTYPE = 0;

	do
	{
		CHECK(BFINAL = bsGetBits(&z->stream, 1), error);
		CHECK(BTYPE = bsGetBits(&z->stream, 2), error);

		switch (BTYPE) {
		case 0:
		{
			bsReaderFlush(&z->stream);

			u16 LEN = 0;
			u16 NLEN = 0;
			CHECK(LEN = (u16)bsGetBits(&z->stream, 16), error);
			CHECK(NLEN = (u16)bsGetBits(&z->stream, 16), error);

			if ((u16)(~LEN) != (u16)(NLEN)) {
				return -4;
			}

			for (u16 i = 0; i < LEN; ++i) {
				if (outIdx > outSize) {
					return -3;
				}
				CHECK(output[outIdx++] = bsGetByte(&z->stream), error);
			}
			break;
		}
		case 1:
		{
			_HuffTable(sizeof(FIXED_LITLEN_CL)) litlen = { 0 };
			_HuffTable(sizeof(FIXED_DIST_CL)) dist = { 0 };

			litlen.size = sizeof(FIXED_LITLEN_CL);
			dist.size = sizeof(FIXED_DIST_CL);
			hBuild((HuffTable*)&litlen, FIXED_LITLEN_CL);
			hBuild((HuffTable*)&dist, FIXED_DIST_CL);

			while (1) {
				u32 code = 0;
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
					u8 baseIdx = code - 257;
					u32 baseLen = BASE_LENGTH[baseIdx] + bsGetBits(&z->stream, BASE_LENGTH_EXTRA_BITS[baseIdx]);

					u8 distanceIdx = hDecode((HuffTable*)&dist, &z->stream);
					u32 distanceLen = BASE_DIST[distanceIdx] + bsGetBits(&z->stream, BASE_DIST_EXTRA_BITS[distanceIdx]);

					u64 backIdx = outIdx - distanceLen;
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
			u8 CL[19] = { 0 };
			u8 HUFF_TREE[320] = { 0 };

			u32 HLIT = 0;
			u32 HDIST = 0;
			u32 HCLEN = 0;

			_HuffTable(19) clcl = { 0 };
			_HuffTable(286) litlen = { 0 };
			_HuffTable(32) dist = { 0 };

			CHECK((u32)HLIT = (bsGetBits(&z->stream, 5) + 257), error);
			CHECK((u32)HDIST = (bsGetBits(&z->stream, 5) + 1), error);
			CHECK((u32)HCLEN = (bsGetBits(&z->stream, 4) + 4), error);

			if (HLIT > 286 || HDIST > 32 || HCLEN > 19) {
				return -4;
			}

			litlen.size = HLIT;
			dist.size = HDIST;
			clcl.size = 19;

			for (u32 i = 0; i < HCLEN; ++i) {
				CHECK(CL[CL_ORDER[i]] = (u8)bsGetBits(&z->stream, 3), error);
			}

			hBuild((HuffTable*)&clcl, CL);

			for (u32 i = 0; i < litlen.size + dist.size;) {
				u32 code = 0;
				CHECK(code = hDecode((HuffTable*)&clcl, &z->stream), error);
				if (code <= 15) {
					HUFF_TREE[i++] = code;
				}
				else if (code == 16) {
					DEBUG(i > 0);
					u32 rep = 0;
					CHECK(rep = (u32)bsGetBits(&z->stream, 2) + 3, error);
					for (u32 r = 0; r < rep; r++) HUFF_TREE[i++] = HUFF_TREE[i - 1];
				}
				else if (code == 17) {
					u32 rep = 0;
					CHECK(rep = (u32)bsGetBits(&z->stream, 3) + 3, error);
					for (u32 r = 0; r < rep; r++) HUFF_TREE[i++] = 0;
				}
				else {
					u32 rep = 0;
					CHECK(rep = (u32)bsGetBits(&z->stream, 7) + 11, error);
					for (u32 r = 0; r < rep; r++) HUFF_TREE[i++] = 0;
				}
			}

			hBuild((HuffTable*)&litlen, HUFF_TREE);
			hBuild((HuffTable*)&dist, HUFF_TREE + litlen.size);

			while (1) {
				u32 code = 0;
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
					u8 baseIdx = code - 257;
					u32 baseLen = 0;
					baseLen = BASE_LENGTH[baseIdx] + bsGetBits(&z->stream, BASE_LENGTH_EXTRA_BITS[baseIdx]);

					u8 distIdx = 0;
					CHECK(distIdx = hDecode((HuffTable*)&dist, &z->stream), error);
					u32 basedist = BASE_DIST[distIdx] + bsGetBits(&z->stream, BASE_DIST_EXTRA_BITS[distIdx]);

					u64 backIdx = outIdx - basedist;
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

	((byte*)&z->adler32)[0] = bsGetByte(&z->stream);
	((byte*)&z->adler32)[1] = bsGetByte(&z->stream);
	((byte*)&z->adler32)[2] = bsGetByte(&z->stream);
	((byte*)&z->adler32)[3] = bsGetByte(&z->stream);

	if (z->adler32 != lzComputeAdler32(output, outIdx)) {
		return -5;
	}

	return 1;
}

int lzDeflateInit(ZlibWriter* z, const byte* data, u64 size)
{
	DEBUG(z);
	DEBUG(data);
	DEBUG(size);

	z->size = size;
	z->uncompressed = data;

	return 1;
}

int lzDeflate(ZlibWriter* z, byte* output, u64 outSize)
{
	DEBUG(z);
	DEBUG(output);
	DEBUG(outSize);
#ifdef _DEBUG
	i32 error = 0;
#endif // _DEBUG

	u64 outIdx = 0;
	u64 remaining = z->size;
	CHECK(bsWriterInit(&z->stream, output, outSize), error);

	z->header.cm = 0x08;
	z->header.cinfo = 0x07;
	z->header.fcheck = 0x01;
	z->header.fdict = 0x00;
	z->header.flevel = 0x00;

	CHECK(bsWriteBits(&z->stream, *(u64*)&z->header, 16), error);

	u8 BFINAL = 0;
	do {
		BFINAL = z->size < FIXED_BLOCK_SIZE;

		DeflateBlock block = { 0 };
		block.BFINAL = BFINAL;
		block.BTYPE = 0x00;

		CHECK(bsWriteBits(&z->stream, *(u64*)&block, 3), error);
		bsWriterFlush(&z->stream);

		u16 LEN = (u16)BFINAL ? (u16)z->size : FIXED_BLOCK_SIZE;
		u16 NLEN = ~LEN;
		remaining -= LEN;

		CHECK(bsWriteBits(&z->stream, LEN, 16), error);
		CHECK(bsWriteBits(&z->stream, NLEN, 16), error);

		CHECK(bsWriteBytes(&z->stream, z->uncompressed + outIdx, LEN), error);
		outIdx += LEN;

	} while (!BFINAL);

	u32 adler32 = lzComputeAdler32(z->uncompressed, z->size);

	CHECK(bsWriteBits(&z->stream, adler32, 32), error);

	return 1;
}