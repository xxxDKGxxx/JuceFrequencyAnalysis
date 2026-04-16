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

  // Default selection: full signal
  selectionStart = 0.0;
  selectionEnd = getLengthInSeconds();
  updateAnalysis();
  calculateGlobalFeatures();
}

void AudioModel::setSelection(double startSeconds, double endSeconds) {
  selectionStart = std::clamp(startSeconds, 0.0, getLengthInSeconds());
  selectionEnd = std::clamp(endSeconds, selectionStart, getLengthInSeconds());
  updateAnalysis();
}

void AudioModel::setWindowType(WindowFunctions::Type type) {
  windowType = type;
  updateAnalysis();
  calculateGlobalFeatures(); // Re-calculate trends because window changed
}

void AudioModel::setFrameSize(int newSize) {
  if (frameSize == newSize)
    return;
  frameSize = newSize;
  calculateGlobalFeatures();
}

void AudioModel::calculateGlobalFeatures() {
  if (lengthInSamples == 0)
    return;

  globalSeries.time.clear();
  globalSeries.volume.clear();
  globalSeries.centroid.clear();
  globalSeries.bandwidth.clear();
  globalSeries.flatness.clear();
  globalSeries.crestFactor.clear();

  int numFrames = static_cast<int>(lengthInSamples / frameSize);
  if (numFrames == 0)
    return;

  globalSeries.time.reserve(numFrames);
  globalSeries.volume.reserve(numFrames);
  globalSeries.centroid.reserve(numFrames);
  globalSeries.bandwidth.reserve(numFrames);
  globalSeries.flatness.reserve(numFrames);
  globalSeries.crestFactor.reserve(numFrames);

  int fftOrder = static_cast<int>(std::ceil(std::log2(frameSize)));
  int fftSize = 1 << fftOrder;
  juce::dsp::FFT fft(fftOrder);
  int numBins = fftSize / 2 + 1;

  std::vector<float> frameBuffer(frameSize);
  std::vector<float> fftBuffer(fftSize * 2);
  std::vector<float> magSpectrum(numBins);

  const float *pRead = audioBuffer->getReadPointer(0);

  for (int f = 0; f < numFrames; ++f) {
    int start = f * frameSize;
    for (int i = 0; i < frameSize; ++i) {
      frameBuffer[i] = pRead[start + i];
    }

    WindowFunctions::apply(windowType, frameBuffer);

    std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::copy(frameBuffer.begin(), frameBuffer.end(), fftBuffer.begin());
    fft.performFrequencyOnlyForwardTransform(fftBuffer.data());

    for (int i = 0; i < numBins; ++i) {
      magSpectrum[i] = fftBuffer[i];
    }

    AudioFeatures f_calc = AudioFeatures::calculate(magSpectrum, sampleRate);

    globalSeries.time.push_back(static_cast<float>(start) /
                                static_cast<float>(sampleRate));
    globalSeries.volume.push_back(f_calc.volume);
    globalSeries.centroid.push_back(f_calc.frequencyCentroid);
    globalSeries.bandwidth.push_back(f_calc.effectiveBandwidth);
    globalSeries.flatness.push_back(f_calc.spectralFlatness);
    globalSeries.crestFactor.push_back(f_calc.spectralCrestFactor);
  }
}

void AudioModel::updateAnalysis() {
  if (lengthInSamples == 0)
    return;

  int startSample = static_cast<int>(std::round(selectionStart * sampleRate));
  int endSample = static_cast<int>(std::round(selectionEnd * sampleRate));
  int numSamples = std::max(0, endSample - startSample);

  if (numSamples == 0) {
    magnitudeSpectrum.clear();
    windowedSignal.clear();
    features = AudioFeatures();
    return;
  }

  // 1. Prepare frame
  windowedSignal.assign(numSamples, 0.0f);
  const float *pRead = audioBuffer->getReadPointer(0);
  for (int i = 0; i < numSamples; ++i) {
    windowedSignal[i] = pRead[startSample + i];
  }

  // 2. Apply window
  WindowFunctions::apply(windowType, windowedSignal);

  // 3. FFT
  int fftOrder = static_cast<int>(std::ceil(std::log2(numSamples)));
  int fftSize = 1 << fftOrder;
  juce::dsp::FFT fft(fftOrder);

  std::vector<float> fftBuffer(fftSize * 2, 0.0f);
  std::copy(windowedSignal.begin(), windowedSignal.end(), fftBuffer.begin());

  fft.performFrequencyOnlyForwardTransform(fftBuffer.data());

  // Magnitude Spectrum (first half)
  int numBins = fftSize / 2 + 1;
  magnitudeSpectrum.assign(numBins, 0.0f);
  for (int i = 0; i < numBins; ++i) {
    magnitudeSpectrum[i] = fftBuffer[i];
  }

  // 4. Calculate features
  features = AudioFeatures::calculate(magnitudeSpectrum, sampleRate);
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

  // 4000 points for the visualization (2000 min-max pairs)
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
    int endSample =
        std::min(static_cast<int>(lengthInSamples), (i + 1) * samplesPerBucket);

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
