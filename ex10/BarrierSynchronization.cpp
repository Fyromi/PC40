#include <iostream>
#include <thread>
#include <vector>
#include <barrier>
#include <chrono>

void phase1(int id){
    int duration_ms = 200 * (id + 1);   // each thread takes a different time
    std::cout << "Thread " << id << " starts phase 1 (" << duration_ms << "ms)\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    std::cout << "Thread " << id << " finished phase 1\n";
}

void phase2(int id){
    std::cout << "Thread " << id << " starts phase 2\n";
}

void performtask(int id, std::barrier<> &wait_point){
    phase1(id);
    wait_point.arrive_and_wait();
    phase2(id);
}

int main()
{
    const int N = 5;
    std::vector<std::thread> workers;
    std::barrier wait_point(N);

    for (int i = 0; i < N; ++i)
    {
        workers.emplace_back(performtask, i, std::ref(wait_point));
    }

    for(std::thread& t : workers) {t.join();}

    return 0;
}
