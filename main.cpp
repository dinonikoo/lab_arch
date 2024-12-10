#include <opencv2/opencv.hpp>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <chrono> 
#include <filesystem>
#include <limits> // для проверки переполнения числовых типов

std::queue<cv::Mat> taskQueue; // очередь задач
std::mutex queueMutex;         // мьютекс для защиты очереди
std::condition_variable condVar;    // condition variable для уведомления потребителей
bool done = false;


void producer(const std::vector<std::string>& imagePaths) { // производитель загружает изображения и помещает их в очередь
    for (size_t i = 0; i < imagePaths.size(); ++i) {
        std::string path = imagePaths[i];

        // проверка существования файла. нужна эта проверка, чтобы opencv не выводил свои логи и варнинги
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

            // проверка переполнения очереди задач
            if (taskQueue.size() >= std::numeric_limits<size_t>::max()) {
                std::cerr << "Ошибка: очередь задач переполнена!" << std::endl;
                break; // завершаем чтобы избежать переполнения очереди
            }

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

            // Проверка таймаута ожидания
            if (!condVar.wait_for(lock, std::chrono::seconds(100), [] { return !taskQueue.empty() || done; })) {
                std::cerr << "Предупреждение: потребитель " << id << " ожидал 100 секунд и завершает работу." << std::endl;
                break;
            }

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
        try {
            // проверка на переполнение памяти перед инверсией
            if (image.total() > std::numeric_limits<size_t>::max() / sizeof(uchar)) {
                std::cerr << "Ошибка: изображение слишком велико для обработки." << std::endl;
                continue;
            }
            cv::bitwise_not(image, invertedImage);
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка: исключение при инверсии изображения: " << e.what() << std::endl;
            continue;
        }
        catch (...) {
            std::cerr << "Неизвестная ошибка при инверсии изображения." << std::endl;
            continue;
        }

        // сохраняем результат
        static int fileIndex = 0;

        // защита от переполнения индекса файлов
        if (fileIndex >= std::numeric_limits<int>::max()) {
            std::cerr << "Ошибка: достигнут предел индекса файлов." << std::endl;
            break;
        }

        std::string outputPath = "output" + std::to_string(++fileIndex) + ".jpg";

        // проверка успешной записи файла
        if (!cv::imwrite(outputPath, invertedImage)) {
            std::cerr << "Ошибка: не удалось сохранить файл " << outputPath << std::endl;
            continue;
        }

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
    imagePaths.push_back("input10.jpg");
    imagePaths.push_back("input11.jpg");
    imagePaths.push_back("input1.jpg");
    imagePaths.push_back("input2.jpg");
    imagePaths.push_back("input3.jpg");
    imagePaths.push_back("input4.jpg");
    imagePaths.push_back("input5.jpg");
    imagePaths.push_back("input6.jpg");
    imagePaths.push_back("input7.jpg");
    imagePaths.push_back("input8.jpg");
    imagePaths.push_back("input9.jpg");


    std::cout << "Введите число потребителей: ";

    // количество потребителей
    int numConsumers = 0;

    // проверка на корректность ввода числа потребителей
    while (!(std::cin >> numConsumers) || numConsumers <= 0) {
        std::cerr << "Ошибка: Введите корректное положительное число потребителей." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    // защита от переполнения числа потребителей
    if (numConsumers > 1000) {
        std::cerr << "Предупреждение: слишком много потоков потребителей, возможно снижение производительности." << std::endl;
    }

    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

    std::thread producerThread(producer, imagePaths); // поток производителя

    std::vector<std::thread> consumerThreads; // вектор потоков потребителей

    for (int i = 0; i < numConsumers; ++i) {
        try {
            std::thread consumerThread(consumer, i + 1);
            consumerThreads.push_back(std::move(consumerThread));
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка: исключение при создании потока потребителя: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "Неизвестная ошибка при создании потока потребителя." << std::endl;
        }
    }

    // ожидаем завершения работы производителя
    producerThread.join();

    // ожидаем завершения работы всех потребителей
    for (size_t i = 0; i < consumerThreads.size(); ++i) {
        if (consumerThreads[i].joinable()) {
            consumerThreads[i].join();
        }
        else {
            std::cerr << "Предупреждение: поток потребителя " << i + 1 << " не подлежит присоединению." << std::endl;
        }
    }

    std::cout << "Все изображения обработаны." << std::endl;

    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> totalDuration = endTime - startTime;

    std::cout << "Общее время с начала работы производителя: " << totalDuration.count() << " секунд." << std::endl;

    return 0;
}
