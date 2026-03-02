#pragma once

#include <glm/gtc/noise.hpp>

namespace my {

class PerlinGenerator {
  public:
    // Returns a pseudo-random unit gradient vector for grid point (ix, iy)
    static glm::vec2 randomGradient(int ix, int iy) {
        const unsigned BITS = 8 * sizeof(unsigned);
        const unsigned HALF_BITS = BITS / 2;

        // Hash the grid coordinates into a pseudo-random angle
        unsigned hashX = ix, hashY = iy;
        hashX *= 3284157443;
        hashY ^= hashX << HALF_BITS | hashX >> BITS - HALF_BITS;
        hashY *= 1911520717;
        hashX ^= hashY << HALF_BITS | hashY >> BITS - HALF_BITS;
        hashX *= 2048419325;

        float angle = hashX * (3.14159265f / ~(~0u >> 1));

        return {sin(angle), cos(angle)};
    }

    static float dotGridGradient(int ix, int iy, float x, float y) {
        glm::vec2 gradient = randomGradient(ix, iy);
        float distanceX = x - (float)ix;
        float distanceY = y - (float)iy;
        return (distanceX * gradient.x + distanceY * gradient.y);
    }

    static float smoothstep(float from, float to, float weight) {
        float w = weight * weight * weight * (weight * (weight * 6.0f - 15.0f) + 10.0f);
        return from + (to - from) * w;
    }

    static float perlin(float x, float y) {
        // Grid cell corner indices — must use floor(), not int(), to handle negative coords
        int x0 = (int)glm::floor(x), x1 = x0 + 1;
        int y0 = (int)glm::floor(y), y1 = y0 + 1;

        // Local position within the cell, guaranteed in [0, 1]
        float localX = x - float(x0);
        float localY = y - float(y0);

        // Dot products at the four corners
        float dotBL = dotGridGradient(x0, y0, x, y);
        float dotBR = dotGridGradient(x1, y0, x, y);
        float dotTL = dotGridGradient(x0, y1, x, y);
        float dotTR = dotGridGradient(x1, y1, x, y);

        // Interpolate along x for bottom and top rows, then along y
        float interpBottom = smoothstep(dotBL, dotBR, localX);
        float interpTop = smoothstep(dotTL, dotTR, localX);

        return smoothstep(interpBottom, interpTop, localY);
    }

  private:
};

} // namespace my
