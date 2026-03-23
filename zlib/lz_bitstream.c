#include "lz_bitstream.h"
#include "lz_debug.h"

#define MASK(x, n) (x & (((u64)(1) << (u64)(n)) - (u64)(1)))

i32 bsReaderInit(BitReader* stream, const byte* data, u64 size)
{
	DEBUG(stream);
	DEBUG(stream);
	DEBUG(size);
	stream->bits = data;
	stream->size = size;
	return 1;
}

i32 bsGetBits(BitReader* stream, u64 n) 
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
		u64 b = *stream->bits++;
		stream->size--;
		stream->bitBuff |= (b << stream->bitCount);
		stream->bitCount += 8;
	}
	i32 ret = MASK(stream->bitBuff, n);
	stream->bitBuff >>= n;
	stream->bitCount -= n;
	return ret;
}

i32 bsPeakBits(BitReader* stream, u64 n) 
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
		u64 b = *stream->bits++;
		stream->size--;
		stream->bitBuff |= (b << stream->bitCount);
		stream->bitCount += 8;
	}
	return MASK(stream->bitBuff, n);
}

i32 bsGetByte(BitReader* reader)
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

i32 bsWriterInit(BitWriter* writer, byte* output, u64 size)
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

i32 bsWriteBits(BitWriter* writer, u64 value, u64 nBits)
{
	DEBUG(writer);
	DEBUG(writer->bits);

	if (nBits == 0) return 1; // not an error
	if (nBits > 64) return -2;
	do {
		u8 bit = value & 0x01;
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

i32 bsWriteBytes(BitWriter* writer, const byte* values, u64 size)
{
	DEBUG(writer);
	DEBUG(writer->bits);
	DEBUG(values);

	if (size == 0) return 1;
	for (u64 i = 0; i < size; ++i) {
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
