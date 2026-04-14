#pragma once

#include "../Model/AudioModel.h"
#include <imgui.h>

class HeaderPanel {
public:
  static void render(const AudioModel *model);
};
