#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"

namespace ospray {
  namespace streamer_plugin {

    struct PanelStreamer : public Panel
    {
      PanelStreamer(std::shared_ptr<StudioContext> _context, std::string _panelName);

      void buildUI(void *ImGuiCtx) override;

    private:
      std::string panelName;
    };

  }  // namespace streamer_plugin
}  // namespace ospray
