#include "AudioModel.h"
#include "juce_core/juce_core.h"
#include <algorithm>
#include <cmath>

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
  calculateSpectrogram();
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
  calculateSpectrogram();
}

void AudioModel::setFrameSize(int newSize) {
  if (frameSize == newSize)
    return;
  frameSize = newSize;
  calculateGlobalFeatures();
  calculateSpectrogram();
}

void AudioModel::setSpectrogramOverlap(float newOverlap) {
  float clamped = std::clamp(newOverlap, 0.0f, 0.95f);
  if (std::abs(clamped - spectrogramOverlap) < 1e-4f)
    return;

  spectrogramOverlap = clamped;
  calculateSpectrogram();
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
  globalSeries.f0Cepstrum.clear();

  int numFrames = static_cast<int>(lengthInSamples / frameSize);
  if (numFrames == 0)
    return;

  globalSeries.time.reserve(numFrames);
  globalSeries.volume.reserve(numFrames);
  globalSeries.centroid.reserve(numFrames);
  globalSeries.bandwidth.reserve(numFrames);
  globalSeries.flatness.reserve(numFrames);
  globalSeries.crestFactor.reserve(numFrames);
  globalSeries.f0Cepstrum.reserve(numFrames);

  int fftOrder = static_cast<int>(std::ceil(std::log2(frameSize)));
  int fftSize = 1 << fftOrder;
  juce::dsp::FFT fft(fftOrder);
  int numBins = fftSize / 2 + 1;

  std::vector<float> frameBuffer(frameSize);
  std::vector<float> fftBuffer(fftSize * 2);
  std::vector<float> magSpectrum(numBins);
  std::vector<float> cepstrumBuffer(fftSize * 2, 0.0f);

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

    // Cepstrum-based F0 estimation: C(tau) = |IFFT(log(|X| + eps))|
    std::fill(cepstrumBuffer.begin(), cepstrumBuffer.end(), 0.0f);
    std::copy(frameBuffer.begin(), frameBuffer.end(), cepstrumBuffer.begin());
    fft.performRealOnlyForwardTransform(cepstrumBuffer.data());

    constexpr float epsilon = 1e-12f;
    for (int k = 0; k < numBins; ++k) {
      float re = cepstrumBuffer[2 * k];
      float im = cepstrumBuffer[2 * k + 1];
      float mag = std::sqrt(re * re + im * im);
      cepstrumBuffer[2 * k] = std::log(mag + epsilon);
      cepstrumBuffer[2 * k + 1] = 0.0f;
    }

    for (int i = 2 * numBins; i < static_cast<int>(cepstrumBuffer.size()); ++i) {
      cepstrumBuffer[i] = 0.0f;
    }

    fft.performRealOnlyInverseTransform(cepstrumBuffer.data());

    int minQuefrency = static_cast<int>(std::floor(sampleRate / 400.0));
    int maxQuefrency = static_cast<int>(std::ceil(sampleRate / 50.0));
    minQuefrency = std::clamp(minQuefrency, 1, fftSize / 2);
    maxQuefrency = std::clamp(maxQuefrency, minQuefrency + 1, fftSize - 1);

    int bestQuefrency = minQuefrency;
    float bestValue = 0.0f;
    for (int q = minQuefrency; q <= maxQuefrency; ++q) {
      float c = std::abs(cepstrumBuffer[q]);
      if (c > bestValue) {
        bestValue = c;
        bestQuefrency = q;
      }
    }

    float f0 = 0.0f;
    if (bestQuefrency > 0) {
      f0 = static_cast<float>(sampleRate / static_cast<double>(bestQuefrency));
    }

    globalSeries.time.push_back(static_cast<float>(start) /
                                static_cast<float>(sampleRate));
    globalSeries.volume.push_back(f_calc.volume);
    globalSeries.centroid.push_back(f_calc.frequencyCentroid);
    globalSeries.bandwidth.push_back(f_calc.effectiveBandwidth);
    globalSeries.flatness.push_back(f_calc.spectralFlatness);
    globalSeries.crestFactor.push_back(f_calc.spectralCrestFactor);
    globalSeries.f0Cepstrum.push_back(f0);
  }
}

void AudioModel::calculateSpectrogram() {
  spectrogramData = SpectrogramData{};

  if (lengthInSamples <= 0 || frameSize <= 0)
    return;

  int fftOrder = static_cast<int>(std::ceil(std::log2(frameSize)));
  int fftSize = 1 << fftOrder;
  int numBins = fftSize / 2 + 1;

  int overlapSamples = static_cast<int>(std::round(frameSize * spectrogramOverlap));
  int hopSize = std::max(1, frameSize - overlapSamples);
  int usableSamples = static_cast<int>(lengthInSamples);

  int numFrames = 1;
  if (usableSamples > frameSize) {
    numFrames = 1 + (usableSamples - frameSize) / hopSize;
  }

  spectrogramData.timeBins = numFrames;
  spectrogramData.freqBins = numBins;
  spectrogramData.durationSeconds = getLengthInSeconds();
  spectrogramData.maxFrequencyHz = sampleRate / 2.0;

  std::vector<float> dbValues(static_cast<size_t>(numBins) * numFrames,
                              -120.0f);
  std::vector<float> frameBuffer(frameSize, 0.0f);
  std::vector<float> fftBuffer(fftSize * 2, 0.0f);

  juce::dsp::FFT fft(fftOrder);
  const float *pRead = audioBuffer->getReadPointer(0);

  constexpr float epsilon = 1e-12f;
  float maxDb = -1e9f;

  for (int frame = 0; frame < numFrames; ++frame) {
    int start = frame * hopSize;

    std::fill(frameBuffer.begin(), frameBuffer.end(), 0.0f);
    for (int i = 0; i < frameSize; ++i) {
      int idx = start + i;
      if (idx >= usableSamples)
        break;
      frameBuffer[i] = pRead[idx];
    }

    WindowFunctions::apply(windowType, frameBuffer);

    std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::copy(frameBuffer.begin(), frameBuffer.end(), fftBuffer.begin());
    fft.performFrequencyOnlyForwardTransform(fftBuffer.data());

    for (int bin = 0; bin < numBins; ++bin) {
      float mag = fftBuffer[bin];
      float db = 20.0f * std::log10(mag + epsilon);

      // ImPlot heatmap row 0 appears at the top. Store high frequencies first
      // so that low frequencies are shown at the bottom.
      int row = (numBins - 1 - bin);
      dbValues[static_cast<size_t>(row) * numFrames + frame] = db;
      if (db > maxDb)
        maxDb = db;
    }
  }

  spectrogramData.minDb = -80.0f;
  spectrogramData.maxDb = 0.0f;
  spectrogramData.valuesDb.resize(static_cast<size_t>(numBins) * numFrames,
                                  spectrogramData.minDb);

  for (size_t i = 0; i < dbValues.size(); ++i) {
    // Normalize so strongest bin is 0 dB, then clamp displayed range.
    float relDb = dbValues[i] - maxDb;
    spectrogramData.valuesDb[i] =
        std::clamp(relDb, spectrogramData.minDb, spectrogramData.maxDb);
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
