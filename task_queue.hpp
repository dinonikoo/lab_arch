#ifndef TASK_QUEUE_HPP
#define TASK_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <opencv2/opencv.hpp>

extern std::queue<cv::Mat> taskQueue;
extern std::mutex queueMutex;
extern std::condition_variable condVar;
extern bool done;

#endif // TASK_QUEUE_HPP
