#include "ecs.h"

using namespace Byte;

//TODO: Implement a hashmap for faster include.

int main() {
	World world;

	auto ids{ Spawner<World>::spawn(world,100000,1,1.0,1U) };
}