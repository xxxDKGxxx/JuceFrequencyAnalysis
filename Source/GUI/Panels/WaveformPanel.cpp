#include "WaveformPanel.h"
#include "imgui.h"
#include "implot.h"
#include <algorithm>

void WaveformPanel::render(const AudioModel *pAudioModel, int width,
                           int height) {
  if (pAudioModel == nullptr || pAudioModel->getLengthInSamples() <= 0) {
    ImGui::TextDisabled("No audio loaded.");
    return;
  }

  if (ImPlot::BeginPlot("Audio Waveform", ImVec2(0.7 * width, 0.25 * height))) {
    // Setup axis limits for the first load
    ImPlot::SetupAxisLimits(ImAxis_X1, 0, pAudioModel->getLengthInSeconds(),
                            ImGuiCond_FirstUseEver);

    // Get current visible range
    auto limits = ImPlot::GetPlotLimits();
    double xMin = limits.X.Min;
    double xMax = limits.X.Max;
    double visibleDuration = xMax - xMin;
    double sampleRate = pAudioModel->getSampleRate();

    // Decision: How many samples are in the visible range?
    double visibleSamples = visibleDuration * sampleRate;

    // If more than 100,000 samples are visible, use the thumbnail
    if (visibleSamples > 100000) {
      const auto &visualWaveform = pAudioModel->getVisualWaveform();
      const auto &visualTime = pAudioModel->getVisualTime();

      if (!visualWaveform.empty()) {
        ImPlot::PlotLine("Waveform", visualTime.data(), visualWaveform.data(),
                         static_cast<int>(visualWaveform.size()));
      }
    } else {
      // Zoomed in: Plot raw samples for the visible range only
      int startSample =
          std::max(0, static_cast<int>(std::floor(xMin * sampleRate)));
      int endSample =
          std::min(static_cast<int>(pAudioModel->getLengthInSamples()),
                   static_cast<int>(std::ceil(xMax * sampleRate)));

      int count = endSample - startSample;
      if (count > 1) {
        const float *pRead =
            pAudioModel->getAudioBuffer().getReadPointer(0) + startSample;
        ImPlot::PlotLine("Signal", pRead, count, 1.0 / sampleRate,
                         static_cast<double>(startSample) / sampleRate);
      }
    }

    ImPlot::EndPlot();
  }
}
