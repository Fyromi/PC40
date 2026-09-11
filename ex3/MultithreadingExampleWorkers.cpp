#include <thread>
#include <iostream>
#include <chrono>
#include <vector>

const int WAIT_MS = 200;
const int TOTAL_TASKS = 50;

void performTask(int nbTask){
    for (int i = 0; i < nbTask; i++){
            std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_MS));
    }
}

int main() {

    for (int p = 1; p <= 5; p++) {

        double totalSeconds = 0;
        int repetitions = 5;

        for (int r = 0; r < repetitions; r++){

            std::vector<std::thread> workers;

            std::chrono::time_point<std::chrono::high_resolution_clock>
                start = std::chrono::high_resolution_clock::now();

            for (int j = 0; j < p; j++){
                workers.emplace_back(performTask, TOTAL_TASKS/p);
            }

            for(std::thread& t : workers){
                t.join();
            }

            std::chrono::time_point<std::chrono::high_resolution_clock>
                end = std::chrono::high_resolution_clock::now();

            totalSeconds += std::chrono::duration<double>(end - start).count();
        }

        double avgSeconds = totalSeconds / repetitions;
        std::cout << "p = " << p << " -> avg: " << avgSeconds << " s\n";
    }

    return 0;
}