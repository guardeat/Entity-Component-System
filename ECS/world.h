#pragma once

#include <unordered_map>
#include <type_traits>
#include <vector>

#include "archetype.h"
#include "component.h"
#include "signature.h"

namespace Byte {

	template<
	typename _EntityID,
	template<typename> class _EntityIDGenerator,
	template<typename> class _Container,
	size_t _MAX_COMPONENT_COUNT>
	class _World {
	public:
		inline static constexpr size_t MAX_COMPONENT_COUNT{ _MAX_COMPONENT_COUNT };

		using EntityID = _EntityID;
		using EntityIDGenerator = _EntityIDGenerator<EntityID>;
		template<typename Component>
		using Container = _Container<Component>;
		using Archetype = Archetype<EntityID, Container, MAX_COMPONENT_COUNT>;
		using Signature = Signature<MAX_COMPONENT_COUNT>;
		using ArcheMap = std::unordered_map<Signature, Archetype>;

		struct EntityData {
			size_t index;
			Archetype* arche{ nullptr };
		};

		using EntityMap = std::unordered_map<EntityID, EntityData>;

	private:
		ArcheMap arches;
		EntityMap entities;

	public:
		_World() = default;

		_World(const _World& left)
			: _World{ copy() } {
		}

		_World(_World&& right) noexcept = default;

		_World& operator=(const _World& left) {
			*this = left.copy();
			return *this;
		}

		_World& operator=(_World&& right) noexcept = default;

		EntityID createEntity() {
			EntityID id{ EntityIDGenerator::generate() };
			entities.emplace(id,EntityData{});
			return id;
		}

		template<typename Component, typename... Components>
		EntityID createEntity(Component&& component, Components&&... components) {
			EntityID out{ createEntity() };
			attach(out, std::forward<Component>(component), std::forward<Components...>(components)...);
			return out;
		}

		void destroyEntity(EntityID id) {
			entities[id].arche->erase(entities[id].index);
			entities.erase(id);
		}

		EntityID copyEntity(EntityID source) {
			EntityID out{ createEntity() };
			entities[source].arche->copyEntity(entities[source], out, *entities[source].arche);
			entities[out].arche = entities[source].arche;
		}

		template<typename Component, typename... Components>
		void attach(EntityID id, Component&& component, Components&&... components) {
			Signature signature{ Signature::template build<Component,Components...>() };

			Archetype* oldArche{ entities[id].arche };
			Archetype* newArche{ nullptr };

			if (oldArche) {
				signature += oldArche->signature();
			}

			auto result{ arches.find(signature) };
			if (result != arches.end()){
				newArche = &result->second;
			}
			else {
				if (oldArche) {
					arches[signature] = Archetype::template build<Component, Components...>(*oldArche);
				}
				else {
					arches[signature] = Archetype::template build<Component, Components...>();
				}
				newArche = &arches[signature];
			}

			size_t newIndex;
			if (oldArche) {
				newIndex = newArche->carryEntity(entities[id].index, id, *oldArche);

				EntityID changedEntity{ oldArche->erase(entities[id].index) };
				entities[changedEntity].index = entities[id].index;
			}
			else {
				newIndex = newArche->pushEntity(id);
			}

			newArche->pushComponent<Component>(std::forward<Component>(component));
			(newArche->pushComponent<Components>(std::forward<Components>(components)), ...);

			entities[id].arche = newArche;
			entities[id].index = newIndex;
		}

		template<typename Component>
		void detach(EntityID id) {
			Archetype* oldArche{ entities[id].arche };

			Signature signature{ oldArche->signature() };
			signature.set(Registry<Component>::id(), false);

			size_t newIndex{ 0 };
			Archetype* newArche{ nullptr };

			if (signature.any()) {
				auto result{ arches.find(signature) };

				if (result == arches.end()) {
					arches[signature] = Archetype::build(*oldArche, Registry<Component>::id());
					newArche = &arches[signature];
				}
				else {
					newArche = &result->second;
				}
				newIndex = newArche->carryEntity(entities[id].index, id, *oldArche);
			}
			EntityID changedEntity{ oldArche->erase(entities[id].index) };
			entities[changedEntity].index = entities[id].index;

			entities[id].index = newIndex;
			entities[id].arche = newArche;
		}

		template<typename Component>
		Component& get(EntityID id) {
			return entities[id].arche->getComponent<Component>(entities[id].index);
		}

		template<typename Component>
		const Component& get(EntityID id) const {
			return entities[id].arche->getComponent<Component>(entities[id].index);
		}

		template<typename Component>
		bool has(EntityID id) {
			Archetype* arche{ entities[id].arche };

			if (arche) {
				return arche->signature().test(Registry<Component>::id());
			}

			return false;
		}
		
		size_t size() const {
			return entities.size();
		}

		_World copy() const {
			_World out;
			out.arches = arches;
			out.entities = entities;

			for (auto it{ entities.begin() }; it < entities.end(); ++it) {
				out.entities[it.index()] = &out.arches[it->arche->signature];
			}

			return out;
		}

		template<typename... Components>
		class ViewIterator {
		public:
			using ArcheVector = std::vector<Archetype*>;
			using Cache = Archetype::template Cache<Components...>;
			using ComponentGroup = typename Cache::ComponentGroup;

		private:
			ArcheVector* arches;
			size_t cacheIndex;
			size_t index;
			Cache cache;

		public:
			ViewIterator(ArcheVector& archeVector, size_t cacheIndex, size_t index)
				: arches{ &archeVector }, cacheIndex{ cacheIndex }, index{ index } {
				if (cacheIndex < arches->size()) {
					cache = Cache{ *arches->at(cacheIndex) };
				}
			}

			ViewIterator& operator++() {
				++index;

				if (index == cache.size()) {
					index = 0;
					++cacheIndex;
					if (cacheIndex != arches->size()) {
						cache = Cache{ *arches->at(cacheIndex) };
					}
				}

				return *this;
			}

			ComponentGroup operator*() {
				return cache.group(index);
			}

			bool operator==(const ViewIterator& left) const {
				return cacheIndex == left.cacheIndex;
			}

			bool operator!=(const ViewIterator& left) const {
				return !(*this == left);
			}

		};

		template<typename... Components>
		class View {
		public:
			using ArcheVector = std::vector<Archetype*>;
			using Iterator = ViewIterator<Components...>;

		private:
			ArcheVector archeVector;

		public:
			View(_World& world) {
				Signature signature{ Signature::template build<Components...>() };

				for (auto& pair : world.arches) {
					if (pair.second.signature().includes(signature) && !pair.second.empty()) {
						archeVector.push_back(&pair.second);
					}
				}
			}

			Iterator begin() {
				return Iterator{ archeVector,0,0 };
			}

			Iterator end() {
				return Iterator{ archeVector, 0, archeVector.size() };
			}

			template<typename... _Components>
			View include() {
				Signature signature{ Signature::template build<_Components...>() };
				ArcheVector newArches;

				for (auto arche : archeVector) {
					if (!arche->signature().matches(signature) && !arche->empty()) {
						newArches.push_back(arche);
					}
				}

				archeVector = newArches;
			}

			template<typename... _Components>
			View exclude() {
				Signature signature{ Signature::template build<_Components...>() };
				ArcheVector newArches;

				for (auto arche : archeVector) {
					if (!arche->signature().matches(signature) && !arche->empty()) {
						newArches.push_back(arche);
					}
				}

				archeVector = newArches;
			}

		};

		template<typename... Components>
		View<Components...> components() {
			return View<Components...>{ *this };
		}

	};

}