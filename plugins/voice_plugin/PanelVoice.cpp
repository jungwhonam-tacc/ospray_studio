#include "PanelVoice.h"

#include "app/MainWindow.h"
#include "app/widgets/GenerateImGuiWidgets.h"

#include "imgui.h"

namespace ospray {
  namespace voice_plugin {

  PanelVoice::PanelVoice(std::shared_ptr<StudioContext> _context, std::string _panelName)
      : Panel(_panelName.c_str(), _context)
      , panelName(_panelName)
  {}

  void PanelVoice::buildUI(void *ImGuiCtx)
  {
    // Allows plugin to still do other work if the UI isn't shown.
    if (!isShown())
      return;

    // Need to set ImGuiContext in *this* address space
    ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
    ImGui::OpenPopup(panelName.c_str());

    if (ImGui::BeginPopupModal(
            panelName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("%s", "Hello from the voice plugin!");
      ImGui::Separator();

      ImGui::Text("%s", "Current application state");
      ImGui::Text("Frame: %p", (void *) context->frame.get());
      auto pMW = (MainWindow*)context->getMainWindow();
      if (pMW)
        ImGui::Text("Arcball Camera: %p", (void *) pMW->arcballCamera.get());
      ImGui::Text("Current scenegraph:");
      context->frame->traverse<sg::GenerateImGuiWidgets>(
          sg::TreeState::ROOTOPEN);

      if (ImGui::Button("Close")) {
        setShown(false);
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
    }

  }  // namespace voice_plugin
}  // namespace ospray
