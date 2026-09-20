#include <iostream>
#include <vector>
#include <random>
#include <unistd.h>
#include <sys/wait.h>


int& mapPhase(int& val){
    val = val*val;
    return val;
}

int mapReduce(const std::vector<int>& workers){
    int result = 0;
    for (int i = 0; i < workers.size(); ++i)
    {
       result += workers[i];
    }
    return result;
}

std::vector<int> initialiseTabNumber(const int &size){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(1, 100);

    std::vector<int> tabNumber;

    for (int i = 0; i < size; i++)
    {
        tabNumber.emplace_back(distrib(gen));
    }
    
    return tabNumber;
    
}

int main()
{
    //nombre de valeur générée (et donc de process associé)
    int tabNumberSize = 1500000;

    std::vector<int> tabNumber = initialiseTabNumber(tabNumberSize);

    std::vector<pid_t> workers;
    std::cout << "Tab initial [";

    for (int i = 0; i < tabNumberSize; ++i)
    {
        int fd[2];
        pipe(fd);

        std::cout << tabNumber[i] << ", ";

        pid_t process_id = fork();

        workers.emplace_back(process_id);

        if (process_id == 0)
        {
            int val = mapPhase(tabNumber[i]);
            close(fd[0]);

            write(fd[1], &val , sizeof(int));

            _exit(0);
        }
        else if(process_id > 0){
            close(fd[1]);
            read(fd[0], &tabNumber[i], sizeof(int));
        }
    }
    std::cout << "]" << std::endl;

    for (pid_t pid : workers)
        waitpid(pid, nullptr, 0);

    int result = mapReduce(tabNumber);

    std::cout << result << std::endl;

    return 0;
}
