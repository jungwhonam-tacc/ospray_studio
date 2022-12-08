#include <ctime>

#include "PanelStreamer.h"

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
    loadServerInfo();
    addVisuals();

    scaleOffset[0] = -0.001;
    scaleOffset[1] = -0.001;
    scaleOffset[2] = 0.001;
    for (int i = 0; i < 3; i++) {
      rotationOffset[i] = 0;
      translationOffset[i] = 0;
    }
  }

  void PanelStreamer::loadServerInfo() {
    std::ifstream info("streamer.json");
    if (info) {
      JSON j;
      info >> j;
      for (auto &cs : j) {
        if (cs.find("ipAddress") != cs.end()) {
          ipAddress = cs.at("ipAddress");
          addStatus("Read from streamer.json... ip address: " + ipAddress);
        }
        if (cs.find("portNumber") != cs.end()) {
          portNumber = cs.at("portNumber");
          addStatus("Read from streamer.json... port number: " + std::to_string(portNumber));
        }
      }
    } else {
      addStatus("Using default values... ip address: " + ipAddress + ", port number: " + std::to_string(portNumber));
    }
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

    if (ImGui::BeginPopupModal(panelName.c_str(), nullptr, ImGuiWindowFlags_None)) {
      if (tcpSocket != nullptr) { // tcpSocket is not NULL.
        ImGui::Text("%s", "Currently connected to the server...");

        if (ImGui::Button("Disconnect")) {
          tcpSocket->Close();
        }

        ImGui::Separator();
        ImGui::Text("%s", "Apply offset(s)...");
        ImGui::DragFloat3("Scale", scaleOffset, 0.0001);
        ImGui::DragFloat3("Rotate (euler angles)", rotationOffset, .1, -180, 180, "%.1f");
        ImGui::DragFloat3("Translate", translationOffset, .1, -1000, 1000, "%.1f");
      } 
      else { // tcpSocket is NULL.
        ImGui::Text("%s", "Currently NOT connected to the server...");

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

            if (j == nullptr || j["HAND_LEFT"] == nullptr || j["HAND_RIGHT"] == nullptr || j["SPINE_CHEST"]  == nullptr) return;

            auto &joints = context->frame->child("world").child("gestures_generator").child("joints");
            
            // offset
            const AffineSpace3f s = AffineSpace3f::scale(vec3f(scaleOffset));
            const AffineSpace3f r = LinearSpace3f(sg::eulerToQuaternion(deg2rad(vec3f(rotationOffset))));
            const AffineSpace3f t = AffineSpace3f::translate(vec3f(translationOffset));
            {
              vec3f pos = j["HAND_LEFT"].get<vec3f>();
              auto &joint = joints.child("joint0");
              joint.createChildData("sphere.position", xfmPoint(t * r * s, pos));
            }
            {
              vec3f pos = j["HAND_RIGHT"].get<vec3f>();
              auto &joint = joints.child("joint1");
              joint.createChildData("sphere.position", xfmPoint(t * r * s, pos));
            }
            {
              vec3f pos = j["SPINE_CHEST"].get<vec3f>();
              auto &joint = joints.child("joint2");
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
      if (ImGui::Button("Close")) {
        setShown(false);
        ImGui::CloseCurrentPopup();
      }

      // Display statuses in a scrolling region
      ImGui::Separator();
      ImGui::BeginChild("Scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysAutoResize);
      for (std::string status : statuses) {
        ImGui::Text("%s", status.c_str());
      }
      ImGui::EndChild();

      ImGui::EndPopup();
    }
  }

  void PanelStreamer::addVisuals()
  {
    auto &world = context->frame->child("world");
    world.createChildAs<sg::Generator>("axes_generator", "generator_axes");
    world.createChildAs<sg::Generator>("gestures_generator", "generator_gestures");
  }

  }  // namespace streamer_plugin
}  // namespace ospray
