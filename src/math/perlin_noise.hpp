#pragma once

#include <glm/common.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>

namespace my {

class PerlinGenerator {
  public:
    // Returns a pseudo-random unit gradient vector for grid point (ix, iy)
    static glm::vec2 randomGradient(int ix, int iy, unsigned seed) {
        const unsigned BITS = 8 * sizeof(unsigned);
        const unsigned HALF_BITS = BITS / 2;

        // Hash the grid coordinates into a pseudo-random angle
        unsigned hashX = ix ^ seed, hashY = iy;
        hashX *= 3284157443;
        hashY ^= hashX << HALF_BITS | hashX >> BITS - HALF_BITS;
        hashY *= 1911520717;
        hashX ^= hashY << HALF_BITS | hashY >> BITS - HALF_BITS;
        hashX *= 2048419325;

        float angle = hashX * (3.14159265f / ~(~0u >> 1));

        return {sin(angle), cos(angle)};
    }

    static float dotGridGradient(int ix, int iy, float x, float y, unsigned seed) {
        glm::vec2 gradient = randomGradient(ix, iy, seed);
        float distanceX = x - (float)ix;
        float distanceY = y - (float)iy;
        return (distanceX * gradient.x + distanceY * gradient.y);
    }

    static float smoothstep(float from, float to, float weight) {
        float w = weight * weight * weight * (weight * (weight * 6.0f - 15.0f) + 10.0f);
        return from + (to - from) * w;
    }

    static float perlin(float x, float y, unsigned seed) {
        int x0 = (int)glm::floor(x), x1 = x0 + 1;
        int y0 = (int)glm::floor(y), y1 = y0 + 1;

        float localX = x - float(x0);
        float localY = y - float(y0);

        // Dot products at the four corners
        float dotBL = dotGridGradient(x0, y0, x, y, seed);
        float dotBR = dotGridGradient(x1, y0, x, y, seed);
        float dotTL = dotGridGradient(x0, y1, x, y, seed);
        float dotTR = dotGridGradient(x1, y1, x, y, seed);

        // Interpolate along x for bottom and top rows, then along y
        float interpBottom = smoothstep(dotBL, dotBR, localX);
        float interpTop = smoothstep(dotTL, dotTR, localX);

        return smoothstep(interpBottom, interpTop, localY);
    }

    static void populateNoise(int OCTAVES, int NOISE_SCALE, int noiseWidth, int noiseHeight,
                              float ROTATION_ANGLE, std::vector<uint8_t> &noisePixels,
                              std::vector<float> &heightMap, int seed, float lacunarity = 2.0f,
                              float persistence = 0.45f) {
        for (int x = 0; x < noiseWidth; x++) {
            for (int y = 0; y < noiseHeight; y++) {
                float height = 0, frequency = 1, amplitude = 1, angle = 0.0f;

                for (int octave = 0; octave < OCTAVES; octave++) {
                    float cosA = glm::cos(angle), sinA = glm::sin(angle);
                    float sx = (x * cosA - y * sinA) * frequency / NOISE_SCALE;
                    float sy = (x * sinA + y * cosA) * frequency / NOISE_SCALE;
                    float n = PerlinGenerator::perlin(sx, sy, seed);

                    height += n * amplitude;

                    frequency *= lacunarity;
                    amplitude *= persistence;
                    angle += ROTATION_ANGLE;
                }

                height = glm::clamp(height, -1.0f, 1.0f);

                heightMap[y * noiseWidth + x] = height;

                int index = (y * noiseWidth + x) * 4;
                int grayscale = (int)(((height + 1.0f) * 0.5f) * 255);
                noisePixels[index] = noisePixels[index + 1] = noisePixels[index + 2] = grayscale;
                noisePixels[index + 3] = 255;
            }
        }
    }

  private:
};

} // namespace my
