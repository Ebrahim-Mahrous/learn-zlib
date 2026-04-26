#define _CRT_SECURE_NO_WARNINGS
#include "zlib/lz.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_LIMIT 1 << 16 // 65 KB for this example

int32_t main(int32_t argc, char* argv[]) {
	// Example decompression, the example file is a compressed 100x100 image of a red rectangle.

	//uint8_t* input = NULL;
	//uint8_t *output = NULL;

	//uint64_t size = 0;
	//FILE* file = NULL;

	//file = fopen("output.bin", "rb");
	//
	//	fseek(file, 0, SEEK_END);
	//	size = ftell(file);
	//	fseek(file, 0, SEEK_SET);

	//	if (size > DATA_LIMIT) return -1;

	//	input = (uint8_t*)calloc(1, size);
	//	if (!input) return -1;

	//	fread(input, 1, size, file);

	//fclose(file);

	//output = (uint8_t*)calloc(1, DATA_LIMIT);
	//if (!output) return -1;

	//int32_t error = 0;
	//ZlibReader zlib = { 0 };

	//	error = lzInflateInit(&zlib, input, size);
	//	printf("%i\n", error);

	//	error = lzInflate(&zlib, output, DATA_LIMIT);
	//	printf("%i\n", error);

	//puts(output);

	//free(output);

	//return 0;

	// Example compression

	static uint8_t input[1 << 16] = { 0 };
	static uint8_t output[(1 << 17) + 1024] = { 0 };

	memset(input, 'q', sizeof(input));

	FILE* file = NULL;
	int32_t error = 0;
	uint64_t outSize = sizeof(output);
	ZlibWriter zlib = { 0 };

		error = lzDeflateInit(&zlib, input, sizeof(input));
		printf("lzDeflateInit() = %i\n", error);

		error = lzDeflate(&zlib, output, &outSize);
		printf("lzDeflate() = %i\n", error);

	file = fopen("output.bin", "wb");

		if (!file) return -10;
		fwrite(output, 1, outSize, file);
		//for (uint64_t i = 0; i < outSize; ++i) {
		//	fprintf(file, "%02x", output[i]);
		//}

	fclose(file);

	return 0;
}