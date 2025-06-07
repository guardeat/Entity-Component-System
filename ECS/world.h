#pragma once

#include <unordered_map>
#include <type_traits>
#include <vector>

#include "archetype.h"
#include "component.h"
#include "signature.h"
#include "hash_map.h"

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
			size_t _index{};
			Archetype* arche{ nullptr };
		};

		using EntityMap = hash_map<EntityID, EntityData>;

	private:
		template<typename WorldType>
		friend struct Spawner;

		ArcheMap _arches;
		EntityMap _entities;

	public:
		_World() = default;

		_World(const _World& left)
			: _World{ left.copy() } {
		}

		_World(_World&& right) noexcept = default;

		_World& operator=(const _World& left) {
			*this = left.copy();
			return *this;
		}

		_World& operator=(_World&& right) noexcept = default;

		EntityID createEntity() {
			EntityID id{ EntityIDGenerator::generate() };
			_entities.emplace(id,EntityData{});
			return id;
		}

		template<typename Component, typename... Components>
		EntityID createEntity(Component&& component, Components&&... components) {
			EntityID out{ createEntity() };
			attach(out, std::forward<Component>(component), std::forward<Components>(components)...);
			return out;
		}

		void destroyEntity(EntityID id) {
			EntityData& data{ _entities.at(id) };
			data.arche->erase(data._index);
			_entities.erase(id);
		}

		EntityID copyEntity(EntityID source) {
			EntityID out{ createEntity() };
			EntityData& sourceData{ _entities.at(source) };
			sourceData.arche->copyEntity(sourceData._index, out, *sourceData.arche);
			_entities.at(out).arche = sourceData.arche;
			return out;
		}

		template<typename Component, typename... Components>
		void attach(EntityID id, Component&& component, Components&&... components) {
			Signature signature{ Signature::template build<EntityID,Component,Components...>() };

			EntityData& data{ _entities.at(id) };

			Archetype* oldArche{ data.arche };
			Archetype* newArche{ nullptr };

			if (oldArche) {
				signature += oldArche->signature();
			}

			auto result{ _arches.find(signature) };
			if (result != _arches.end()){
				newArche = &result->second;
			}
			else {
				if (oldArche) {
					_arches.emplace(signature,Archetype::template build<Component, Components...>(*oldArche));
				}
				else {
					_arches.emplace(signature,Archetype::template build<Component, Components...>());
				}
				newArche = &_arches[signature];
			}

			size_t newIndex;
			if (oldArche) {
				newIndex = newArche->carryEntity(data._index, id, *oldArche);

				EntityID changedEntity{ oldArche->erase(data._index) };
				_entities[changedEntity]._index = _entities[id]._index;
			}
			else {
				newIndex = newArche->pushEntity(id);
			}

			newArche->pushComponent<Component>(std::forward<Component>(component));
			(newArche->pushComponent<Components>(std::forward<Components>(components)), ...);

			data.arche = newArche;
			data._index = newIndex;
		}

		template<typename Component>
		void detach(EntityID id) {
			EntityData& data{ _entities.at(id) };

			Archetype* oldArche{ data.arche };

			Signature signature{ oldArche->signature() };
			signature.set(Registry<Component>::id(), false);

			size_t newIndex{ 0 };
			Archetype* newArche{ nullptr };

			if (signature.any()) {
				auto result{ _arches.find(signature) };

				if (result == _arches.end()) {
					_arches.emplace(signature, Archetype::build(*oldArche, Registry<Component>::id()));
					newArche = &_arches[signature];
				}
				else {
					newArche = &result->second;
				}
				newIndex = newArche->carryEntity(data._index, id, *oldArche);
			}
			EntityID changedEntity{ oldArche->erase(data._index) };
			_entities[changedEntity]._index = data._index;

			data._index = newIndex;
			data.arche = newArche;
		}

		template<typename Component>
		Component& get(EntityID id) {
			EntityData& data{ _entities.at(id) };
			return data.arche->getComponent<Component>(data._index);
		}

		template<typename Component>
		const Component& get(EntityID id) const {
			EntityData& data{ _entities.at(id) };
			return data.arche->getComponent<Component>(data._index);
		}

		template<typename Component>
		bool has(EntityID id) {
			Archetype* arche{ _entities.at(id).arche };

			if (arche) {
				return arche->signature().test(Registry<Component>::id());
			}

			return false;
		}
		
		size_t size() const {
			return _entities.size();
		}

		_World copy() const {
			_World out;
			out._arches = _arches;
			out._entities = _entities;

			for (auto pair: _entities) {
				auto& test{ out._entities.at(pair.first) };
				auto& test2{ out._arches.at(pair.second.arche->signature()) };
				out._entities.at(pair.first).arche = &out._arches.at(pair.second.arche->signature());
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
			ArcheVector* _arches;
			size_t _cacheIndex;
			size_t _index;
			Cache _cache;

		public:
			ViewIterator(ArcheVector& _archeVector, size_t _cacheIndex, size_t _index)
				: _arches{ &_archeVector }, _cacheIndex{ _cacheIndex }, _index{ _index } {
				if (_cacheIndex < _arches->size()) {
					_cache = Cache{ *_arches->at(_cacheIndex) };
				}
			}

			ViewIterator& operator++() {
				++_index;

				if (_index == _cache.size()) {
					_index = 0;
					++_cacheIndex;
					if (_cacheIndex != _arches->size()) {
						_cache = Cache{ *_arches->at(_cacheIndex) };
					}
				}

				return *this;
			}

			ComponentGroup operator*() {
				return _cache.group(_index);
			}

			bool operator==(const ViewIterator& left) const {
				return _cacheIndex == left._cacheIndex;
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
			ArcheVector _archeVector;

		public:
			View(_World& world) {
				Signature signature{ Signature::template build<Components...>() };

				for (auto& pair : world._arches) {
					if (pair.second.signature().includes(signature) && !pair.second.empty()) {
						_archeVector.push_back(&pair.second);
					}
				}
			}

			Iterator begin() {
				return Iterator{ _archeVector,0,0 };
			}

			Iterator end() {
				return Iterator{ _archeVector, _archeVector.size(), 0 };
			}

			template<typename... _Components>
			View include() {
				Signature signature{ Signature::template build<_Components...>() };
				ArcheVector newArches;

				for (auto arche : _archeVector) {
					if (!arche->signature().matches(signature) && !arche->empty()) {
						newArches.push_back(arche);
					}
				}

				_archeVector = newArches;
			}

			template<typename... _Components>
			View exclude() {
				Signature signature{ Signature::template build<_Components...>() };
				ArcheVector newArches;

				for (auto arche : _archeVector) {
					if (!arche->signature().matches(signature) && !arche->empty()) {
						newArches.push_back(arche);
					}
				}

				_archeVector = newArches;
			}

		};

		template<typename... Components>
		View<Components...> components() {
			return View<Components...>{ *this };
		}

	};

}