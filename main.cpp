#include "task_queue.hpp"
#include "producer.hpp"
#include "consumer.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

int main() {
    std::vector<std::string> imagePaths = {
        "input10.jpg", "input11.jpg", "input1.jpg", "input2.jpg", "input3.jpg",
        "input4.jpg", "input5.jpg", "input6.jpg", "input7.jpg", "input8.jpg", "input9.jpg"
    };

    std::cout << "Введите число потребителей: ";
    int numConsumers = 0;

    while (!(std::cin >> numConsumers) || numConsumers <= 0) {
        std::cerr << "Ошибка: Введите корректное положительное число потребителей." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    if (numConsumers > 1000) {
        std::cerr << "Предупреждение: слишком много потоков потребителей, возможно снижение производительности." << std::endl;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    std::thread producerThread(producer, imagePaths);
    std::vector<std::thread> consumerThreads;

    for (int i = 0; i < numConsumers; ++i) {
        try {
            std::thread consumerThread(consumer, i + 1);
            consumerThreads.push_back(std::move(consumerThread));
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка: исключение при создании потока потребителя: " << e.what() << std::endl;
        }
    }

    producerThread.join();

    for (auto& thread : consumerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> totalDuration = endTime - startTime;

    std::cout << "Все изображения обработаны." << std::endl;
    std::cout << "Общее время с начала работы производителя: " << totalDuration.count() << " секунд." << std::endl;

    return 0;
}
