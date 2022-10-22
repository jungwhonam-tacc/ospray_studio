#include "PanelStreamer.h"

#include "app/widgets/GenerateImGuiWidgets.h"

#include "imgui.h"

namespace ospray {
  namespace streamer_plugin {

  PanelStreamer::PanelStreamer(std::shared_ptr<StudioContext> _context, std::string _panelName)
      : Panel(_panelName.c_str(), _context)
      , panelName(_panelName)
  {}

  static void showCountingResults(int max, PanelStreamer *streamer) {
    int cnt = 0;
    while(cnt < max) {
      streamer->message = "(" + std::to_string(cnt) + "/" + std::to_string(max-1) + ")";
      std::cout << streamer->message << std::endl;
      cnt++;
      usleep(1 * 1000000); // Sleeps for 1 second
    }
  }

  void PanelStreamer::buildUI(void *ImGuiCtx)
  {
    // Need to set ImGuiContext in *this* address space
    ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
    ImGui::OpenPopup(panelName.c_str());

    if (ImGui::BeginPopupModal(
            panelName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("%s", "Hello from the streamer plugin!");
      ImGui::Separator();

      ImGui::Text("%s", "Current application state");
      ImGui::Text("Frame: %p", (void *) context->frame.get());
      ImGui::Text("Arcball Camera: %p", (void *) context->arcballCamera.get());
      ImGui::Text("Current scenegraph:");
      context->frame->traverse<sg::GenerateImGuiWidgets>(
          sg::TreeState::ROOTOPEN);

      if (ImGui::Button("Close")) {
        setShown(false);
        ImGui::CloseCurrentPopup();
      }

      ImGui::Separator();
      if (ImGui::Button("Start")) {
        std::thread t(showCountingResults, 5, this);
        t.detach();
      }
      ImGui::Text("Result(s): %s", message.c_str());

      ImGui::EndPopup();
    }
  }
  
  }  // namespace streamer_plugin
}  // namespace ospray
