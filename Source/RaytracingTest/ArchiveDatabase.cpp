#include "ArchiveDatabase.h"
#include "Path.h"

static FNALLOC(fdiAlloc)
{
    return operator new(cb);
}

static FNFREE(fdiFree)
{
    operator delete(pv);
}

static FNOPEN(fdiOpen)
{
    hl::stream* stream;
    sscanf(pszFile, "%p", &stream);

    return (INT_PTR)stream;
}

static FNREAD(fdiRead)
{
    return (UINT)((hl::stream*)hf)->read(cb, pv);
}

static FNWRITE(fdiWrite)
{
    return (UINT)((hl::stream*)hf)->write(cb, pv);
}

static FNCLOSE(fdiClose)
{
    return 0;
}

static FNSEEK(fdiSeek)
{
    hl::stream* stream = (hl::stream*)hf;
    stream->seek((hl::seek_mode)seektype, dist);
    return (long)stream->tell();
}

static FNFDINOTIFY(fdiNotify)
{
    return fdint == fdintCOPY_FILE ? (INT_PTR)pfdin->pv : 0;
}

static void loadArchiveDatabase(hl::archive& archive, void* data, size_t dataSize)
{
    auto header = (hl::hh::ar::header*)data;

    header->fix(dataSize);
    header->parse(dataSize, archive);
}

static void loadCabinetArchiveDatabase(hl::archive& archive, void* data, size_t dataSize)
{
    if (*(uint32_t*)data != 'FCSM')
    {
        loadArchiveDatabase(archive, data, dataSize);
        return;
    }

    hl::readonly_mem_stream source(data, dataSize);
    hl::mem_stream destination;

    char cabinet[1]{};
    char cabPath[24]{};

    sprintf(cabPath, "%p", &source);

    ERF erf;

    HFDI fdi = FDICreate(
        fdiAlloc,
        fdiFree,
        fdiOpen,
        fdiRead,
        fdiWrite,
        fdiClose,
        fdiSeek,
        cpuUNKNOWN,
        &erf);

    FDICopy(fdi, cabinet, cabPath, 0, fdiNotify, nullptr, &destination);
    FDIDestroy(fdi);

    loadArchiveDatabase(archive, destination.get_data_ptr(), destination.get_size());
}

hl::archive ArchiveDatabase::load(void* data, size_t dataSize)
{
    hl::archive archive;
    loadCabinetArchiveDatabase(archive, data, dataSize);
    return archive;
}

hl::archive ArchiveDatabase::load(const std::string& filePath)
{
    const auto myFilePath = toNchar(filePath);
    const hl::nchar* ext = hl::path::get_ext(myFilePath.data());

    hl::archive archive;

    if (hl::path::ext_is_split(ext))
    {
        hl::nstring splitPathBuf(myFilePath.data());

        for (const auto splitPath : hl::path::split_iterator2<>(splitPathBuf))
        {
            if (!hl::path::exists(splitPath))
                break;

            hl::blob blob(splitPath);
            loadCabinetArchiveDatabase(archive, blob.data(), blob.size());
        }
    }

    else
    {
        hl::blob blob(myFilePath.data());
        loadCabinetArchiveDatabase(archive, blob.data(), blob.size());
    }

    return archive;
}
