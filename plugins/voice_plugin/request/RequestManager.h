#pragma once

#include "tcpsocket.hpp"
#include "sg/JSONDefs.h"

#include <string>
#include <mutex>
#include <queue>

namespace ospray {
namespace voice_plugin {

using namespace rkcommon::math;

class RequestManager
{
public:
    RequestManager(std::string configFilePath);
    ~RequestManager();

    void start();
    void close();
    bool isRunning();
    bool isUpdated();

    void pollRequests(std::queue<nlohmann::ordered_json>& requests);

    void send(std::string message);

    std::string getResultsInReadableForm();

    std::string ipAddress { "localhost" };
    uint portNumber { 8889 };

    std::list<std::string> statuses;
private:
    void addStatus(std::string status);

    TCPSocket<> *tcpSocket;

    std::queue<nlohmann::ordered_json> pendingRequests;
    std::mutex mtx;
};

}  // namespace voice_plugin
}  // namespace ospray