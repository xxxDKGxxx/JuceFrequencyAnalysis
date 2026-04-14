#pragma once

#include "../Model/AudioModel.h"

class WaveformPanel {
public:
  static void render(const AudioModel *pAudioModel, int width, int height);
};
