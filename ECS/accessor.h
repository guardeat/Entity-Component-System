#pragma once

#include <memory>
#include <utility>
#include <type_traits>
#include <algorithm>

namespace Byte {

	template<template<typename> class Container>
	class IAccessor;

	template<template<typename> class Container>
	using UAccessor = std::unique_ptr<IAccessor<Container>>;

	template<typename Component, template<typename> class Container>
	class Accessor;

	template<template<typename> class Container>
	class IAccessor {
	public:
		virtual ~IAccessor() = default;

		virtual UAccessor<Container> copy() const = 0;

		virtual UAccessor<Container> carry() = 0;

		virtual void copyComponent(size_t _index, const UAccessor<Container>& from) = 0;

		virtual void carryComponent(size_t _index, UAccessor<Container>& from) = 0;

		virtual void swapComponents(size_t first, size_t second) = 0;

		virtual void popBack() = 0;

		virtual size_t size() const = 0;

		virtual void reserve(size_t newCapacity) = 0;

		virtual size_t capacity() const = 0;

		virtual void clear() = 0;

		virtual UAccessor<Container> clone() const = 0;

		template<typename Component>
		Accessor<Component, Container>& receive() {
			return *static_cast<Accessor<Component, Container>*>(this);
		}

	};

	template <typename Component, template<typename> class Container>
	class Accessor: public IAccessor<Container> {
	public:
		using ComponentContainer = Container<Component>;

	private:
		ComponentContainer container;

	public:
		UAccessor<Container> copy() const override {
			return std::make_unique<Accessor>(*this);
		}

		UAccessor<Container> carry() override {
			return std::make_unique<Accessor>(std::move(*this));
		}

		void copyComponent(size_t _index, const UAccessor<Container>& from) override {
			Accessor* castedFrom{ static_cast<Accessor*>(from.get()) };
			container.push_back(castedFrom->get(_index));
		}

		void carryComponent(size_t _index, UAccessor<Container>& from) override {
			Accessor* castedFrom{ static_cast<Accessor*>(from.get()) };
			container.push_back(std::move(castedFrom->get(_index)));
		}

		void swapComponents(size_t first, size_t second) override {
			std::swap(container.at(first), container.at(second));
		}

		void popBack()  override {
			container.pop_back();
		}

		size_t size() const override {
			return container.size();
		}

		void reserve(size_t newCapacity) override {
			container.reserve(newCapacity);
		}

		size_t capacity() const override {
			return container.capacity();
		}

		void clear() override {
			container.clear();
		}

		UAccessor<Container> clone() const override {
			return std::make_unique<Accessor>();
		}

		Component& get(size_t _index) {
			return container.at(_index);
		}

		const Component& get(size_t _index) const {
			return container.at(_index);
		}

		void pushBack(const Component& component) {
			container.push_back(component);
		}

		void pushBack(Component&& component) {
			container.push_back(std::move(component));
		}
		
		template<typename... Args>
		void emplaceBack(Args&&... args) {
			container.emplace_back(std::forward<Args>(args)...);
		}
 
	};

}