//
// Created by step305 on 26.11.2022.
//

#ifndef CAMERA_PROG_CLIENTWORKER_H
#define CAMERA_PROG_CLIENTWORKER_H

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <cstring>
#include <cstdio>
#include <thread>
#include <vector>
#include "base64.h"
#include "utils.h"
#include "FIFO.h"

#include <opencv4/opencv2/imgproc/imgproc.hpp>
#include <opencv4/opencv2/highgui/highgui.hpp>
#include <opencv4/opencv2/features2d.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include "opencv4/opencv2/core/core.hpp"

namespace server {

    int worker_id = 0;
    class ClientWorker {
    public:
        ClientWorker(int client, sockaddr_in addr);
        virtual ~ClientWorker();
        void run_loop();
        bool isAlive() const;
        utils::FIFO<cv::Mat>* stream();
        int id;
    private:
        utils::FIFO<cv::Mat> queue_;
        int client_;
        sockaddr_in client_addr_{};
        std::thread worker_;
        bool stopped_;
        bool stop_;
    };

    ClientWorker::ClientWorker(int client, sockaddr_in addr) : queue_(true, 10) {
        id = worker_id;
        worker_id++;
        client_ = client;
        client_addr_ = addr;
        stopped_ = false;
        stop_= false;
        worker_ = std::thread ([this]{run_loop();});
    }

    void ClientWorker::run_loop() {
        char buffer[1024 * 1024];
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr_.sin_addr), ip, INET_ADDRSTRLEN);
        std::cout << ip << std::endl;
        struct timeval timeout{};
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        setsockopt (client_, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                    sizeof(timeout));

        std::vector<uchar> buff;//buffer for coding
        std::vector<int> param(4);
        cv::Mat src = cv::imread("test.jpg");
        cv::Mat frame;
        cv::resize(src, frame, cv::Size(960, 540));
        param[0] = cv::IMWRITE_JPEG_QUALITY;
        param[1] = 40;//default(95) 0-100
        param[2] = cv::IMWRITE_JPEG_OPTIMIZE;
        param[3] = 1;

        std::string encoded_frame_str;

        std::cout << "Start communication with " << ip << std::endl;

        while (!stop_) {
            memset(buffer, 0, 1000);
            long n = recv(client_, buffer, 100, 0);
            if (n < 0) {
                std::cout << "-1" << std::endl;
                continue;
            }
            if (n == 0) {
                std::cout << "disconnected client by timeout: " << ip << std::endl;
                break;
            }
            auto str = std::string(buffer);
            //randu(frame, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));
            queue_.pop(frame);
            cv::imencode(".jpg", frame, buff, param);
            auto *encoded_frame = reinterpret_cast<unsigned char*>(buff.data());
            encoded_frame_str = R"({"image": ")" + base64_encode(encoded_frame, buff.size()) +
                    R"(", "timestamp": )" + std::to_string(utils::get_us()) + "}\r\n";
            send(client_, encoded_frame_str.c_str(), strlen(encoded_frame_str.c_str()), 0);
            if (str == "quit") {
                break;
            }
            if (stop_) {
                break;
            }
        }
        close(client_);
        std::cout << "disconnected client: " << ip << std::endl;
        std::cout.flush();
        stopped_ = true;
    }

    ClientWorker::~ClientWorker(){
        std::cout << "deleted" << std::endl;
        stop_ = true;
        worker_.join();
    }

    bool ClientWorker::isAlive() const {
        return !stopped_;
    }

    utils::FIFO<cv::Mat> *ClientWorker::stream() {
        return &queue_;
    }
} // server

#endif //CAMERA_PROG_CLIENTWORKER_H
