#pragma once

#include "ModelData.h"

struct ModelReplacer
{
    static void createPendingModels();

    static void processFhlMaterials(InstanceInfoEx& instanceInfoEx, const MaterialMap& materialMap);

    static void init();
};