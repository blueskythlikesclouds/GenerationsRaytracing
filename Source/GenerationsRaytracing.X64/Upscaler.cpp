#include "Upscaler.h"

Upscaler::~Upscaler() = default;

uint32_t Upscaler::getWidth() const
{
    return m_width;
}

uint32_t Upscaler::getHeight() const
{
    return m_height;
}
