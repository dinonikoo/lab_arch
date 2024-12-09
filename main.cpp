#include <opencv2/opencv.hpp>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <chrono>  // Для измерения времени

std::queue<cv::Mat> taskQueue; // Очередь задач
std::mutex queueMutex;         // Мьютекс для защиты очереди
std::condition_variable condVar;    // Условная переменная для уведомления
bool done = false;             // Флаг завершения работы

// Производитель: загружает изображения и помещает их в очередь
void producer(const std::vector<std::string> &imagePaths) {
    for (size_t i = 0; i < imagePaths.size(); ++i) {
        std::string path = imagePaths[i];
        cv::Mat image = cv::imread(path);

        if (image.empty()) {
            std::cerr << "Ошибка: не удалось загрузить изображение " << path << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            taskQueue.push(image);
        }

        condVar.notify_one(); // Уведомляем потребителя
        std::cout << "Производитель добавил изображение: " << path << std::endl;
    }

    // Завершаем работу производителя
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        done = true;
    }
    condVar.notify_all(); // Уведомляем всех потребителей
}

// Потребитель: обрабатывает изображения из очереди
void consumer(int id) {
    while (true) {
        cv::Mat image;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condVar.wait(lock, [] { return !taskQueue.empty() || done; });

            if (taskQueue.empty() && done) break; // Если всё сделано, выходим

            image = taskQueue.front();
            taskQueue.pop();
        }

        std::cout << "Изображение передано потребителю " << id << std::endl;

        // Замер времени
        std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

        // Выполняем инверсию цветов
        cv::Mat invertedImage;
        cv::bitwise_not(image, invertedImage);

        // Сохраняем результат
        static int fileIndex = 0;
        std::string outputPath = "output" + std::to_string(++fileIndex) + ".jpg";
        cv::imwrite(outputPath, invertedImage);

        // Замер времени после завершения обработки
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        std::cout << "Потребитель " << id << " обработал изображение, результат: " << outputPath 
                  << ", время: " << duration.count() << " секунд." << std::endl;
    }
}

int main() {
    // Пути к изображениям
    std::vector<std::string> imagePaths;
    imagePaths.push_back("input1.jpg");
    imagePaths.push_back("input2.jpg");
    imagePaths.push_back("input3.jpg");

    std::cout << "Введите число потребителей: ";

    // Количество потребителей
    int numConsumers = 0;
    std::cin >> numConsumers;

    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

    // Создаём поток производителя
    std::thread producerThread(producer, imagePaths);

    // Создаём вектор потоков для потребителей
    std::vector<std::thread> consumerThreads;

    // Запускаем потребителей
    for (int i = 0; i < numConsumers; ++i) {
        std::thread consumerThread(consumer, i + 1);
        consumerThreads.push_back(std::move(consumerThread));
    }

    // Ожидаем завершения работы производителя
    producerThread.join();

    // Ожидаем завершения работы всех потребителей
    for (size_t i = 0; i < consumerThreads.size(); ++i) {
        consumerThreads[i].join();
    }

    std::cout << "Все изображения обработаны." << std::endl;

    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> totalDuration = endTime - startTime;

    std::cout << "Общее время с начала работы производителя: " << totalDuration.count() << " секунд." << std::endl;

    return 0;
}
