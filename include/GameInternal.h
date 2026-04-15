#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>

#include "Vec2.h"

namespace game_internal {

constexpr float PI = 3.14159265358979323846f;
constexpr float EPSILON = 1e-6f;

constexpr float TEAM_A_SHOT_ASSIST_X = 9.5f;
constexpr float TEAM_A_PASS_ASSIST_X = 3.0f;

inline std::string trim(const std::string& text) {
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }

    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(begin, end - begin);
}

inline float parseFloatOrDefault(const std::string& value, float defaultValue) {
    const char* begin = value.c_str();
    char* end = nullptr;
    const float parsed = static_cast<float>(std::strtof(begin, &end));
    if (end == begin) {
        return defaultValue;
    }
    return parsed;
}

inline Vec2 normalizedOrFallback(const Vec2& value, const Vec2& fallback) {
    if (value.lengthSquared() < EPSILON) {
        return fallback;
    }
    return value.normalized();
}

inline float hash01(int a, int b) {
    const float v = std::sin(static_cast<float>(a * 127 + b * 311) * 0.131f) * 43758.5453f;
    return v - std::floor(v);
}

inline float degreesToRadians(float degrees) {
    return degrees * PI / 180.0f;
}

inline float dot2(const Vec2& a, const Vec2& b) {
    return a.x * b.x + a.y * b.y;
}

}  // namespace game_internal
