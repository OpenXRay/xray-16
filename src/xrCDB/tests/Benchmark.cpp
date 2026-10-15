#include "TestSupport.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>

namespace
{
constexpr int gridSize = 128;
constexpr size_t queryCount = 20000;
constexpr int repetitions = 5;
using Clock = std::chrono::steady_clock;

Fvector TerrainPoint(int x, int z)
{
    return Vector(float(x), .25f * std::sin(float(x) * .1f) * std::cos(float(z) * .1f), float(z));
}

template <typename Work>
double MedianMilliseconds(Work work)
{
    work(); // Warm-up is not measured.
    std::array<double, repetitions> timings;
    for (auto& timing : timings)
    {
        const auto begin = Clock::now();
        work();
        timing = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
    }
    std::sort(timings.begin(), timings.end());
    return timings[timings.size() / 2];
}

void Benchmark(pcstr output)
{
    Mesh mesh;
    for (int z = 0; z < gridSize; z++)
    {
        for (int x = 0; x < gridSize; x++)
        {
            mesh.Add(TerrainPoint(x, z), TerrainPoint(x, z + 1), TerrainPoint(x + 1, z));
            mesh.Add(TerrainPoint(x + 1, z + 1), TerrainPoint(x + 1, z), TerrainPoint(x, z + 1));
        }
    }

    const double buildMilliseconds = MedianMilliseconds([&]
    {
        CDB::MODEL model;
        mesh.Build(model);
        model.syncronize();
    }); // Includes destruction, equally for both backends.

    CDB::MODEL model;
    mesh.Build(model);
    model.syncronize();
    model.set_model_crc32(2139);

    std::mt19937 random(2139);
    std::uniform_real_distribution<float> coordinate(3.f, float(gridSize) - 3.f);
    std::vector<Fvector> positions;
    positions.reserve(queryCount);
    for (size_t i = 0; i < queryCount; i++)
    {
        const float x = coordinate(random);
        const float z = coordinate(random);
        positions.push_back(Vector(x, 0, z));
    }

    CDB::COLLIDER collider;
    double rayDistanceSum = 0;
    const double rayMilliseconds = MedianMilliseconds([&]
    {
        rayDistanceSum = 0;
        for (const auto& position : positions)
        {
            collider.ray_query(CDB::OPT_ONLYNEAREST | CDB::OPT_CULL, &model,
                Vector(position.x, 10, position.z), Vector(0, -1, 0), 20);
            Require(collider.r_count() == 1, "Benchmark ray missed the terrain");
            rayDistanceSum += collider.r_begin()->range;
        }
    });

    size_t boxHits = 0;
    const double boxMilliseconds = MedianMilliseconds([&]
    {
        boxHits = 0;
        for (const auto& position : positions)
        {
            collider.box_query(CDB::OPT_FULL_TEST, &model, position, Vector(2, 2, 2));
            Require(collider.r_count() != 0, "Benchmark box missed the terrain");
            boxHits += collider.r_count();
        }
    });

    const auto cache = TestPath("cdb-benchmark.cache");
#ifdef XRAY_USE_JOLT_CDB
    const std::string storedCache = cache + ".jolt";
    constexpr pcstr backend = "Jolt";
#else
    const std::string storedCache = cache;
    constexpr pcstr backend = "OPCODE";
#endif
    struct CacheCleanup
    {
        const std::string& path;
        ~CacheCleanup()
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    } cleanup{ storedCache };
    Require(model.serialize(cache.c_str()), "Could not write benchmark cache");
    const double cacheMilliseconds = MedianMilliseconds([&]
    {
        CDB::MODEL loaded;
        loaded.set_model_crc32(2139);
        Require(loaded.deserialize(cache.c_str()), "Could not load benchmark cache");
    }); // Includes CRC validation, tree restoration/rebuild and destruction.

    std::ofstream report(output);
    Require(report.is_open(), "Could not open benchmark report");
    report << std::fixed << std::setprecision(3)
           << "{\n  \"backend\": \"" << backend << "\",\n"
           << "  \"triangles\": " << mesh.triangles.size() << ",\n"
           << "  \"queries_per_sample\": " << queryCount << ",\n"
           << "  \"samples\": " << repetitions << ",\n"
           << "  \"build_destroy_median_ms\": " << buildMilliseconds << ",\n"
           << "  \"nearest_ray_median_ms\": " << rayMilliseconds << ",\n"
           << "  \"full_box_median_ms\": " << boxMilliseconds << ",\n"
           << "  \"warm_cache_load_destroy_median_ms\": " << cacheMilliseconds << ",\n"
           << "  \"model_reported_bytes\": " << model.memory() << ",\n"
           << "  \"cache_bytes\": " << std::filesystem::file_size(storedCache) << ",\n"
           << "  \"ray_distance_sum\": " << rayDistanceSum << ",\n"
           << "  \"box_hits\": " << boxHits << "\n}\n";
    report.close();
    Require(!report.fail(), "Could not write benchmark report");
}
}

int main(int argc, char** argv)
{
    Core.Initialize("xrCDBBenchmark", "", false);
    int result = 0;
    try
    {
        Benchmark(argc > 1 ? argv[1] : "cdb-benchmark.json");
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        result = 1;
    }
    Core._destroy();
    return result;
}
