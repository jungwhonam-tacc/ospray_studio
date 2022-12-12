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

    angleThreshold = 50;
    confidenceLevelThreshold = K4ABT_JOINT_CONFIDENCE_MEDIUM;

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
    if (j.contains("angleThreshold")) {
      angleThreshold = j["angleThreshold"];
    }
    if (j.contains("confidenceLevelThreshold")) {
      confidenceLevelThreshold = j["confidenceLevelThreshold"];
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
    j["angleThreshold"] = angleThreshold;
    j["confidenceLevelThreshold"] = confidenceLevelThreshold;

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

          std::vector<vec3f> raw;
          raw.resize(K4ABT_JOINT_COUNT);
          std::vector<int> confidence;
          confidence.resize(K4ABT_JOINT_COUNT);

          std::vector<vec3f> processed;
          processed.resize(K4ABT_JOINT_COUNT);

          float angleLeftHand;
          float angleRightHand;
          bool leftHandClosed;
          bool rightHandClosed;

          // get raw data
          for (int i = 0; i < K4ABT_JOINT_COUNT; i++) {
            std::string id = k4abt_joint_id_t_str[i];
            raw[i] = j.contains(id) ? j[id]["pos"].get<vec3f>() : vec3f(0, 0, 0);
            confidence[i] = j.contains(id) ? j[id]["confidence"].get<int>() : K4ABT_JOINT_CONFIDENCE_NONE;
          }

          // compute processed data (apply the offset)
          const AffineSpace3f s = AffineSpace3f::scale(vec3f(scaleOffset));
          const AffineSpace3f r = LinearSpace3f(sg::eulerToQuaternion(deg2rad(vec3f(rotationOffset))));
          const AffineSpace3f t = AffineSpace3f::translate(vec3f(translationOffset));
          for (int i = 0; i < K4ABT_JOINT_COUNT; i++) {
            std::string id = k4abt_joint_id_t_str[i];
            processed[i] = xfmPoint(t * r * s, raw[i]);
          }

          // update the visualizer (joint spheres)
          auto &spheres = context->frame->child("world").child("gestures_generator").child("joints").child("spheres");
          spheres.createChildData("sphere.position", processed);

          { // detect gestures (fist??? left hand)
            const int wristIndex = K4ABT_JOINT_WRIST_LEFT;
            const int palmIndex = K4ABT_JOINT_HAND_LEFT;
            const int tipIndex = K4ABT_JOINT_HANDTIP_LEFT;
            const int thumbIndex = K4ABT_JOINT_THUMB_LEFT;

            // when the hand is positioned below the belly, the tracking of the hand becomes unrealiable.
            // also in Kinect, y is down.
            if (confidence[wristIndex] < confidenceLevelThreshold || 
                confidence[palmIndex] < confidenceLevelThreshold || 
                confidence[tipIndex] < confidenceLevelThreshold ||
                raw[palmIndex].y > raw[K4ABT_JOINT_SPINE_NAVEL].y) {
                  angleLeftHand = -1;
                  leftHandClosed = false;
            }
            else {
              vec3f palmToTip = raw[tipIndex] - raw[palmIndex];
              vec3f wristToPalm = raw[palmIndex] - raw[wristIndex];
              float angle = acos(dot(palmToTip, wristToPalm) / (length(palmToTip) * length(wristToPalm))) * (180 / 3.14159);

              angleLeftHand = angle;
              leftHandClosed = angleLeftHand > (float) angleThreshold;
            }
          }
          { // detect gestures (fist??? right hand)
            const int wristIndex = K4ABT_JOINT_WRIST_RIGHT;
            const int palmIndex = K4ABT_JOINT_HAND_RIGHT;
            const int tipIndex = K4ABT_JOINT_HANDTIP_RIGHT;
            const int thumbIndex = K4ABT_JOINT_THUMB_RIGHT;

            // when the hand is positioned below the belly, the tracking of the hand becomes unrealiable.
            // also in Kinect, y is down.
            if (confidence[wristIndex] < confidenceLevelThreshold || 
                confidence[palmIndex] < confidenceLevelThreshold || 
                confidence[tipIndex] < confidenceLevelThreshold ||
                raw[palmIndex].y > raw[K4ABT_JOINT_SPINE_NAVEL].y) {
                  angleRightHand = -1;
                  rightHandClosed = false;
            }
            else {
              vec3f palmToTip = raw[tipIndex] - raw[palmIndex];
              vec3f wristToPalm = raw[palmIndex] - raw[wristIndex];
              float angle = acos(dot(palmToTip, wristToPalm) / (length(palmToTip) * length(wristToPalm))) * (180 / 3.14159);

              angleRightHand = angle;
              rightHandClosed = angleRightHand > (float) angleThreshold;
            }
          }

          std::cout << (leftHandClosed ? "closed" : "opened") << "  ";
          std::cout << setprecision(2) << angleLeftHand << "  ";
          std::cout << (rightHandClosed ? "closed" : "opened") << "  ";
          std::cout << setprecision(2) << angleRightHand << "  >>>>  ";
          
          // update the scene (grabbing???)
          if (rightHandClosed && prevClosedRightHand && leftHandClosed && prevClosedLeftHand) {
            float prevDist = length(prevPosLeftHand - prevPosRightHand);
            float currDist = length(processed[K4ABT_JOINT_HAND_LEFT] - processed[K4ABT_JOINT_HAND_RIGHT]);
            float diffRatio = (currDist - prevDist) * 0.5f;

            auto &parameters = context->frame->child("world").child("sphere_generator").child("parameters");
            parameters["radius"] = parameters["radius"].valueAs<float>() + diffRatio;
            parameters["color"] = vec4f(1, 0, 1, 1);

            std::cout << "scale" << "  ";
            std::cout << setprecision(2) << diffRatio << "  ";
          }
          else if (leftHandClosed && prevClosedLeftHand) {
            vec3f amt = (processed[K4ABT_JOINT_HAND_LEFT] - prevPosLeftHand) * 1.25f;
            
            // change the color of the sphere
            auto &parameters = context->frame->child("world").child("sphere_generator").child("parameters");
            parameters["color"] = vec4f(0, 0, 1, 1);

            // apply the translation
            auto &transform = context->frame->child("world").child("sphere_generator").child("xfm");
            vec3f translation = transform.child("translation").valueAs<vec3f>();
            transform.child("translation") = translation + amt;

            std::cout << "move" << "  ";
            std::cout << setprecision(2) << amt << "  ";
          }
          else if (rightHandClosed && prevClosedRightHand) {
            vec3f amt = (processed[K4ABT_JOINT_HAND_RIGHT] - prevPosRightHand) * 1.25f;
            
            // change the color of the sphere
            auto &parameters = context->frame->child("world").child("sphere_generator").child("parameters");
            parameters["color"] = vec4f(1, 0, 0, 1);

            // apply the translation
            auto &transform = context->frame->child("world").child("sphere_generator").child("xfm");
            vec3f translation = transform.child("translation").valueAs<vec3f>();
            transform.child("translation") = translation + amt;

            std::cout << "move" << "  ";
            std::cout << setprecision(2) << amt << "  ";
          }
          else {
            // change the color of the sphere
            auto &parameters = context->frame->child("world").child("sphere_generator").child("parameters");
            parameters["color"] = vec4f(0, 1, 0, 1);
          }
          std::cout << std::endl << std::flush;

          // update the status
          prevClosedLeftHand = leftHandClosed;
          prevPosLeftHand.x = processed[K4ABT_JOINT_HAND_LEFT].x;
          prevPosLeftHand.y = processed[K4ABT_JOINT_HAND_LEFT].y;
          prevPosLeftHand.z = processed[K4ABT_JOINT_HAND_LEFT].z;
          prevClosedRightHand = rightHandClosed;
          prevPosRightHand.x = processed[K4ABT_JOINT_HAND_RIGHT].x;
          prevPosRightHand.y = processed[K4ABT_JOINT_HAND_RIGHT].y;
          prevPosRightHand.z = processed[K4ABT_JOINT_HAND_RIGHT].z;
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
      ImGui::Text("%s", "Offset(s)");
      ImGui::DragFloat3("Scale", scaleOffset, 0.0001, -1000, 1000, "%.3f");
      ImGui::DragFloat3("Rotate (euler angles)", rotationOffset, .1, -180, 180, "%.1f");
      ImGui::DragFloat3("Translate", translationOffset, .1, -1000, 1000, "%.1f");

      ImGui::Text("%s", "Gestures(s)");
      ImGui::DragInt("Angle Threshold (hand)", &angleThreshold, 1, 0, 180);
      ImGui::DragInt("Confidence Level Threshold", &confidenceLevelThreshold, 1, 0, K4ABT_JOINT_CONFIDENCE_LEVELS_COUNT);

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

    auto &gen = world.createChildAs<sg::Generator>("sphere_generator", "generator_sphere");
    auto &parameters = gen.child("parameters");
    parameters["radius"] = 0.15f;
    parameters["center"] = vec3f();
    parameters["color"] = vec4f(0, 1, 0, 1);
  }

  }  // namespace streamer_plugin
}  // namespace ospray
