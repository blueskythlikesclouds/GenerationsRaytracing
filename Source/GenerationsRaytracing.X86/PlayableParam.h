#pragma once

class TerrainInstanceInfoDataEx;

struct PlayableParam
{
    static void trackInstance(TerrainInstanceInfoDataEx* terrainInstanceInfoDataEx);
    static void removeInstance(TerrainInstanceInfoDataEx* terrainInstanceInfoDataEx);

    static void updatePlayableParams(Hedgehog::Mirage::CRenderingDevice* renderingDevice);
};