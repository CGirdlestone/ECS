#pragma once

#include <stdint.h>
#include <string>
#include <map>
#include <array>
#include <vector>
#include <algorithm>
#include <memory>
#include <stdexcept>

#include "IComponent.h"

static int component_counter{ 0 };
const int MAX_COMPONENTS{ 32 };
const int MAX_ENTITIES{ 512 };

template <class Component>
int GetID() {
	/* Increase the static component counter for each new component type used up to a maximum of MAX_COMPONENTS. */
	if (component_counter == MAX_COMPONENTS) {
		throw std::runtime_error("Max number of components exceeded.");
	}

	static int component_id = component_counter++;
	return component_id;
}

/*
* Entity - 64 bits
* 
* | 16 bits			|	16 bits		|	32 bits				| 
* | unique ID		|	version		|	component bit mask	|
* 
*/

using EntityList = std::vector<uint64_t>;
using ComponentPool = std::array<IComponent*, MAX_ENTITIES>;


class World 
{
private:
	uint16_t m_entity_counter{ 0 };
	std::map<int, ComponentPool> m_component_pools;
	std::array<uint64_t, MAX_ENTITIES> m_entities{ 0 };
	EntityList m_free_entities;

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

	uint16_t GetEntityVersion(uint64_t entity) {
		/* Get the 3rd set of 16 bits. */
		return (uint16_t)((entity << 16) >> 48);
	}

	void IncreaseEntityVersion(uint64_t& entity) {
		/* Overwrite the version (3rd set of 16 bits) with the previous value + 1. */
		uint16_t current_version = GetEntityVersion(entity);
		current_version++;
		entity = (((entity >> 32) & (0 << 16)) | (GetEntityVersion(entity) + 1)) << 32;
	}

	uint64_t NewEntity() {
		/* Generate a completely new entity. */
		uint64_t entity{ 0 };
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

public:
	World() {};

	~World() {

		std::map<int, ComponentPool>::iterator it;
		for (it = m_component_pools.begin(); it != m_component_pools.end(); it++) {
			auto& pool = it->second;

			for (int i = 0; i < MAX_ENTITIES; i++) {
				auto& component = pool[i];
				delete component;
				pool[i] = nullptr;
			}
		}
	};

	void test_destructor() {
		/* Only used for testing the destructor code. */
		std::map<int, ComponentPool>::iterator it;
		for (it = m_component_pools.begin(); it != m_component_pools.end(); it++) {
			auto& pool = it->second;

			for (int i = 0; i < MAX_ENTITIES; i++) {
				auto& component = pool[i];
				delete component;
				pool[i] = nullptr;
			}
		}
	}

	uint64_t CreateEntity() {
		/* Create a new entity by recycling an 'killed' id, or by creating a new id. */
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

	template <class Component, typename... Args>
	void AddComponent(uint64_t& entity, Args... args) {
		/* Create an instance of Component type and ties it to the specified entity. */
		uint16_t entity_id = GetEntityID(entity);
		int component_id = GetID<Component>();
		
		// Set the associated bit (if the entity doesn't have this component already).
		if (Extract(entity, component_id) == 0) {
			Flip(entity, component_id);
			m_entities[entity_id] = entity;
		}
		
		if (m_component_pools.find(component_id) == m_component_pools.end()) {
			// First use of this Component, so create a new ComponentPool and fill it with nullptr.
			ComponentPool component{ nullptr };
			component.fill(nullptr);
			m_component_pools.insert({ component_id, component });
		}

		// If this entity already has a leftover component instance from the previous version, delete it.
		if (m_component_pools.at(component_id)[entity_id] != nullptr) {
			delete m_component_pools.at(component_id)[entity_id];
		}

		m_component_pools.at(component_id)[entity_id] = new Component(std::forward<Args>(args)...);
	}

	template <class Component>
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
			p_component = (Component*)m_component_pools.at(component_id)[entity_id];
		}
		
		return p_component;
	}

	template <typename Component>
	void RemoveComponent(uint64_t& entity) {
		/* If the entity has a component of this type, it is deleted, otherwise nothing happens. */
		uint16_t entity_id = GetEntityID(entity);
		int component_id = GetID<Component>();

		if (Extract(entity, component_id) == 1) {
			Flip(entity, component_id);
			m_entities[entity_id] = entity;
			delete m_component_pools.at(component_id)[entity_id];
			m_component_pools.at(component_id)[entity_id] = nullptr;
		}
	}

	void KillEntity(uint64_t& entity) {
		/* Zeros the entity's component mask and then moves it id to the free entities vector ready for re-use. */
		for (int i = 0; i < component_counter; i++) {
			Zero(entity, i);
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
		
		for (uint16_t i = 0; i < m_entity_counter; i++) {
			if (Extract(m_entities[i], component1_id) == 1 && Extract(m_entities[i], component2_id)) {
				entities.push_back(m_entities[i]);
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