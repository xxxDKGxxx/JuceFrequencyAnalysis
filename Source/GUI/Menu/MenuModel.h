#include "juce_gui_basics/juce_gui_basics.h"
#include <functional>

class MenuModel : public juce::MenuBarModel {
public:
  std::function<void()> onLoadSoundFileClick;

  juce::StringArray getMenuBarNames() override;

  void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

  juce::PopupMenu getMenuForIndex(int topLevelMenuIndex,
                                  const juce::String &menuName) override;
};
