#include <assert.h>
#include "svpng/svpng.inc"

#include "NoiseGenerator.h"

void NoiseGenerator::CreateFloatNoiseTexture(const char *texture_name, unsigned int size)
{
	// size must be non-zero and power of 2
	assert(size > 0 && ((size - 1) & size) == 0);
	float *data = new float[size * size];
	FILE* texture;
	fopen_s(&texture, texture_name, "wb");
	for (unsigned int i = 0; i < size * size; i++)
	{
		data[i] = RandomFloat01();
	}
	// create 16bit grayscale png with alpha
	svpng(texture, size, size, (unsigned char *)data, 1);
	fclose(texture);
}

float NoiseGenerator::RandomFloat01()
{
	return rng() / (float)rng.max();
}
