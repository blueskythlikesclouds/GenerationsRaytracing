#pragma once

struct StageSelection
{
    static inline std::atomic<size_t*> s_stageIndex;
    static inline std::atomic<bool*> s_rememberSelection;

    static void update(XXH32_hash_t hash);
    static void init(ModInfo_t* modInfo);
};