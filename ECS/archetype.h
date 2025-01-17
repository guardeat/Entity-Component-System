#pragma once

#include <unordered_map>
#include <vector>
#include <type_traits>
#include <tuple>
#include <iostream>

#include "accessor.h"
#include "component.h"
#include "signature.h"

namespace Byte {

	template<
		typename _EntityID, 
		template<typename> class _Container,
		size_t _MAX_COMPONENT_COUNT>
	class Archetype {
	public:
		inline static constexpr size_t MAX_COMPONENT_COUNT{ _MAX_COMPONENT_COUNT };
		
		using EntityID = _EntityID;
		template<typename Component>
		using Container = _Container<Component>;
		template<typename Component>
		using Accessor = Accessor<Component, Container>;
		using UAccessor = UAccessor<Container>;
		using AccessorMap = std::unordered_map<ComponentID, UAccessor>;
		using Signature = Signature<MAX_COMPONENT_COUNT>;
		
	private:
		AccessorMap _accessors;
		Signature _signature;

	public:
		Archetype() {
			emplaceAccessor<EntityID>();
		};

		Archetype(const Archetype& left)
			: Archetype{ left.copy() } {
		}

		Archetype(Archetype&& right) noexcept = default;

		Archetype& operator=(const Archetype& left) {
			*this = left.copy();
			return *this;
		}

		Archetype& operator=(Archetype&& left) noexcept = default;

		~Archetype() = default;

		const Signature& signature() {
			return _signature;
		}

		size_t pushEntity(EntityID id) {
			pushComponent(std::move(id));
			return size() - 1;
		}

		template<typename Component>
		void pushComponent(Component&& component) {
			_accessors[Registry<Component>::id()]->receive<Component>().pushBack(std::move(component));
		}

		template<typename Component, typename... Args>
		void emplaceComponent(Args&&... args) {
			_accessors[Registry<Component>::id()]->receive<Component>().emplaceBack(std::forward<Args>(args)...);
		}

		template<typename Component>
		Component& getComponent(size_t index) {
			return _accessors[Registry<Component>::id()]->receive<Component>().get(index);
		}

		template<typename Component>
		const Component& getComponent(size_t index) const {
			return _accessors[Registry<Component>::id()]->receive<Component>().get(index);
		}

		EntityID erase(size_t index) {
			size_t lastIndex{ size() - 1 };

			EntityID out{ getComponent<EntityID>(lastIndex) };

			for (auto& pair : _accessors) {
				pair.second->swapComponents(index, lastIndex);
				pair.second->popBack();
			}

			return out;
		}

		size_t carryEntity(size_t index, EntityID id, Archetype& from) {
			pushEntity(id);
			for (auto& pair : from._accessors) {
				auto accessor{ _accessors.find(pair.first) };
				if (accessor != _accessors.end()) {
					accessor->second->carryComponent(index, pair.second);
				}
			}
			return size() - 1;
		}

		size_t copyEntity(size_t index, EntityID id, const Archetype& from) {
			pushEntity(id);
			for (auto& pair : from._accessors) {
				auto accessor{ _accessors.find(pair.first) };
				if (accessor != _accessors.end()) {
					accessor->second->copyComponent(index, pair.second);
				}
			}
			return size() - 1;
		}

		size_t size() const {
			return _accessors.at(Registry<EntityID>::id())->size();
		}

		bool empty() const {
			return _accessors.at(Registry<EntityID>::id())->size() == 0;
		}

		Archetype copy() const {
			Archetype out;

			out._signature = _signature;

			for (auto& pair : _accessors) {
				out._accessors.emplace(pair.first, pair.second->copy());
			}

			return out;
		}
		
		void clear() {
			for (auto& pair : _accessors) {
				pair.second->clear();
			}
		}

		template<typename Component>
		void emplaceAccessor() {
			_accessors.emplace(Registry<Component>::id(), std::make_unique<Accessor<Component>>());
			_signature.set(Registry<Component>::id());
		}

		void eraseAccessor(ComponentID id) {
			_accessors.erase(id);
			_signature.set(id, false);
		}

		void reserve(size_t newCapacity) {
			for (auto& pair : _accessors) {
				pair.second->reserve(newCapacity);
			}
		}

		template<typename... Components>
		static Archetype build() {
			Archetype out;
			((out.emplaceAccessor<Components>()), ...);

			return out;
		}

		template<typename... Components>
		static Archetype build(Archetype& source) {
			Archetype out{ build<Components...>() };

			for (auto& pair : source._accessors) {
				out._accessors[pair.first] = pair.second->instance();
				out._signature.set(pair.first);
			}
			return out;
		}

		static Archetype build(Archetype& source, ComponentID without) {
			Archetype out;
			out._signature = source._signature;
			out._signature.set(without, false);

			for (auto& pair : source._accessors) {
				if (pair.first != without) {
					out._accessors[pair.first] = pair.second->instance();
				}
			}
			return out;
		}

		template<typename... Components>
		class Cache {
		public:
			using ComponentGroup = std::tuple<Components&...>;
			using AccessorVector = std::vector<IAccessor<Container>*>;

		private:
			AccessorVector _accessors;

		public:
			Cache() = default;

			Cache(Archetype& arche) {
				((_accessors.push_back(arche._accessors.at(Registry<Components>::id()).get())), ...);
			}

			ComponentGroup group(size_t index) {
				size_t iterator{ this->_accessors.size() };
				return ComponentGroup(this->get<Components>(index, --iterator)...);
			}

			size_t size() const {
				if (_accessors.empty()) {
					return 0;
				}
				return _accessors[0]->size();
			}

		private:
			template<typename Component>
			Component& get(size_t index, size_t accessorIndex) {
				return static_cast<Accessor<Component>*>(_accessors[accessorIndex])->get(index);
			}

		};

		template<typename... Components>
		Cache<Components...> cache() {
			return Cache<Components...>(*this);
		}

	};

}