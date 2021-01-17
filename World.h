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
const int MAX_ENTITIES{ 16382 }; // (2^14 - 1) - 1

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

	inline void* get_addr(const size_t index) const {
		return components + index * stride;
	};

	template <typename Component, typename... Args>
	void add(Args... args) {
		new (get_addr(num_elements++)) Component(std::forward<Args>(args)...);
	};

	template <typename Component>
	Component* get(const size_t index) const {
		if (index >= num_elements) {
			return nullptr;
		}

		return reinterpret_cast<Component*>(components + index * stride);
	}

	void swap(const size_t i, const size_t j) {
		auto* c1 = components + i * stride;
		auto* c2 = components + j * stride;
		std::memcpy(c1, c2, stride);
	}

	void erase(const size_t index) {
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

	inline void Flip(uint64_t& entity, const int n) {
		/* Flip the nth bit of entity. */
		entity ^= (static_cast<uint64_t>(1) << n);
	}

	inline void Zero(uint64_t& entity, const int n) {
		/* Zero the nth bit of entity. */
		entity &= (static_cast<uint64_t>(0) << n);
	}

	inline const int Extract(uint64_t entity, const int n) const {
		/* Extract the nth bit of entity. */
		return (entity >> n) & 1;
	}

	inline const uint16_t GetEntityID(const uint64_t entity) const {
		/* Get the top 16 bits of entity. */
		return static_cast<uint16_t>(entity >> 48);
	}

	inline void SetEntityID(uint64_t& entity, const uint16_t id) {
		/* Set the upper 16 bits of uint16_t to the bit pattern of id. */
		entity = ((entity >> 48) | id) << 48;
	}

	inline uint8_t GetEntityVersion(const uint64_t entity) const {
		/* Get the 3rd set of 16 bits. */
		return static_cast<uint8_t>((entity << 16) >> 56);
	}

	void IncreaseEntityVersion(uint64_t& entity) {
		/* Overwrite the version (3rd set of 16 bits) with the previous 
		*  value + 1. 
		*/
		auto current_version = GetEntityVersion(entity);
		current_version++;
		entity = (((entity >> 40) & (0 << 8) | current_version)) << 40;
	}

	uint64_t NewEntity() {
		/* Generate a completely new entity. */
		if (m_entity_counter == MAX_ENTITIES) {
			throw std::runtime_error("Maximum number of entities reached!");
		}

		uint64_t entity{ 0 };
		auto uid = m_entity_counter++;

		SetEntityID(entity, uid);
		m_entities[uid] = entity;
		return entity;
	};

	uint64_t RecycleEntity() {
		/* Retrieve an entity from the free entities pool. */
		auto entity = m_free_entities.back();
		m_free_entities.pop_back();

		return entity;
	}

	void SwapPackedEntities(const int component_id, const uint16_t entity_id, const uint16_t packed_index) {
		auto final_entity_id = m_packed.at(component_id).back();
		// update the values in the sparse array		
		m_sparse.at(component_id)[final_entity_id] = packed_index;
		m_sparse.at(component_id)[entity_id] = MAX_ENTITIES + 1;
		
		// swap entity ids in the packed array and then remove the last 
		// id (the entity which is having the component removed)
		std::iter_swap(m_packed.at(component_id).begin() + packed_index, 
						m_packed.at(component_id).end() - 1);
		m_packed.at(component_id).pop_back();
	}

	void __RemoveComponent(const int component_id, uint64_t & entity) {
		const auto entity_id = GetEntityID(entity);

		if (Extract(entity, component_id) == 1) {
			Zero(entity, component_id);
			auto* pool = m_component_pools.at(component_id);
			m_entities[entity_id] = entity;
			auto packed_index = m_sparse.at(component_id)[entity_id];
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
		for (auto* pool : m_component_pools) {
			delete pool;
			pool = nullptr;
		}
	};

	void test_destructor() {
		for (auto* pool : m_component_pools) {
			delete pool;
			pool = nullptr;
		}
	}

	uint64_t CreateEntity() {
		/* Create a new entity by recycling an 'killed' id, 
		*  or by creating a new id. 
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
		/* Increase the static component counter for each new component 
		*  type used up to a maximum of MAX_COMPONENTS.
		*/
		if (component_counter == MAX_COMPONENTS) {
			throw std::runtime_error("Max number of components exceeded.");
		}

		static int component_id = component_counter++;
		return component_id;
	}


	template <typename Component, typename... Args>
	void AddComponent(uint64_t& entity, Args... args) {
		/* Create an instance of Component type and ties it to the specified 
		*  entity.
		*/
		auto entity_id = GetEntityID(entity);
		auto component_id = GetID<Component>();
		
		// Set the associated bit (if the entity doesn't have this 
		// component already).
		if (Extract(entity, component_id) == 0) {
			Flip(entity, component_id);
			m_entities[entity_id] = entity;
		}
		
		if (m_component_pools.empty() ||
			m_component_pools.size() == component_id) {

			// First use of this Component, so create a new Pool.
			auto* p = new Pool(MAX_ENTITIES, sizeof(Component));
			m_component_pools.emplace_back(p);

			// create and populate the sparse array
			std::array<uint16_t, MAX_ENTITIES> sparse;
			sparse.fill(MAX_ENTITIES + 1);
			m_sparse.insert({ component_id, sparse});
			m_sparse.at(component_id)[entity_id] = 0;
			
			std::vector<uint16_t> packed;
			packed.push_back(entity_id);
			m_packed.insert({ component_id, packed });
		}
		else {
			auto packed_index = m_packed.at(component_id).size();
			m_packed.at(component_id).push_back(entity_id);
			m_sparse.at(component_id)[entity_id] = packed_index;
		}

		auto* pool = m_component_pools.at(component_id);
		pool->add<Component>(std::forward<Args>(args)...);
	}

	template <typename Component>
	bool HasComponent(const uint64_t entity) const {
		/* Query whether the specified entity has the given component. */
		auto component_id = GetID<Component>();

		return Extract(entity, component_id) == 1;
	}

	template <typename Component>
	Component* GetComponent(const uint64_t entity) {
		/* Retrieves a pointer to the given component that is associated 
		*  with the specified entity. If that entity does not have a component 
		*  of that type, nullptr is returned 
		*/
		auto component_id = GetID<Component>();

		Component* p_component{ nullptr };
		
		if (Extract(entity, component_id) == 1) {
			auto entity_id = GetEntityID(entity);
			auto packed_index = m_sparse.at(component_id)[entity_id];
			auto* pool = m_component_pools.at(component_id);
			p_component = pool->get<Component>(packed_index);
		}
		
		return p_component;
	}

	template <typename Component>
	void RemoveComponent(uint64_t& entity) {
		/* If the entity has a component of this type, it is deleted, otherwise nothing happens. */
		auto component_id = GetID<Component>();
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
		auto component_id = GetID<Component>();
		
		for (auto entity_id : m_packed.at(component_id)) {
			if (Extract(m_entities[entity_id], component_id) == 1) {
				entities.push_back(m_entities[entity_id]);
			}
		}
	}

	template <typename Component1, typename Component2>
	void GetEntitiesWith(EntityList& entities) {
		/* Gets all the entities which have both specified components. 
		*  Iterate over the smallest component group, checking whether they 
		*  also have the other component.
		*/
		auto component1_id = GetID<Component1>();
		auto component2_id = GetID<Component2>();

		auto num_component1_elements = m_packed.at(component1_id).size();
		auto num_component2_elements = m_packed.at(component2_id).size();

		if (num_component1_elements <= num_component2_elements) {
			// first type has the fewest entities
			for (auto entity_id : m_packed.at(component1_id)) {
				if (Extract(m_entities[entity_id], component2_id)) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
		else {
			for (auto entity_id : m_packed.at(component2_id)) {
				// second type has the fewest entities
				if (Extract(m_entities[entity_id], component1_id)) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
	}

	template <typename Component1, typename Component2, typename Component3>
	void GetEntitiesWith(EntityList& entities) {
		/* Gets all the entities which have all three specified components. */
		auto component1_id = GetID<Component1>();
		auto component2_id = GetID<Component2>();
		auto component3_id = GetID<Component3>();

		auto num_component1_elements = m_packed.at(component1_id).size();
		auto num_component2_elements = m_packed.at(component2_id).size();
		auto num_component3_elements = m_packed.at(component3_id).size();

		if (num_component1_elements <= num_component2_elements && 
			num_component1_elements <= num_component3_elements) {
			// first type has the fewest entities
			for (auto entity_id : m_packed.at(component1_id)) {
				if (Extract(m_entities[entity_id], component2_id) && Extract(m_entities[entity_id], component3_id)) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
		else if (num_component2_elements <= num_component1_elements &&
				num_component2_elements <= num_component3_elements) {
			// second type has the fewest entities
			for (auto entity_id : m_packed.at(component2_id)) {
				if (Extract(m_entities[entity_id], component1_id) && Extract(m_entities[entity_id], component3_id)) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
		else {
			// third type has the fewest entities
			for (auto entity_id : m_packed.at(component3_id)) {
				if (Extract(m_entities[entity_id], component1_id) && Extract(m_entities[entity_id], component2_id)) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
	}
};