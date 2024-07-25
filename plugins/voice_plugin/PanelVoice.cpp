#include "PanelVoice.h"

#include "app/MainWindow.h"
#include "app/widgets/GenerateImGuiWidgets.h"

#include "sg/Frame.h"
#include "sg/fb/FrameBuffer.h"

#include "imgui.h"
#include "stb_image_write.h"
#include "npy.hpp"

#include <filesystem>
#ifdef __APPLE__
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif

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
    
    while (!requests.empty()) {
      nlohmann::ordered_json req = requests.front();

      // only handles req with "type": "request" and "action":?
      if (!req.contains("type") || req["type"] != "request" || !req.contains("action")) {
        requests.pop();
        continue;
      }

      nlohmann::ordered_json res = {
        {"type", "response"}
      };

      if (req["action"] == "capture.rgb") {
        fs::path file("studio.rgb.png");
        fs::path full_path = fs::current_path() / file;

        if (saveImage(full_path)) {
          res["action"] = "capture.rgb";
          res["fpath"] = full_path;
        }
        else {
          std::cerr << "Could not save " << full_path << std::endl;
        }
      }
      else if (req["action"] == "capture.depth") {
        fs::path file("studio.depth.npy");
        fs::path full_path = fs::current_path() / file;

        if (saveNPY(full_path)) {
          res["action"] = "capture.depth";
          res["fpath"] = full_path;
        }
        else {
          std::cerr << "Could not save " << full_path << std::endl;
        }
      }
      else if (req["action"] == "capture.camera") {
        fs::path file("studio.camera.sg");
        fs::path full_path = fs::current_path() / file;

        if (saveSG(full_path)) {
          res["action"] = "capture.camera";
          res["fpath"] = full_path;
        }
        else {
          std::cerr << "Could not save " << full_path << std::endl;
        }
      }
      else if (req["action"] == "set.camera") {
        CameraState state;
        from_json(req["camera"], state);

        MainWindow* pMW = reinterpret_cast<MainWindow*>(context->getMainWindow());
        pMW->arcballCamera->setState(state);
        context->updateCamera();

        res["action"] = "set.camera";
      }
      else if (req["action"] == "move.camera") {
        CameraState state;
        from_json(req["camera"], state);

        moveCamera(
          state,
          req["fovy"].get<float>(),
          req["center"].get<vec3f>(),
          req["radius"].get<float>()
        );

        res["action"] = "move.camera"; 
      }

      // send the response
      if (res.contains("action"))
        requestManager->send(res.dump());

      // continue
      requests.pop();
    }
  }

  vec2f PanelVoice::getScale(int preferredWidth, int preferredHeight) {
    // info about current frame buffer
    auto &fb = context->frame->childAs<sg::FrameBuffer>("framebuffer");
    auto size = fb.child("size").valueAs<vec2i>();
    // info about new frame buffer for saving
    vec2i sizeNew = vec2i(
      size.x < preferredWidth ? size.x : preferredWidth, 
      size.y < preferredHeight ? size.y : preferredHeight
    );
    vec2f scale = vec2f((float) size.x / sizeNew.x, (float) size.y / sizeNew.y);
    // scale uniformly down to 512x384 but keep min width = 512 and height = 384
    if (scale.x < 1 || scale.y < 1) {
      sizeNew = size;
      scale.x = 1.;
      scale.y = 1.;
    }
    // otherwise scale uniformly...
    else if (scale.x != scale.y) {
      scale.x = std::min(scale.x, scale.y);
      scale.y = scale.x;
      sizeNew = size / scale;
    }

    return scale;
  }
  
  bool PanelVoice::saveImage(std::string fname) {
    // info about current frame buffer
    auto &fb = context->frame->childAs<sg::FrameBuffer>("framebuffer");
    auto size = fb.child("size").valueAs<vec2i>();
    const vec4f *mapped = static_cast<const vec4f *>(fb.map(OSP_FB_COLOR));

    // info about new frame buffer for saving
    vec2f scale = getScale(512, 384);
    vec2i sizeNew = size / scale;
    vec4uc *fbNew = (vec4uc *)malloc(sizeNew.x * sizeNew.y * sizeof(vec4uc));
    if (!fbNew) return false;

    // float to char
    auto gamma = [](float x) -> float {
      return pow(std::max(std::min(x, 1.f), 0.f), 1.f / 2.2f);
    };
    for (size_t y = 0; y < sizeNew.y; y++) { 
      for (size_t x = 0; x < sizeNew.x; x++) { 
        int iNew = y * sizeNew.x + x; // index for fbNew
        int i = (int) (y * scale.y) * size.x + (int) (x * scale.x); // index for fb

        fbNew[iNew].x = uint8_t(255 * gamma(mapped[i].x));
        fbNew[iNew].y = uint8_t(255 * gamma(mapped[i].y));
        fbNew[iNew].z = uint8_t(255 * gamma(mapped[i].z));
        fbNew[iNew].w = uint8_t(255 * std::max(std::min(mapped[i].w, 1.f), 0.f));
      }
    }

    // save image
    stbi_flip_vertically_on_write(1);
    int res = stbi_write_png(fname.c_str(), sizeNew.x, sizeNew.y, 4, (const void *)fbNew, 4 * sizeNew.x);

    // free memory
    free(fbNew);

    return res;
  }

  bool PanelVoice::saveNPY(std::string fname) {   
    // info about current frame buffer 
    auto &fb = context->frame->childAs<sg::FrameBuffer>("framebuffer");
    auto size = fb.child("size").valueAs<vec2i>(); // (1024, 768)
    const float *mapped = static_cast<const float *>(fb.map(OSP_FB_DEPTH));

    // info about new frame buffer for saving
    vec2f scale = getScale(512, 384);
    vec2i sizeNew = size / scale;
    size_t npix = sizeNew.x * sizeNew.y;
    float *xyz = (float *)malloc(npix * 3 * sizeof(float));
    if (!xyz) return false;

    // compute image plane, du, dv...
    auto &camera = context->frame->child("camera");
    auto fovy = camera["fovy"].valueAs<float>(); // 60
    auto aspect = camera["aspect"].valueAs<float>(); // 1.3333

    vec2f imgPlaneSize; // assume the focal length is 1.
    imgPlaneSize.y = 2.f * tanf(deg2rad(0.5f * fovy)); // 1.5396
    imgPlaneSize.x = imgPlaneSize.y * aspect; // 1.1547

    vec3f du_size{imgPlaneSize.x, 0.f, 0.f};
    vec3f dv_up{0.f, imgPlaneSize.y, 0.f};
    vec3f du = du_size / sizeNew.x;
    vec3f dv = dv_up / sizeNew.y;

    // dir_00 points to the bottom left corner of the image plane
    vec3f dir_00{0.f, 0.f, -1.f};
    dir_00 -= 0.5f * du_size;
    dir_00 -= 0.5f * dv_up;

    int idx = 0;
    for (int v = sizeNew.y - 1; v >= 0; v--) {
      for (int u = 0; u < sizeNew.x; u++) {
        int i = (int) (v * scale.y) * size.x + (int) (u * scale.x); // index for fb
        float depth = mapped[i];
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

    try {
      npy::write_npy(fname, d);
    } 
    catch(std::runtime_error &e) {
      free(xyz);
      return false;
    }
    free(xyz);
    return true;
  }

  bool PanelVoice::saveSG(std::string fname) {
    MainWindow* pMW = reinterpret_cast<MainWindow*>(context->getMainWindow());

    std::ofstream file(fname);
    if (file.fail()) return false;

    JSON j = {
      {"camera", pMW->arcballCamera->getState()}
    };
    file << j.dump();

    return true;
  }

  void PanelVoice::moveCamera(const CameraState &state, float fovy, const vec3f &spherePos, float sphereRadius) {
    // compute the position of the sphere in world-space
    vec3f sphereCenterInWorld = xfmPoint(state.cameraToWorld, spherePos);

    // dist from the sphere center
    auto &camera = context->frame->child("camera");
    float dist = (sphereRadius * 1.5f) * tanf(deg2rad(0.5f * fovy));

    // compute the position of the camera
    vec3f lookDir = xfmVector(state.cameraToWorld, vec3f(0, 0, -1));
    vec3f pos = sphereCenterInWorld + lookDir * -dist;

    MainWindow* pMW = reinterpret_cast<MainWindow*>(context->getMainWindow());
    pMW->arcballCamera->setCenter(pos);
    pMW->arcballCamera->setZoomLevel(-dist);
    context->updateCamera();
  }

  }  // namespace voice_plugin
}  // namespace ospray
