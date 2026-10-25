#include "benchmark.h"

#include <glm/trigonometric.hpp>

#include "frame_stats.h"
#include "render_settings.h"
#include "camera.h"

#include <string>
#include <utility>
#include <stdexcept>
#include <iostream>

static constexpr uint32_t WARMUP_FRAMES = 120;
static constexpr uint32_t MEASURE_FRAMES = 300;

Benchmark::Benchmark(std::vector<BenchmarkRun> runs, const char *outputPath) :
    m_runs(std::move(runs)),
    m_file(outputPath)
{
    if(!m_file.is_open()){
        throw std::runtime_error(std::string("Failed to open benchmark outputPath: ") + outputPath);
    }

    m_file << "experiment,view,density,blades,orientation,frustum,distance,distance_max,"
              "sim_ms,grass_ms,total_ms,visible\n";
}

bool Benchmark::update(const FrameStats &stats, RenderSettings &settings, Camera &camera)
{
    if(m_runIndex >= m_runs.size()){
        return false;
    }

    const BenchmarkRun& run = m_runs[m_runIndex];

    if(m_frame == 0){
        settings.bUseVsync = false;
        settings.bSimulate = true;
        settings.bFreezeCulling = false;
        settings.grassRadius = 100.0f;

        settings.grassDensity = run.density;
        settings.bOrientationCullingEnabled = run.bOrientationCullingEnabled;
        settings.bFrustumCullingEnabled = run.bFrustumCullingEnabled;
        settings.bDistanceCullingEnabled = run.bDistanceCullingEnabled;
        settings.distanceCullingMaxDistance = run.distanceCullingMaxDistance;

        camera.setView(run.cameraView.position, glm::radians(run.cameraView.yawDegrees), glm::radians(run.cameraView.pitchDegrees));

        m_simAccumMs = 0.0f;
        m_grassAccumMs = 0.0f;
        m_totalAccumMs = 0.0f;
        m_visibleAccum = 0;
        m_sampleCount = 0;
    }
    else if(m_frame > WARMUP_FRAMES) {
        m_simAccumMs += stats.gpuSimMs;
        m_grassAccumMs += stats.gpuGrassMs;
        m_totalAccumMs += stats.gpuTotalMs;
        m_visibleAccum += stats.visibleBladeCount;
        m_sampleCount++;
    }
    m_frame++;

    if(m_frame > WARMUP_FRAMES + MEASURE_FRAMES){
        writeRow(run, stats.bladeCount);
        std::cout << "Benchmark run " << m_runIndex + 1 << "/" << m_runs.size() << " done\n";
        m_runIndex++;
        m_frame = 0;
        return m_runIndex < m_runs.size();
    }
    return true;
}

std::vector<BenchmarkRun> Benchmark::makeBenchmarkRuns()
{
    const CameraView views[] = {
        {
            .name = "Horizon",
            .position = glm::vec3(0.0, 1.7, 0.0),
            .yawDegrees = 0.0f,
            .pitchDegrees = 0.0f,
        },
        {
            .name = "Top-Down",
            .position = glm::vec3(0.0, 100.0, 0.0),
            .yawDegrees = 0.0f,
            .pitchDegrees = -85.0f,
        },
        {
            .name = "Corner",
            .position = glm::vec3(100.0, 1.7, 100.0),
            .yawDegrees = -45.0f,
            .pitchDegrees = -5.0f,
        }
    };

    std::vector<BenchmarkRun> runs;

    for (float density : {1.f, 2.f, 4.f, 8.f, 16.f, 32.f, 64.f, 128.f, 256.f}) {
        runs.push_back({ "blade_count", views[0], density, true, true, true, 30.0f });
        runs.push_back({ "blade_count", views[0], density, false, false, false, 30.0f });
    }

    struct { bool orientation, frustum, distance; } cullingConfigs[] = {
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {1, 1, 1}
    };
    for (const CameraView& view : views){
        for (const auto& cullingConfig : cullingConfigs){
            runs.push_back({ "culling", view, 64.0f, cullingConfig.orientation, cullingConfig.frustum, cullingConfig.distance, 30.0f });
        }
    }

    for (float maxDistance : {10.f, 20.f, 30.f, 50.f, 70.f}){
        runs.push_back({ "distance", views[0], 64.0f, false, false, true, maxDistance });
    }

    return runs;
}

void Benchmark::writeRow(const BenchmarkRun &run, uint32_t blades)
{
    double n = static_cast<double>(m_sampleCount);
    m_file << run.experimentName << ',' << run.cameraView.name << ',' << run.density << ',' << blades << ','
       << run.bOrientationCullingEnabled << ',' << run.bFrustumCullingEnabled << ',' << run.bDistanceCullingEnabled << ','
       << run.distanceCullingMaxDistance << ','
       << m_simAccumMs / n << ',' << m_grassAccumMs / n << ',' << m_totalAccumMs / n << ','
       << static_cast<double>(m_visibleAccum) / n << std::endl;
}
