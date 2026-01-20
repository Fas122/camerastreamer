#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <opencv2/opencv.hpp>

class FrameQueue {
    std::queue<cv::Mat> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    size_t max_size_;

public:
    FrameQueue(size_t max_size = 5) : max_size_(max_size) {}

    void push(const cv::Mat &frame) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (queue_.size() >= max_size_) {
            queue_.pop(); // drop oldest frame
        }
        queue_.push(frame.clone());
        cv_.notify_one();
    }

    cv::Mat pop() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this]{ return !queue_.empty(); });
        cv::Mat frame = queue_.front();
        queue_.pop();
        return frame;
    }
};

#endif // FRAME_QUEUE_H
