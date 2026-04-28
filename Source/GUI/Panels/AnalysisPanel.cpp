#include "AnalysisPanel.h"
#include "imgui.h"
#include "implot.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

bool AnalysisPanel::normalise = true;

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

  // 1.7 Spectrogram overlap (hop control)
  float overlapPercent = pAudioModel->getSpectrogramOverlap() * 100.0f;
  if (ImGui::SliderFloat("Spectrogram overlap (%)", &overlapPercent, 0.0f,
                         90.0f, "%.0f")) {
    pAudioModel->setSpectrogramOverlap(overlapPercent / 100.0f);
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

    ImGui::Checkbox("Normalised?", &AnalysisPanel::normalise);

    float trendsHeight = 300.0f;
    const char *plotTitle = normalise ? "Global Feature Trends (Normalized)"
                                      : "Global Feature Trends";
    if (ImPlot::BeginPlot(plotTitle, ImVec2(-1, trendsHeight))) {
      ImPlot::SetupAxisLimits(ImAxis_X1, 0, pAudioModel->getLengthInSeconds());
      if (normalise) {
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1.1);
        ImPlot::SetupAxes("Time (s)", "Normalized Value");
      } else {
        ImPlot::SetupAxes("Time (s)", "Value");
      }

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

      if (normalise) {
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
      } else {
        ImPlot::PlotLine("Volume", series.time.data(), series.volume.data(),
                         static_cast<int>(series.time.size()));
        ImPlot::PlotLine("Centroid", series.time.data(), series.centroid.data(),
                         static_cast<int>(series.time.size()));
        ImPlot::PlotLine("Bandwidth", series.time.data(),
                         series.bandwidth.data(),
                         static_cast<int>(series.time.size()));
        ImPlot::PlotLine("Flatness", series.time.data(), series.flatness.data(),
                         static_cast<int>(series.time.size()));
        ImPlot::PlotLine("Crest Factor", series.time.data(),
                         series.crestFactor.data(),
                         static_cast<int>(series.time.size()));
      }

      ImPlot::EndPlot();
    }

    ImGui::Separator();

    if (!series.f0Cepstrum.empty()) {
      float f0PlotHeight = 260.0f;
      if (ImPlot::BeginPlot("Glottal Frequency (Cepstrum)",
                            ImVec2(-1, f0PlotHeight))) {
        ImPlot::SetupAxes("Time (s)", "Frequency (Hz)");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, pAudioModel->getLengthInSeconds(),
                                ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 50, 400, ImGuiCond_Always);
        ImPlot::PlotLine("F0", series.time.data(), series.f0Cepstrum.data(),
                         static_cast<int>(series.time.size()));
        ImPlot::EndPlot();
      }
    }
  }

  ImGui::Separator();

  // 2 & 3. Side-by-side plots: Windowed Signal and Magnitude Spectrum
  const auto &windowedSignal = pAudioModel->getWindowedSignal();
  const auto &magnitudeSpectrum = pAudioModel->getMagnitudeSpectrum();

  float plotWidth =
      (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) *
      0.5f;
  float plotHeight = 260.0f;

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

  ImGui::Separator();

  const auto &spectrogram = pAudioModel->getSpectrogramData();
  float spectrogramHeight = std::max(420.0f, ImGui::GetContentRegionAvail().y);

  if (!spectrogram.valuesDb.empty() && spectrogram.timeBins > 0 &&
      spectrogram.freqBins > 0) {
    if (ImPlot::BeginPlot("Spectrogram (dB)", ImVec2(-1, spectrogramHeight))) {
      ImPlot::SetupAxes("Time (s)", "Frequency (Hz)");
      ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, spectrogram.durationSeconds,
                              ImGuiCond_Always);
      ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, spectrogram.maxFrequencyHz,
                              ImGuiCond_Always);

      ImPlot::PushColormap(ImPlotColormap_Viridis);
      ImPlot::PlotHeatmap("##spec", spectrogram.valuesDb.data(),
                          spectrogram.freqBins, spectrogram.timeBins,
                          spectrogram.minDb, spectrogram.maxDb, nullptr,
                          ImPlotPoint(0.0, 0.0),
                          ImPlotPoint(spectrogram.durationSeconds,
                                      spectrogram.maxFrequencyHz));
      ImPlot::PopColormap();

      ImPlot::EndPlot();
    }
  } else {
    ImGui::TextDisabled("Spectrogram unavailable for current signal.");
  }
}
