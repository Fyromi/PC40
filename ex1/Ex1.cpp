#include <iostream>
#include <thread>
#include <vector>
#include <string>

void afficherCompte(std::string name, int id) {
    for (int i = 0; i < 5; i++)
    {
        std::cout << "Thread " << name << ", valeur = " << i+id << std::endl;
    }
}

int main() {

    std::thread t1(afficherCompte, "Numéro 1", 1);
    std::thread t2(afficherCompte, "Numéro 2", 6);

    t1.join();
    t2.join();
}