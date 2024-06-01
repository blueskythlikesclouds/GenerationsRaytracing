#pragma once

enum class QualityMode : uint32_t
{
    Unspecified,
    Auto,
    Native,
    Quality,
    Balanced,
    Performance,
    UltraPerformance
};

inline QualityMode getAutoQualityMode(uint32_t height)
{
    if (height <= 1080)
        return QualityMode::Quality;

    else if (height < 2160)
        return QualityMode::Balanced;

    else if (height < 4320)
        return QualityMode::Performance;

    else
        return QualityMode::UltraPerformance;
}

inline uint32_t getRenderResolutionFromQualityMode(QualityMode qualityMode, uint32_t resolution)
{
    switch (qualityMode)
    {
    case QualityMode::Quality:
        return (resolution * 10) / 15;

    case QualityMode::Balanced:
        return (resolution * 10) / 17;

    case QualityMode::Performance:
        return resolution / 2;

    case QualityMode::UltraPerformance:
        return resolution / 3;
    }

    return resolution;
}