#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>


std::queue<int> buffer;
std::mutex mu;
std::condition_variable not_full;
std::condition_variable not_empty;

void producer(){
    for (int i = 0; i < 10; i++)
    {
        std::unique_lock<std::mutex> lock(mu);
        not_full.wait(lock, [] {return buffer.size() < 15; });
        buffer.push(i);
        not_empty.notify_one();
    }
}

void consumer(){
    for (int i = 0; i < 10; i++)
    {
        std::unique_lock<std::mutex> lock(mu);
        not_empty.wait(lock, [] { return !buffer.size() == 0; });
        buffer.pop();
        not_full.notify_one();
    }
}

int main()
{
    std::thread t1(producer);
    std::thread t2(consumer);

    t1.join();
    t2.join();

    return 0;
}
