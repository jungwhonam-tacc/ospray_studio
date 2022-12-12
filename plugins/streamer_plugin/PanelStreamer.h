#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"
#include "tcpsocket.hpp"

namespace ospray {
  namespace streamer_plugin {

    struct PanelStreamer : public Panel
    {
      PanelStreamer(std::shared_ptr<StudioContext> _context, std::string _panelName);

      void buildUI(void *ImGuiCtx) override;

    private:
      std::string panelName;

      std::string ipAddress;
      int portNumber;
      TCPSocket* tcpSocket;
      std::list<std::string> statuses;

      float scaleOffset[3];
      float rotationOffset[3];
      float translationOffset[3];

      int angleThreshold; // 0 ~ 180
      int confidenceLevelThreshold;

      // previous states
      bool prevClosedLeftHand;
      vec3f prevPosLeftHand;
      bool prevClosedRightHand;
      vec3f prevPosRightHand;

      void loadConfig();
      void saveConfig();

      void addStatus(std::string status);
      void addVisuals();
    };

  }  // namespace streamer_plugin
}  // namespace ospray
