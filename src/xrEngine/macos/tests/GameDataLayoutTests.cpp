#include "../GameDataLayout.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <unistd.h>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using xray::macos::GameDataLayoutPaths;
using xray::macos::PrepareGameDataLayout;

void Check(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void Write(const fs::path& path, const std::string& contents)
{
    fs::create_directories(path.parent_path());
    std::ofstream file(path);
    file << contents;
    Check(file.good(), "Cannot write fixture: " + path.string());
}

std::string Read(const fs::path& path)
{
    std::ifstream file(path);
    Check(file.good(), "Cannot read file: " + path.string());
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

std::map<std::string, std::string> Snapshot(const fs::path& root)
{
    std::map<std::string, std::string> result;
    for (const auto& entry : fs::recursive_directory_iterator(root))
    {
        const auto name = entry.path().lexically_relative(root).string();
        if (entry.is_symlink())
            result[name] = "link: " + fs::read_symlink(entry.path()).string();
        else if (entry.is_directory())
            result[name] = "directory";
        else
            result[name] = "file: " + Read(entry.path());
    }
    return result;
}

void CreateGame(const fs::path& root)
{
    for (const auto& directory : { "levels", "resources", "localization" })
        Write(root / directory / "data.db0", directory);
}

struct Fixture
{
    fs::path root;
    GameDataLayoutPaths paths;

    Fixture()
    {
        std::string pattern = (fs::temp_directory_path() / "openxray-layout-XXXXXX").string();
        Check(mkdtemp(pattern.data()) != nullptr, "Cannot create temporary directory");
        root = fs::canonical(pattern);
        paths = { root / "game installation", root / "app bundle", root / "Application Support/runtime", root / "Application Support/_appdata_" };
        CreateGame(paths.gameRoot);
        Write(paths.bundleResourcesRoot / "fsgame.ltx", "bundled filesystem configuration");
        Write(paths.bundleResourcesRoot / "gamedata/shaders/gl/default.ps", "bundled shader");
        Write(paths.bundleResourcesRoot / "gamedata/scripts/optional.script", "optional engine script");
        Write(paths.bundleResourcesRoot / "gamedata/configs/optional.ltx", "optional engine configuration");
    }

    ~Fixture()
    {
        std::error_code error;
        // Restore permissions used by the read-only source test before removing its temporary files.
        for (const auto& entry : fs::recursive_directory_iterator(root))
        {
            if (!entry.is_symlink() && entry.is_directory())
                fs::permissions(entry.path(), fs::perms::owner_all, fs::perm_options::add, error);
        }
        fs::remove_all(root, error);
    }

    void Prepare()
    {
        std::string error;
        const bool prepared = PrepareGameDataLayout(paths, error);
        Check(prepared, "Layout preparation failed: " + error);
        Check(error.empty(), "Successful preparation returned an error");
    }
};

void CleanInstallation()
{
    Fixture fixture;
    const auto original = Snapshot(fixture.paths.gameRoot);
    const auto bundled = Snapshot(fixture.paths.bundleResourcesRoot);
    fixture.Prepare();

    const auto runtime = fixture.paths.runtimeRoot;
    Check(Read(runtime / "fsgame.ltx") == "bundled filesystem configuration", "Missing default configuration");
    Check(Read(runtime / "gamedata/shaders/gl/default.ps") == "bundled shader", "Missing shader fallback");
    Check(!fs::exists(runtime / "gamedata/scripts"), "Optional bundled scripts became active");
    Check(!fs::exists(runtime / "gamedata/configs"), "Optional bundled configurations became active");
    Check(fs::equivalent(runtime / "resources", fixture.paths.gameRoot / "resources"), "Game archives were not linked");
    Check(fs::equivalent(runtime / "_appdata_", fixture.paths.appDataRoot), "Wrong save directory");
    Check(Snapshot(fixture.paths.gameRoot) == original, "Clean installation was modified");
    Check(Snapshot(fixture.paths.bundleResourcesRoot) == bundled, "App bundle was modified");
}

void ModOverrides()
{
    Fixture fixture;
    Write(fixture.paths.gameRoot / "fsgame.ltx", "custom filesystem configuration");
    Write(fixture.paths.gameRoot / "gamedata/scripts/mod.script", "mod script");
    Write(fixture.paths.gameRoot / "gamedata/shaders/gl/custom.ps", "custom shader");
    Write(fixture.paths.gameRoot / "custom_data/settings.ltx", "custom resource path");
    const auto original = Snapshot(fixture.paths.gameRoot);
    fixture.Prepare();

    const auto runtime = fixture.paths.runtimeRoot;
    Check(Read(runtime / "fsgame.ltx") == "custom filesystem configuration", "Custom configuration was replaced");
    Check(Read(runtime / "gamedata/scripts/mod.script") == "mod script", "Mod scripts were lost");
    Check(Read(runtime / "gamedata/shaders/gl/custom.ps") == "custom shader", "Custom shaders were replaced");
    Check(!fs::exists(runtime / "gamedata/shaders/gl/default.ps"), "Bundled shaders were merged into a custom shader directory");
    Check(Read(runtime / "custom_data/settings.ltx") == "custom resource path", "Custom root directories were lost");
    Check(Snapshot(fixture.paths.gameRoot) == original, "Mod installation was modified");
}

void MissingOpenGLShaders()
{
    Fixture fixture;
    Write(fixture.paths.gameRoot / "gamedata/scripts/mod.script", "mod script");
    Write(fixture.paths.gameRoot / "gamedata/shaders/r4/custom.ps", "DirectX shader");
    const auto original = Snapshot(fixture.paths.gameRoot);
    fixture.Prepare();

    const auto data = fixture.paths.runtimeRoot / "gamedata";
    Check(!fs::is_symlink(data) && !fs::is_symlink(data / "shaders"), "Fallback directories point into the installation");
    Check(Read(data / "scripts/mod.script") == "mod script", "Mod files were not retained");
    Check(Read(data / "shaders/r4/custom.ps") == "DirectX shader", "Other shaders were lost");
    Check(Read(data / "shaders/gl/default.ps") == "bundled shader", "Missing OpenGL shader fallback");
    Check(Snapshot(fixture.paths.gameRoot) == original, "Shader fallback was written into the installation");
}

void LinkedGamedata()
{
    Fixture fixture;
    const auto external = fixture.root / "external-mod";
    Write(external / "scripts/mod.script", "external mod");
    fs::create_directory_symlink(external, fixture.paths.gameRoot / "gamedata");
    const auto original = Snapshot(fixture.paths.gameRoot);
    const auto externalOriginal = Snapshot(external);
    fixture.Prepare();

    Check(Read(fixture.paths.runtimeRoot / "gamedata/scripts/mod.script") == "external mod", "Linked mod was lost");
    Check(fs::is_directory(fixture.paths.runtimeRoot / "gamedata/shaders/gl"), "Linked mod has no shader fallback");
    Check(Snapshot(fixture.paths.gameRoot) == original, "Installation links were modified");
    Check(Snapshot(external) == externalOriginal, "Linked mod directory was modified");
}

void SwitchInstallations()
{
    Fixture fixture;
    Write(fixture.paths.gameRoot / "fsgame.ltx", "old mod config");
    Write(fixture.paths.gameRoot / "gamedata/scripts/old.script", "old mod script");
    Write(fixture.paths.gameRoot / "gamedata/shaders/gl/old.ps", "old shader");
    Write(fixture.paths.gameRoot / "patches/old.db", "old patch");
    fixture.Prepare();
    Write(fixture.paths.runtimeRoot / "_appdata_/savedgames/existing.sav", "existing save");
    Write(fixture.paths.runtimeRoot / "custom_saves/existing.sav", "custom save");

    fixture.paths.gameRoot = fixture.root / "other-game";
    CreateGame(fixture.paths.gameRoot);
    Write(fixture.paths.gameRoot / "resources/data.db0", "new game archive");
    fixture.Prepare();

    const auto runtime = fixture.paths.runtimeRoot;
    Check(!fs::exists(runtime / "patches"), "Old patch archives leaked into the new installation");
    Check(!fs::exists(runtime / "gamedata/scripts"), "Old mod scripts leaked into the new installation");
    Check(!fs::exists(runtime / "gamedata/shaders/gl/old.ps"), "Old shaders leaked into the new installation");
    Check(Read(runtime / "resources/data.db0") == "new game archive", "Archive links were not updated");
    Check(Read(runtime / "fsgame.ltx") == "bundled filesystem configuration", "Old custom configuration was retained");
    Check(Read(runtime / "gamedata/shaders/gl/default.ps") == "bundled shader", "Shader fallback was not restored");
    Check(Read(runtime / "_appdata_/savedgames/existing.sav") == "existing save", "Existing saves were lost");
    Check(Read(runtime / "custom_saves/existing.sav") == "custom save", "Custom runtime saves were lost");
}

void RepeatAndRefresh()
{
    Fixture fixture;
    Write(fixture.paths.gameRoot / "gamedata/scripts/mod.script", "mod script");
    Write(fixture.paths.gameRoot / "gamedata/shaders/gl/custom.ps", "custom shader");
    fixture.Prepare();
    const auto original = Snapshot(fixture.paths.runtimeRoot);
    fixture.Prepare();
    Check(Snapshot(fixture.paths.runtimeRoot) == original, "Repeated setup changed the layout");

    fs::remove_all(fixture.paths.gameRoot / "gamedata/shaders/gl");
    fixture.Prepare();
    Check(Read(fixture.paths.runtimeRoot / "gamedata/shaders/gl/default.ps") == "bundled shader", "Removed shaders did not fall back");

    Write(fixture.paths.gameRoot / "gamedata/shaders/gl/custom.ps", "new custom shader");
    fixture.Prepare();
    Check(Read(fixture.paths.runtimeRoot / "gamedata/shaders/gl/custom.ps") == "new custom shader", "New custom shaders were ignored");
    Check(!fs::exists(fixture.paths.runtimeRoot / "gamedata/shaders/gl/default.ps"), "Old fallback remained active");
}

void LegacyApplicationSupport()
{
    Fixture fixture;
    fixture.paths.gameRoot = fixture.root / "legacy-application-support";
    CreateGame(fixture.paths.gameRoot);
    Write(fixture.paths.gameRoot / "gamedata/scripts/mod.script", "legacy mod");
    fixture.paths.appDataRoot = fixture.paths.gameRoot / "_appdata_";
    Write(fixture.paths.appDataRoot / "savedgames/existing.sav", "legacy save");
    const auto original = Snapshot(fixture.paths.gameRoot);
    fixture.Prepare();

    Check(fs::equivalent(fixture.paths.runtimeRoot / "_appdata_", fixture.paths.appDataRoot), "Legacy saves were relocated");
    Check(Read(fixture.paths.runtimeRoot / "_appdata_/savedgames/existing.sav") == "legacy save", "Legacy save is inaccessible");
    Check(Snapshot(fixture.paths.gameRoot) == original, "Legacy installation was modified");
}

void RuntimeEditsAreBackedUp()
{
    Fixture fixture;
    Write(fixture.paths.gameRoot / "gamedata/scripts/mod.script", "selected mod");
    fixture.Prepare();
    const auto scripts = fixture.paths.runtimeRoot / "gamedata/scripts";
    fs::remove(scripts);
    Write(scripts / "local.script", "local runtime edit");
    fixture.Prepare();

    Check(Read(scripts / "mod.script") == "selected mod", "Selected installation did not take precedence");
    Check(Read(fixture.paths.runtimeRoot / "gamedata/scripts.openxray-backup/local.script") == "local runtime edit",
        "A real runtime file was discarded instead of backed up");
}

void ReadOnlySources()
{
    Fixture fixture;
    Write(fixture.paths.gameRoot / "gamedata/scripts/mod.script", "read-only mod");
    const auto original = Snapshot(fixture.paths.gameRoot);
    const auto bundled = Snapshot(fixture.paths.bundleResourcesRoot);
    for (const auto& root : { fixture.paths.gameRoot, fixture.paths.bundleResourcesRoot })
    {
        for (const auto& entry : fs::recursive_directory_iterator(root))
            fs::permissions(entry.path(), fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write, fs::perm_options::remove);
        fs::permissions(root, fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write, fs::perm_options::remove);
    }
    fixture.Prepare();
    Check(Snapshot(fixture.paths.gameRoot) == original, "Read-only installation changed");
    Check(Snapshot(fixture.paths.bundleResourcesRoot) == bundled, "Read-only bundle changed");
}

void RejectUnsafeRuntimePaths()
{
    Fixture fixture;
    const auto original = Snapshot(fixture.paths.gameRoot);
    std::string error;
    fixture.paths.runtimeRoot = fixture.paths.gameRoot / "runtime";
    Check(!PrepareGameDataLayout(fixture.paths, error), "Created runtime inside the installation");
    Check(Snapshot(fixture.paths.gameRoot) == original, "Overlapping runtime modified the installation");

    fixture.paths.runtimeRoot = fixture.root / "linked-runtime";
    fs::create_directory_symlink(fixture.paths.gameRoot, fixture.paths.runtimeRoot);
    Check(!PrepareGameDataLayout(fixture.paths, error), "Accepted a runtime link into the installation");
    Check(Snapshot(fixture.paths.gameRoot) == original, "Linked runtime modified the installation");

    fixture.paths.runtimeRoot = fixture.root / "support/runtime";
    fixture.Prepare();
    fs::remove_all(fixture.paths.runtimeRoot / "gamedata");
    fs::create_directory_symlink(fixture.paths.gameRoot, fixture.paths.runtimeRoot / "gamedata");
    Check(!PrepareGameDataLayout(fixture.paths, error), "Accepted a gamedata link into the installation");
    Check(Snapshot(fixture.paths.gameRoot) == original, "Linked gamedata modified the installation");

    fs::remove(fixture.paths.runtimeRoot / "gamedata");
    fixture.Prepare();
    fs::remove_all(fixture.paths.runtimeRoot / "gamedata/shaders");
    fs::create_directory_symlink(fixture.paths.gameRoot, fixture.paths.runtimeRoot / "gamedata/shaders");
    Check(!PrepareGameDataLayout(fixture.paths, error), "Accepted a shaders link into the installation");
    Check(Snapshot(fixture.paths.gameRoot) == original, "Linked shaders modified the installation");
}

void ReportMissingResources()
{
    Fixture fixture;
    std::string error;
    fs::create_symlink("missing-fsgame.ltx", fixture.paths.gameRoot / "fsgame.ltx");
    Check(!PrepareGameDataLayout(fixture.paths, error) && !error.empty(), "Broken custom configuration was silently replaced");
    fs::remove(fixture.paths.gameRoot / "fsgame.ltx");
    Write(fixture.paths.gameRoot / "gamedata/shaders/gl", "not a directory");
    Check(!PrepareGameDataLayout(fixture.paths, error) && !error.empty(), "Invalid custom shader path was silently replaced");
    fs::remove(fixture.paths.gameRoot / "gamedata/shaders/gl");
    fs::remove_all(fixture.paths.bundleResourcesRoot / "gamedata/shaders/gl");
    Check(!PrepareGameDataLayout(fixture.paths, error) && !error.empty(), "Missing shader fallback was accepted");
    fs::remove_all(fixture.paths.gameRoot / "levels");
    Check(!PrepareGameDataLayout(fixture.paths, error) && !error.empty(), "Missing proprietary data was accepted");
}
} // namespace

int main()
{
    const std::vector<std::pair<const char*, void (*)()>> tests = {
        {          "clean installation",        CleanInstallation },
        {               "mod overrides",             ModOverrides },
        {      "missing OpenGL shaders",     MissingOpenGLShaders },
        {             "linked gamedata",           LinkedGamedata },
        {        "switch installations",      SwitchInstallations },
        {          "repeat and refresh",         RepeatAndRefresh },
        {  "legacy Application Support", LegacyApplicationSupport },
        {       "back up runtime edits",  RuntimeEditsAreBackedUp },
        {           "read-only sources",          ReadOnlySources },
        { "reject unsafe runtime paths", RejectUnsafeRuntimePaths },
        {    "report missing resources",   ReportMissingResources },
    };
    for (const auto& test : tests)
    {
        try
        {
            test.second();
            std::cout << "PASS: " << test.first << '\n';
        }
        catch (const std::exception& failure)
        {
            std::cerr << "FAIL: " << test.first << ": " << failure.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
