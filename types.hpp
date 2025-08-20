#pragma once

struct vec {
    float x;
    float y;

    vec(float x_=0, float y_=0) : x(x_), y(y_) {}

    vec operator+(const vec& other) const { return vec(x + other.x, y + other.y); }
    vec operator-(const vec& other) const { return vec(x - other.x, y - other.y); }
    vec operator*(float s) const { return vec(x * s, y * s); }
};