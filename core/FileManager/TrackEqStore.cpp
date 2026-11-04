#include "TrackEqStore.h"

#include "Platform.h"
#include "fileManager.h"
#include "fileManagerDefs.h"
#include "mtAudioEngine.h"
#include "mtStructs.h"

#include <string.h>

namespace
{
constexpr uint32_t kHeader = 8;
constexpr uint32_t kBandBytes = 8;
constexpr uint32_t kBodyBytes =
  kHeader + trackeq::kEqTracks * trackeq::kEqBands * kBandBytes;
static_assert(kBodyBytes + 4 == TrackEqStore::kFileSize, "track EQ file layout");

uint32_t crc32(const uint8_t* data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (int k = 0; k < 8; ++k)
        {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

void wr16(uint8_t* p, uint16_t v)
{
    p[0] = uint8_t(v);
    p[1] = uint8_t(v >> 8);
}
uint16_t rd16(const uint8_t* p)
{
    return uint16_t(p[0] | (p[1] << 8));
}
} // namespace

void TrackEqStore::init(FileSystem* fileSystem)
{
    this->fileSystem = fileSystem;
}

uint32_t TrackEqStore::encode(const trackeq::TrackEqSet& set, uint8_t* out)
{
    out[0] = 'O';
    out[1] = 'T';
    out[2] = 'E';
    out[3] = 'Q';
    out[4] = 1; // version
    out[5] = trackeq::kEqTracks;
    out[6] = trackeq::kEqBands;
    out[7] = 0;
    uint8_t* p = out + kHeader;
    for (int t = 0; t < trackeq::kEqTracks; ++t)
    {
        for (int i = 0; i < trackeq::kEqBands; ++i)
        {
            trackeq::EqBand b = set.track[t].band[i];
            trackeq::eqSanitize(b);
            if (!trackeq::eqSlopeAllowed(i, b.type))
            {
                b.slope = 0;
            }
            p[0] = b.type;
            p[1] = uint8_t((b.on ? 1 : 0) | (b.slope ? 2 : 0));
            wr16(p + 2, b.freqHz);
            wr16(p + 4, uint16_t(b.gainDb10));
            wr16(p + 6, b.q100);
            p += kBandBytes;
        }
    }
    const uint32_t crc = crc32(out, kBodyBytes);
    out[kBodyBytes] = uint8_t(crc);
    out[kBodyBytes + 1] = uint8_t(crc >> 8);
    out[kBodyBytes + 2] = uint8_t(crc >> 16);
    out[kBodyBytes + 3] = uint8_t(crc >> 24);
    return kFileSize;
}

bool TrackEqStore::decode(const uint8_t* data, uint32_t len, trackeq::TrackEqSet& set)
{
    if (len < kFileSize || memcmp(data, "OTEQ", 4) != 0)
    {
        return false;
    }
    // Version 1: 8 tracks, 4 bands. Files with more tracks (a later version) are not read here.
    if (data[4] != 1 || data[5] != trackeq::kEqTracks || data[6] != trackeq::kEqBands)
    {
        return false;
    }
    const uint32_t crc = uint32_t(data[kBodyBytes]) | (uint32_t(data[kBodyBytes + 1]) << 8) |
                         (uint32_t(data[kBodyBytes + 2]) << 16) |
                         (uint32_t(data[kBodyBytes + 3]) << 24);
    if (crc != crc32(data, kBodyBytes))
    {
        return false;
    }
    trackeq::TrackEqSet r;
    const uint8_t* p = data + kHeader;
    for (int t = 0; t < trackeq::kEqTracks; ++t)
    {
        for (int i = 0; i < trackeq::kEqBands; ++i)
        {
            trackeq::EqBand& b = r.track[t].band[i];
            b.type = p[0];
            b.on = (p[1] & 1) != 0;
            b.slope = (p[1] & 2) ? 1 : 0;
            b.freqHz = rd16(p + 2);
            b.gainDb10 = int16_t(rd16(p + 4));
            b.q100 = rd16(p + 6);
            trackeq::eqSanitize(b);
            if (!trackeq::eqSlopeAllowed(i, b.type))
            {
                b.slope = 0;
            }
            p += kBandBytes;
        }
    }
    set = r;
    return true;
}

bool TrackEqStore::isDefault(const trackeq::TrackEqSet& set)
{
    uint8_t a[kFileSize], b[kFileSize];
    encode(set, a);
    encode(trackeq::TrackEqSet(), b);
    return memcmp(a, b, kFileSize) == 0;
}

bool TrackEqStore::readFile(const char* path, trackeq::TrackEqSet& set)
{
    if (!fileSystem || !fileSystem->exists(path))
    {
        return false;
    }
    auto info = fileSystem->openFile(path);
    if (!std::get<0>(info))
    {
        return false;
    }
    auto file = std::get<1>(info);
    uint8_t buf[kFileSize];
    const size_t got = fileSystem->readFile(buf, sizeof(buf), file);
    fileSystem->closeFile(file);
    return decode(buf, uint32_t(got), set);
}

bool TrackEqStore::writeFile(const char* path, const trackeq::TrackEqSet& set)
{
    uint8_t buf[kFileSize];
    const uint32_t n = encode(set, buf);
    auto info = fileSystem->openFile(path, FileSystem::FileOpenMode::kWriteOpen);
    if (!std::get<0>(info))
    {
        return false;
    }
    auto file = std::get<1>(info);
    const size_t put = fileSystem->writeFile(buf, n, file);
    fileSystem->closeFile(file);
    return put == n;
}

void TrackEqStore::loadFromWorkspace()
{
    trackeq::TrackEqSet set;
    if (!readFile(cWorkspaceTrackEqFilePath, set))
    {
        set = trackeq::TrackEqSet();
    }
    engine.setTrackEqSet(set);
}

void TrackEqStore::saveToWorkspace()
{
    if (!fileSystem)
    {
        return;
    }
    const trackeq::TrackEqSet& set = engine.getTrackEqSet();
    if (isDefault(set))
    {
        // Back at the factory layout: no file at all.
        if (fileSystem->exists(cWorkspaceTrackEqFilePath))
        {
            fileSystem->remove(cWorkspaceTrackEqFilePath);
        }
    }
    else
    {
        newFileManager.createWorkspaceDirs();
        writeFile(cWorkspaceTrackEqFilePath, set);
    }
    mtProject.values.projectNotSavedFlag |= 1;
    newFileManager.setProjectStructChanged();
}

bool TrackEqStore::copyFile(const char* src, const char* dest)
{
    if (!fileSystem)
    {
        return false;
    }
    trackeq::TrackEqSet set;
    if (!readFile(src, set))
    {
        if (fileSystem->exists(dest))
        {
            fileSystem->remove(dest);
        }
        return true;
    }
    return writeFile(dest, set);
}
