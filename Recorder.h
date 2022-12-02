//
// Created by sanch on 30.11.2022.
//

#ifndef CAMERA_PROG_RECORDER_H
#define CAMERA_PROG_RECORDER_H
#include "FIFO.h"
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <thread>

namespace video {

    class Recorder {
    public:
        explicit Recorder(const std::string &file_name);
        virtual ~Recorder();
        utils::FIFO<cv::Mat> stream;
    private:
        void run_loop();
        bool stop_;
        std::string file_name_;
        std::thread loop_thread_;
    };

    Recorder::~Recorder() {
        stop_ = true;
        stream.push(cv::Mat(960, 540, CV_8UC3));
        if (loop_thread_.joinable()) {
            loop_thread_.join();
        }
    }

    Recorder::Recorder(const std::string &file_name) : stream(true, 20) {
        file_name_ = file_name;
        stop_ = false;
        loop_thread_ = std::thread ([this]{ run_loop(); });
    }

    void Recorder::run_loop() {
        cv::VideoWriter rec;
        rec.open(std::string("appsrc ! video/x-raw, format=BGR, width=960, height=540 ! autovideoconvert ! ") +
        std::string("x264enc ! h264parse ! mpegtsmux ! filesink location=") + file_name_,
                 cv::CAP_GSTREAMER, 0, 10.0, cv::Size(960, 540), true);
        //rec.open(file_name_, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, cv::Size(960, 540));

        if (!rec.isOpened()) {
            std::cerr << "Error: Can't record video!" << std::endl;
            exit(EXIT_FAILURE);
        }

        cv::Mat frame;
        while (!stop_) {
            stream.pop(frame);
            rec.write(frame);
        }

        rec.release();
    }

} // video

#endif //CAMERA_PROG_RECORDER_H
