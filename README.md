# Learn: zlib
A tiny zlib implementation for personal learning.

# Features
- `lzInflate`: Supports all decompression schemes. It can decompress any zlib stream.
- `lzDeflate`: Only supports BTYPE 00 (No compression) but produces a valid zlib stream for any given data.

# Examples
There is an example in `example.c`.