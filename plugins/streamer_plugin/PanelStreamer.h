#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"
#if defined(__linux__) || defined(__APPLE__)
#include "async-sockets/tcpsocket.hpp"
#elif _WIN32
#include "async-sockets-wins/tcpsocket.hpp"
#endif

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
      float speedMultiplier;

      void loadServerInfo();
      void addStatus(std::string status);
    };

  }  // namespace streamer_plugin
}  // namespace ospray
