#include <vector>
#include <iostream>

const int MATRIX_SIZE = 5;

int main()
{
    std::vector<std::vector<int>> matrixA(
        MATRIX_SIZE, std::vector<int>(MATRIX_SIZE, 2));

    std::vector<std::vector<int>> matrixB(
        MATRIX_SIZE, std::vector<int>(MATRIX_SIZE, 3));

    std::vector<std::vector<int>> matrixC(
        MATRIX_SIZE, std::vector<int>(MATRIX_SIZE, 0));

    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            for (int k = 0; k < MATRIX_SIZE; k++) {
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }

    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            std::cout << matrixC[i][j] << " ";
        }
        std::cout << "\n";
    }

    return 0;
}
