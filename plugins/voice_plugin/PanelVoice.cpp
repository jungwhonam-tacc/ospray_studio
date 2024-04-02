#include "PanelVoice.h"

#include "app/MainWindow.h"
#include "app/widgets/GenerateImGuiWidgets.h"

#include "sg/Frame.h"
#include "sg/fb/FrameBuffer.h"

#include "imgui.h"
#include "stb_image_write.h"
#include "npy.hpp"

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

    if (ImGui::Button("Capture (npy)")) {
      auto &fb = context->frame->childAs<sg::FrameBuffer>("framebuffer");
      auto size = fb.child("size").valueAs<vec2i>(); // (1024, 768)
      const float *mapped = static_cast<const float *>(fb.map(OSP_FB_DEPTH));
      size_t npix = size.x * size.y; // 786432
      float *xyz = (float *)malloc(npix * 3 * sizeof(float));

      if (xyz) {
        auto &camera = context->frame->child("camera");
        auto fovy = camera["fovy"].valueAs<float>(); // 60
        auto aspect = camera["aspect"].valueAs<float>(); // 1.3333

        // assume the focal length is 1.
        vec2f imgPlaneSize;
        imgPlaneSize.y = 2.f * tanf(deg2rad(0.5f * fovy)); // 1.5396
        imgPlaneSize.x = imgPlaneSize.y * aspect; // 1.1547

        vec3f du_size{imgPlaneSize.x, 0.f, 0.f};
        vec3f dv_up{0.f, imgPlaneSize.y, 0.f};
        vec3f du = du_size / size.x;
        vec3f dv = dv_up / size.y;

        // dir_00 points to the bottom left corner of the image plane
        vec3f dir_00{0.f, 0.f, -1.f};
        dir_00 -= 0.5f * du_size;
        dir_00 -= 0.5f * dv_up;

        int idx = 0;
        for (int v = size.y - 1; v >= 0; v--) {
          for (int u = 0; u < size.x; u++) {
            float depth = mapped[size.x * v + u];
            vec3f ray_dir = normalize(dir_00 + u * du + v * dv);
            vec3f pos = ray_dir * depth; // assume the origin is (0,0,0)

            xyz[idx++] = pos.x;
            xyz[idx++] = pos.y;
            xyz[idx++] = pos.z;
          }
        }

        // save npy file
        const std::vector<float> data(xyz, xyz + (npix * 3));
        npy::npy_data_ptr<float> d;
        d.data_ptr = data.data();
        d.shape = {npix, 3};
        d.fortran_order = false; // optional
        const std::string path{"studio.depth.npy"};
        npy::write_npy(path, d);
        std::cout << "Saved to " << path << std::endl;
      }
      
      free(xyz);
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
