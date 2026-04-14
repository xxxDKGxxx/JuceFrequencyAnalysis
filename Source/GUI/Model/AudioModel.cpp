#include "AudioModel.h"
#include "juce_core/juce_core.h"
#include <algorithm>

AudioModel::AudioModel(std::unique_ptr<juce::AudioBuffer<float>> audioBuffer,
                       double sampleRate, unsigned int bitsPerSample,
                       unsigned int numChannels, juce::int64 lengthInSamples)
    : audioBuffer(std::move(audioBuffer)), sampleRate(sampleRate),
      bitsPerSample(bitsPerSample), numChannels(numChannels),
      lengthInSamples(lengthInSamples) {
  calculateVisualWaveform();
}
const juce::AudioBuffer<float> &AudioModel::getAudioBuffer() const {
  return *audioBuffer;
}
double AudioModel::getSampleRate() const { return sampleRate; };
unsigned int AudioModel::getBitsPerSample() const { return bitsPerSample; };
unsigned int AudioModel::getNumChannels() const { return numChannels; };
juce::int64 AudioModel::getLengthInSamples() const { return lengthInSamples; };
double AudioModel::getLengthInSeconds() const {
  if (sampleRate > 0.0)
    return static_cast<double>(lengthInSamples) / sampleRate;
  return 0.0;
}

const std::vector<float> &AudioModel::getVisualWaveform() const {
  return visualWaveform;
}
const std::vector<float> &AudioModel::getVisualTime() const {
  return visualTime;
}

void AudioModel::calculateVisualWaveform() {
  if (lengthInSamples == 0)
    return;

  // We want roughly 4000 points for the visualization (2000 min-max pairs)
  const int numBuckets = 2000;
  const int samplesPerBucket =
      std::max(1, static_cast<int>(lengthInSamples / numBuckets));

  visualWaveform.clear();
  visualWaveform.reserve(numBuckets * 2);
  visualTime.clear();
  visualTime.reserve(numBuckets * 2);

  const float *pRead = audioBuffer->getReadPointer(0);

  for (int i = 0; i < numBuckets; ++i) {
    int startSample = i * samplesPerBucket;
    int endSample = std::min(static_cast<int>(lengthInSamples),
                             (i + 1) * samplesPerBucket);

    if (startSample >= lengthInSamples)
      break;

    float minVal = 0.0f;
    float maxVal = 0.0f;

    for (int s = startSample; s < endSample; ++s) {
      float val = pRead[s];
      if (val < minVal)
        minVal = val;
      if (val > maxVal)
        maxVal = val;
    }

    // Add min and max points for this bucket to create the waveform look
    visualWaveform.push_back(minVal);
    visualTime.push_back(static_cast<float>(startSample) /
                         static_cast<float>(sampleRate));

    visualWaveform.push_back(maxVal);
    visualTime.push_back(static_cast<float>(startSample) /
                         static_cast<float>(sampleRate));
  }
}
