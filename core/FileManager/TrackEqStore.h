#ifndef FILEMANAGER_TRACKEQSTORE_H_
#define FILEMANAGER_TRACKEQSTORE_H_

#include "track_eq.h"
#include <stddef.h>
#include <stdint.h>

class FileSystem;

// Saves the per-track EQ settings in a small sidecar file ("track_eq.ote") next to project.mt,
// in the Workspace and in the project folder. project.mt itself is not changed. No file means
// the EQ is off, and a project that never used the EQ gets no file.
class TrackEqStore
{
  public:
    static constexpr uint32_t kFileSize = 268;

    void init(FileSystem* fileSystem);

    // Workspace -> audio engine (no file or a damaged file = every EQ off).
    void loadFromWorkspace();
    // Audio engine -> Workspace. Called after the EQ was edited.
    void saveToWorkspace();
    // Copies the sidecar between the Workspace and a project folder. A missing source
    // removes the destination, so no stale EQ survives.
    bool copyFile(const char* src, const char* dest);

    // Pure file format helpers (also used for unit checks).
    static uint32_t encode(const trackeq::TrackEqSet& set, uint8_t* out);
    static bool decode(const uint8_t* data, uint32_t len, trackeq::TrackEqSet& set);

  private:
    bool readFile(const char* path, trackeq::TrackEqSet& set);
    bool writeFile(const char* path, const trackeq::TrackEqSet& set);
    static bool isDefault(const trackeq::TrackEqSet& set);

    FileSystem* fileSystem = nullptr;
};

#endif /* FILEMANAGER_TRACKEQSTORE_H_ */
