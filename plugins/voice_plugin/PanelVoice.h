#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"

namespace ospray {
  namespace voice_plugin {

    struct PanelVoice : public Panel
    {
      PanelVoice(std::shared_ptr<StudioContext> _context, std::string _panelName);

      void buildUI(void *ImGuiCtx) override;

    private:
      std::string panelName;
    };

  }  // namespace voice_plugin
}  // namespace ospray
