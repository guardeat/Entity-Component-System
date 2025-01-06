#include "ecs.h"
#include <iostream>
#include <chrono>

using namespace Byte;

int main() {
    World world;

    auto start = std::chrono::high_resolution_clock::now();

    auto ids = Spawner<World>::spawn(world, 1000000, 1, 1.0, 1U);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << " seconds\n";

    return 0;
}