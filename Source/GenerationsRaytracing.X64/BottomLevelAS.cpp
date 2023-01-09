#include "BottomLevelAS.h"
#include "DescriptorTableAllocator.h"

BottomLevelAS::Geometry::Geometry(DescriptorTableAllocator& allocator): allocator(allocator)
{
}

BottomLevelAS::Geometry::~Geometry()
{
    releaseDescriptors();
}

void BottomLevelAS::Geometry::releaseDescriptors()
{
    if (skinningBufferId != 0)
        allocator.erase(skinningBufferId);

    if (prevSkinningBufferId != 0)
        allocator.erase(prevSkinningBufferId);

    skinningBufferId = NULL;
    prevSkinningBufferId = NULL;
}
