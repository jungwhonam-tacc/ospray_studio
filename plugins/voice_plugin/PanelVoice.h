#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"

#include "request/RequestManager.h"

namespace ospray {
  namespace voice_plugin {

    struct PanelVoice : public Panel
    {
      PanelVoice(std::shared_ptr<StudioContext> _context, std::string _panelName,  std::string _configFilePath);

      void buildUI(void *ImGuiCtx) override;

      void processRequests();

    private:
      std::string panelName;
      std::string configFilePath;
      std::unique_ptr<RequestManager> requestManager;
    };

  }  // namespace voice_plugin
}  // namespace ospray
