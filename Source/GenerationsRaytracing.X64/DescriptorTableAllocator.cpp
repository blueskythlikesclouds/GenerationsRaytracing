#include "DescriptorTableAllocator.h"

uint32_t DescriptorTableAllocator::find(size_t data) const
{
    const auto pair = descriptorData.find(data);
    return pair != descriptorData.end() ? pair->second : 0;
}

uint32_t DescriptorTableAllocator::getCapacity() const
{
    return (uint32_t)(descriptorAllocations.size() * 64);
}

bool DescriptorTableAllocator::put(size_t data, uint32_t& index)
{
    const auto pair = descriptorData.find(data);
    if (pair != descriptorData.end())
    {
        index = pair->second;
        return false;
    }

    for (size_t i = searchPosition; i < descriptorAllocations.size(); i++)
    {
        if (descriptorAllocations[i] == 0 || !_BitScanForward64((DWORD*)&index, descriptorAllocations[i]))
            continue;

        descriptorAllocations[i] &= ~(1ull << index);
        index += (uint32_t)(i * 64);
        descriptorData.emplace(data, index);
        searchPosition = i;

        return true;
    }

    index = getCapacity();
    descriptorData.emplace(data, index);
    searchPosition = descriptorAllocations.size();
    descriptorAllocations.push_back(~(size_t)1);

    return true;
}

void DescriptorTableAllocator::erase(size_t data)
{
    const auto pair = descriptorData.find(data);
    if (pair == descriptorData.end())
        return;

    size_t index = pair->second / 64;
    descriptorAllocations[index] |= 1ull << (pair->second % 64);
    descriptorData.erase(pair);

    if (searchPosition > index)
        searchPosition = index;
}

void DescriptorTableAllocator::clear()
{
    descriptorAllocations.clear();
    descriptorData.clear();
    searchPosition = 0;
}
