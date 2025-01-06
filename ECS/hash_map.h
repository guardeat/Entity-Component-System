#pragma once

#include <vector>
#include <utility>
#include <limits>
#include <stdexcept>

namespace Byte {

	struct double_hash_probe {
		inline static constexpr size_t prime_factor{ 7 };

		size_t operator()(const size_t primary_hash, const size_t attempt) const {
			return primary_hash + attempt * (prime_factor - (primary_hash % prime_factor));
		}
	};

	struct linear_probe {
		size_t operator()(const size_t primary_hash, const size_t attempt) const {
			return primary_hash + attempt;
		}
	};

	struct quadratic_probe {
		size_t operator()(const size_t primary_hash, const size_t attempt) const {
			return primary_hash + attempt * attempt;
		}
	};

	template<
		typename _Key,
		typename _Value,
		typename _Hasher = std::hash<_Key>,
		typename _Keyequal = std::equal_to<_Key>,
		typename _Probe = double_hash_probe>
	class hash_map {
	public:
		using key_type = _Key;
		using value_type = _Value;

		using hasher = _Hasher;
		using key_equal = _Keyequal;
		using probe = _Probe;

		using map_node = std::pair<_Key, _Value>;

	private:
		inline static constexpr size_t empty_index{ std::numeric_limits<size_t>::max() };
		inline static constexpr size_t deleted_index{ empty_index - 1 };
		inline static constexpr size_t rehash_factor{ 3 };

		using node_vector = std::vector<map_node>;
		node_vector _nodes;

		struct index_node {
			size_t index{ empty_index };
			size_t hash_value{};
		};

		using index_vector = std::vector<index_node>;
		index_vector _indices;

		hasher _hash;
		key_equal _equal;
		probe _probe;

	public:
		using iterator = typename node_vector::iterator;
		using const_iterator = typename node_vector::const_iterator;

	public:
		hash_map() {
			_indices.resize(rehash_factor);
		}

		value_type& at(const key_type& key) {
			size_t index{ find_index(key) };

			if (index == empty_index) {
				throw std::out_of_range("Key not found");
			}

			return _nodes[_indices[index].index].second;
		}

		const value_type& at(const key_type& key) const {
			size_t index{ find_index(key) };

			if (index == empty_index) {
				throw std::out_of_range("Key not found");
			}

			return _nodes[_indices[index].index].second;
		}

		template<typename... _Args >
		void emplace(_Args... args) {
			_nodes.emplace_back(std::forward<_Args>(args)...);
			size_t primary_hash{ _hash(_nodes.back().first) };
			size_t free_index{ find_free_index(primary_hash) };
			_indices[free_index].index = _nodes.size() - 1;
			_indices[free_index].hash_value = primary_hash;

			if (need_expand()) {
				rehash(_indices.size() * rehash_factor);
			}
		}

		const value_type& operator[](const key_type& key) const {
			return at(key);
		}

		value_type& operator[](const key_type& key) {
			size_t index{ find_index(key) };

			if (index != empty_index) {
				return _nodes[_indices[index].index].second;
			}

			emplace(key,value_type{});

			return _nodes.back().second;
		}

		iterator find(const key_type& key) {
			size_t index = find_index(key);
			if (index != empty_index) {
				return iterator{ _nodes.begin() + _indices[index].index };
			}
			return end();
		}

		const_iterator find(const key_type& key) const {
			size_t index = find_index(key);
			if (index != empty_index) {
				return const_iterator{ _nodes.begin() + _indices[index].index };
			}
			return end();
		}

		void erase(const key_type& key) {
			size_t node_index{ find_index(key) };

			if (node_index != empty_index) {
				size_t back_index{ find_index(_nodes.back().first) };
				size_t node_pos{ _indices[node_index].index };
				_indices[back_index].index = _indices[node_index].index;
				_indices[node_index].index = deleted_index;

				std::swap(_nodes[node_pos], _nodes.back());

				_nodes.pop_back();

				if (need_shrink()) {
					rehash(_indices.size() / rehash_factor);
					_nodes.shrink_to_fit();
				}
			}
		}

		size_t size() const {
			return _nodes.size();
		}

		size_t capacity() const {
			return _nodes.capacity();
		}

		void reserve(size_t new_capacity) {
			_nodes.reserve(new_capacity);
			_indices.resize(rehash_factor * new_capacity);
		}

		iterator begin() {
			return _nodes.begin();
		}

		iterator end() {
			return _nodes.end();
		}

		const_iterator begin() const {
			return _nodes.begin();
		}

		const_iterator end() const {
			return _nodes.end();
		}

		void clear() {
			_nodes.clear();
			_indices.clear();
			_indices.resize(rehash_factor);
		}

	private:
		bool need_expand() const {
			return size() > _indices.size() / rehash_factor;
		}

		bool need_shrink() const {
			return size() * rehash_factor < _indices.size() / rehash_factor;
		}

		bool full_index(size_t index) const {
			return _indices[index].index < deleted_index;
		}

		size_t find_free_index(size_t primary_hash) const {
			size_t hash_value{ primary_hash % _indices.size() };

			for (size_t attempt{ 1 }; _indices[hash_value].index != empty_index; ++attempt) {
				hash_value = _probe(primary_hash, attempt) % _indices.size();
			}

			return hash_value;
		}

		size_t find_index(const key_type& key) const {
			size_t primary_hash{ _hash(key) };
			size_t hash_value{ primary_hash % _indices.size() };

			if (full_index(hash_value) && _equal(_nodes[_indices[hash_value].index].first, key)) {
				return hash_value;
			}

			for (size_t attempt{ 1 }; attempt < _indices.size(); ++attempt) {
				hash_value = _probe(primary_hash, attempt) % _indices.size();
				if (full_index(hash_value) && _equal(_nodes[_indices[hash_value].index].first, key)) {
					return hash_value;
				}
			}

			return empty_index;
		}

		void rehash(size_t new_capacity) {
			index_vector old_indices{ std::move(_indices) };
			_indices.clear();
			_indices.resize(new_capacity);

			for (size_t idx{}; idx < old_indices.size(); ++idx) {
				if (old_indices[idx].index < deleted_index) {
					size_t new_index{ find_free_index(old_indices[idx].hash_value) };
					_indices[new_index] = old_indices[idx];
				}
			}
		}
	};

}