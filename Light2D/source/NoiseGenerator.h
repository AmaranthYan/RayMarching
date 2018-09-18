#pragma once
#include <random>

class NoiseGenerator
{
public:
	NoiseGenerator(unsigned int seed) : rng(seed) { };
	void CreateFloatNoiseTexture(const char *texture_name, unsigned int size);

private:
	std::mt19937 rng;

	float RandomFloat01();
};