#include "lz_bitstream.h"
#include "lz_debug.h"

#define MASK(x, n) (x & (((uint64_t)(1) << (uint64_t)(n)) - (uint64_t)(1)))

int32_t bsReaderInit(BitReader* stream, const uint8_t* data, uint64_t size)
{
	DEBUG(stream);
	DEBUG(stream);
	DEBUG(size);
	stream->bits = data;
	stream->size = size;
	return 1;
}

int32_t bsGetBits(BitReader* stream, uint64_t n) 
{
	DEBUG(stream);

	if (n > 32) {
		return -2;
	}
	if (!n) return 0; // This is not an error.
	while (stream->bitCount < n) {
		if (stream->size == 0) {
			return -2;
		}
		uint64_t b = *stream->bits++;
		stream->size--;
		stream->bitBuff |= (b << stream->bitCount);
		stream->bitCount += 8;
	}
	int32_t ret = MASK(stream->bitBuff, n);
	stream->bitBuff >>= n;
	stream->bitCount -= n;
	return ret;
}

int32_t bsPeakBits(BitReader* stream, uint64_t n) 
{
	DEBUG(stream);

	if (n > 31) {
		return -2;
	}
	if (!n) return 0; // This is not an error.
	while (stream->bitCount < n) {
		if (stream->size == 0) {
			return -3;
		}
		uint64_t b = *stream->bits++;
		stream->size--;
		stream->bitBuff |= (b << stream->bitCount);
		stream->bitCount += 8;
	}
	return MASK(stream->bitBuff, n);
}

int32_t bsGetByte(BitReader* reader)
{
	DEBUG(reader);
	reader->size--;
	return *reader->bits++;
}

void bsReaderFlush(BitReader* reader)
{
	if (!reader) return; // NOP;
	reader->bitBuff = 0;
	reader->bitCount = 0;
}

int32_t bsWriterInit(BitWriter* writer, uint8_t* output, uint64_t size)
{
	DEBUG(writer);
	DEBUG(output);
	DEBUG(size);
	writer->bitIdx = 0;
	writer->writeIdx = 0;
	writer->bits = output;
	writer->size = size;
	return 1;
}

int32_t bsWriteBits(BitWriter* writer, uint64_t value, uint64_t nBits)
{
	DEBUG(writer);
	DEBUG(writer->bits);

	if (nBits == 0) return 1; // not an error
	if (nBits > 64) return -2;
	do {
		uint8_t bit = value & 0x01;
		writer->bits[writer->writeIdx] |= (bit << writer->bitIdx++);
		if (writer->bitIdx >= 8) {
			if (++writer->writeIdx >= writer->size) {
				return -3;
			}
			writer->bitIdx = 0;
		}
		value >>= 1;
	} while (--nBits);

	return 1;
}

int32_t bsWriteBytes(BitWriter* writer, const uint8_t* values, uint64_t size)
{
	DEBUG(writer);
	DEBUG(writer->bits);
	DEBUG(values);

	if (size == 0) return 1;
	for (uint64_t i = 0; i < size; ++i) {
		if (writer->writeIdx >= writer->size) {
			return -3;
		}
		writer->bits[writer->writeIdx++] = values[i];
	}

	return 1;
}

void bsWriterFlush(BitWriter* writer)
{
	if (!writer) return; // NOP
	writer->writeIdx++;
	writer->bitIdx = 0;
}
