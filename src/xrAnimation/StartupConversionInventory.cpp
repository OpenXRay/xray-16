#include "stdafx.h"
#include "StartupConversionInventory.h"
#include "LegacyOgfSkeleton.h"
#include "LegacyChunkIO.h"
#include "Common/LevelStructure.hpp"
#include "xrCore/LocatorAPI.h"
#include "xrCore/xr_ini.h"

#include <map>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace XRay::Animation::Startup
{
namespace
{
struct ReaderCloser
{
    void operator()(IReader* reader) const { FS.r_close(reader); }
};
using Reader = std::unique_ptr<IReader, ReaderCloser>;

Reader Open(const xr_string& source)
{
    Reader reader(FS.r_open(source.c_str()));
    if (!reader)
        throw std::runtime_error("cannot open animation source " + std::string(source.c_str()));
    return reader;
}

xr_string Root(pcstr alias)
{
    string_path path;
    FS.update_path(path, alias, "");
    return path;
}

xr_string WithExtension(xr_string path, pcstr extension)
{
    const size_t length = xr_strlen(extension);
    if (path.size() < length || path.compare(path.size() - length, length, extension) != 0)
        path += extension;
    return path;
}

xr_string Resolve(const xr_string& relative, const xr_string& level, const xr_string& meshes)
{
    if (!level.empty())
    {
        const auto candidate = level + relative;
        if (FS.exist(candidate.c_str()))
            return candidate;
    }
    const auto candidate = meshes + relative;
    if (FS.exist(candidate.c_str()))
        return candidate;
    throw std::runtime_error("missing motion source " + std::string(relative.c_str()) +
        " in level namespace " + std::string(level.c_str()));
}

xr_vector<xr_string> ResolveReferences(const xr_vector<shared_str>& references,
    const xr_string& level, const xr_string& meshes)
{
    xr_vector<xr_string> sources;
    for (const auto& reference : references)
    {
        auto name = CanonicalPath(reference.c_str());
        if (name.empty())
            throw std::runtime_error("empty OGF motion reference");
        name = WithExtension(std::move(name), ".omf");
        if (name.find_first_of("*?") == xr_string::npos)
            sources.push_back(Resolve(name, level, meshes));
        else
        {
            std::map<xr_string, xr_string> selected;
            FS_FileSet meshFiles;
            FS.file_list(meshFiles, meshes.c_str(), FS_ListFiles, name.c_str());
            for (const auto& file : meshFiles)
                selected.emplace(CanonicalPath(file.name.c_str()), meshes + file.name.c_str());
            if (!level.empty())
            {
                FS_FileSet levelFiles;
                FS.file_list(levelFiles, level.c_str(), FS_ListFiles, name.c_str());
                for (const auto& file : levelFiles)
                    selected[CanonicalPath(file.name.c_str())] = level + file.name.c_str();
            }
            if (selected.empty())
                throw std::runtime_error("motion wildcard matched no sources: " + std::string(name.c_str()));
            for (const auto& item : selected)
                sources.push_back(item.second);
        }
    }
    if (sources.empty() || sources.size() > MAX_ANIM_SLOT)
        throw std::runtime_error("invalid resolved animation slot count");
    return sources;
}

void ApplyPartitions(const xr_string& relativeModel, const xr_string& level, const xr_string& meshes,
    const xr_vector<OzzBoneDesc>& bones, CPartition& partition)
{
    if (relativeModel.compare(0, 13, "@level_visual") == 0 || relativeModel.find(':') != xr_string::npos)
        return;
    xr_string relative = relativeModel;
    const auto extension = relative.rfind('.');
    if (extension != xr_string::npos)
        relative.resize(extension);
    relative += ".ltx";
    xr_string source;
    if (!level.empty() && FS.exist((level + relative).c_str()))
        source = level + relative;
    else if (FS.exist((meshes + relative).c_str()))
        source = meshes + relative;
    else
        return;
    const auto reader = Open(source);
    const auto slash = source.find_last_of("\\/");
    const xr_string directory = slash == xr_string::npos ? xr_string{} : source.substr(0, slash + 1);
    CInifile ini(reader.get(), directory.c_str());
    std::unordered_map<xr_string, u16> indices;
    for (u16 bone = 0; bone < bones.size(); ++bone)
        indices.emplace(CanonicalPath(bones[bone].name.c_str()), bone);
    for (u16 index = 0; index < MAX_PARTS; ++index)
    {
        string32 name;
        xr_sprintf(name, "part_%u", unsigned(index));
        if (!ini.section_exist(name))
            continue;
        const auto& section = ini.r_section(name);
        auto& part = partition[index];
        if (!section.Data.empty())
            part.bones.clear();
        for (const auto& item : section.Data)
        {
            if (item.first == "partition_name")
                part.Name = item.second;
            else
            {
                const auto found = indices.find(CanonicalPath(item.first.c_str()));
                if (found == indices.end())
                    throw std::runtime_error(std::string(source.c_str()) + ": unknown partition bone " + item.first.c_str());
                part.bones.push_back(found->second);
            }
        }
    }
}

void InspectVisual(const Chunk& visual, const xr_string& source, const xr_string& relative,
    const xr_vector<xr_string>& levels, const xr_string& meshes, unsigned depth = 0)
{
    try
    {
        if (depth > 256)
            throw std::runtime_error("OGF child hierarchy too deep");
        ChunkStorage storage;
        const auto chunks = ParseChunks(visual.data, visual.size, storage);
        const auto header = chunks.find(OGF_HEADER);
        if (header == chunks.end())
            throw std::runtime_error("missing OGF header");
        BinaryReader headerReader{header->second.data, header->second.size};
        const auto information = headerReader.read<ogf_header>();
        if (information.format_version != xrOGF_FormatVersion)
            throw std::runtime_error("unsupported OGF version");
        if (information.type == MT_SKELETON_ANIM)
        {
            IReader ogf(const_cast<std::byte*>(visual.data), visual.size);
            xr_vector<OzzBoneDesc> bones;
            xr_vector<shared_str> references;
            bool embedded = false;
            ReadOgfSkeleton(&ogf, bones, references, embedded);
            std::shared_ptr<const OzzSkeletonMirror> mirror;
            const auto skeleton = PrepareSkeleton(bones, mirror);
            for (const auto& level : levels)
            {
                PreparedModel model;
                model.skeleton = skeleton;
                if (embedded)
                {
                    model.libraries.push_back(PrepareLibrary(source.c_str(), visual.data, visual.size,
                        skeleton, *mirror, model.partition));
                }
                else
                {
                    const auto sources = ResolveReferences(references, level, meshes);
                    for (const auto& motion : sources)
                    {
                        CPartition partition;
                        model.libraries.push_back(PrepareLibrary(motion.c_str(), nullptr, 0,
                            skeleton, *mirror, partition));
                        if (model.libraries.size() == 1)
                            model.partition = std::move(partition);
                    }
                }
                ApplyPartitions(relative, level, meshes, bones, model.partition);
                RegisterModel(source, level, std::move(model));
            }
        }
        const auto children = chunks.find(OGF_CHILDREN);
        if (children != chunks.end())
        {
            const auto entries = ParseChunks(children->second.data, children->second.size, storage);
            for (u32 index = 0; index < entries.size(); ++index)
            {
                const auto child = entries.find(index);
                if (child == entries.end())
                    throw std::runtime_error("non-contiguous OGF children");
                string32 suffix;
                xr_sprintf(suffix, ":%u", index + 1);
                InspectVisual(child->second, source + suffix, relative + suffix, levels, meshes, depth + 1);
            }
        }
    }
    catch (const std::exception& error)
    {
        throw std::runtime_error(std::string(source.c_str()) + ": " + error.what());
    }
}

void InspectFile(const xr_string& source, const xr_string& relative,
    const xr_vector<xr_string>& levels, const xr_string& meshes)
{
    const auto reader = Open(source);
    InspectVisual({static_cast<const std::byte*>(reader->pointer()), size_t(reader->length())},
        source, relative, levels, meshes);
}
}

void BuildInventory()
{
    for (auto& archive : FS.m_archives)
        if (!archive.is_open())
            FS.LoadArchive(archive);
    const auto meshes = Root("$game_meshes$");
    const auto gameLevels = Root("$game_levels$");
    FS_FileSet directories;
    FS.file_list(directories, "$game_levels$", FS_ListFolders | FS_RootOnly);
    xr_vector<xr_string> levelRoots;
    xr_vector<xr_string> meshContexts{xr_string{}};
    for (const auto& directory : directories)
    {
        const auto root = gameLevels + directory.name.c_str();
        levelRoots.push_back(root);
        meshContexts.push_back(root);
    }
    FS_FileSet meshFiles;
    FS.file_list(meshFiles, "$game_meshes$", FS_ListFiles, "*.ogf");
    for (const auto& file : meshFiles)
        InspectFile(meshes + file.name.c_str(), file.name.c_str(), meshContexts, meshes);
    for (const auto& level : levelRoots)
    {
        const xr_vector<xr_string> context{level};
        FS_FileSet models;
        FS.file_list(models, level.c_str(), FS_ListFiles, "*.ogf");
        for (const auto& file : models)
            InspectFile(level + file.name.c_str(), file.name.c_str(), context, meshes);
        const auto levelFile = level + "level";
        if (!FS.exist(levelFile.c_str()))
            continue;
        const auto reader = Open(levelFile);
        ChunkStorage storage;
        const auto chunks = ParseChunks(static_cast<const std::byte*>(reader->pointer()), reader->length(), storage);
        const auto visuals = chunks.find(fsL_VISUALS);
        if (visuals == chunks.end())
            throw std::runtime_error(std::string(levelFile.c_str()) + ": missing level visuals");
        const auto entries = ParseChunks(visuals->second.data, visuals->second.size, storage);
        for (u32 index = 0; index < entries.size(); ++index)
        {
            const auto found = entries.find(index);
            if (found == entries.end())
                throw std::runtime_error(std::string(levelFile.c_str()) + ": non-contiguous level visuals");
            string64 name;
            xr_sprintf(name, "@level_visual:%u", index);
            InspectVisual(found->second, level + name, name, context, meshes);
        }
    }
}
}
