#pragma once

#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class WindowFunctions {
public:
  enum class Type {
    Rectangular,
    Triangular,
    Hamming,
    Hann,
    Blackman,
    Nuttall // Extra window
  };

  static void apply(Type type, std::vector<float>& buffer) {
    size_t N = buffer.size();
    if (N == 0) return;

    for (size_t n = 0; n < N; ++n) {
      float w = 1.0f;
      switch (type) {
        case Type::Rectangular:
          w = 1.0f;
          break;
        case Type::Triangular:
          w = 1.0f - std::abs(2.0f * (static_cast<float>(n) - static_cast<float>(N - 1) / 2.0f) / static_cast<float>(N - 1));
          break;
        case Type::Hamming:
          w = 0.54f - 0.46f * std::cos(2.0f * static_cast<float>(M_PI) * n / static_cast<float>(N - 1));
          break;
        case Type::Hann:
          w = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * n / static_cast<float>(N - 1)));
          break;
        case Type::Blackman:
          w = 0.42f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * n / static_cast<float>(N - 1)) +
              0.08f * std::cos(4.0f * static_cast<float>(M_PI) * n / static_cast<float>(N - 1));
          break;
        case Type::Nuttall:
          w = 0.355768f - 0.487396f * std::cos(2.0f * static_cast<float>(M_PI) * n / static_cast<float>(N - 1)) +
              0.144232f * std::cos(4.0f * static_cast<float>(M_PI) * n / static_cast<float>(N - 1)) -
              0.012604f * std::cos(6.0f * static_cast<float>(M_PI) * n / static_cast<float>(N - 1));
          break;
      }
      buffer[n] *= w;
    }
  }

  static std::vector<float> generate(Type type, size_t N) {
    std::vector<float> window(N, 1.0f);
    apply(type, window);
    return window;
  }
};
