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

      vec2f getScale(int preferredWidth, int preferredHeight);
      bool saveImage(std::string fname);
      bool saveNPY(std::string fname);
      bool saveSG(std::string fname);
      void moveCamera(const CameraState &state, float fovy, const vec3f &spherePos, float sphereRadius);

    private:
      std::string panelName;
      std::string configFilePath;
      std::unique_ptr<RequestManager> requestManager;
    };

  }  // namespace voice_plugin
}  // namespace ospray
