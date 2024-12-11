#include "producer.hpp"
#include "task_queue.hpp"
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>

void producer(const std::vector<std::string>& imagePaths) {
    for (const auto& path : imagePaths) {
        if (!std::filesystem::exists(path)) {
            std::cerr << "Ошибка: файл " << path << " не существует!" << std::endl;
            continue;
        }

        cv::Mat image = cv::imread(path);

        if (image.empty()) {
            std::cerr << "Ошибка: не удалось загрузить изображение " << path << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            taskQueue.push(image);
        }

        condVar.notify_one();
        std::cout << "Производитель добавил изображение: " << path << std::endl;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        done = true;
    }
    condVar.notify_all();
}
