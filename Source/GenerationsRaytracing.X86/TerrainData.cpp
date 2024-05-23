#include "TerrainData.h"

#include "LightData.h"
#include "StageSelection.h"
#include "Configuration.h"

HOOK(void, __cdecl, MakeTerrainData, 0x7346F0,
     const Hedgehog::Base::CSharedString& name,
     const void* data,
     uint32_t dataSize,
     const boost::shared_ptr<Hedgehog::Database::CDatabase>& database,
     Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    const XXH32_hash_t hash = XXH32(data, dataSize, 0);

    StageSelection::update(hash);
    LightData::update(hash);

    originalMakeTerrainData(name, data, dataSize, database, renderingInfrastructure);
}

void TerrainData::init()
{
    if (Configuration::s_enableRaytracing)
        INSTALL_HOOK(MakeTerrainData);
}