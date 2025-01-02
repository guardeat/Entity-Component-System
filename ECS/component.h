#pragma once

#include <cstdint>

namespace Byte {

	using ComponentID = uint32_t;

	class ComponentIDGenerator {
	private:
		inline static ComponentID nextID{ 0 };

	public:
		template<typename Component>
		static ComponentID generate() {
			return nextID++;
		}

	};

	template<typename Component>
	class Registry {
	private:
		inline static ComponentID _id{ ComponentIDGenerator::template generate<Component>() };

	public:
		static ComponentID id() {
			return _id;
		}

		static void set(ComponentID newID) {
			_id = newID;
		}

	};

}