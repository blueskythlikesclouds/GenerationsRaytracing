#include "CpkBinder.h"

#include "Path.h"

static std::string cpkFilePath;

HOOK(void, __fastcall, CFileBinderCriBindCpk, 0x66AAC0, void* This, void* Edx, const hh::base::CSharedString& filePath, int32_t priority)
{
    if (!cpkFilePath.empty())
    {
        originalCFileBinderCriBindCpk(This, Edx, cpkFilePath.c_str(), 20221217);

        cpkFilePath.clear();
        cpkFilePath.shrink_to_fit();
    }

    originalCFileBinderCriBindCpk(This, Edx, filePath, priority);
}

void CpkBinder::init()
{
    if (!std::filesystem::exists("mod.cpk"))
        return;

    WCHAR fullPath[1024];
    GetFullPathName(TEXT("mod.cpk"), _countof(fullPath), fullPath, nullptr);
    cpkFilePath = wideCharToMultiByte(fullPath);

    INSTALL_HOOK(CFileBinderCriBindCpk);
}
