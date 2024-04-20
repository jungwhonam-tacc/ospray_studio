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

  PanelVoice::PanelVoice(std::shared_ptr<StudioContext> _context, std::string _panelName, std::string _configFilePath)
      : Panel(_panelName.c_str(), _context)
      , panelName(_panelName)
      , configFilePath(_configFilePath)
  {
    requestManager.reset(new RequestManager(configFilePath));

    // enable floatFormat for OSP_FB_DEPTH channel
    auto &fb = _context->frame->childAs<sg::FrameBuffer>("framebuffer");
    fb["floatFormat"] = true;
    fb.commit();
  }

  void PanelVoice::buildUI(void *ImGuiCtx)
  {
    processRequests();

    // Allows plugin to still do other work if the UI isn't shown.
    if (!isShown())
      return;

    // Need to set ImGuiContext in *this* address space
    ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
    ImGui::OpenPopup(panelName.c_str());

    if (!ImGui::BeginPopupModal(panelName.c_str(), nullptr, ImGuiWindowFlags_None)) return;

    if (requestManager->isRunning()) {
      ImGui::Text("%s", "Currently connected to the voice server...");

      if (ImGui::Button("Disconnect")) {
        requestManager->close();
      }
    }
    else {
      ImGui::Text("%s", "Currently NOT connected to the voice server...");

      std::string str = "- Connect to " + requestManager->ipAddress + ":" + std::to_string(requestManager->portNumber);
      ImGui::Text("%s", str.c_str());

      if (ImGui::Button("Connect")) {
        requestManager->start();
      }
    }
    ImGui::Separator();

    ImGui::Separator();
    if (ImGui::Button("Close")) {
      setShown(false);
      ImGui::CloseCurrentPopup();
    }

    // Display statuses in a scrolling region
    if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::BeginChild("Scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
      for (std::string status : requestManager->statuses) {
        ImGui::Text("%s", status.c_str());
      }
      ImGui::EndChild();
    }

    ImGui::EndPopup();
  }

  void PanelVoice::processRequests() {
    if (!requestManager->isRunning())
      return;

    std::queue<nlohmann::ordered_json> requests;
    requestManager->pollRequests(requests);
    if (requests.size() <= 0) 
      return;

    std::cout << "Processing " << requests.size() << " request(s)..." << std::endl;
    
    MainWindow* pMW = reinterpret_cast<MainWindow*>(context->getMainWindow()); 

    while (!requests.empty()) {
      nlohmann::ordered_json req = requests.front();

      if (req.contains("type") && req["type"] == "request" && 
          req.contains("action") && req["action"] == "capture.rgb") {
        // 1) save the rgb image
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
          if (res == 0) {
            std::cerr << "STBI error; could not save image" << std::endl;
          } else {
            std::cout << "Saved to " << fname << std::endl;

            // 2) send the ack
            nlohmann::ordered_json res = 
            {
              {"type", "response"},
              {"action", "capture.rgb"},
              {"fpath", fname}
            };
            requestManager->send(res.dump());
          }
        }
        free(newfb);
      }
      else if (req.contains("type") && req["type"] == "request" && 
               req.contains("action") && req["action"] == "capture.depth") {
        // 1) save the npy file        
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

          // 2) send the ack
          nlohmann::ordered_json res = 
          {
            {"type", "response"},
            {"action", "capture.depth"},
            {"fpath", path}
          };
          requestManager->send(res.dump());
        }
        free(xyz);
      }
      else if (req.contains("type") && req["type"] == "request" && 
               req.contains("action") && req["action"] == "capture.camera") {
        const std::string path{"studio.camera.sg"};
        std::ofstream camera(path);
        JSON j = {{"camera", pMW->arcballCamera->getState()}};
        camera << j.dump();

        nlohmann::ordered_json res = 
        {
          {"type", "response"},
          {"action", "capture.camera"},
          {"fpath", path}
        };
        requestManager->send(res.dump());
      }
      else if (req.contains("type") && req["type"] == "request" && 
               req.contains("action") && req["action"] == "set.camera") {
        if (req.contains("camera")) {
          CameraState state;
          from_json(req["camera"], state);

          // update the state
          pMW->arcballCamera->setState(state);
          context->updateCamera();
        }

        nlohmann::ordered_json res = 
        {
          {"type", "response"},
          {"action", "set.camera"}
        };
        requestManager->send(res.dump());
      }
      else if (req.contains("type") && req["type"] == "request" && 
               req.contains("action") && req["action"] == "move.camera") {
        // get cameraToWorld
        CameraState state;
        from_json(req["camera"], state);
        affine3f cameraToWorld = state.cameraToWorld;

        // compute the position of the sphere in world-space
        vec3f sphereCenter = req["center"].get<vec3f>();
        vec3f sphereCenterInWorld = xfmPoint(cameraToWorld, sphereCenter);

        // dist from the sphere center
        float sphereRadius = req["radius"].get<float>();
        auto &camera = context->frame->child("camera");
        float fovy = req["fovy"].get<float>();
        float dist = (sphereRadius * 1.5f) * tanf(deg2rad(0.5f * fovy));

        // compute the position of the camera
        vec3f lookDir = xfmVector(cameraToWorld, vec3f(0, 0, -1));
        vec3f pos = sphereCenterInWorld + lookDir * -dist;

        pMW->arcballCamera->setCenter(pos);
        pMW->arcballCamera->setZoomLevel(-dist);
        context->updateCamera();
        
        // 2) send the ack
        nlohmann::ordered_json res = 
        {
          {"type", "response"},
          {"action", "move.camera"}
        };
        requestManager->send(res.dump());  
      }

      requests.pop();
    }
  }
  
  }  // namespace voice_plugin
}  // namespace ospray
