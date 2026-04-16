#pragma once

#include "../Model/AudioModel.h"

class AnalysisPanel {
public:
  static void render(AudioModel *pAudioModel, int width, int height);
  static bool normalise;
};
