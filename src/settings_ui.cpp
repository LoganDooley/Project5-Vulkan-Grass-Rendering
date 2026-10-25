#include "settings_ui.h"

#include <imgui.h>

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

    if(ImGui::CollapsingHeader("Culling Settings", ImGuiTreeNodeFlags_DefaultOpen)){
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

    ImGui::End();
}