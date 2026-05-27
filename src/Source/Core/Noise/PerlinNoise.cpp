#include "../../../Headers/Core/Noise/PerlinNoise.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

PerlinNoise::PerlinNoise(unsigned int seed) {
  std::iota(p.begin(), p.begin() + 256, 0);

  std::default_random_engine engine(seed);
  std::shuffle(p.begin(), p.begin() + 256, engine);

  for (int i = 0; i < 256; i++)
    p[256 + i] = p[i];
}

float PerlinNoise::noise(float x, float z) const {
  int X = (int)std::floor(x) & 255;
  int Z = (int)std::floor(z) & 255;

  x -= std::floor(x);
  z -= std::floor(z);

  float u = fade(x);
  float v = fade(z);

  int aa = p[p[X] + Z];
  int ba = p[p[X + 1] + Z];
  int ab = p[p[X] + Z + 1];
  int bb = p[p[X + 1] + Z + 1];

  return lerp(v, lerp(u, grad(aa, x, z), grad(ba, x - 1.0f, z)),
              lerp(u, grad(ab, x, z - 1.0f), grad(bb, x - 1.0f, z - 1.0f)));
}
