#pragma once

/// <summary>
/// 2次元ベクトル
/// </summary>
struct Vector2 final {
    float x;
    float y;

    // 加算演算子
    Vector2 operator+(const Vector2& other) const {
        return Vector2{ x + other.x, y + other.y };
    }

    // 減算演算子
    Vector2 operator-(const Vector2& other) const {
        return Vector2{ x - other.x, y - other.y };
    }

    // 乗算演算子（スカラーとの乗算）
    Vector2 operator*(float scalar) const {
        return Vector2{ x * scalar, y * scalar };
    }

    // 除算演算子（スカラーとの除算）
    Vector2 operator/(float scalar) const {
        return Vector2{ x / scalar, y / scalar };
    }

    // 加算代入演算子
    Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    // 減算代入演算子
    Vector2& operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    // 乗算代入演算子（スカラーとの乗算）
    Vector2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    // 除算代入演算子（スカラーとの除算）
    Vector2& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }
};