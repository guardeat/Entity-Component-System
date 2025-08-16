#include "ecs.h"
#include <iostream>
#include <chrono>

using namespace Byte;

struct Test {
    Test() {
        std::cout << "Default constructor\n";
    }

    // Copy constructor
    Test(const Test& other) {
        std::cout << "Copy constructor\n";
    }

    // Move constructor
    Test(Test&& other) noexcept {
        std::cout << "Move constructor\n";
    }

    // Copy assignment
    Test& operator=(const Test& other) {
        std::cout << "Copy assignment\n";
        return *this;
    }

    // Move assignment
    Test& operator=(Test&& other) noexcept {
        std::cout << "Move assignment\n";
        return *this;
    }
};

int main() {
    World world;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i{}; i < 1; ++i) {
        Test t{};
        EntityID id{ world.create(t) };

        world.attach(id, i);
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << " seconds\n";

    auto view{ world.components<EntityID, int>() };

    return 0;
}