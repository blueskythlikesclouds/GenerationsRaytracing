#pragma once

struct DescriptorTableAllocator
{
    std::vector<size_t> descriptorAllocations;
    ankerl::unordered_dense::map<size_t, uint32_t> descriptorData;
    size_t searchPosition = 0;

    uint32_t find(size_t data) const;
    uint32_t getCapacity() const;

    bool put(size_t data, uint32_t& index);
    void erase(size_t data);

    void clear();
};