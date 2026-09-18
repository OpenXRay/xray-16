#include "stdafx.h"

#include "Common/Platform.hpp"

#include "xrCore/Animation/Bone.hpp"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include "xrCore/FMesh.hpp"
#include "xrCore/_matrix.h"
#include "xrCore/_quaternion.h"
#include "xrCore/_vector3d.h"
#include "xrCore/xrCore.h"

#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector_archive.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/log.h>
#include <ozz/base/maths/quaternion.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>

#include "../Externals/ozz-animation/samples/framework/mesh.h"
#include "../OzzConversion.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <numeric>

#include "xrCore/Animation/SkeletonMotionDefs.hpp"

#include "../LegacyOgfConverter.h"
#include "../LegacyOmfConverter.h"
#include "../OzzBundle.h"

#ifdef main
#    undef main
#endif

namespace fs = std::filesystem;

struct MotionMark
{
    std::string name;
    std::vector<std::pair<float, float>> intervals;
};

struct MotionMetadata
{
    std::string name;
    uint32_t flags = 0;
    uint16_t bone_or_part = 0;
    uint16_t motion_id = 0;
    float speed = 0.f;
    float power = 0.f;
    float accrue = 0.f;
    float falloff = 0.f;
    std::vector<MotionMark> marks;
};

std::string escape_json(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value)
    {
        switch (ch)
        {
        case '"':  escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20)
            {
                char buffer[7];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned char>(ch));
                escaped += buffer;
            }
            else
            {
                escaped += ch;
            }
            break;
        }
    }
    return escaped;
}

void SerializeString(ozz::io::OArchive& archive, const std::string& value)
{
    const uint32_t length = static_cast<uint32_t>(value.size());
    archive << length;
    if (length > 0)
        archive << ozz::io::MakeArray(value.c_str(), length);
}

void SerializeMotionMarks(ozz::io::OArchive& archive, const MotionMetadata& metadata)
{
    const uint32_t mark_count = static_cast<uint32_t>(metadata.marks.size());
    archive << mark_count;
    for (const auto& mark : metadata.marks)
    {
        SerializeString(archive, mark.name);
        const uint32_t interval_count = static_cast<uint32_t>(mark.intervals.size());
        archive << interval_count;
        for (const auto& interval : mark.intervals)
        {
            archive << interval.first;
            archive << interval.second;
        }
    }
}

void SerializeMotionMetadata(ozz::io::OArchive& archive, const MotionMetadata& metadata)
{
    SerializeString(archive, metadata.name);
    archive << metadata.flags;
    archive << metadata.bone_or_part;
    archive << metadata.motion_id;
    archive << metadata.speed;
    archive << metadata.power;
    archive << metadata.accrue;
    archive << metadata.falloff;
    SerializeMotionMarks(archive, metadata);
}

void write_metadata_json(const fs::path& path, const std::vector<MotionMetadata>& metadata_list, const fs::path& source_omf)
{
    std::error_code ec;
    if (const auto parent = path.parent_path(); !parent.empty())
        fs::create_directories(parent, ec);

    std::ofstream stream(path);
    if (!stream)
        throw std::runtime_error("failed to open metadata output file: " + path.string());

    stream << std::fixed << std::setprecision(6);
    stream << "{\n";
    stream << "  \"source_omf\": \"" << escape_json(source_omf.string()) << "\",\n";
    stream << "  \"motions\": [\n";
    for (size_t index = 0; index < metadata_list.size(); ++index)
    {
        const MotionMetadata& metadata = metadata_list[index];
        stream << "    {\n";
        stream << "      \"motion_name\": \"" << escape_json(metadata.name) << "\",\n";
        stream << "      \"flags\": " << metadata.flags << ",\n";
        stream << "      \"bone_or_part\": " << metadata.bone_or_part << ",\n";
        stream << "      \"motion_id\": " << metadata.motion_id << ",\n";
        stream << "      \"speed\": " << metadata.speed << ",\n";
        stream << "      \"power\": " << metadata.power << ",\n";
        stream << "      \"accrue\": " << metadata.accrue << ",\n";
        stream << "      \"falloff\": " << metadata.falloff << ",\n";
        stream << "      \"marks\": [\n";
        for (size_t mark_index = 0; mark_index < metadata.marks.size(); ++mark_index)
        {
            const auto& mark = metadata.marks[mark_index];
            stream << "        {\n";
            stream << "          \"name\": \"" << escape_json(mark.name) << "\",\n";
            stream << "          \"intervals\": [";
            for (size_t interval_index = 0; interval_index < mark.intervals.size(); ++interval_index)
            {
                const auto& interval = mark.intervals[interval_index];
                stream << '[' << interval.first << ", " << interval.second << ']';
                if (interval_index + 1 < mark.intervals.size())
                    stream << ", ";
            }
            stream << "]\n";
            stream << "        }";
            if (mark_index + 1 < metadata.marks.size())
                stream << ',';
            stream << "\n";
        }
        stream << "      ]\n";
        stream << "    }";
        if (index + 1 < metadata_list.size())
            stream << ',';
        stream << "\n";
    }
    stream << "  ]\n";
    stream << "}\n";
}

void dump_bind_pose_csv(const fs::path& output, const xr_vector<XRay::Animation::LegacyOgfBone>& bones)
{
    std::ofstream stream(output);
    if (!stream)
        throw std::runtime_error("failed to open dump file: " + output.string());

    stream << std::fixed << std::setprecision(6);
    stream << "bone,parent,local_tx,local_ty,local_tz,global_tx,global_ty,global_tz\n";
    for (const auto& bone : bones)
    {
        const char* parent = bone.parent_name.empty() ? "" : bone.parent_name.c_str();
        stream << bone.name.c_str() << ',' << parent << ',' << bone.local_transform.c.x << ',' << bone.local_transform.c.y << ',' << bone.local_transform.c.z
               << ',' << bone.global_transform.c.x << ',' << bone.global_transform.c.y << ',' << bone.global_transform.c.z
               << '\n';
    }
}

struct SkeletonConfig
{
    fs::path input_ogf;
    fs::path output_ozz;
    std::optional<fs::path> dump_csv;
};

struct AnimationConfig
{
    fs::path input_omf;
    fs::path output_ozz;
    fs::path skeleton_ogf;
    std::optional<std::string> motion_name;
    std::optional<fs::path> metadata_path;
    bool optimize = false;
};

struct MeshConfig
{
    fs::path input_ogf;
    fs::path output_ozz;
};

struct BundleConfig
{
    fs::path input_ogf;
    fs::path output_ozzx;
};

SkeletonConfig parse_skeleton_arguments(int argc, char** argv)
{
    if (argc < 4)
        throw std::runtime_error("usage: xray_to_ozz_converter skeleton <input.ogf> <output.ozz> [--dump-bind <csv>]");

    SkeletonConfig config;
    config.input_ogf = fs::path(argv[2]);
    config.output_ozz = fs::path(argv[3]);

    for (int idx = 4; idx < argc; ++idx)
    {
        std::string_view arg(argv[idx]);
        if (arg == "--dump-bind")
        {
            if (idx + 1 >= argc)
                throw std::runtime_error("--dump-bind requires a path argument");
            config.dump_csv = fs::path(argv[++idx]);
        }
        else
        {
            throw std::runtime_error("unknown option: " + std::string(arg));
        }
    }

    return config;
}

AnimationConfig parse_animation_arguments(int argc, char** argv)
{
    if (argc < 5)
        throw std::
            runtime_error("usage: xray_to_ozz_converter animation <input.omf> <output.ozz> <skeleton.ogf> [--motion <name>] [--metadata <json>] [--optimize]");

    AnimationConfig config;
    config.input_omf = fs::path(argv[2]);
    config.output_ozz = fs::path(argv[3]);
    config.skeleton_ogf = fs::path(argv[4]);

    for (int idx = 5; idx < argc; ++idx)
    {
        std::string_view arg(argv[idx]);
        if (arg == "--motion")
        {
            if (idx + 1 >= argc)
                throw std::runtime_error("--motion requires a name argument");
            config.motion_name = std::string(argv[++idx]);
        }
        else if (arg == "--metadata")
        {
            if (idx + 1 >= argc)
                throw std::runtime_error("--metadata requires a path argument");
            config.metadata_path = fs::path(argv[++idx]);
        }
        else if (arg == "--optimize")
        {
            config.optimize = true;
        }
        else
        {
            throw std::runtime_error("unknown option: " + std::string(arg));
        }
    }

    return config;
}

MeshConfig parse_mesh_arguments(int argc, char** argv)
{
    if (argc < 4)
        throw std::runtime_error("usage: xray_to_ozz_converter mesh <input.ogf> <output.ozz>");

    MeshConfig config;
    config.input_ogf = fs::path(argv[2]);
    config.output_ozz = fs::path(argv[3]);
    return config;
}

BundleConfig parse_bundle_arguments(int argc, char** argv)
{
    if (argc < 4)
        throw std::runtime_error("usage: xray_to_ozz_converter bundle <input.ogf> <output.ozzx>");

    BundleConfig config;
    config.input_ogf = fs::path(argv[2]);
    config.output_ozzx = fs::path(argv[3]);
    return config;
}

void convert_skeleton(const SkeletonConfig& config)
{
    const auto start_time = std::chrono::steady_clock::now();

    XRay::Animation::LegacyVisualConversionResult conversion;
    XRay::Animation::LegacyVisualConversionOptions options;
    options.build_skeleton = true;
    options.build_mesh = false;

    xr_string error;
    if (!XRay::Animation::ConvertLegacyVisualToOzzBundle(config.input_ogf, conversion, options, &error))
    {
        throw std::runtime_error("legacy skeleton conversion failed: " + std::string(error.c_str()));
    }

    if (!conversion.skeleton)
        throw std::runtime_error("legacy skeleton conversion returned null skeleton");

    std::error_code ec;
    if (const auto parent = config.output_ozz.parent_path(); !parent.empty())
        fs::create_directories(parent, ec);

    ozz::io::File output(config.output_ozz.string().c_str(), "wb");
    if (!output.opened())
        throw std::runtime_error("failed to open output file: " + config.output_ozz.string());

    ozz::io::OArchive archive(&output);
    archive << *conversion.skeleton;

    if (config.dump_csv)
        dump_bind_pose_csv(*config.dump_csv, conversion.bones);

    const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    std::cout << "Converted skeleton written to " << config.output_ozz << " (" << duration_ms << " ms)" << std::endl;
}

void convert_animation(const AnimationConfig& config)
{
    const auto start_time = std::chrono::steady_clock::now();

    if (fs::exists(fs::weakly_canonical(config.output_ozz)))
    {
        std::cout << "Skipping animation conversion; output already exists at " << config.output_ozz << std::endl;
        return;
    }

    XRay::Animation::LegacyVisualConversionResult skeleton_conversion;
    XRay::Animation::LegacyVisualConversionOptions skeleton_options;
    skeleton_options.build_skeleton = true;
    skeleton_options.build_mesh = false;

    xr_string error;
    if (!XRay::Animation::ConvertLegacyVisualToOzzBundle(config.skeleton_ogf, skeleton_conversion, skeleton_options, &error))
    {
        throw std::runtime_error("legacy skeleton conversion failed: " + std::string(error.c_str()));
    }

    if (!skeleton_conversion.skeleton)
        throw std::runtime_error("legacy skeleton conversion produced null skeleton");

    xr_vector<XRay::Animation::ConvertedOmfAnimation> converted;
    std::optional<xr_string> motion_filter;
    if (config.motion_name)
        motion_filter = xr_string(config.motion_name->c_str());

    if (!XRay::Animation::ConvertLegacyOmf(config.input_omf,
                                           skeleton_conversion.bone_names,
                                           *skeleton_conversion.skeleton,
                                           converted,
                                           motion_filter,
                                           config.optimize))
    {
        throw std::runtime_error("legacy animation conversion failed for: " + config.input_omf.string());
    }

    if (converted.empty())
        throw std::runtime_error("legacy animation conversion produced no animations");

    std::error_code ec;
    if (const auto parent = config.output_ozz.parent_path(); !parent.empty())
        fs::create_directories(parent, ec);

    ozz::io::File output(config.output_ozz.string().c_str(), "wb");
    if (!output.opened())
        throw std::runtime_error("failed to open output animation file: " + config.output_ozz.string());

    ozz::io::OArchive archive(&output);
    const uint32_t animation_count = static_cast<uint32_t>(converted.size());
    archive << animation_count;

    std::vector<MotionMetadata> metadata_to_write;
    metadata_to_write.reserve(converted.size());

    for (const auto& entry : converted)
    {
        if (!entry.animation)
            continue;

        archive << *entry.animation;

        MotionMetadata metadata;
        metadata.name = entry.metadata.name.c_str();
        metadata.flags = entry.metadata.flags;
        metadata.bone_or_part = entry.metadata.bone_or_part;
        metadata.motion_id = entry.metadata.motion_id;
        metadata.speed = entry.metadata.speed;
        metadata.power = entry.metadata.power;
        metadata.accrue = entry.metadata.accrue;
        metadata.falloff = entry.metadata.falloff;
        metadata.marks.reserve(entry.metadata.marks.size());
        for (const auto& mark : entry.metadata.marks)
        {
            MotionMark converted_mark;
            converted_mark.name = mark.name.c_str();
            converted_mark.intervals.reserve(mark.intervals.size());
            for (const auto& interval : mark.intervals)
                converted_mark.intervals.emplace_back(interval.first, interval.second);
            metadata.marks.emplace_back(std::move(converted_mark));
        }

        SerializeMotionMetadata(archive, metadata);
        XRay::Animation::SerializeBoneMotions(archive, entry);
        metadata_to_write.push_back(std::move(metadata));
    }

    fs::path metadata_path = config.metadata_path.value_or(config.output_ozz);
    if (!config.metadata_path)
        metadata_path.replace_extension(".json");
    write_metadata_json(metadata_path, metadata_to_write, config.input_omf);

    const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    if (metadata_to_write.size() == 1)
    {
        std::cout << "Converted animation '" << metadata_to_write.front().name << "' written to " << config.output_ozz << " (" << duration_ms << " ms)" << std::endl;
    }
    else
    {
        std::cout << "Converted " << metadata_to_write.size() << " animations written to " << config.output_ozz << " (" << duration_ms << " ms)" << std::endl;
    }
}

void convert_mesh(const MeshConfig& config)
{
    const auto start_time = std::chrono::steady_clock::now();

    if (fs::exists(fs::weakly_canonical(config.output_ozz)))
    {
        std::cout << "Skipping mesh conversion; output already exists at " << config.output_ozz << std::endl;
        return;
    }

    XRay::Animation::LegacyVisualConversionResult conversion;
    XRay::Animation::LegacyVisualConversionOptions options;
    options.build_skeleton = true;
    options.build_mesh = true;

    xr_string error;
    if (!XRay::Animation::ConvertLegacyVisualToOzzBundle(config.input_ogf, conversion, options, &error))
    {
        throw std::runtime_error("legacy mesh conversion failed: " + std::string(error.c_str()));
    }

    if (conversion.mesh_binary.empty())
        throw std::runtime_error("legacy mesh conversion returned empty mesh payload");

    std::error_code ec;
    if (const auto parent = config.output_ozz.parent_path(); !parent.empty())
        fs::create_directories(parent, ec);

    std::ofstream output(config.output_ozz, std::ios::binary | std::ios::trunc);
    if (!output)
        throw std::runtime_error("failed to open output mesh file: " + config.output_ozz.string());

    output.write(reinterpret_cast<const char*>(conversion.mesh_binary.data()), static_cast<std::streamsize>(conversion.mesh_binary.size()));
    if (!output)
        throw std::runtime_error("failed to write mesh payload to: " + config.output_ozz.string());

    const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    std::cout << "Converted " << conversion.mesh_surface_count << " mesh surface" << (conversion.mesh_surface_count == 1 ? "" : "s")
              << " written to " << config.output_ozz << " (" << duration_ms << " ms)" << std::endl;
}

void convert_bundle(const BundleConfig& config)
{
    const auto start_time = std::chrono::steady_clock::now();

    if (fs::exists(fs::weakly_canonical(config.output_ozzx)))
    {
        std::cout << "Skipping bundle conversion; output already exists at " << config.output_ozzx << std::endl;
        return;
    }

    XRay::Animation::LegacyVisualConversionResult conversion;
    XRay::Animation::LegacyVisualConversionOptions options;
    options.build_skeleton = true;
    options.build_mesh = true;

    xr_string error;
    if (!XRay::Animation::ConvertLegacyVisualToOzzBundle(config.input_ogf, conversion, options, &error))
    {
        throw std::runtime_error("legacy bundle conversion failed: " + std::string(error.c_str()));
    }

    if (conversion.skeleton_binary.empty())
        throw std::runtime_error("legacy bundle conversion produced empty skeleton payload");
    if (conversion.mesh_binary.empty())
        throw std::runtime_error("legacy bundle conversion produced empty mesh payload");

    XRay::Animation::OzzxBundle bundle;
    bundle.version = 2u;
    bundle.skeleton = conversion.skeleton_binary;
    bundle.mesh = conversion.mesh_binary;
    bundle.motion_refs = conversion.motion_refs;
    bundle.bone_metadata = conversion.bone_metadata;
    bundle.user_data = conversion.user_data;
    bundle.embedded_animation_data = conversion.embedded_animation_binary;

    if (!XRay::Animation::WriteOzzxBundle(config.output_ozzx, bundle))
        throw std::runtime_error("failed to write .ozzx bundle: " + config.output_ozzx.string());

    const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    std::cout << "Converted bundle written to " << config.output_ozzx << " (" << duration_ms << " ms, version=" << bundle.version << ")" << std::endl;
}

int main(int argc, char** argv)
{
    std::string command_line;
    command_line.reserve(256);
    for (int i = 0; i < argc; ++i)
    {
        if (i > 0)
            command_line.push_back(' ');
        if (argv[i])
            command_line += argv[i];
    }

    xrDebug::Initialize(command_line.c_str());

    bool core_initialized = false;

    try
    {
        std::string fs_ltx_override;
        int command_start = 1;
        while (command_start < argc) {
            std::string_view opt(argv[command_start] ? argv[command_start] : "");
            if (opt == "-fsltx" || opt == "--fsltx") {
                if (command_start + 1 >= argc) {
                    throw std::runtime_error("-fsltx requires a path argument");
                }
                fs_ltx_override = argv[command_start + 1];
                command_start += 2;
                continue;
            }
            break;
        }

        std::vector<char*> forwarded_args;
        forwarded_args.reserve(static_cast<size_t>(argc - command_start + 1));
        forwarded_args.push_back(argv[0]);
        for (int i = command_start; i < argc; ++i) {
            forwarded_args.push_back(argv[i]);
        }

        int forwarded_argc = static_cast<int>(forwarded_args.size());
        char** forwarded_argv = forwarded_args.data();

        Core.Initialize("xray_to_ozz_converter", command_line.c_str(), true,
            fs_ltx_override.empty() ? nullptr : fs_ltx_override.c_str());
        core_initialized = true;

        struct CoreShutdownGuard
        {
            ~CoreShutdownGuard()
            {
                Core._destroy();
                xrDebug::Finalize();
            }
        } core_shutdown_guard;

        if (forwarded_argc < 2)
            throw std::runtime_error("usage: xray_to_ozz_converter <command> ...\n"
                                     "Commands:\n"
                                     "  skeleton <input.ogf> <output.ozz> [--dump-bind <csv>]\n"
                                     "  animation <input.omf> <output.ozz> <skeleton.ogf> [--motion <name>] [--metadata <json>]\n"
                                     "  mesh <input.ogf> <output.ozz>\n"
                                     "  bundle <input.ogf> <output.ozzx>");

        const std::string command = forwarded_argv[1];
        if (command == "skeleton")
        {
            convert_skeleton(parse_skeleton_arguments(forwarded_argc, forwarded_argv));
        }
        else if (command == "animation")
        {
            convert_animation(parse_animation_arguments(forwarded_argc, forwarded_argv));
        }
        else if (command == "mesh")
        {
            convert_mesh(parse_mesh_arguments(forwarded_argc, forwarded_argv));
        }
        else if (command == "bundle")
        {
            convert_bundle(parse_bundle_arguments(forwarded_argc, forwarded_argv));
        }
        else
        {
            throw std::runtime_error("unknown command: " + command);
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        if (!core_initialized)
            xrDebug::Finalize();
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "ERROR: unknown failure" << std::endl;
        if (!core_initialized)
            xrDebug::Finalize();
        return EXIT_FAILURE;
    }
}
