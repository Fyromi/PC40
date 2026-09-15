#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <iostream>

int tab[] = { 2 , 5 , 6 , 7};

int factorial(int val){
    int result = 1;
    for (size_t i = 1; i <= val ; i++)
    {
        result *= i;
    }
    return result;
}

int main(int argc, char const *argv[])
{
    std::vector<pid_t> children;

    for (size_t i = 0; i < 4 ; i++)
    {
        int fd[2];
        pipe(fd);

        pid_t id_process = fork();

        if(id_process == 0){
            int resultat = factorial(tab[i]);
            close(fd[0]);
            write(fd[1], &resultat ,sizeof(int));
            close(fd[1]);
            _exit(0);
        }
        else if(id_process > 0){
            int resultat;
            close(fd[1]);
            read(fd[0], &resultat, sizeof(int));
            close(fd[0]);
            std::cout << tab[i] << "! = " << resultat << "\n";
            children.push_back(id_process);
        }
    }

    for (pid_t pid : children)
        waitpid(pid, nullptr, 0);

    return 0;
}
