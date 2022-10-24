#include "PanelStreamer.h"

#include "app/widgets/GenerateImGuiWidgets.h"

#include "imgui.h"

namespace ospray {
  namespace streamer_plugin {

  PanelStreamer::PanelStreamer(std::shared_ptr<StudioContext> _context, std::string _panelName)
      : Panel(_panelName.c_str(), _context)
      , panelName(_panelName)
  { tcpSocket = 0; status = "Hello~"; speedMultiplier = 0.05f; }

  void PanelStreamer::buildUI(void *ImGuiCtx)
  {
    // Need to set ImGuiContext in *this* address space
    ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
    ImGui::OpenPopup(panelName.c_str());

    if (ImGui::BeginPopupModal(
            panelName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("%s", "The Streamer Plugin receives data from Gesture Tracking Server.");
      ImGui::Separator();

      ImGui::Text("Status: \n%s", status.c_str());
      ImGui::Separator();

      if (tcpSocket) { // tcpSocket is not NULL.
        ImGui::Text("%s", "Currently connected to the server...");

        if (ImGui::Button("Disconnect")) {
          tcpSocket->Close();
        }

        ImGui::Separator();
        ImGui::SliderFloat("Rotating speed multiplier", &speedMultiplier, 0.01f, 0.1f);
      }
      else {
        ImGui::Text("%s", "Currently NOT connected to the server...");

        if (ImGui::Button("Connect")) {
          // Initialize socket.
          tcpSocket = new TCPSocket([](int errorCode, std::string errorMessage){
              std::cout << "Socket creation error:" << errorCode << " : " << errorMessage << std::endl;
          });

          // Start receiving from the host.
          tcpSocket->onMessageReceived = [&](std::string message) {
              std::cout << "Message from the Server: " << message << std::endl;
              status = message;

              vec2f from(0.f, 0.f);
              vec2f to(std::stof(message) * speedMultiplier, 0.f);
              context->arcballCamera->rotate(from, to);
              context->updateCamera();
          };
          
          // On socket closed:
          tcpSocket->onSocketClosed = [&](int errorCode){
              std::cout << "Connection closed: " << errorCode << std::endl;
              delete tcpSocket;
              tcpSocket = NULL;
          };

          // Connect to the host.
          tcpSocket->Connect("localhost", 8888, [&] {
              std::cout << "Connected to the server successfully." << std::endl;
          },
          [](int errorCode, std::string errorMessage){
              // CONNECTION FAILED
              std::cout << errorCode << " : " << errorMessage << std::endl;
          });
        }
      } 

      ImGui::Separator();
      if (ImGui::Button("Close")) {
        setShown(false);
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

  }  // namespace streamer_plugin
}  // namespace ospray
