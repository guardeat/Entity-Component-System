#include <iostream>

#include "ecs.h"

using namespace Byte;

int main() {
	World world;

	EntityID id{ world.createEntity(17, 11.7f) };

	std::cout << world.get<float>(id);
}