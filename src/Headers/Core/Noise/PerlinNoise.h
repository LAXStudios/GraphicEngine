#pragma once
#include <array>
class PerlinNoise {
public:
  PerlinNoise(unsigned int seed);
  float noise(float x, float z) const;
  float noise01(float x, float z) const;

private:
  std::array<int, 512> p;

  static float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
  }

  static float lerp(float t, float a, float b) { return a + t * (b - a); }

  static float grad(int hash, float x, float z) {
    switch (hash & 3) {
    case 0:
      return x + z;
    case 1:
      return -x + z;
    case 2:
      return x - z;
    case 3:
      return -x - z;
    }
    return 0.0f;
  }
};
