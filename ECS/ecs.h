#pragma once

#include <cstdint>
#include <vector>
#include <random>

#include "world.h"

namespace Byte {

	using EntityID = uint64_t;

	template<typename EntityID>
	struct EntityIDGenerator {
		inline static std::random_device rd;
		inline static std::mt19937_64 generator;             
		inline static std::uniform_int_distribution<uint64_t> distribution;

		static EntityID generate() {
			return EntityID{ distribution(generator) };
		}
	};

	using World = _World<EntityID, EntityIDGenerator, std::vector, 1024>;

}