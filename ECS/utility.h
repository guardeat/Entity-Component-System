#pragma once

#include <cstdint>
#include <type_traits>

#include "world.h"

namespace Byte {

	template<typename WorldType>
	struct Spawner {
		using World = WorldType;
		using Archetype = typename WorldType::Archetype;
		using Signature = typename WorldType::Signature;
		using EntityID = typename WorldType::EntityID;
		using IDContainer = std::vector<EntityID>;

		template<typename Component, typename... Components>
		static IDContainer spawn(
			World& world, 
			size_t count, 
			Component&& component, 
			Components&&... components) {
			Signature signature{ Signature::template build<Component, Components...>() };

			Archetype* dest;

			auto result{ world._arches.find(signature) };
			if (result != world._arches.end()) {
				dest = &result->second;
			}
			else {
				world._arches[signature] = Archetype::template build<Component, Components...>();
				dest = &world._arches[signature];
			}

			dest->reserve(dest->size() + count);

			IDContainer out;
			out.reserve(dest->size() + count);
			for (size_t idx{}; idx < count; ++idx) {
				EntityID id{ world.createEntity() };
				world._entities[id].arche = dest;
				world._entities[id].index = push(id, dest, component, components...);
				out.push_back(id);
			}

			return out;
		}

	private:
		template<typename Component, typename... Components>
		static size_t push(
			EntityID id, 
			Archetype* dest, 
			Component component, 
			Components... components) {
			size_t index{ dest->pushEntity(id) };

			dest->pushComponent<Component>(std::forward<Component>(component));
			(dest->pushComponent<Components>(std::forward<Components>(components)), ...);

			return index;
		}
	};

}