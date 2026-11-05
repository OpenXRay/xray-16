#include "stdafx.h"
#pragma hdrstop

#if defined(XR_PLATFORM_APPLE)
#    include "GameDataResolver.h"
#    include "GameDataLayout.h"

#    include <SDL.h>
#    include <cerrno>
#    include <cstdio>
#    include <cstdlib>
#    include <limits.h>
#    include <string>
#    include <sys/stat.h>
#    include <unistd.h>
#    include <vector>

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

bool HasRequiredGameData(const std::string& root)
{
    if (root.empty())
        return false;

    return IsDirectory(JoinPath(root, "levels")) &&
        IsDirectory(JoinPath(root, "resources")) &&
        IsDirectory(JoinPath(root, "localization"));
}

GameInfo GetGameInfo(pcstr commandLine)
{
    if (HasCommandLineOption(commandLine, "-shoc") || HasCommandLineOption(commandLine, "-soc"))
    {
        return {
            "S.T.A.L.K.E.R. - Shadow of Chernobyl",
            "S.T.A.L.K.E.R.: Shadow of Chernobyl",
            { "STALKER Shadow of Chernobyl", "Stalker Shadow of Chernobyl", "S.T.A.L.K.E.R. Shadow of Chernobyl" },
            { "S.T.A.L.K.E.R. - Shadow of Chernobyl" }
        };
    }

    if (HasCommandLineOption(commandLine, "-cs"))
    {
        return {
            "S.T.A.L.K.E.R. - Clear Sky",
            "S.T.A.L.K.E.R.: Clear Sky",
            { "STALKER Clear Sky", "Stalker Clear Sky", "S.T.A.L.K.E.R. Clear Sky" },
            { "S.T.A.L.K.E.R. - Clear Sky" }
        };
    }

    return {
        "S.T.A.L.K.E.R. - Call of Pripyat",
        "S.T.A.L.K.E.R.: Call of Pripyat",
        { "STALKER Call of Pripyat", "Stalker Call of Pripyat", "S.T.A.L.K.E.R. Call of Pripyat" },
        { "S.T.A.L.K.E.R. - Call of Pripyat" }
    };
}

std::string GetPrefPath(pcstr appName)
{
    char* prefPath = SDL_GetPrefPath(CompanyName, appName);
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

void AddCandidate(std::vector<std::string>& candidates, const std::string& path)
{
    if (!HasRequiredGameData(path))
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

    AddCandidate(candidates, prefPath);
    AddCandidate(candidates, ExpandHomePath(JoinPath(".local/share/GSC Game World", gameInfo.appSupportName.c_str()).c_str()));

    for (const auto& steamName : gameInfo.steamNames)
    {
        AddCandidate(candidates, ExpandHomePath(JoinPath("Library/Application Support/Steam/steamapps/common", steamName.c_str()).c_str()));
        AddCandidate(candidates, ExpandHomePath(JoinPath(".local/share/Steam/steamapps/common", steamName.c_str()).c_str()));
        AddCandidate(candidates, ExpandHomePath(JoinPath(".steam/steam/steamapps/common", steamName.c_str()).c_str()));
    }

    for (const auto& gogName : gameInfo.gogNames)
    {
        AddCandidate(candidates, ExpandHomePath(JoinPath("GOG Games", gogName.c_str()).c_str()));
        AddCandidate(candidates, ExpandHomePath(JoinPath("Applications", gogName.c_str()).c_str()));
        AddCandidate(candidates, JoinPath("/Applications", gogName.c_str()));
    }

    AddCandidate(candidates, GetBundleNeighborRoot());
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
        " directory that contains levels, resources, and localization.";
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

bool UseGameDataLayout(const xray::macos::GameDataLayoutPaths& paths)
{
    std::string error;
    if (!xray::macos::PrepareGameDataLayout(paths, error))
    {
        Msg("! macOS game data setup: %s", error.c_str());
        return false;
    }

    // xrCore uses fsgame.ltx in the working directory before its legacy SDL preference path.
    if (chdir(paths.runtimeRoot.c_str()) != 0)
    {
        Msg("! Cannot enter macOS game data layout: %s", strerror(errno));
        return false;
    }

    return true;
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
    const std::string legacyPrefPath = GetPrefPath(gameInfo.appSupportName.c_str());
    const std::string openxrayPrefPath = GetPrefPath("OpenXRay");
    if (legacyPrefPath.empty() || openxrayPrefPath.empty())
        return;

    // Keep the runtime view separate even when the selected installation is in the legacy SDL directory.
    const std::string profilePath = JoinPath(openxrayPrefPath, gameInfo.appSupportName.c_str());
    const std::string runtimeRoot = JoinPath(profilePath, "runtime");
    const std::string legacyAppData = JoinPath(legacyPrefPath, "_appdata_");
    const std::string profileAppData = JoinPath(profilePath, "_appdata_");
    const std::string appDataRoot = IsDirectory(profileAppData) || !IsDirectory(legacyAppData) ? profileAppData : legacyAppData;

    const bool forceSelection = HasCommandLineOption(commandLine, "-select_gamedata") ||
        HasCommandLineOption(commandLine, "-reset_gamedata_path");

    std::string savedRoot = GetSavedRoot(profilePath);
    if (savedRoot.empty())
        savedRoot = GetSavedRoot(legacyPrefPath);
    savedRoot = NormalizeExistingPath(savedRoot);
    if (!forceSelection && HasRequiredGameData(savedRoot) && UseGameDataLayout({ savedRoot, bundleResourcesRoot, runtimeRoot, appDataRoot }))
    {
        SaveRoot(profilePath, savedRoot);
        return;
    }

    const std::vector<std::string> candidates = DiscoverCandidates(gameInfo, legacyPrefPath);
    std::string selectedRoot;

    while (ChooseRootFromDialog(gameInfo, candidates, selectedRoot))
    {
        if (!HasRequiredGameData(selectedRoot))
        {
            ShowAppleScriptAlert(
                "The selected directory does not contain levels, resources, and localization. "
                "Please choose the root directory of a licensed game installation.");
            continue;
        }

        selectedRoot = NormalizeExistingPath(selectedRoot);
        if (UseGameDataLayout({ selectedRoot, bundleResourcesRoot, runtimeRoot, appDataRoot }))
        {
            SaveRoot(profilePath, selectedRoot);
            return;
        }

        ShowAppleScriptAlert(
            "OpenXRay could not prepare the selected directory in Application Support. "
            "Check file permissions and try again.");
    }

    // Only a successfully prepared layout may reach xrCore; cancelling must not use a partial layout.
    std::exit(EXIT_SUCCESS);
}
#endif
