#pragma once

#include <cstdint>
#include <vector>
#include <random>

#include "world.h"
#include "utility.h"

namespace Byte {

    template<typename Type>
    class shrink_vector : public std::vector<Type> {
    public:
        using std::vector<Type>::vector;

        void pop_back() {
            std::vector<Type>::pop_back();
            check_shrink();
        }

        typename std::vector<Type>::iterator erase(typename std::vector<Type>::iterator pos) {
            auto it{ std::vector<Type>::erase(pos) };
            check_shrink();
            return it;
        }

        void resize(size_t new_size) {
            std::vector<Type>::resize(new_size);
            check_shrink();
        }

    private:
        void check_shrink() {
            if (this->size() < this->capacity() / 2) {
                this->shrink_to_fit();
            }
        }
    };

    struct EntityID {
        uint64_t id{};

        explicit operator uint64_t() const {
            return id;
        }

        explicit operator bool() const {
            return static_cast<bool>(id);
        }

        bool operator==(const EntityID& entity) const {
            return entity.id == id;
        }

        bool operator!=(const EntityID& entity) const {
            return entity.id != id;
        }
    };

    template<typename EntityID>
    struct EntityIDGenerator {
        inline static std::random_device rd;
        inline static std::mt19937_64 generator;
        inline static std::uniform_int_distribution<uint64_t> distribution;

        static EntityID generate() {
            return EntityID{ distribution(generator) };
        }
    };

    using World = _World<EntityID, EntityIDGenerator, shrink_vector, 1024>;

}

namespace std {

    template<>
    struct hash<Byte::EntityID> {
        size_t operator()(const Byte::EntityID& entity) const noexcept {
            return std::hash<uint64_t>{}(entity.id);
        }
    };

}