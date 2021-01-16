#pragma once

#include <stdint.h>
#include <string>
#include <map>
#include <array>
#include <vector>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <cstring>

#include "IComponent.h"

const int MAX_COMPONENTS{ 40 };
const int MAX_ENTITIES{ 65534 }; // max 16 bit - 1

/*
* Entity - 64 bits
* 
* | 16 bits			|	8 bits		|	40 bits				| 
* | unique ID		|	version		|	component bit mask	|
* 
*/

struct Pool
{
	char* components{ nullptr };
	size_t stride{ 1 };
	uint16_t num_elements{ 0 };
	uint16_t max_elements{ 0 };

	Pool(uint16_t elements, size_t component_size) {
		components = new char[elements * component_size];
		stride = component_size;
		max_elements = elements;
	};

	~Pool() {
		delete[] components;
	};

	inline void* get_addr(const size_t index) {
		return components + index * stride;
	};

	template <typename T, typename... Args>
	void add(Args... args) {
		new (get_addr(num_elements++)) T(std::forward<Args>(args)...);
	};

	template <typename T>
	T* get(const size_t index) {
		if (index >= num_elements) {
			return nullptr;
		}

		return (T*)(components + index * stride);
	}

	void swap(const size_t i, const size_t j) {
		auto* c1 = components + i * stride;
		auto* c2 = components + j * stride;
		std::memcpy(c1, c2, stride);
	}

	void erase(size_t index) {
		if (num_elements > 1) {
			size_t final_element = num_elements - 1;
			swap(index, final_element);
		}
		num_elements--;
	}
};


using EntityList = std::vector<uint64_t>;
using ComponentPool = std::vector<Pool*>;

class World 
{
private:
	uint16_t m_entity_counter{ 0 };
	std::map<int, std::array<uint16_t, MAX_ENTITIES>> m_sparse;
	std::map<int, std::vector<uint16_t>> m_packed;
	ComponentPool m_component_pools;
	std::array<uint64_t, MAX_ENTITIES> m_entities{ 0 };
	EntityList m_free_entities;

	int component_counter{ 0 };

	void Flip(uint64_t& entity, int n) {
		/* Flip the nth bit of entity. */
		entity ^= ((uint64_t)1 << n);
	}

	void Zero(uint64_t& entity, int n) {
		/* Zero the nth bit of entity. */
		entity &= ((uint64_t)0 << n);
	}

	int Extract(uint64_t entity, int n) {
		/* Extract the nth bit of entity. */
		return (entity >> n) & 1;
	}

	uint16_t GetEntityID(uint64_t entity) {
		/* Get the top 16 bits of entity. */
		return (uint16_t)(entity >> 48);
	}

	void SetEntityID(uint64_t& entity, uint16_t id) {
		/* Set the upper 16 bits of uint16_t to the bit pattern of id. */
		entity = ((entity >> 48) | id) << 48;
	}

	uint8_t GetEntityVersion(uint64_t entity) {
		/* Get the 3rd set of 16 bits. */
		return (uint8_t)((entity << 16) >> 56);
	}

	void IncreaseEntityVersion(uint64_t& entity) {
		/* Overwrite the version (3rd set of 16 bits) with the previous value + 1. */
		uint8_t current_version = GetEntityVersion(entity);
		current_version++;
		entity = (((entity >> 40) & (0 << 8) | current_version)) << 40;
	}

	uint64_t NewEntity() {
		/* Generate a completely new entity. */
		uint64_t entity{ 0 };
		if (m_entity_counter == MAX_ENTITIES) {
			throw std::runtime_error("Maximum number of entities reached!");
		}
		uint16_t uid = m_entity_counter++;

		SetEntityID(entity, uid);
		m_entities[uid] = entity;
		return entity;
	};

	uint64_t RecycleEntity() {
		/* Retrieve an entity from the free entities pool. */
		uint64_t entity = m_free_entities.back();
		m_free_entities.pop_back();

		return entity;
	}

	void SwapPackedEntities(int component_id, uint16_t entity_id, uint16_t packed_index) {
		uint16_t final_entity_id = m_packed.at(component_id).back();
		// update the values in the sparse array		
		m_sparse.at(component_id)[final_entity_id] = packed_index;
		m_sparse.at(component_id)[entity_id] = MAX_ENTITIES + 1;
		
		// swap entity ids in the packed array and then remove the last id (the entity which is having the component removed)
		std::iter_swap(m_packed.at(component_id).begin() + packed_index, m_packed.at(component_id).end() - 1);
		m_packed.at(component_id).pop_back();
	}

	void __RemoveComponent(int component_id, uint64_t & entity) {
		uint16_t entity_id = GetEntityID(entity);

		if (Extract(entity, component_id) == 1) {
			Zero(entity, component_id);
			auto* pool = m_component_pools.at(component_id);
			m_entities[entity_id] = entity;
			uint16_t packed_index = m_sparse.at(component_id)[entity_id];
			if (m_packed.at(component_id).size() > 1) {
				SwapPackedEntities(component_id, entity_id, packed_index);
			}
			else {
				m_packed.at(component_id).pop_back();
			}
			pool->erase(packed_index);
		}
	}

public:
	World() {};

	~World() {

	};

	void test_destructor() {
		// TO DO
	}

	uint64_t CreateEntity() {
		/* Create a new entity by recycling an 'killed' id, 
		* or by creating a new id. 
		*/
		uint64_t entity;

		if (!m_free_entities.empty()) {
			entity = RecycleEntity();

			IncreaseEntityVersion(entity);
			m_entities[GetEntityID(entity)] = entity;
		}
		else {
			entity = NewEntity();
		}

		return entity;
	}

	template <typename Component>
	int GetID() {
		/* Increase the static component counter for each new component type used up to a maximum of MAX_COMPONENTS. */
		if (component_counter == MAX_COMPONENTS) {
			throw std::runtime_error("Max number of components exceeded.");
		}

		static int component_id = component_counter++;
		return component_id;
	}


	template <typename Component, typename... Args>
	void AddComponent(uint64_t& entity, Args... args) {
		/* Create an instance of Component type and ties it to the specified entity. */
		uint16_t entity_id = GetEntityID(entity);
		int component_id = GetID<Component>();
		
		
		// Set the associated bit (if the entity doesn't have this component already).
		if (Extract(entity, component_id) == 0) {
			Flip(entity, component_id);
			m_entities[entity_id] = entity;
		}
		
		if (m_component_pools.empty() || m_component_pools.size() == component_id) {
			// First use of this Component, so create a new Pool.
			auto* p = new Pool(MAX_ENTITIES, sizeof(Component));
			m_component_pools.emplace_back(p);

			std::array<uint16_t, MAX_ENTITIES> sparse;
			sparse.fill(MAX_ENTITIES + 1);
			m_sparse.insert({ component_id, sparse});
			m_sparse.at(component_id)[entity_id] = 0;
			
			std::vector<uint16_t> packed;
			packed.push_back(entity_id);
			m_packed.insert({ component_id, packed });
		}
		else {
			uint16_t packed_index = m_packed.at(component_id).size();
			m_packed.at(component_id).push_back(entity_id);
			m_sparse.at(component_id)[entity_id] = packed_index;
		}

		auto* pool = m_component_pools.at(component_id);
		pool->add<Component>(std::forward<Args>(args)...);
	}

	template <typename Component>
	bool HasComponent(uint64_t entity) {
		/* Query whether the specified entity has the given component. */
		int component_id = GetID<Component>();

		return Extract(entity, component_id) == 1;
	}

	template <typename Component>
	Component* GetComponent(uint64_t entity) {
		/* Retrieves a pointer to the given component that is associated with the specified entity.
		*  If that entity does not have a component of that type, nullptr is returned 
		*/
		int component_id = GetID<Component>();

		Component* p_component{ nullptr };
		
		if (Extract(entity, component_id) == 1) {
			uint16_t entity_id = GetEntityID(entity);
			uint16_t packed_index = m_sparse.at(component_id)[entity_id];
			auto* pool = m_component_pools.at(component_id);
			p_component = pool->get<Component>(packed_index);
		}
		
		return p_component;
	}

	template <typename Component>
	void RemoveComponent(uint64_t& entity) {
		/* If the entity has a component of this type, it is deleted, otherwise nothing happens. */
		int component_id = GetID<Component>();
		__RemoveComponent(component_id, entity);
	}

	void KillEntity(uint64_t& entity) {
		/* Zeros the entity's component mask and then moves it id to the free entities vector ready for re-use. */
		for (int i = 0; i < component_counter; i++) {
			if (Extract(entity, i) == 1) {
				__RemoveComponent(i, entity);
			}
		}

		m_free_entities.push_back(entity);
	}

	template <typename Component>
	void GetEntitiesWith(EntityList& entities) {
		/* Gets all the entities which have the specified component. */
		int component_id = GetID<Component>();
		
		for (uint16_t i = 0; i < m_entity_counter; i++) {
			if (Extract(m_entities[i], component_id) == 1) {
				entities.push_back(m_entities[i]);
			}
		}
	}

	template <typename Component1, typename Component2>
	void GetEntitiesWith(EntityList& entities) {
		/* Gets all the entities which have both specified components. */
		int component1_id = GetID<Component1>();
		int component2_id = GetID<Component2>();

		auto num_component1_elements = m_packed.at(component1_id).size();
		auto num_component2_elements = m_packed.at(component2_id).size();

		if (num_component1_elements <= num_component2_elements) {
			for (auto entity : m_packed.at(component1_id)) {
				if (Extract(entity, component2_id)) {
					entities.push_back(m_packed.at(component2_id)[entity]);
				}
			}
		}
		else {
			for (auto entity : m_packed.at(component2_id)) {
				if (Extract(entity, component1_id)) {
					entities.push_back(m_packed.at(component1_id)[entity]);
				}
			}
		}
	}

	template <typename Component1, typename Component2, typename Component3>
	void GetEntitiesWith(EntityList& entities) {
		/* Gets all the entities which have all three specified components. */
		int component1_id = GetID<Component1>();
		int component2_id = GetID<Component2>();
		int component3_id = GetID<Component3>();

		for (uint16_t i = 0; i < m_entity_counter; i++) {
			if (Extract(m_entities[i], component1_id) == 1 && Extract(m_entities[i], component2_id) && Extract(m_entities[i], component3_id)) {
				entities.push_back(m_entities[i]);
			}
		}
	}
};