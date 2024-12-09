#include <opencv2/opencv.hpp>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <chrono> 
#include <filesystem>

std::queue<cv::Mat> taskQueue; // очередь задач
std::mutex queueMutex;         // мьютекс для защиты очереди
std::condition_variable condVar;    // condition variable для уведомления потребителей
bool done = false;    


void producer(const std::vector<std::string> &imagePaths) { // производитель загружает изображения и помещает их в очередь
    for (size_t i = 0; i < imagePaths.size(); ++i) {
        std::string path = imagePaths[i];

        // проверка существования файла. нужна эта проверка, чтобы opencv не выводил свои логи и варнинги
        if (!std::filesystem::exists(path)) {
            std::cerr << "Ошибка: файл " << path << " не существует!" << std::endl;
            continue;;
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

        condVar.notify_one(); // уведомляем потребителя
        std::cout << "Производитель добавил изображение: " << path << std::endl;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        done = true;
    }
    condVar.notify_all(); // уведомляем всех потребителей
}


void consumer(int id) { // потребитель обрабатывает изображения из очереди
    while (true) {
        cv::Mat image;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condVar.wait(lock, [] { return !taskQueue.empty() || done; });

            if (taskQueue.empty() && done) {
                break; // если всё сделано, выходим
            }

            image = taskQueue.front();
            taskQueue.pop();
        }

        std::cout << "Изображение передано потребителю " << id << std::endl;

        // замер времени
        std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

        // инверсия
        cv::Mat invertedImage;
        cv::bitwise_not(image, invertedImage);

        // сохраняем результат
        static int fileIndex = 0;
        std::string outputPath = "output" + std::to_string(++fileIndex) + ".jpg";
        cv::imwrite(outputPath, invertedImage); // запись в файл

        // замер времени после завершения обработки
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        std::cout << "Потребитель " << id << " обработал изображение, результат: " << outputPath 
                  << ", время: " << duration.count() << " секунд." << std::endl;
    }
}


int main() {
    // пути к изображениям
    std::vector<std::string> imagePaths;

    //std::cout << "Введите пути к изображениям. Для того, чтобы завершить ввод, введите '.': ";
    //std::string input_string = "";
    //std::cin >> input_string;
    //while (input_string != ".") {
    //    imagePaths.push_back(input_string);
    //    std::cin >> input_string;
    //}

    imagePaths.push_back("input1.jpg");
    imagePaths.push_back("input2.jpg");
    imagePaths.push_back("input3.jpg");

    std::cout << "Введите число потребителей: ";

    // количество потребителей
    int numConsumers = 0;
    std::cin >> numConsumers;

    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

    std::thread producerThread(producer, imagePaths); // поток производителя

    std::vector<std::thread> consumerThreads; // вектор потоков потребителей

    for (int i = 0; i < numConsumers; ++i) {
        std::thread consumerThread(consumer, i + 1);
        consumerThreads.push_back(std::move(consumerThread));
    }

    // ожидаем завершения работы производителя
    producerThread.join();

    // ожидаем завершения работы всех потребителей
    for (size_t i = 0; i < consumerThreads.size(); ++i) {
        consumerThreads[i].join();
    }

    std::cout << "Все изображения обработаны." << std::endl;

    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> totalDuration = endTime - startTime;

    std::cout << "Общее время с начала работы производителя: " << totalDuration.count() << " секунд." << std::endl;

    return 0;
}
