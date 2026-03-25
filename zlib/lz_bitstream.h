#ifndef _BITSTREAM_H
#define _BITSTREAM_H
#include <stdint.h>

typedef struct BitReader {
	uint64_t bitBuff;
	uint64_t bitCount;
	uint64_t size;
	const uint8_t* bits;
} BitReader;

typedef struct BitWriter {
	uint64_t bitIdx;
	uint64_t writeIdx;
	uint64_t size;
	uint8_t* bits;
} BitWriter;

int32_t bsReaderInit(BitReader* reader, const uint8_t* data, uint64_t size);

int32_t bsGetBits(BitReader* reader, uint64_t nBits);

int32_t bsPeakBits(BitReader* reader, uint64_t nBits);

int32_t bsGetByte(BitReader* reader);

void bsReaderFlush(BitReader* reader);

int32_t bsWriterInit(BitWriter* writer, uint8_t* data, uint64_t size);

int32_t bsWriteBits(BitWriter* writer, uint64_t value, uint64_t nBits);

int32_t bsWriteBytes(BitWriter* writer, const uint8_t* values, uint64_t size);

void bsWriterFlush(BitWriter* writer);

#endif
