#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <vector> 
#include <fstream>

struct FrameStats;
struct RenderSettings;
class Camera;

struct CameraView {
    const char* name = "";
    glm::vec3 position = glm::vec3(0.0f);
    float yawDegrees = 0.0f;
    float pitchDegrees = 0.0f;
};

struct BenchmarkRun {
    const char* experimentName = "";
    CameraView cameraView{};
    float density = 0.0f;
    bool bOrientationCullingEnabled = true;
    bool bFrustumCullingEnabled = true;
    bool bDistanceCullingEnabled = true;
    float distanceCullingMaxDistance = 30.0f;
};

class Benchmark {
public:
    Benchmark(std::vector<BenchmarkRun> runs, const char* outputPath);

    bool update(const FrameStats& stats, RenderSettings& settings, Camera& camera);

    static std::vector<BenchmarkRun> makeBenchmarkRuns();

    uint32_t getCurrentRun() const { return static_cast<uint32_t>(m_runIndex); }
    uint32_t getRunCount() const { return static_cast<uint32_t>(m_runs.size()); }

private:
    void writeRow(const BenchmarkRun& run, uint32_t blades);

    std::vector<BenchmarkRun> m_runs;
    std::ofstream m_file;
    size_t m_runIndex = 0;
    uint32_t m_frame = 0;

    double m_simAccumMs = 0.0f;
    double m_grassAccumMs = 0.0f;
    double m_totalAccumMs = 0.0f;
    uint64_t m_visibleAccum = 0;
    uint32_t m_sampleCount = 0;
};