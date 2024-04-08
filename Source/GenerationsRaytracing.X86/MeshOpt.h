#pragma once

// https://github.com/zeux/meshoptimizer/blob/master/src/meshoptimizer.h
inline int quantizeUnorm(float v, int N)
{
    const float scale = float((1 << N) - 1);

    v = (v >= 0) ? v : 0;
    v = (v <= 1) ? v : 1;

    return int(v * scale + 0.5f);
}

inline int quantizeSnorm(float v, int N)
{
    const float scale = float((1 << (N - 1)) - 1);

    float round = (v >= 0 ? 0.5f : -0.5f);

    v = (v >= -1) ? v : -1;
    v = (v <= +1) ? v : +1;

    return int(v * scale + round);
}

// https://github.com/zeux/meshoptimizer/blob/master/src/quantization.cpp
inline unsigned short quantizeHalf(float v)
{
    union { float f; unsigned int ui; } u = { v };
    unsigned int ui = u.ui;

    int s = (ui >> 16) & 0x8000;
    int em = ui & 0x7fffffff;

    // bias exponent and round to nearest; 112 is relative exponent bias (127-15)
    int h = (em - (112 << 23) + (1 << 12)) >> 13;

    // underflow: flush to zero; 113 encodes exponent -14
    h = (em < (113 << 23)) ? 0 : h;

    // overflow: infinity; 143 encodes exponent 16
    h = (em >= (143 << 23)) ? 0x7c00 : h;

    // NaN; note that we convert all types of NaN to qNaN
    h = (em > (255 << 23)) ? 0x7e00 : h;

    return static_cast<unsigned short>(s | h);
}

template<typename T>
inline uint32_t quantizeUnorm10(const T& value)
{
    return (quantizeUnorm(value.x(), 10) & 0x3FF) | ((quantizeUnorm(value.y(), 10) & 0x3FF) << 10) | ((quantizeUnorm(value.z(), 10) & 0x3FF) << 20);
}

template<typename T>
inline uint32_t quantizeSnorm10(const T& value)
{
    const T s_coeffs(0.5f, 0.5f, 0.5f);
    return quantizeUnorm10(value.cwiseProduct(s_coeffs) + s_coeffs);
}