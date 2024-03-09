#include "PanelVoice.h"

#include "app/MainWindow.h"
#include "app/widgets/GenerateImGuiWidgets.h"

#include "sg/Frame.h"
#include "sg/fb/FrameBuffer.h"

#include "imgui.h"
#include "stb_image_write.h"

namespace ospray {
  namespace voice_plugin {

  PanelVoice::PanelVoice(std::shared_ptr<StudioContext> _context, std::string _panelName)
      : Panel(_panelName.c_str(), _context)
      , panelName(_panelName)
  {
    // enable floatFormat for OSP_FB_DEPTH channel
    auto &fb = _context->frame->childAs<sg::FrameBuffer>("framebuffer");
    fb["floatFormat"] = true;
    fb.commit();
  }

  void PanelVoice::buildUI(void *ImGuiCtx)
  {
    // Allows plugin to still do other work if the UI isn't shown.
    if (!isShown())
      return;

    // Need to set ImGuiContext in *this* address space
    ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
    ImGui::OpenPopup(panelName.c_str());

    if (!ImGui::BeginPopupModal(panelName.c_str(), nullptr, ImGuiWindowFlags_None)) return;

    if (ImGui::Button("Capture (RGB)")) {
      auto &fb = context->frame->childAs<sg::FrameBuffer>("framebuffer");
      auto size = fb.child("size").valueAs<vec2i>();
      const vec4f *mapped = static_cast<const vec4f *>(fb.map(OSP_FB_COLOR));
      size_t npix = size.x * size.y;
      vec4uc *newfb = (vec4uc *)malloc(npix * sizeof(vec4uc));
      
      if (newfb) {
        // float to char
        for (size_t i = 0; i < npix; i++) {
          auto gamma = [](float x) -> float {
            return pow(std::max(std::min(x, 1.f), 0.f), 1.f / 2.2f);
          };
          newfb[i].x = uint8_t(255 * gamma(mapped[i].x));
          newfb[i].y = uint8_t(255 * gamma(mapped[i].y));
          newfb[i].z = uint8_t(255 * gamma(mapped[i].z));
          newfb[i].w = uint8_t(255 * std::max(std::min(mapped[i].w, 1.f), 0.f));
        }

        // save image
        std::string fname = "studio.rgb.png";
        stbi_flip_vertically_on_write(1);
        int res = stbi_write_png(fname.c_str(), size.x, size.y, 4, (const void *)newfb, 4 * size.x);
        if (res == 0)
          std::cerr << "STBI error; could not save image" << std::endl;
        else
          std::cout << "Saved to " << fname << std::endl;
      }
      free(newfb);
    }

    if (ImGui::Button("Capture (Depth)")) {
      auto &fb = context->frame->childAs<sg::FrameBuffer>("framebuffer");
      auto size = fb.child("size").valueAs<vec2i>();
      const float *mapped = static_cast<const float *>(fb.map(OSP_FB_DEPTH));
      size_t npix = size.x * size.y;
      uint8_t *newfb = (uint8_t *)malloc(npix * sizeof(uint8_t));

      if (newfb) {
        // scale OSPRay's 0 -> inf depth range to 0 -> 255, ignoring all inf values
        float minValue = rkcommon::math::inf;
        float maxValue = rkcommon::math::neg_inf;
        for (size_t i = 0; i < npix; i++) {
          if (isinf(mapped[i]))
            continue;
          minValue = std::min(minValue, mapped[i]);
          maxValue = std::max(maxValue, mapped[i]);
        }
        std::cout << "min, max: " << minValue << " " << maxValue << std::endl;

        const float rcpRange = 1.f / (maxValue - minValue);
        for (size_t i = 0; i < npix; i++) {
          float v = isinf(mapped[i]) ? 1.f : (mapped[i] - minValue) * rcpRange;
          newfb[i] = uint8_t(v * 255.99f);
        }

        // save the depth image
        std::string fname = "studio.depth.png";
        stbi_flip_vertically_on_write(1);
        int res = stbi_write_png(fname.c_str(), size.x, size.y, 1, newfb, 1 * size.x);
        if (res == 0)
          std::cerr << "STBI error; could not save image" << std::endl;
        else
          std::cout << "Saved to " << fname << std::endl;
      }
      free(newfb);
    }
    
    ImGui::Separator();
    if (ImGui::Button("Close")) {
      setShown(false);
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
  
  }  // namespace voice_plugin
}  // namespace ospray
