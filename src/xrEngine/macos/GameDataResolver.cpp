#include "stdafx.h"
#pragma hdrstop

#if defined(XR_PLATFORM_APPLE)
#include "GameDataResolver.h"

#include <SDL.h>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace
{
constexpr pcstr CompanyName = "GSC Game World";
constexpr pcstr SavedPathFileName = "openxray_gamedata_path.txt";
constexpr pcstr ChooseAnotherItem = "Choose another folder...";
constexpr pcstr QuitItem = "Quit";

struct GameInfo
{
    std::string appSupportName;
    std::string displayName;
    std::vector<std::string> steamNames;
    std::vector<std::string> gogNames;
    bool supportsFlatArchives;
    std::string requiredDataDescription;
};

bool HasCommandLineOption(pcstr commandLine, pcstr option)
{
    return commandLine && strstr(commandLine, option);
}

std::string TrimLineEnd(std::string value)
{
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r'))
        value.pop_back();
    return value;
}

std::string EnsureTrailingSlash(std::string path)
{
    if (!path.empty() && path.back() != '/')
        path.push_back('/');
    return path;
}

std::string RemoveTrailingSlash(std::string path)
{
    while (path.size() > 1 && path.back() == '/')
        path.pop_back();
    return path;
}

std::string JoinPath(const std::string& left, pcstr right)
{
    if (left.empty())
        return right ? right : "";

    std::string result = left;
    if (result.back() != '/')
        result.push_back('/');
    result += right;
    return result;
}

std::string ExpandHomePath(pcstr suffix)
{
    const char* home = SDL_getenv("HOME");
    if (!home || !home[0])
        return {};

    std::string path = home;
    if (suffix && suffix[0])
    {
        if (path.back() != '/' && suffix[0] != '/')
            path.push_back('/');
        path += suffix;
    }
    return path;
}

std::string NormalizeExistingPath(const std::string& path)
{
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved))
        return resolved;
    return RemoveTrailingSlash(path);
}

bool IsDirectory(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool IsFile(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool IsSymlink(const std::string& path)
{
    struct stat st;
    return lstat(path.c_str(), &st) == 0 && S_ISLNK(st.st_mode);
}

bool PathExistsNoFollow(const std::string& path)
{
    struct stat st;
    return lstat(path.c_str(), &st) == 0;
}

bool HasDirectoryGameData(const std::string& root)
{
    if (root.empty())
        return false;

    return IsDirectory(JoinPath(root, "levels")) &&
        IsDirectory(JoinPath(root, "resources")) &&
        IsDirectory(JoinPath(root, "localization"));
}

bool HasSoCFlatArchives(const std::string& root)
{
    static constexpr pcstr RequiredArchives[] = {
        "gamedata.db0", "gamedata.db1", "gamedata.db2", "gamedata.db3", "gamedata.db4",
        "gamedata.db5", "gamedata.db6", "gamedata.db7", "gamedata.db8", "gamedata.db9",
        "gamedata.dba", "gamedata.dbb", "gamedata.dbc"
    };

    if (root.empty())
        return false;

    for (pcstr archiveName : RequiredArchives)
    {
        if (!IsFile(JoinPath(root, archiveName)))
            return false;
    }
    return true;
}

bool HasRequiredGameData(const std::string& root, const GameInfo& gameInfo)
{
    return HasDirectoryGameData(root) || (gameInfo.supportsFlatArchives && HasSoCFlatArchives(root));
}

bool HasRuntimeLayout(const std::string& root, const GameInfo& gameInfo)
{
    return HasRequiredGameData(root, gameInfo) &&
        IsFile(JoinPath(root, "fsgame.ltx")) &&
        IsDirectory(JoinPath(root, "gamedata"));
}

GameInfo GetGameInfo(pcstr commandLine)
{
    if (HasCommandLineOption(commandLine, "-shoc") || HasCommandLineOption(commandLine, "-soc"))
    {
        return {
            "S.T.A.L.K.E.R. - Shadow of Chernobyl",
            "S.T.A.L.K.E.R.: Shadow of Chernobyl",
            { "STALKER Shadow of Chernobyl", "Stalker Shadow of Chernobyl", "S.T.A.L.K.E.R. Shadow of Chernobyl" },
            { "S.T.A.L.K.E.R. - Shadow of Chernobyl" },
            true,
            "the gamedata.db* archives"
        };
    }

    if (HasCommandLineOption(commandLine, "-cs"))
    {
        return {
            "S.T.A.L.K.E.R. - Clear Sky",
            "S.T.A.L.K.E.R.: Clear Sky",
            { "STALKER Clear Sky", "Stalker Clear Sky", "S.T.A.L.K.E.R. Clear Sky" },
            { "S.T.A.L.K.E.R. - Clear Sky" },
            false,
            "levels, resources, and localization"
        };
    }

    return {
        "S.T.A.L.K.E.R. - Call of Pripyat",
        "S.T.A.L.K.E.R.: Call of Pripyat",
        { "STALKER Call of Pripyat", "Stalker Call of Pripyat", "S.T.A.L.K.E.R. Call of Pripyat" },
        { "S.T.A.L.K.E.R. - Call of Pripyat" },
        false,
        "levels, resources, and localization"
    };
}

std::string GetPrefPath(const GameInfo& gameInfo)
{
    char* prefPath = SDL_GetPrefPath(CompanyName, gameInfo.appSupportName.c_str());
    if (!prefPath)
        return {};

    std::string result = EnsureTrailingSlash(prefPath);
    SDL_free(prefPath);
    return result;
}

bool GetBundleResourcesRoot(std::string& resourcesRoot)
{
    char* basePathRaw = SDL_GetBasePath();
    if (!basePathRaw)
        return false;

    std::string basePath = EnsureTrailingSlash(basePathRaw);
    SDL_free(basePathRaw);

    std::string candidate = NormalizeExistingPath(JoinPath(basePath, "../Resources/openxray"));
    if (!IsFile(JoinPath(candidate, "fsgame.ltx")) || !IsDirectory(JoinPath(candidate, "gamedata")))
        return false;

    resourcesRoot = candidate;
    return true;
}

std::string GetBundleNeighborRoot()
{
    char* basePathRaw = SDL_GetBasePath();
    if (!basePathRaw)
        return {};

    std::string basePath = EnsureTrailingSlash(basePathRaw);
    SDL_free(basePathRaw);
    return NormalizeExistingPath(JoinPath(basePath, "../../.."));
}

std::string GetSavedRoot(const std::string& prefPath)
{
    const std::string pathFile = JoinPath(prefPath, SavedPathFileName);
    FILE* file = fopen(pathFile.c_str(), "r");
    if (!file)
        return {};

    char buffer[PATH_MAX];
    const bool hasValue = fgets(buffer, sizeof(buffer), file) != nullptr;
    fclose(file);

    if (!hasValue)
        return {};

    return RemoveTrailingSlash(TrimLineEnd(buffer));
}

void SaveRoot(const std::string& prefPath, const std::string& root)
{
    const std::string pathFile = JoinPath(prefPath, SavedPathFileName);
    FILE* file = fopen(pathFile.c_str(), "w");
    if (!file)
        return;

    fprintf(file, "%s\n", root.c_str());
    fclose(file);
}

void AddCandidate(std::vector<std::string>& candidates, const std::string& path, const GameInfo& gameInfo)
{
    if (!HasRequiredGameData(path, gameInfo))
        return;

    const std::string normalized = NormalizeExistingPath(path);
    for (const auto& candidate : candidates)
    {
        if (candidate == normalized)
            return;
    }
    candidates.emplace_back(normalized);
}

std::vector<std::string> DiscoverCandidates(const GameInfo& gameInfo, const std::string& prefPath)
{
    std::vector<std::string> candidates;

    AddCandidate(candidates, prefPath, gameInfo);
    AddCandidate(candidates,
        ExpandHomePath(JoinPath(".local/share/GSC Game World", gameInfo.appSupportName.c_str()).c_str()), gameInfo);

    for (const auto& steamName : gameInfo.steamNames)
    {
        AddCandidate(candidates,
            ExpandHomePath(JoinPath("Library/Application Support/Steam/steamapps/common", steamName.c_str()).c_str()), gameInfo);
        AddCandidate(candidates,
            ExpandHomePath(JoinPath(".local/share/Steam/steamapps/common", steamName.c_str()).c_str()), gameInfo);
        AddCandidate(candidates,
            ExpandHomePath(JoinPath(".steam/steam/steamapps/common", steamName.c_str()).c_str()), gameInfo);
    }

    for (const auto& gogName : gameInfo.gogNames)
    {
        AddCandidate(candidates, ExpandHomePath(JoinPath("GOG Games", gogName.c_str()).c_str()), gameInfo);
        AddCandidate(candidates, ExpandHomePath(JoinPath("Applications", gogName.c_str()).c_str()), gameInfo);
        AddCandidate(candidates, JoinPath("/Applications", gogName.c_str()), gameInfo);
    }

    AddCandidate(candidates, GetBundleNeighborRoot(), gameInfo);
    return candidates;
}

std::string EscapeAppleScriptString(const std::string& value)
{
    std::string result;
    result.reserve(value.size() + 2);
    result.push_back('"');
    for (const char c : value)
    {
        if (c == '\\' || c == '"')
            result.push_back('\\');
        if (c == '\n' || c == '\r')
            result.push_back(' ');
        else
            result.push_back(c);
    }
    result.push_back('"');
    return result;
}

bool RunAppleScript(const std::string& script, std::string& output)
{
    char scriptPath[] = "/tmp/openxray_osascript_XXXXXX";
    const int fd = mkstemp(scriptPath);
    if (fd == -1)
        return false;

    FILE* file = fdopen(fd, "w");
    if (!file)
    {
        close(fd);
        xr_unlink(scriptPath);
        return false;
    }

    fwrite(script.data(), 1, script.size(), file);
    fclose(file);

    const std::string command = std::string("/usr/bin/osascript ") + scriptPath + " 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        xr_unlink(scriptPath);
        return false;
    }

    char buffer[1024];
    output.clear();
    while (fgets(buffer, sizeof(buffer), pipe))
        output += buffer;

    const int status = pclose(pipe);
    xr_unlink(scriptPath);
    output = TrimLineEnd(output);
    return status == 0 && !output.empty();
}

void ShowAppleScriptAlert(const std::string& message)
{
    std::string ignored;
    RunAppleScript(
        "display alert \"OpenXRay\" message " + EscapeAppleScriptString(message) + " as warning\n",
        ignored);
}

bool ChooseFolder(const GameInfo& gameInfo, std::string& selectedRoot)
{
    const std::string prompt = "Select the " + gameInfo.displayName +
        " directory that contains " + gameInfo.requiredDataDescription + ".";
    std::string output;
    if (!RunAppleScript("POSIX path of (choose folder with prompt " + EscapeAppleScriptString(prompt) + ")\n", output))
        return false;

    selectedRoot = RemoveTrailingSlash(output);
    return true;
}

bool ChooseRootFromDialog(const GameInfo& gameInfo, const std::vector<std::string>& candidates, std::string& selectedRoot)
{
    std::vector<std::string> choices = candidates;
    choices.emplace_back(ChooseAnotherItem);
    choices.emplace_back(QuitItem);

    std::string choicesLiteral = "{";
    for (size_t i = 0; i < choices.size(); ++i)
    {
        if (i != 0)
            choicesLiteral += ", ";
        choicesLiteral += EscapeAppleScriptString(choices[i]);
    }
    choicesLiteral += "}";

    const std::string defaultItem = candidates.empty() ? ChooseAnotherItem : candidates.front();
    const std::string prompt = candidates.empty()
        ? "OpenXRay could not find game data automatically. Choose the " + gameInfo.displayName + " directory."
        : "Choose the " + gameInfo.displayName + " game data directory.";

    const std::string script =
        "set openxrayChoices to " + choicesLiteral + "\n"
        "set openxraySelection to choose from list openxrayChoices with title \"OpenXRay\" with prompt " +
        EscapeAppleScriptString(prompt) + " default items {" + EscapeAppleScriptString(defaultItem) +
        "} OK button name \"Use Selected\" cancel button name \"Quit\"\n"
        "if openxraySelection is false then\n"
        "    return " + EscapeAppleScriptString(QuitItem) + "\n"
        "end if\n"
        "return item 1 of openxraySelection\n";

    std::string output;
    if (!RunAppleScript(script, output) || output == QuitItem)
        return false;

    if (output == ChooseAnotherItem)
        return ChooseFolder(gameInfo, selectedRoot);

    selectedRoot = RemoveTrailingSlash(output);
    return true;
}

bool MoveExistingAside(const std::string& path)
{
    if (!PathExistsNoFollow(path))
        return true;

    for (u32 i = 0; i < 100; ++i)
    {
        std::string backupPath = path + ".openxray-backup";
        if (i != 0)
            backupPath += std::to_string(i);

        if (PathExistsNoFollow(backupPath))
            continue;

        return rename(path.c_str(), backupPath.c_str()) == 0;
    }

    return false;
}

bool EnsureManagedSymlink(const std::string& source, const std::string& linkPath)
{
    if (source.empty() || linkPath.empty())
        return false;

    if (RemoveTrailingSlash(source) == RemoveTrailingSlash(linkPath))
        return true;

    if (IsSymlink(linkPath))
        xr_unlink(linkPath.c_str());
    else if (!MoveExistingAside(linkPath))
        return false;

    return symlink(source.c_str(), linkPath.c_str()) == 0;
}

void LinkDirectoryIfPresent(const std::string& prefPath, const std::string& gameRoot, pcstr dirName)
{
    const std::string source = JoinPath(gameRoot, dirName);
    if (!IsDirectory(source))
        return;

    const std::string linkPath = JoinPath(prefPath, dirName);
    if (RemoveTrailingSlash(source) == RemoveTrailingSlash(linkPath))
        return;

    EnsureManagedSymlink(source, linkPath);
}

void LinkFlatArchivesIfPresent(const std::string& prefPath, const std::string& gameRoot)
{
    DIR* directory = opendir(gameRoot.c_str());
    if (!directory)
        return;

    while (const dirent* entry = readdir(directory))
    {
        if (strncmp(entry->d_name, "gamedata.db", 11) != 0)
            continue;

        const std::string source = JoinPath(gameRoot, entry->d_name);
        if (IsFile(source))
            EnsureManagedSymlink(source, JoinPath(prefPath, entry->d_name));
    }
    closedir(directory);
}

bool ApplyRuntimeLayout(const std::string& prefPath, const std::string& bundleResourcesRoot,
    const std::string& gameRoot, const GameInfo& gameInfo)
{
    if (!HasRequiredGameData(gameRoot, gameInfo))
        return false;

    EnsureManagedSymlink(JoinPath(bundleResourcesRoot, "fsgame.ltx"), JoinPath(prefPath, "fsgame.ltx"));
    EnsureManagedSymlink(JoinPath(bundleResourcesRoot, "gamedata"), JoinPath(prefPath, "gamedata"));

    LinkDirectoryIfPresent(prefPath, gameRoot, "levels");
    LinkDirectoryIfPresent(prefPath, gameRoot, "resources");
    LinkDirectoryIfPresent(prefPath, gameRoot, "localization");
    LinkDirectoryIfPresent(prefPath, gameRoot, "mp");
    LinkDirectoryIfPresent(prefPath, gameRoot, "patches");
    if (gameInfo.supportsFlatArchives)
        LinkFlatArchivesIfPresent(prefPath, gameRoot);

    return HasRuntimeLayout(prefPath, gameInfo);
}
} // namespace

void ResolveMacOSGameDataPath(pcstr commandLine)
{
    if (HasCommandLineOption(commandLine, "-fsltx "))
        return;

    std::string bundleResourcesRoot;
    if (!GetBundleResourcesRoot(bundleResourcesRoot))
        return;

    const GameInfo gameInfo = GetGameInfo(commandLine);
    const std::string prefPath = GetPrefPath(gameInfo);
    if (prefPath.empty())
        return;

    const bool forceSelection = HasCommandLineOption(commandLine, "-select_gamedata") ||
        HasCommandLineOption(commandLine, "-reset_gamedata_path");

    const std::string savedRoot = GetSavedRoot(prefPath);
    if (!forceSelection && HasRequiredGameData(savedRoot, gameInfo) &&
        ApplyRuntimeLayout(prefPath, bundleResourcesRoot, savedRoot, gameInfo))
        return;

    const std::vector<std::string> candidates = DiscoverCandidates(gameInfo, prefPath);
    std::string selectedRoot;

    while (ChooseRootFromDialog(gameInfo, candidates, selectedRoot))
    {
        if (!HasRequiredGameData(selectedRoot, gameInfo))
        {
            ShowAppleScriptAlert(
                "The selected directory does not contain " + gameInfo.requiredDataDescription + ". "
                "Please choose the root directory of a licensed game installation.");
            continue;
        }

        if (ApplyRuntimeLayout(prefPath, bundleResourcesRoot, selectedRoot, gameInfo))
        {
            SaveRoot(prefPath, NormalizeExistingPath(selectedRoot));
            return;
        }

        ShowAppleScriptAlert(
            "OpenXRay could not prepare the selected directory in Application Support. "
            "Check file permissions and try again.");
    }

    if (HasRuntimeLayout(prefPath, gameInfo))
        return;

    if (!HasRequiredGameData(prefPath, gameInfo))
    {
        const std::string message = "OpenXRay could not find required game files.\nChoose a directory that contains " +
            gameInfo.requiredDataDescription + ".";
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_WARNING,
            "OpenXRay: game files are required",
            message.c_str(),
            nullptr);
    }
    else
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_WARNING,
            "OpenXRay: setup is incomplete",
            "OpenXRay could not prepare bundled engine resources in Application Support.",
            nullptr);
    }

    std::exit(EXIT_SUCCESS);
}
#endif
