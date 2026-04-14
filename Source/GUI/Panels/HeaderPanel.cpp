#include "HeaderPanel.h"

void HeaderPanel::render(const AudioModel *model) {
  if (model == nullptr) {
    ImGui::TextDisabled("No audio file loaded.");
    return;
  }

  ImGui::Text("Sample Rate:      %.0f Hz", model->getSampleRate());
  ImGui::Text("Bits Per Sample:  %u", model->getBitsPerSample());
  ImGui::Text("Channels:         %u", model->getNumChannels());
  ImGui::Text("Length (samples): %lld",
              static_cast<long long>(model->getLengthInSamples()));
  ImGui::Text("Length (seconds): %.3f s", model->getLengthInSeconds());
}
