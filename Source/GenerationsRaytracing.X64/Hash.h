#pragma once

template<typename T>
XXH64_hash_t computeHash(const T& value, XXH64_hash_t seed)
{
    return XXH64(&value, sizeof(T), seed);
}

template<typename T>
XXH64_hash_t computeHash(const T* value, const size_t size, XXH64_hash_t seed)
{
    return XXH64(value, sizeof(T) * size, seed);
}

