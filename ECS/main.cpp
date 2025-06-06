#include "ecs.h"
#include <iostream>
#include <chrono>

using namespace Byte;

int main() {
    World world;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i{}; i < 1000; ++i) {
        world.createEntity(static_cast<int>(i));
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << " seconds\n";

    auto view{ world.components<EntityID, int>() };

    for (auto [id, i] : view) {
        std::cout << id << " " << i << std::endl;
    }

    return 0;
}