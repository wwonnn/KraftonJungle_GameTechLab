#pragma once

#include <cmath>

struct FVector
{
    float x, y, z;
    FVector(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

    float Length() const
    {
        return sqrtf(x * x + y * y + z * z);
    }

    FVector Normalize() const
    {
        float len = Length();
        if (len > 0.0001f)
            return FVector(x / len, y / len, z / len);
        return FVector(0, 0, 0);
    }

    float Dot(const FVector& other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }

    FVector operator-(const FVector& other) const
    {
        return FVector(x - other.x, y - other.y, z - other.z);
    }

    FVector operator+(const FVector& other) const
    {
        return FVector(x + other.x, y + other.y, z + other.z);
    }

    FVector operator*(float scalar) const
    {
        return FVector(x * scalar, y * scalar, z * scalar);
    }

    FVector operator/(float scalar) const
    {
        return FVector(x / scalar, y / scalar, z / scalar);
    }

    float LengthSquared() const
    {
        return x * x + y * y + z * z;
    }
};
