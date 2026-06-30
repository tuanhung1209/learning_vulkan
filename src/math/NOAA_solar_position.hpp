#pragma once

#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace my {

struct SolarResult {
    float zenith{};  // radians, 0 = overhead, π/2 = horizon, π = nadir
    float azimuth{}; // radians, 0 = North, π/2 = East, π = South, 3π/2 = West
};

class SolarPosition {
  public:
    static SolarResult calculate(float latitudeDeg, float longitudeDeg, int year, int month, int day,
                                 int hour, int minute, float second) {
        constexpr double PI = 3.14159265358979323846;
        constexpr double DEG2RAD = PI / 180.0;
        constexpr double RAD2DEG = 180.0 / PI;

        auto floorMod = [](double x, double y) { return x - std::floor(x / y) * y; };

        double latRad = latitudeDeg * DEG2RAD;

        // Julian Date calculation using double precision so sub-day changes are preserved.
        int y = year;
        int m = month;
        if (m <= 2) {
            y -= 1;
            m += 12;
        }
        int A = y / 100;
        int B = 2 - A + A / 4;
        double JD = std::floor(365.25 * (y + 4716)) + std::floor(30.6001 * (m + 1)) + day + B - 1524.5;
        JD += (static_cast<double>(hour) + static_cast<double>(minute) / 60.0 +
               static_cast<double>(second) / 3600.0) /
              24.0;

        double JC = (JD - 2451545.0) / 36525.0;

        double geomMeanLongDeg =
            floorMod(280.46646 + JC * (36000.76983 + JC * 0.0003032), 360.0);
        double geomMeanAnomDeg =
            floorMod(357.52911 + JC * (35999.05029 - 0.0001537 * JC), 360.0);

        double ecc = 0.016708634 - JC * (0.000042037 + 0.0000001267 * JC);

        double geomMeanAnomRad = geomMeanAnomDeg * DEG2RAD;
        double sinA = std::sin(geomMeanAnomRad);
        double sin2A = std::sin(2.0 * geomMeanAnomRad);
        double sin3A = std::sin(3.0 * geomMeanAnomRad);

        double eqCenter = sinA * (1.914602 - JC * (0.004817 + 0.000014 * JC)) +
                          sin2A * (0.019993 - 0.000101 * JC) + sin3A * 0.000289;

        double trueLongDeg = geomMeanLongDeg + eqCenter;
        double trueAnomDeg = geomMeanAnomDeg + eqCenter;

        double omegaDeg = 125.04 - 1934.136 * JC;
        double appLongDeg = trueLongDeg - 0.00569 - 0.00478 * std::sin(omegaDeg * DEG2RAD);

        double meanObliqDeg = 23.0 + 26.0 / 60.0 + 21.448 / 3600.0 -
                              JC * (46.815 + JC * (0.00059 - 0.001813 * JC)) / 3600.0;
        double obliqDeg = meanObliqDeg + 0.00256 * std::cos(omegaDeg * DEG2RAD);

        double obliqRad = obliqDeg * DEG2RAD;
        double appLongRad = appLongDeg * DEG2RAD;

        double declinRad = std::asin(std::sin(obliqRad) * std::sin(appLongRad));
        double raRad = std::atan2(std::cos(obliqRad) * std::sin(appLongRad), std::cos(appLongRad));
        double raDeg = floorMod(raRad * RAD2DEG, 360.0);

        double GMST = floorMod(280.46061837 + 360.98564736629 * (JD - 2451545.0) +
                                   0.000387933 * JC * JC - JC * JC * JC / 38710000.0,
                               360.0);

        double LST = floorMod(GMST + longitudeDeg, 360.0);

        double haDeg = floorMod(LST - raDeg + 180.0, 360.0) - 180.0;
        double haRad = haDeg * DEG2RAD;

        double cosZenith =
            std::sin(latRad) * std::sin(declinRad) +
            std::cos(latRad) * std::cos(declinRad) * std::cos(haRad);
        cosZenith = std::clamp(cosZenith, -1.0, 1.0);
        double zenith = std::acos(cosZenith);

        double azRad = std::atan2(-std::sin(haRad) * std::cos(declinRad),
                                  std::sin(declinRad) * std::cos(latRad) -
                                      std::cos(declinRad) * std::sin(latRad) * std::cos(haRad));
        if (azRad < 0.0)
            azRad += 2.0 * PI;

        return {static_cast<float>(zenith), static_cast<float>(azRad)};
    }

    static glm::vec3 toDirection(const SolarResult &solar) {
        float elev = glm::pi<float>() / 2.0f - solar.zenith;
        float x = glm::cos(elev) * glm::sin(solar.azimuth);
        float y = glm::sin(elev);
        float z = glm::cos(elev) * glm::cos(solar.azimuth);
        return glm::vec3(x, y, z);
    }
};

} // namespace my
