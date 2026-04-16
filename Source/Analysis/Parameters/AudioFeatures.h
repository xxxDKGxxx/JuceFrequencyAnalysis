#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

struct AudioFeatures {
  float volume = 0.0f;
  float frequencyCentroid = 0.0f;
  float effectiveBandwidth = 0.0f;
  std::vector<float> bandEnergy;
  std::vector<float> bandEnergyRatio;
  float spectralFlatness = 0.0f;
  float spectralCrestFactor = 0.0f;

  static AudioFeatures calculate(const std::vector<float> &magnitudeSpectrum,
                                 double sampleRate) {
    AudioFeatures features;
    size_t N = magnitudeSpectrum.size();
    if (N == 0)
      return features;

    std::vector<float> powerSpectrum(N);
    float totalPower = 0.0f;
    float totalMagnitude = 0.0f;
    for (size_t i = 0; i < N; ++i) {
      powerSpectrum[i] = magnitudeSpectrum[i] * magnitudeSpectrum[i];
      totalPower += powerSpectrum[i];
      totalMagnitude += magnitudeSpectrum[i];
    }

    // 1. Volume
    features.volume = totalPower / static_cast<float>(N);

    if (totalMagnitude > 1e-9f) {
      // 2. Frequency Centroid
      float weightedSum = 0.0f;
      for (size_t i = 0; i < N; ++i) {
        float freq = static_cast<float>(i * (sampleRate / 2.0) / (N - 1));
        weightedSum += freq * magnitudeSpectrum[i];
      }
      features.frequencyCentroid = weightedSum / totalMagnitude;

      // 3. Effective Bandwidth
      if (totalPower > 1e-9f) {
        float weightedVarSum = 0.0f;
        for (size_t i = 0; i < N; ++i) {
          float freq = static_cast<float>(i * (sampleRate / 2.0) / (N - 1));
          float diff = freq - features.frequencyCentroid;
          weightedVarSum += diff * diff * powerSpectrum[i];
        }
        features.effectiveBandwidth = std::sqrt(weightedVarSum / totalPower);
      }
    }

    // 4 & 5. Band Energy & BER
    // Sub-bands: 0-630 Hz, 630-1720 Hz, 1720-4400 Hz, 4400-11025 Hz
    std::vector<float> subBandEdges = {0.0f, 630.0f, 1720.0f, 4400.0f,
                                       11025.0f};
    features.bandEnergy.resize(4, 0.0f);
    features.bandEnergyRatio.resize(4, 0.0f);

    float binWidth = static_cast<float>((sampleRate / 2.0) / (N - 1));
    for (int b = 0; b < 4; ++b) {
      float f0 = subBandEdges[b];
      float f1 = subBandEdges[b + 1];
      int i0 = static_cast<int>(std::round(f0 / binWidth));
      int i1 = static_cast<int>(std::round(f1 / binWidth));
      i0 = std::clamp(i0, 0, static_cast<int>(N - 1));
      i1 = std::clamp(i1, 0, static_cast<int>(N - 1));

      float bandPower = 0.0f;
      for (int i = i0; i <= i1; ++i) {
        bandPower += powerSpectrum[i];
      }
      features.bandEnergy[b] = bandPower / static_cast<float>(N);
      if (features.volume > 1e-9f) {
        features.bandEnergyRatio[b] = features.bandEnergy[b] / features.volume;
      }
    }

    // 6. Spectral Flatness (Geometric Mean / Arithmetic Mean)
    if (totalPower > 1e-9f) {
      double logSum = 0.0;
      for (size_t i = 0; i < N; ++i) {
        logSum += std::log(std::max(powerSpectrum[i], 1e-12f));
      }
      float geoMean = std::exp(static_cast<float>(logSum / N));
      float ariMean = totalPower / static_cast<float>(N);
      features.spectralFlatness = geoMean / ariMean;
    } else {
      features.spectralFlatness = 1.0f; // As per MPEG7
    }

    // 7. Spectral Crest Factor (Max / Mean)
    if (totalPower > 1e-9f) {
      float maxPower =
          *std::max_element(powerSpectrum.begin(), powerSpectrum.end());
      float ariMean = totalPower / static_cast<float>(N);
      features.spectralCrestFactor = maxPower / ariMean;
    }

    return features;
  }
};
