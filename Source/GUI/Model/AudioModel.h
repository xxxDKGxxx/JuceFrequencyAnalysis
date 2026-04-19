#pragma once

#include "../../Analysis/Parameters/AudioFeatures.h"
#include "../../Analysis/WindowFunctions.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include <memory>
#include <vector>

class AudioModel {
public:
  AudioModel(std::unique_ptr<juce::AudioBuffer<float>> audioBuffer,
             double sampleRate, unsigned int bitsPerSample,
             unsigned int numChannels, juce::int64 lengthInSamples);

  const juce::AudioBuffer<float> &getAudioBuffer() const;
  double getSampleRate() const;
  unsigned int getBitsPerSample() const;
  unsigned int getNumChannels() const;
  juce::int64 getLengthInSamples() const;
  double getLengthInSeconds() const;

  struct Peak {
    float min;
    float max;
  };

  const std::vector<float> &getVisualWaveform() const;
  const std::vector<float> &getVisualTime() const;

  // Analysis
  void setSelection(double startSeconds, double endSeconds);
  void setWindowType(WindowFunctions::Type type);
  void setFrameSize(int newSize);
  void setSpectrogramOverlap(float newOverlap);

  const std::vector<float> &getMagnitudeSpectrum() const {
    return magnitudeSpectrum;
  }
  const std::vector<float> &getWindowedSignal() const { return windowedSignal; }
  const AudioFeatures &getFeatures() const { return features; }

  double getSelectionStart() const { return selectionStart; }
  double getSelectionEnd() const { return selectionEnd; }
  WindowFunctions::Type getWindowType() const { return windowType; }
  int getFrameSize() const { return frameSize; }
  float getSpectrogramOverlap() const { return spectrogramOverlap; }

  // Global Time-Series Features
  struct GlobalSeries {
    std::vector<float> time;
    std::vector<float> volume;
    std::vector<float> centroid;
    std::vector<float> bandwidth;
    std::vector<float> flatness;
    std::vector<float> crestFactor;
  };
  const GlobalSeries &getGlobalSeries() const { return globalSeries; }

  struct SpectrogramData {
    std::vector<float> valuesDb;
    int timeBins = 0;
    int freqBins = 0;
    double durationSeconds = 0.0;
    double maxFrequencyHz = 0.0;
    float minDb = -120.0f;
    float maxDb = 0.0f;
  };
  const SpectrogramData &getSpectrogramData() const { return spectrogramData; }

private:
  std::unique_ptr<juce::AudioBuffer<float>> audioBuffer;
  double sampleRate;
  unsigned int bitsPerSample;
  unsigned int numChannels;
  juce::int64 lengthInSamples;

  std::vector<float> visualWaveform;
  std::vector<float> visualTime;

  // Analysis Data
  double selectionStart = 0.0;
  double selectionEnd = 0.0;
  WindowFunctions::Type windowType = WindowFunctions::Type::Rectangular;
  int frameSize = 512;
  float spectrogramOverlap = 0.5f;

  std::vector<float> windowedSignal;
  std::vector<float> magnitudeSpectrum;
  AudioFeatures features;

  GlobalSeries globalSeries;
  SpectrogramData spectrogramData;

  void calculateVisualWaveform();
  void updateAnalysis();
  void calculateGlobalFeatures();
  void calculateSpectrogram();
};
