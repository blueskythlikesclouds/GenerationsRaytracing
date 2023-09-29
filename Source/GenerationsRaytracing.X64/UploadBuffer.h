#pragma once

inline constexpr uint32_t UPLOAD_BUFFER_SIZE = 16 * 1024 * 1024;

struct UploadBuffer
{
    ComPtr<D3D12MA::Allocation> allocation;
    uint8_t* memory = nullptr;
};