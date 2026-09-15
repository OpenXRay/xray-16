#include "GameDataLayout.h"

#include <algorithm>
#include <initializer_list>
#include <system_error>
#include <vector>

namespace xray::macos
{
namespace
{
namespace fs = std::filesystem;

bool IsWithin(const fs::path& child, const fs::path& parent)
{
    return std::mismatch(parent.begin(), parent.end(), child.begin(), child.end()).first == parent.end();
}

void EnsureRuntimeDirectory(const fs::path& path)
{
    const auto status = fs::symlink_status(path);
    if (fs::is_symlink(status) || (fs::exists(status) && !fs::is_directory(status)))
        throw fs::filesystem_error("Runtime directory is not a regular directory", path, std::make_error_code(std::errc::file_exists));

    fs::create_directories(path);
}

void MoveRuntimeEntryAside(const fs::path& path)
{
    for (unsigned int i = 0; i < 100; ++i)
    {
        fs::path backup = path.string() + ".openxray-backup";
        if (i != 0)
            backup += std::to_string(i);
        if (fs::exists(fs::symlink_status(backup)))
            continue;

        fs::rename(path, backup);
        return;
    }

    throw fs::filesystem_error("Cannot back up runtime entry", path, std::make_error_code(std::errc::file_exists));
}

void LinkRuntimeEntry(const fs::path& source, const fs::path& destination)
{
    const auto status = fs::symlink_status(destination);
    if (fs::is_symlink(status))
    {
        if (fs::read_symlink(destination) == source)
            return;
        fs::remove(destination);
    }
    else if (fs::exists(status))
        MoveRuntimeEntryAside(destination);

    fs::create_symlink(source, destination);
}

void SyncDirectoryLinks(const fs::path& source, const fs::path& destination, std::initializer_list<fs::path> excluded)
{
    const auto include = [&](const fs::path& path)
    {
        return std::find(excluded.begin(), excluded.end(), path.filename()) == excluded.end();
    };

    // Remove links to files no longer in this installation, including when switching games or mods.
    // Real files written into the runtime directory (such as saves) are retained.
    std::vector<fs::path> staleLinks;
    for (const auto& entry : fs::directory_iterator(destination))
    {
        if (include(entry.path()) && fs::is_symlink(entry.symlink_status()) && !fs::exists(fs::symlink_status(source / entry.path().filename())))
            staleLinks.push_back(entry.path());
    }
    for (const auto& path : staleLinks)
        fs::remove(path);

    if (!fs::is_directory(source))
    {
        if (fs::exists(source))
            throw fs::filesystem_error("Game resource path is not a directory", source, std::make_error_code(std::errc::not_a_directory));
        return;
    }

    for (const auto& entry : fs::directory_iterator(source))
    {
        if (include(entry.path()))
            LinkRuntimeEntry(entry.path(), destination / entry.path().filename());
    }
}

void PrepareGamedata(const fs::path& gameRoot, const fs::path& bundleResourcesRoot, const fs::path& runtimeRoot)
{
    const fs::path source = gameRoot / "gamedata";
    const fs::path destination = runtimeRoot / "gamedata";
    EnsureRuntimeDirectory(destination);
    SyncDirectoryLinks(source, destination, { "shaders" });

    // These two directories must be real runtime directories: creating a fallback beneath a
    // symlink to the installation would write into the installation itself.
    EnsureRuntimeDirectory(destination / "shaders");
    SyncDirectoryLinks(source / "shaders", destination / "shaders", { "gl" });

    const fs::path gameShaders = source / "shaders/gl";
    if (fs::exists(gameShaders) && !fs::is_directory(gameShaders))
        throw fs::filesystem_error("OpenGL shader path is not a directory", gameShaders, std::make_error_code(std::errc::not_a_directory));
    const fs::path shaders = fs::is_directory(gameShaders) ? gameShaders : bundleResourcesRoot / "gamedata/shaders/gl";
    if (!fs::is_directory(shaders))
        throw fs::filesystem_error("Cannot find OpenGL shaders", shaders, std::make_error_code(std::errc::no_such_file_or_directory));

    LinkRuntimeEntry(shaders, destination / "shaders/gl");
}
} // namespace

bool PrepareGameDataLayout(const GameDataLayoutPaths& paths, std::string& error)
{
    error.clear();
    try
    {
        const fs::path gameRoot = fs::canonical(paths.gameRoot);
        const fs::path bundleResourcesRoot = fs::canonical(paths.bundleResourcesRoot);
        const fs::path runtimeRoot = fs::weakly_canonical(paths.runtimeRoot);
        if (IsWithin(runtimeRoot, gameRoot) || IsWithin(gameRoot, runtimeRoot))
            throw fs::filesystem_error("Runtime directory overlaps the game installation", runtimeRoot, gameRoot,
                std::make_error_code(std::errc::invalid_argument));

        for (const auto& directory : { "levels", "resources", "localization" })
        {
            if (!fs::is_directory(gameRoot / directory))
                throw fs::filesystem_error("Missing game resource directory", gameRoot / directory, std::make_error_code(std::errc::no_such_file_or_directory));
        }

        const fs::path gameFsLtx = gameRoot / "fsgame.ltx";
        const fs::path fsLtx = fs::exists(fs::symlink_status(gameFsLtx)) ? gameFsLtx : bundleResourcesRoot / "fsgame.ltx";
        if (!fs::is_regular_file(fsLtx))
            throw fs::filesystem_error("Cannot read filesystem configuration", fsLtx, std::make_error_code(std::errc::no_such_file_or_directory));

        EnsureRuntimeDirectory(paths.runtimeRoot);
        SyncDirectoryLinks(gameRoot, runtimeRoot, { "gamedata", "fsgame.ltx", "_appdata_" });
        PrepareGamedata(gameRoot, bundleResourcesRoot, runtimeRoot);
        LinkRuntimeEntry(fsLtx, runtimeRoot / "fsgame.ltx");

        // Keep the existing Application Support saves and settings when migrating an older bundle.
        fs::create_directories(paths.appDataRoot);
        LinkRuntimeEntry(fs::canonical(paths.appDataRoot), runtimeRoot / "_appdata_");
        return true;
    }
    catch (const fs::filesystem_error& failure)
    {
        error = failure.what();
        return false;
    }
}
} // namespace xray::macos
