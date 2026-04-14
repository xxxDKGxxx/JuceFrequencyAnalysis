#include "MainComponent.h"
#include "imgui.h"
#include "implot.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_formats/juce_audio_formats.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

//==============================================================================
MainComponent::MainComponent() {
  setSize(1200, 800);

  menuModel.onLoadSoundFileClick = [this]() { this->loadWavFile(); };

  pMenuBarComponent =
      std::make_unique<juce::MenuBarComponent>(&this->menuModel);

  setMenuBarBounds();

  addAndMakeVisible(*this->pMenuBarComponent);

  audioFormatManager.registerBasicFormats();

  glctx.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
  glctx.setRenderer(this);
  glctx.attachTo(*this);
  glctx.setContinuousRepainting(true);
}

MainComponent::~MainComponent() { glctx.detach(); }

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
}

void MainComponent::resized() {
  if (pMenuBarComponent == nullptr) {
    return;
  }

  setMenuBarBounds();
}

void MainComponent::loadWavFile() {
  if (isLoading)
    return;

  pFileChooser.reset(
      new juce::FileChooser("Select a Wave file...", juce::File{}, "*.wav"));

  auto chooserFlags = juce::FileBrowserComponent::openMode |
                      juce::FileBrowserComponent::canSelectFiles;

  pFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
    auto file = fc.getResult();

    if (file == juce::File{}) {
      return;
    }

    isLoading = true;
    statusMessage = "Loading file...";

    // Start background thread for processing
    std::thread([this, file]() {
      auto audioFormatReader = audioFormatManager.createReaderFor(file);

      if (audioFormatReader == nullptr) {
        isLoading = false;
        return;
      }

      auto sampleRate = audioFormatReader->sampleRate;
      auto bitsPerSample = audioFormatReader->bitsPerSample;
      auto numChannels = audioFormatReader->numChannels;
      auto lengthInSamples = audioFormatReader->lengthInSamples;

      auto pNewAudioBuffer = std::make_unique<juce::AudioBuffer<float>>(
          numChannels, lengthInSamples);

      audioFormatReader->read(pNewAudioBuffer.get(), 0,
                              static_cast<int>(lengthInSamples), 0, true, true);

      auto pNewAudioModel = std::make_unique<AudioModel>(
          std::move(pNewAudioBuffer), sampleRate, bitsPerSample, numChannels,
          lengthInSamples);

      {
        juce::ScopedLock lock(dataLock);
        pAudioModel = std::move(pNewAudioModel);
      }

      isLoading = false;

      delete audioFormatReader;
    }).detach();
  });
}

void MainComponent::setMenuBarBounds() {
  auto area = getLocalBounds();

  pMenuBarComponent->setBounds(
      area.removeFromTop(getLookAndFeel().getDefaultMenuBarHeight()));

  imguiOffsetY = getLookAndFeel().getDefaultMenuBarHeight();
}

void MainComponent::newOpenGLContextCreated() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr;

  ImPlot::CreateContext();
  ImGui_ImplJuce_Init(*this, glctx);
  ImGui_ImplOpenGL3_Init();
};

void MainComponent::renderOpenGL() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplJuce_NewFrame();
  ImGui::NewFrame();

  if (isLoading) {
    ImGui::SetNextWindowPos(ImVec2(getWidth() * 0.5f, getHeight() * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Status", nullptr,
                 ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("%s", statusMessage.c_str());
    ImGui::End();
  }

  auto commonFlags =
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;

  {
    juce::ScopedLock lock(dataLock);

    // Audio Waveform Window
    ImGui::SetNextWindowPos(ImVec2(0, 0 + imguiOffsetY));
    ImGui::SetNextWindowSize(ImVec2(0.7 * getWidth(), 0.5 * getHeight()));
    ImGui::Begin("Audio waveform", NULL, commonFlags);
    waveformPanel.render(pAudioModel.get(), getWidth(), getHeight());
    ImGui::End();

    // Headers Window
    ImGui::SetNextWindowPos(ImVec2(0.7 * getWidth(), 0 + imguiOffsetY));
    ImGui::SetNextWindowSize(ImVec2(0.3 * getWidth(), 0.5 * getHeight()));
    ImGui::Begin("Headers", NULL, commonFlags);
    headerPanel.render(pAudioModel.get());
    ImGui::End();
  }

  ImGui::Render();

  juce::gl::glClearColor(0, 0, 0, 1);
  juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
};

void MainComponent::openGLContextClosing() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplJuce_Shutdown();
  ImGui::DestroyContext();
  ImPlot::DestroyContext();
};
