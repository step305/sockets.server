#include <iostream>
#include "ClientWorker.h"
#include <map>
#include <algorithm>
#include <csignal>
#include "Camera.h"
#include "Recorder.h"

const auto SERVER_PORT = htons(8008);
volatile sig_atomic_t exit_flag = 0;

video::Recorder recorder("log_video.ts");
video::Camera camera(4, cv::Size(1280, 720), &recorder.stream);

void clearDeadWorkers(std::map<int, server::ClientWorker*> &list){
    std::vector<int> ids;
    for (auto const& w : list) {
        if (!w.second->isAlive()) {
            auto ww = w.second;
            auto id = ww->id;
            camera.remove_sink(id);
            ids.push_back(id);
            ids.push_back(id);
        }
    }
    for (auto x : ids) {
        list.erase(x);
    }
}

void clearAllWorkers(std::map<int, server::ClientWorker*> &list){
    std::vector<int> ids;
    for (auto const& w : list) {
        auto id = w.second->id;
        camera.remove_sink(id);
        ids.push_back(id);
        delete w.second;
        list.erase(list.find(id));
    }
    for (auto x : ids) {
        list.erase(x);
    }
}

void exit_catch(int sig) {
    std::cout << "User stop requested: " << std::to_string(sig) << std::endl;
    exit_flag = 1;
}

void setBreakCallback() {
    struct sigaction sigIntHandler{};
    sigIntHandler.sa_handler = exit_catch;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);
}

void buffer_append_int32(uint8_t* buffer, int32_t number, int32_t *index) {
    buffer[(*index)++] = number >> 24;
    buffer[(*index)++] = number >> 16;
    buffer[(*index)++] = number >> 8;
    buffer[(*index)++] = number;
}

int main() {
    setBreakCallback();

    int serverSock=socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = SERVER_PORT;
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    int reusePort = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEPORT, &reusePort, sizeof(reusePort));

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr)) < 0) {
        std::cout << "Error: bind" << std::endl;
        abort();
    }

    listen(serverSock, 4);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(serverAddr.sin_addr), ip, INET_ADDRSTRLEN);
    std::cout << ip << std::endl;

    std::map<int, server::ClientWorker*> workers;

    while(!exit_flag) {
        sockaddr_in clientAddr{};
        socklen_t sin_size=sizeof(struct sockaddr_in);
        if(int clientSock=accept(serverSock,(struct sockaddr*)&clientAddr, &sin_size)) {
            clearDeadWorkers(workers);
            inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
            if (std::string(ip) != ("0.0.0.0")) {
                std::cout << "Connected to  " << ip << std::endl;
                auto w = new server::ClientWorker(clientSock, clientAddr);
                workers[w->id] = w;
                camera.add_sink(w->id, w->stream());
                std::cout << std::to_string(workers.size()) << " workers used" << std::endl;
            }
        }
    }
    std::cout << std::to_string(workers.size()) << " workers left" << std::endl;
    clearAllWorkers(workers);
    std::cout << std::to_string(workers.size()) << " workers left" << std::endl;
    close(serverSock);

    return 0;
}
