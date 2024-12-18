#include "task_queue.hpp"

std::queue<cv::Mat> taskQueue;
std::mutex queueMutex;
std::condition_variable condVar;
bool done = false;
