#pragma once

#include <bitset>
#include <array>
#include <cstdint>

#include "component.h"

namespace Byte {

	template<size_t _MAX_COMPONENT_COUNT>
	class Signature {
	public:
		inline static constexpr size_t MAX_COMPONENT_COUNT{ _MAX_COMPONENT_COUNT };
		inline static constexpr size_t BITSET_SIZE{ 64 };
		inline static constexpr size_t BITSET_COUNT{ MAX_COMPONENT_COUNT / BITSET_SIZE + 1 };

		using Bitset = std::bitset<BITSET_SIZE>;
		using BitsetArray = std::array<Bitset, BITSET_COUNT>;

	private:
		BitsetArray _bitsets;

	public:
		void set(ComponentID id, bool value = true) {
			_bitsets[id / BITSET_SIZE].set(id % BITSET_SIZE, value);
		}

		bool test(ComponentID id) const {
			return _bitsets[id / BITSET_SIZE].test(id % BITSET_SIZE);
		}

		bool includes(const Signature& signature) const {
			for (size_t index{}; index < BITSET_COUNT; ++index) {
				if ((_bitsets[index] | signature._bitsets[index]) != _bitsets[index]) {
					return false;
				}
			}
			return true;
		}

		bool matches(const Signature& signature) const {
			for (size_t index{}; index < BITSET_COUNT; ++index) {
				if ((_bitsets[index] & signature._bitsets[index]) > 0) {
					return true;
				}
			}
			return false;
		}

		bool any() const {
			for (Bitset bitset: _bitsets) {
				if (bitset.any()) {
					return true;
				}
			}
			return false;
		}

		bool operator==(const Signature& other) const {
			for (size_t index{}; index < BITSET_COUNT; ++index) {
				if (_bitsets[index] != other._bitsets[index]) {
					return false;
				}
			}
			return true;
		}

		bool operator!=(const Signature& other) const {
			return !(*this == other);
		}

		Signature operator+(const Signature& other) const {
			Signature out;
			for (size_t i = 0; i < BITSET_COUNT; ++i) {
				out._bitsets[i] = _bitsets[i] | other._bitsets[i];
			}
			return out;
		}

		Signature& operator+=(const Signature& other) {
			for (size_t i = 0; i < BITSET_COUNT; ++i) {
				_bitsets[i] |= other._bitsets[i];
			}
			return *this;
		}

		const BitsetArray& data() const {
			return _bitsets;
		}

		template<typename... Components>
		static Signature build() {
			Signature out;
			(out.set(Registry<Components>::id()), ...);
			return out;
		}

	};

}

namespace std {

	template<size_t MAX_COMPONENT_COUNT>
	struct hash<Byte::Signature<MAX_COMPONENT_COUNT>> {

		using Signature = Byte::Signature<MAX_COMPONENT_COUNT>;

		size_t operator()(const Signature& signature) const {
			size_t result{};

			for (size_t i{}; i < signature.BITSET_COUNT; ++i) {
				result += signature.data()[i].to_ullong();
			}

			return result;
		}
	};

}

