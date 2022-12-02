//
// Created by sanch on 30.11.2022.
//

#ifndef CAMERA_PROG_CAMERA_H
#define CAMERA_PROG_CAMERA_H
#include "FIFO.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <map>

namespace video {

    class Camera {
    public:
        Camera(int camera, cv::Size resolution, utils::FIFO<cv::Mat>* record_queue);
        Camera(int camera, cv::Size resolution);
        virtual ~Camera();
        void add_sink(int id, utils::FIFO<cv::Mat>* queue);
        void remove_sink(int id);
    private:
        void run_loop();
        std::thread loop_thread_;
        int camera_;
        cv::Size resolution_;
        bool stop_;
        bool record_;
        std::map<int, utils::FIFO<cv::Mat>*> sinks;
        utils::FIFO<cv::Mat>* record_queue_;
    };

    Camera::~Camera() {
        stop_ = true;
        if (loop_thread_.joinable()) {
            loop_thread_.join();
        }
    }

    Camera::Camera(int camera, cv::Size resolution) {
        camera_ = camera;
        resolution_ = resolution;
        stop_ = false;
        record_ = false;
        record_queue_ = nullptr;
        loop_thread_ = std::thread ([this]{ run_loop(); });
    }

    Camera::Camera(int camera, cv::Size resolution, utils::FIFO<cv::Mat> *record_queue) {
        camera_ = camera;
        resolution_ = resolution;
        stop_ = false;
        record_ = true;
        record_queue_ = record_queue;
        loop_thread_ = std::thread ([this]{ run_loop(); });
    }

    void Camera::run_loop() {
        cv::VideoCapture cap("v4l2src device=/dev/video" + std::to_string(camera_) +
        " do-timestamp=true ! video/x-raw, width=" + std::to_string(resolution_.width) +
        ", height=" + std::to_string(resolution_.height) +
        ", framerate=10/1, format=YUY2 ! videoscale ! video/x-raw, width=960, height=540 ! " +
        "videoconvert ! video/x-raw, format=BGR ! appsink");
        if (!cap.isOpened()) {
            std::cerr << "Error: Can't open camera!" << std::endl;
            exit(EXIT_FAILURE);
        }

        cv::Mat frame;
        cv::Mat trash_frame;
        while (!stop_) {
            if (!cap.read(frame)) {
                continue;
            }
            for (auto const& sink : sinks) {
                if (sink.second->full()) {
                    sink.second->pop(trash_frame);
                }
                sink.second->push(frame);
            }
            if (record_) {
                if (record_queue_->full()) {
                    record_queue_->pop(trash_frame);
                }
                record_queue_->push(frame);
            }
        }
        cap.release();
    }

    void Camera::add_sink(int id, utils::FIFO<cv::Mat>* queue) {
        if (queue != nullptr) {
            sinks[id] = queue;
        }
    }

    void Camera::remove_sink(int id) {
        sinks.erase(id);
        std::cout << std::to_string(sinks.size()) << " sinks remain" << std::endl;
    }

} // video

#endif //CAMERA_PROG_CAMERA_H
