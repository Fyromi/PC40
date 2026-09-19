#include <vector>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

const int MATRIX_SIZE = 5;

std::vector<int> computeRow(const std::vector<std::vector<int>> &matrixA,
                             const std::vector<std::vector<int>> &matrixB,
                             int i)
{
    std::vector<int> row(MATRIX_SIZE, 0);
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
        for (int k = 0; k < MATRIX_SIZE; k++)
        {
            row[j] += matrixA[i][k] * matrixB[k][j];
        }
    }
    return row;
}

int main()
{
    std::vector<std::vector<int>> matrixA(
        MATRIX_SIZE, std::vector<int>(MATRIX_SIZE, 2));

    std::vector<std::vector<int>> matrixB(
        MATRIX_SIZE, std::vector<int>(MATRIX_SIZE, 3));

    std::vector<std::vector<int>> matrixC(
        MATRIX_SIZE, std::vector<int>(MATRIX_SIZE, 0));

    std::vector<pid_t> children;

    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        int fd[2];
        pipe(fd);

        pid_t pid = fork();

        if (pid == 0)
        {
            std::vector<int> row = computeRow(matrixA, matrixB, i);
            close(fd[0]);
            write(fd[1], row.data(), MATRIX_SIZE * sizeof(int));
            close(fd[1]);
            _exit(0);
        }
        else
        {
            close(fd[1]);
            read(fd[0], matrixC[i].data(), MATRIX_SIZE * sizeof(int));
            close(fd[0]);
            children.push_back(pid);
        }
    }

    for (pid_t pid : children)
        waitpid(pid, nullptr, 0);

    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            std::cout << matrixC[i][j] << " ";
        }
        std::cout << "\n";
    }

    return 0;
}
