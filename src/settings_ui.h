#pragma once

struct RenderSettings;
struct FrameStats;

namespace SettingsUI {
    void drawSettingsPanel(RenderSettings& renderSettings, const FrameStats& stats);
};