#include <iostream>
#include <thread>
#include <mutex>

std::mutex mu;

void increment(int &i){
     for (size_t j = 0; j < 1000000 ; j++)
    {    
        std::lock_guard<std::mutex> lock(mu);
        i++;
    }
}

void decrement(int &i){
    for (size_t j = 0; j < 1000000 ; j++)
    {    
        std::lock_guard<std::mutex> lock(mu);
        i--;
    }
    
}

int main()
{
    int i = 0; 
    
    std::thread t1(increment, std::ref(i));
    std::thread t2(decrement, std::ref(i));

    t1.join();
    t2.join();

    std::cout << i << std::endl;

    return 0;
}
