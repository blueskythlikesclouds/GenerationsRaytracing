#include "Model.h"
#include "DescriptorTableAllocator.h"

Mesh::Mesh(DescriptorTableAllocator& allocator): allocator(allocator)
{
}

Mesh::~Mesh()
{
    releaseDescriptors();
}

void Mesh::releaseDescriptors()
{
    if (skinningBufferId != 0)
        allocator.erase(skinningBufferId);

    if (prevSkinningBufferId != 0)
        allocator.erase(prevSkinningBufferId);

    skinningBufferId = NULL;
    prevSkinningBufferId = NULL;
}
