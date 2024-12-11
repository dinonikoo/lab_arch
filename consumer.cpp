#include "consumer.hpp"
#include "task_queue.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <string>
#include <limits>

void consumer(int id) {
    static int fileIndex = 0;

    while (true) {
        cv::Mat image;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            if (!condVar.wait_for(lock, std::chrono::seconds(100), [] { return !taskQueue.empty() || done; })) {
                std::cerr << "Предупреждение: потребитель " << id << " ожидал 100 секунд и завершает работу." << std::endl;
                break;
            }

            if (taskQueue.empty() && done) {
                break;
            }

            image = taskQueue.front();
            taskQueue.pop();
        }

        auto start = std::chrono::high_resolution_clock::now();

        cv::Mat invertedImage;
        try {
            cv::bitwise_not(image, invertedImage);
        }
        catch (...) {
            std::cerr << "Ошибка: неизвестная ошибка при обработке изображения." << std::endl;
            continue;
        }

        std::string outputPath = "output" + std::to_string(++fileIndex) + ".jpg";
        if (!cv::imwrite(outputPath, invertedImage)) {
            std::cerr << "Ошибка: не удалось сохранить файл " << outputPath << std::endl;
            continue;
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        std::cout << "Потребитель " << id << " обработал изображение, результат: " << outputPath
            << ", время: " << duration.count() << " секунд." << std::endl;
    }
}
