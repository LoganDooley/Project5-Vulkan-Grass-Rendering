#include "settings_ui.h"

#include <imgui.h>
#include <glm/trigonometric.hpp>

#include "render_settings.h"
#include "frame_stats.h"

void SettingsUI::drawSettingsPanel(RenderSettings &renderSettings, const FrameStats &stats)
{
    ImGui::Begin("Render Settings");

    if(ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::Text("Frame time: %.2f ms (%.0f fps)", stats.cpuFrameMs, 1000.0f / stats.cpuFrameMs);
        ImGui::Text("GPU total: %.2f ms (sim %.2f, grass %.2f)", stats.gpuTotalMs, stats.gpuSimMs, stats.gpuGrassMs);
        ImGui::PlotLines("##frames", stats.frameHistory.data(), (int)stats.frameHistory.size(), (int)stats.historyIndex, "frame ms", 0.0f, 20.0f, ImVec2(0, 60));
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        const glm::vec3& p = stats.cameraPosition;
        // Convert to degrees so it is easier to understand
        float yawDegrees = glm::degrees(stats.cameraYaw);
        float pitchDegrees = glm::degrees(stats.cameraPitch);
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", p.x, p.y, p.z);
        ImGui::Text("Yaw: %.1f degrees  Pitch: %.1f degrees", yawDegrees, pitchDegrees);
    }

    if(ImGui::CollapsingHeader("Performance Settings", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::Checkbox("VSync Enabled", &renderSettings.bUseVsync);
    }

    if(ImGui::CollapsingHeader("Grass Settings", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::SliderFloat("Density (blades/m²)", &renderSettings.grassDensity, 1.0f, 400.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Radius (m)", &renderSettings.grassRadius, 5.0f, 200.0f, "%.0f");
        ImGui::Text("Blades: %u (%.1f MB)", stats.bladeCount, stats.bladeCount * 64.0f / (1024.0f * 1024.0f));
        float percentCulled = stats.bladeCount > 0
            ? 100.0f * (1.0f - static_cast<float>(stats.visibleBladeCount) / static_cast<float>(stats.bladeCount))
            : 0.0f;
        ImGui::Text("Visible: %u / %u (%.1f%% culled)", stats.visibleBladeCount, stats.bladeCount, percentCulled);
    }

    if(ImGui::CollapsingHeader("Culling Settings", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::Checkbox("Freeze Culling", &renderSettings.bFreezeCulling);
        ImGui::Checkbox("Orientation Culling", &renderSettings.bOrientationCullingEnabled);
        ImGui::Checkbox("Frustum Culling", &renderSettings.bFrustumCullingEnabled);
        ImGui::Checkbox("Distance Culling", &renderSettings.bDistanceCullingEnabled);
        ImGui::SliderFloat("Distance Culling Max Distance", &renderSettings.distanceCullingMaxDistance, 1.0f, 500.f);
    }

    if(ImGui::CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::Checkbox("Simulation Enabled", &renderSettings.bSimulate);
        ImGui::SliderFloat("Wind Strength", &renderSettings.windStrength, 0.0f, 50.f);
        ImGui::SliderFloat("Gust Strength", &renderSettings.gustStrength, 0.0f, 50.f);
        ImGui::SliderFloat("Gravity", &renderSettings.gravity, -10.0, 50.f);
    }

    if (ImGui::CollapsingHeader("Terrain Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* types[] = { "Plane", "Heightmap", "Voxel" };
        int type = static_cast<int>(renderSettings.terrainType);
        if (ImGui::Combo("Mode", &type, types, IM_ARRAYSIZE(types))) {
            renderSettings.terrainType = static_cast<TerrainType>(type);
        }
    }

    ImGui::End();
}