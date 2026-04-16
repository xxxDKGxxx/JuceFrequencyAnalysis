#include "AnalysisPanel.h"
#include "imgui.h"
#include "implot.h"
#include <string>
#include <vector>

void AnalysisPanel::render(AudioModel *pAudioModel, int width, int height) {
  if (pAudioModel == nullptr || pAudioModel->getLengthInSamples() <= 0) {
    ImGui::TextDisabled("No audio loaded.");
    return;
  }

  // 1. Window Selection
  const char *windowNames[] = {"Rectangular", "Triangular", "Hamming",
                               "Hann",        "Blackman",   "Nuttall"};
  int currentWindow = static_cast<int>(pAudioModel->getWindowType());
  if (ImGui::Combo("Window Function", &currentWindow, windowNames,
                   IM_ARRAYSIZE(windowNames))) {
    pAudioModel->setWindowType(
        static_cast<WindowFunctions::Type>(currentWindow));
  }

  ImGui::Separator();

  // 1.5 Frame Size Selection
  ImGui::Text("Global Analysis Frame Size:");
  int frameSizes[] = {256, 512, 1024};
  int currentFrameSize = pAudioModel->getFrameSize();
  for (int i = 0; i < 3; ++i) {
    if (ImGui::RadioButton(std::to_string(frameSizes[i]).c_str(),
                           currentFrameSize == frameSizes[i])) {
      pAudioModel->setFrameSize(frameSizes[i]);
    }
    if (i < 2)
      ImGui::SameLine();
  }

  ImGui::Separator();

  // 1.6 Global Trends Plot (Time-Series)
  const auto &series = pAudioModel->getGlobalSeries();
  if (!series.time.empty()) {
    // Use half of available space for trends, or fixed fraction
    float trendsHeight = ImGui::GetContentRegionAvail().y * 0.45f;
    if (ImPlot::BeginPlot("Global Feature Trends (Normalized)",
                          ImVec2(-1, trendsHeight))) {
      ImPlot::SetupAxisLimits(ImAxis_X1, 0, pAudioModel->getLengthInSeconds());
      ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1.1);
      ImPlot::SetupAxes("Time (s)", "Normalized Value");

      auto normalize = [](const std::vector<float> &data) {
        if (data.empty())
          return data;
        float maxVal = *std::max_element(data.begin(), data.end());
        if (maxVal < 1e-9f)
          return data;
        std::vector<float> norm(data.size());
        for (size_t i = 0; i < data.size(); ++i)
          norm[i] = data[i] / maxVal;
        return norm;
      };

      // Plotting normalized versions to see trends together
      auto v_norm = normalize(series.volume);
      auto c_norm = normalize(series.centroid);
      auto b_norm = normalize(series.bandwidth);
      auto f_norm = series.flatness; // Flatness is already 0..1
      auto cr_norm = normalize(series.crestFactor);

      ImPlot::PlotLine("Volume", series.time.data(), v_norm.data(),
                       static_cast<int>(series.time.size()));
      ImPlot::PlotLine("Centroid", series.time.data(), c_norm.data(),
                       static_cast<int>(series.time.size()));
      ImPlot::PlotLine("Bandwidth", series.time.data(), b_norm.data(),
                       static_cast<int>(series.time.size()));
      ImPlot::PlotLine("Flatness", series.time.data(), f_norm.data(),
                       static_cast<int>(series.time.size()));
      ImPlot::PlotLine("Crest Factor", series.time.data(), cr_norm.data(),
                       static_cast<int>(series.time.size()));

      ImPlot::EndPlot();
    }
  }

  ImGui::Separator();

  // 2 & 3. Side-by-side plots: Windowed Signal and Magnitude Spectrum
  const auto &windowedSignal = pAudioModel->getWindowedSignal();
  const auto &magnitudeSpectrum = pAudioModel->getMagnitudeSpectrum();

  float plotWidth =
      (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) *
      0.5f;
  float plotHeight = ImGui::GetContentRegionAvail().y - 10;

  if (!windowedSignal.empty()) {
    if (ImPlot::BeginPlot("Windowed Signal", ImVec2(plotWidth, plotHeight))) {
      ImPlot::SetupAxisLimits(ImAxis_X1, 0,
                              static_cast<double>(windowedSignal.size()) /
                                  pAudioModel->getSampleRate());
      ImPlot::PlotLine("Signal", windowedSignal.data(),
                       static_cast<int>(windowedSignal.size()),
                       1.0 / pAudioModel->getSampleRate());
      ImPlot::EndPlot();
    }
  }

  if (!magnitudeSpectrum.empty()) {
    ImGui::SameLine();
    if (ImPlot::BeginPlot("Magnitude Spectrum",
                          ImVec2(plotWidth, plotHeight))) {
      double binWidth =
          (pAudioModel->getSampleRate() / 2.0) / (magnitudeSpectrum.size() - 1);
      ImPlot::SetupAxisLimits(ImAxis_X1, 0, pAudioModel->getSampleRate() / 2.0);
      ImPlot::PlotLine("Spectrum", magnitudeSpectrum.data(),
                       static_cast<int>(magnitudeSpectrum.size()), binWidth);
      ImPlot::EndPlot();
    }
  }
}
