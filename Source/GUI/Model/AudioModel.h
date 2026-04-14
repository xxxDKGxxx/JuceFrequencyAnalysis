#pragma once

#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <memory>

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

private:
  std::unique_ptr<juce::AudioBuffer<float>> audioBuffer;
  double sampleRate;
  unsigned int bitsPerSample;
  unsigned int numChannels;
  juce::int64 lengthInSamples;

  std::vector<float> visualWaveform;
  std::vector<float> visualTime;

  void calculateVisualWaveform();
};
