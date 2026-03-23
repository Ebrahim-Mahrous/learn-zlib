#ifndef _BITSTREAM_H
#define _BITSTREAM_H
#include <stdint.h>

typedef uint8_t byte;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef char i8;
typedef long long i64;
typedef int i32;

typedef struct BitReader {
	u64 bitBuff;
	u64 bitCount;
	u64 size;
	const byte* bits;
} BitReader;

typedef struct BitWriter {
	u64 bitIdx;
	u64 writeIdx;
	u64 size;
	byte* bits;
} BitWriter;

i32 bsReaderInit(BitReader* reader, const byte* data, u64 size);

i32 bsGetBits(BitReader* reader, u64 nBits);

i32 bsPeakBits(BitReader* reader, u64 nBits);

i32 bsGetByte(BitReader* reader);

void bsReaderFlush(BitReader* reader);

i32 bsWriterInit(BitWriter* writer, byte* data, u64 size);

i32 bsWriteBits(BitWriter* writer, u64 value, u64 nBits);

i32 bsWriteBytes(BitWriter* writer, const byte* values, u64 size);

void bsWriterFlush(BitWriter* writer);

#endif
