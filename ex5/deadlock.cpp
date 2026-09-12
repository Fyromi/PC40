#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

std::mutex mu1;
std::mutex mu2;


void Fonction1(){
    std::lock_guard<std::mutex> lock(mu1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "shared resources\n";
    std::lock_guard<std::mutex> lock2(mu2);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "shared resources\n";
}

void Fonction2(){
    std::lock_guard<std::mutex> lock2(mu1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "shared resources\n";
    std::lock_guard<std::mutex> lock(mu2);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "shared resources\n";
}

int main()
{
    std::thread t1(Fonction1);
    std::thread t2(Fonction2);

    t1.join();
    t2.join();
    return 0;
}
