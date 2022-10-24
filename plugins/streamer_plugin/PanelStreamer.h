#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"
#include "async-sockets/tcpsocket.hpp"

namespace ospray {
  namespace streamer_plugin {

    struct PanelStreamer : public Panel
    {
      PanelStreamer(std::shared_ptr<StudioContext> _context, std::string _panelName);

      void buildUI(void *ImGuiCtx) override;

    private:
      std::string panelName;

      TCPSocket* tcpSocket;
      std::string status;
      float speedMultiplier;
    };

  }  // namespace streamer_plugin
}  // namespace ospray
