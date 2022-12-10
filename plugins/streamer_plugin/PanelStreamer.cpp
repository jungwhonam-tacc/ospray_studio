#include <ctime>

#include "PanelStreamer.h"
#include "Util.h"

#include "app/widgets/GenerateImGuiWidgets.h"

#include "sg/JSONDefs.h"

#include "sg/Math.h"

#include "imgui.h"

namespace ospray {
  namespace streamer_plugin {
  
  PanelStreamer::PanelStreamer(std::shared_ptr<StudioContext> _context, std::string _panelName)
      : Panel(_panelName.c_str(), _context)
      , panelName(_panelName)
  {
    ipAddress = "localhost";
    portNumber = 8888;
    tcpSocket = nullptr;
    
    scaleOffset[0] = -0.001f;
    scaleOffset[1] = -0.001f;
    scaleOffset[2] = 0.001f;
    for (int i = 0; i < 3; i++) {
      rotationOffset[i] = 0.f;
      translationOffset[i] = 0.f;
    }

    loadConfig();
    addVisuals();
  }

  void PanelStreamer::loadConfig() {
    std::ifstream info("streamer.json");

    if (!info) {
      addStatus("Failed to load streamer.json... using default values...");
      return;
    }

    JSON j;
    try {
      info >> j;
    } catch (nlohmann::json::exception& e) {
      addStatus("Failed to parse streamer.json... using default values...");
      return;
    }

    if (j.contains("ipAddress")) {
      ipAddress = j["ipAddress"].get<std::string>();
    }
    if (j.contains("portNumber")) {
      portNumber = j["portNumber"].get<int>();
    }
    if (j.contains("scaleOffset")) {
      std::vector<float> vals = j["scaleOffset"];
      scaleOffset[0] = vals[0];
      scaleOffset[1] = vals[1];
      scaleOffset[2] = vals[2];
    }
    if (j.contains("rotationOffset")) {
      std::vector<float> vals = j["rotationOffset"];
      rotationOffset[0] = vals[0];
      rotationOffset[1] = vals[1];
      rotationOffset[2] = vals[2];
    }
    if (j.contains("translationOffset")) {
      std::vector<float> vals = j["translationOffset"];
      translationOffset[0] = vals[0];
      translationOffset[1] = vals[1];
      translationOffset[2] = vals[2];
    }
    addStatus("Loaded the configuration from streamer.json...");
  }

  void PanelStreamer::saveConfig() {
    std::ofstream config("streamer.json");

    JSON j;
    j["ipAddress"] = ipAddress;
    j["portNumber"] = portNumber;
    j["scaleOffset"] = scaleOffset;
    j["rotationOffset"] = rotationOffset;
    j["translationOffset"] = translationOffset;

    config << std::setw(4) << j << std::endl;
    addStatus("Saved the configuration to streamer.json...");
  }

  void PanelStreamer::addStatus(std::string status) {
    // write time before status
    time_t now = time(0);
    tm *ltm = localtime(&now);
    status.insert(0, "(" + std::to_string(ltm->tm_hour) + ":" + std::to_string(ltm->tm_min) + ":" + std::to_string(ltm->tm_sec) + ") ");

    statuses.push_back(status);
  }

  void PanelStreamer::buildUI(void *ImGuiCtx)
  {
    if (tcpSocket != nullptr && tcpSocket->isClosed) {
      delete tcpSocket;
      tcpSocket = nullptr;
    }

    // Need to set ImGuiContext in *this* address space
    ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
    ImGui::OpenPopup(panelName.c_str());

    if (!ImGui::BeginPopupModal(panelName.c_str(), nullptr, ImGuiWindowFlags_None)) return;

    // Network connection
    if (tcpSocket != nullptr) { // tcpSocket is not NULL.
      ImGui::Text("%s", "Currently connected to the server...");

      if (ImGui::Button("Disconnect")) {
        tcpSocket->Close();
      }
    } 
    else { // tcpSocket is NULL.
      ImGui::Text("%s", "Currently NOT connected to the server...");
      std::string str = "- Connect to " + ipAddress + ":" + std::to_string(portNumber);
      ImGui::Text("%s", str.c_str());

      if (ImGui::Button("Connect")) {
        // Initialize socket.
        tcpSocket = new TCPSocket([&](int errorCode, std::string errorMessage){
          addStatus("Socket creation error: " + std::to_string(errorCode) + " : " + errorMessage);
        });

        // Start receiving from the host.
        tcpSocket->onMessageReceived = [&](std::string message) {
          // std::cout << "Message from the Server: " << message << std::endl;

          nlohmann::ordered_json j;
          try {
            j = nlohmann::ordered_json::parse(message);
          } catch (nlohmann::json::exception& e) {
            std::cout << "Parse exception: " << e.what() << std::endl;
            return;
          }

          if (j == nullptr) return;

          auto &joints = context->frame->child("world").child("gestures_generator").child("joints");

          // offset
          const AffineSpace3f s = AffineSpace3f::scale(vec3f(scaleOffset));
          const AffineSpace3f r = LinearSpace3f(sg::eulerToQuaternion(deg2rad(vec3f(rotationOffset))));
          const AffineSpace3f t = AffineSpace3f::translate(vec3f(translationOffset));
          for (int i = 0; i < K4ABT_JOINT_COUNT; i++) {
            std::string id = k4abt_joint_id_t_str[i];
            
            if (!j.contains(id)) continue;

            vec3f pos = j[id].get<vec3f>();
            auto &joint = joints.child(id);
            joint.createChildData("sphere.position", xfmPoint(t * r * s, pos));
          }
        };

        // On socket closed:
        tcpSocket->onSocketClosed = [&](int errorCode){
          addStatus("Connection closed: " + std::to_string(errorCode));
        };

        // Connect to the host.
        tcpSocket->Connect(ipAddress, portNumber, [&] { 
          addStatus("Connected to the server successfully."); 
        },
        [&](int errorCode, std::string errorMessage){
          addStatus(std::to_string(errorCode) + " : " + errorMessage);
          tcpSocket->Close();
        });
      }
    }
    ImGui::Separator();

    // Close button
    if (ImGui::Button("Close")) {
      setShown(false);
      ImGui::CloseCurrentPopup();
    }
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
      // ImGui::InputText("IP Adress", ipAddress, IM_ARRAYSIZE(ipAddress));
      // ImGui::InputInt("Port Number", &portNumber);
      ImGui::DragFloat3("Scale", scaleOffset, 0.0001, -1000, 1000, "%.3f");
      ImGui::DragFloat3("Rotate (euler angles)", rotationOffset, .1, -180, 180, "%.1f");
      ImGui::DragFloat3("Translate", translationOffset, .1, -1000, 1000, "%.1f");
      ImGui::Separator();
      if (ImGui::Button("Save")) {
        saveConfig();
      }
      ImGui::SameLine();
      if (ImGui::Button("Load")) {
        loadConfig();
      }
    }
    ImGui::Separator();

    // Display statuses in a scrolling region
    if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::BeginChild("Scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
      for (std::string status : statuses) {
        ImGui::Text("%s", status.c_str());
      }
      ImGui::EndChild();
    }
    ImGui::Separator();

    ImGui::EndPopup();
  }

  void PanelStreamer::addVisuals()
  {
    auto &world = context->frame->child("world");
    world.createChildAs<sg::Generator>("axes_generator", "generator_axes");
    world.createChildAs<sg::Generator>("gestures_generator", "generator_gestures");
  }

  }  // namespace streamer_plugin
}  // namespace ospray
