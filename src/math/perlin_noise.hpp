#pragma once

#include <glm/gtc/noise.hpp>

namespace my {

class PerlinGenarator {
  public:
    static glm::vec2 randomGradient(int ix, int iy) {
        // No precomputed gradients mean this works for any number of grid coordinates
        const unsigned w = 8 * sizeof(unsigned);
        const unsigned s = w / 2;
        unsigned a = ix, b = iy;
        a *= 3284157443;

        b ^= a << s | a >> w - s;
        b *= 1911520717;

        a ^= b << s | b >> w - s;
        a *= 2048419325;
        float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]

        // Create the vector from the angle
        glm::vec2 v;
        v.x = sin(random);
        v.y = cos(random);

        return v;
    }

    static float dotGridGradient(int ix, int iy, float x, float y) {
        glm::vec2 gradient = randomGradient(ix, iy);

        float distanceX = x - (float)ix;
        float distanceY = y - (float)iy;

        return (distanceX * gradient.x + distanceY * gradient.y);
    }

    static float interpolate(float a0, float a1, float weight) {
        return (a1 - a0) * (3.0 - weight * 2.0) * weight * weight + a0;
    }

    static float perlin(float x, float y) {
        int ul = int(x);
        int bl = int(y);
        int ur = ul + 1;
        int br = bl + 1;

        float sampleX = x - float(ul);
        float sampleY = y - float(bl);

        float n0 = dotGridGradient(ul, bl, x, y);
        float n1 = dotGridGradient(ur, bl, x, y);
        float ix0 = interpolate(n0, n1, sampleX);

        n0 = dotGridGradient(ul, br, x, y);
        n1 = dotGridGradient(ur, br, x, y);
        float ix1 = interpolate(n0, n1, sampleX);

        float value = interpolate(ix0, ix1, sampleY);

        return value;
    }

  private:
};

} // namespace my
