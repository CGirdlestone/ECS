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
#include <fstream>

#include "Components.h"
#include "Utils.hpp"

const int MAX_COMPONENTS{ 40 };
const int MAX_ENTITIES{ 16382 }; // (2^14 - 1) - 1

/*
* Entity - 32 bits
* 
* | 16 bits			|	8 bits		|	8 bits				| 
* | unique ID		|	version		|	currently unused	|
* 
*/

struct Pool
{
	std::unique_ptr<char[]> components{ nullptr };
	size_t stride{ 1 };
	uint16_t num_elements{ 0 };
	uint16_t max_elements{ 0 };

	Pool(uint16_t elements, size_t component_size) : components(nullptr) {
		components = std::make_unique<char[]>(elements * component_size);
		stride = component_size;
		max_elements = elements;
	};

	~Pool() {

	};

	inline void* get_addr(const size_t index) const {
		return components.get() + index * stride;
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

		return reinterpret_cast<Component*>(components.get() + index * stride);
	}

	void swap(const size_t i, const size_t j) {
		auto* c1 = components.get() + i * stride;
		auto* c2 = components.get() + j * stride;
		std::memcpy(c1, c2, stride);
	}

	void erase(const size_t index) {
		if (num_elements > 1) {
			size_t final_element = num_elements - 1;
			swap(index, final_element);
		}
		num_elements--;
	}

	template <typename Component>
	void serialise(std::ofstream& file) {
		utils::serialiseUint32(file, static_cast<uint32_t>(num_elements));

		for (uint16_t i = 0; i < num_elements; i++) {
			Component* component = get<Component>(i);
			component->serialise(file);
		}
	}

	template <typename Component>
	void deserialise(const char* buffer, size_t& offset) {
		auto _num_elements = static_cast<uint16_t>(utils::deserialiseUint32(buffer, offset));
		num_elements = 0; // reset the number of elements;
		for (uint16_t i = 0; i < _num_elements; i++) {
			add<Component>();
			Component* component = get<Component>(i);
			component->deserialise(buffer, offset);
		}
	}
};


using EntityList = std::vector<uint32_t>;
using ComponentPool = std::vector<std::unique_ptr<Pool>>;
using SparseArray = std::map<int, std::unique_ptr<uint16_t[]>>;
using PackedArray = std::map<int, std::vector<uint16_t>>;
using EntityArray = std::unique_ptr<uint32_t[]>;

class World 
{
private:
	uint16_t m_entity_counter{ 0 };
	SparseArray m_sparse;
	PackedArray m_packed;
	ComponentPool m_component_pools;
	EntityArray m_entities{ std::make_unique<uint32_t[]>(MAX_ENTITIES) };
	EntityList m_free_entities;
	int component_counter{ 0 };

	inline const uint16_t GetEntityID(const uint32_t entity) const {
		/* Get the top 16 bits of entity. */
		return static_cast<uint16_t>(entity >> 16);
	}

	inline void SetEntityID(uint32_t& entity, const uint16_t id) {
		/* Set the upper 16 bits of uint16_t to the bit pattern of id. */
		entity = ((entity >> 16) | id) << 16;
	}

	inline uint8_t GetEntityVersion(const uint32_t entity) const {
		/* Get the 3rd set of 16 bits. */
		return static_cast<uint8_t>((entity << 16) >> 24);
	}

	void IncreaseEntityVersion(uint32_t& entity) {
		/* Overwrite the version (3rd set of 16 bits) with the previous 
		*  value + 1. 
		*/
		auto current_version = GetEntityVersion(entity);
		current_version++;
		entity = (((entity >> 8) & (0 << 8) | current_version)) << 8;
	}

	uint32_t NewEntity() {
		/* Generate a completely new entity. */
		if (m_entity_counter == MAX_ENTITIES) {
			throw std::runtime_error("Maximum number of entities reached!");
		}

		uint32_t entity{ 0 };
		auto uid = m_entity_counter++;

		SetEntityID(entity, uid);
		m_entities[uid] = entity;
		return entity;
	};

	uint32_t RecycleEntity() {
		/* Retrieve an entity from the free entities pool. */
		auto entity = m_free_entities.back();
		m_free_entities.pop_back();

		return entity;
	}

	template <typename Component>
	void InstantiatePool() {
		/* Create the required parts for a new component - the pool, the packed array and the sparse array. */
		
		const int component_id = GetID<Component>();
		
		m_component_pools.emplace_back(std::make_unique<Pool>(MAX_ENTITIES, sizeof(Component)));

		m_sparse.insert({ component_id, std::make_unique<uint16_t[]>(MAX_ENTITIES) });
		// Make sure we initialise all the entries in the sparse array to MAX_ENTITIES + 1 as this means that 
		// the entity doesn't have the component.
		auto* sparse = m_sparse.at(component_id).get();
		for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
			sparse[i] = MAX_ENTITIES + 1;
		}

		m_packed.insert({ component_id, { } });
	}

	void SwapPackedEntities(const int component_id, const uint16_t entity_id, const uint16_t packed_index) {
		/* Updates the packed and sparse arrays when an entity has a component removed, if there are more 
		*  than two entities with the specified component. This is achieved by swapping the positions in the 
		*  packed array, updating the sparse array and then popping the final entity in the packed array.
		*/

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

	void __RemoveComponent(const int component_id, uint32_t & entity) {
		/* If the entity has a component of this type, it is deleted, otherwise nothing happens.
		*  This method allows components to be removed given the integer component id and allows if
		*  therefore to be called from outside of the templated code.
		*/
		
		if (HasComponent(component_id, entity)) {
			const auto entity_id = GetEntityID(entity);
			auto packed_index = m_sparse.at(component_id)[entity_id];

			// if there is more than one of these components, swap the to-be-deleted entry in the packed array
			// with the final entry and then remove it.
			if (m_packed.at(component_id).size() > 1) {
				SwapPackedEntities(component_id, entity_id, packed_index);
			}
			else {
				// otherwise just pop it from the packed array and update the sparse array.
				m_packed.at(component_id).pop_back();
				m_sparse.at(component_id)[entity_id] = MAX_ENTITIES + 1;
			}

			// now erase the component from the pool data.
			auto* pool = m_component_pools.at(component_id).get();
			pool->erase(packed_index);
		}
	}

public:
	World() {};

	~World() {};

	uint32_t CreateEntity() {
		/* Create a new entity by recycling an 'killed' id, 
		*  or by creating a new id. 
		*/
		uint32_t entity;

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

	bool HasComponent(const int component_id, const uint32_t entity) const {
		/* Query whether the specified entity has the given component. */
		if (m_sparse.find(component_id) == m_sparse.end()) {
			// this component has never been added to an entity.
			return false;
		}
		return m_sparse.at(component_id)[GetEntityID(entity)] != MAX_ENTITIES + 1;
	}

	template <typename Component>
	void RegisterComponent() {
		// Register a component before use. This will be used to determine the order in which 
		// component templates are instantiated and therefore allow correct serialisation / 
		// deserialisation. The user must register all components before use.
		auto component_id = GetID<Component>();

		if (m_component_pools.empty() ||
			m_component_pools.size() == component_id) {

			// First use of this Component, so create a new Pool and associated sparse and packed 
			// arrays.
			InstantiatePool<Component>();
		}
	}

	template <typename Component, typename... Args>
	void AddComponent(uint32_t& entity, Args... args) {
		/* Create an instance of Component type and ties it to the specified 
		*  entity.
		*/
		auto entity_id = GetEntityID(entity);
		auto component_id = GetID<Component>();
		
		auto packed_index = m_packed.at(component_id).size();
		m_packed.at(component_id).push_back(entity_id);
		m_sparse.at(component_id)[entity_id] = packed_index;
		

		auto* pool = m_component_pools.at(component_id).get();
		pool->template add<Component>(std::forward<Args>(args)...);
	}

	template <typename Component>
	Component* GetComponent(const uint32_t entity) {
		/* Retrieves a pointer to the given component that is associated 
		*  with the specified entity. If that entity does not have a component 
		*  of that type, nullptr is returned 
		*/
		auto component_id = GetID<Component>();

		Component* p_component{ nullptr };
		
		if (HasComponent(component_id, entity)) {
			auto entity_id = GetEntityID(entity);
			auto packed_index = m_sparse.at(component_id)[entity_id];
			auto* pool = m_component_pools.at(component_id).get();
			p_component = pool->template get<Component>(packed_index);
		}
		
		return p_component;
	}

	template <typename Component>
	void RemoveComponent(uint32_t& entity) {
		/* Forwards the component to be removed. */
		auto component_id = GetID<Component>();
		__RemoveComponent(component_id, entity);
	}

	void KillEntity(uint32_t& entity) {
		/* Sets all components to MAX_ENTITY + 1 to represent no component and 
		*  then moves the id to the free entities vector ready for re-use.
		*/
		for (int i = 0; i < component_counter; i++) {
			if (HasComponent(i, entity)) {
				__RemoveComponent(i, entity);
			}
		}

		m_free_entities.push_back(entity);
	}

	template <typename Component>
	EntityList GetEntitiesWith() {
		/* Gets all the entities which have the specified component. */
		auto component_id = GetID<Component>();
		
		EntityList entities;
		for (auto entity_id : m_packed.at(component_id)) {
			if (HasComponent(component_id, m_entities[entity_id]) == 1) {
				entities.push_back(m_entities[entity_id]);
			}
		}
		return entities;
	}

	template <typename Component1, typename Component2>
	EntityList GetEntitiesWith() {
		/* Gets all the entities which have both specified components. 
		*  Iterate over the smallest component group, checking whether they 
		*  also have the other component.
		*/
		auto component1_id = GetID<Component1>();
		auto component2_id = GetID<Component2>();

		auto num_component1_elements = m_packed.at(component1_id).size();
		auto num_component2_elements = m_packed.at(component2_id).size();

		EntityList entities;
		if (num_component1_elements <= num_component2_elements) {
			// first type has the fewest entities
			for (auto entity_id : m_packed.at(component1_id)) {
				if (HasComponent(component2_id, m_entities[entity_id])) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
		else {
			for (auto entity_id : m_packed.at(component2_id)) {
				// second type has the fewest entities
				if (HasComponent(component1_id, m_entities[entity_id])) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
		return entities;
	}

	template <typename Component1, typename Component2, typename Component3>
	EntityList GetEntitiesWith() {
		/* Gets all the entities which have all three specified components. */
		auto component1_id = GetID<Component1>();
		auto component2_id = GetID<Component2>();
		auto component3_id = GetID<Component3>();

		auto num_component1_elements = m_packed.at(component1_id).size();
		auto num_component2_elements = m_packed.at(component2_id).size();
		auto num_component3_elements = m_packed.at(component3_id).size();

		EntityList entities;
		if (num_component1_elements <= num_component2_elements && 
			num_component1_elements <= num_component3_elements) {
			// first type has the fewest entities
			for (auto entity_id : m_packed.at(component1_id)) {
				if (HasComponent(component2_id, m_entities[entity_id]) && HasComponent(component3_id, m_entities[entity_id])) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
		else if (num_component2_elements <= num_component1_elements &&
				num_component2_elements <= num_component3_elements) {
			// second type has the fewest entities
			for (auto entity_id : m_packed.at(component2_id)) {
				if (HasComponent(component1_id, m_entities[entity_id]) && HasComponent(component3_id, m_entities[entity_id])) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
		else {
			// third type has the fewest entities
			for (auto entity_id : m_packed.at(component3_id)) {
				if (HasComponent(component1_id, m_entities[entity_id]) && HasComponent(component2_id, m_entities[entity_id])) {
					entities.push_back(m_entities[entity_id]);
				}
			}
		}
		return entities;
	}

	template <typename Component>
	std::vector<Component*> GetComponents() {
		/* Gets all the entities which have the specified component. */
		auto component_id = GetID<Component>();

		std::vector<Component*> components;

		for (auto entity_id : m_packed.at(component_id)) {
			if (HasComponent(component_id, m_entities[entity_id]) == 1) {
				auto packed_index = m_sparse.at(component_id)[entity_id];
				components.push_back(GetComponent<Component>(m_entities[entity_id]));
			}
		}

		return components;
	}

	template <typename Component1, typename Component2>
	std::vector<std::tuple<Component1*, Component2*>> GetComponents() {
		/* Gets all the entities which have both specified components.
		*  Iterate over the smallest component group, checking whether they
		*  also have the other component.
		*/

		std::vector<std::tuple<Component1*, Component2*>> components; 

		auto component1_id = GetID<Component1>();
		auto component2_id = GetID<Component2>();

		auto num_component1_elements = m_packed.at(component1_id).size();
		auto num_component2_elements = m_packed.at(component2_id).size();

		if (num_component1_elements <= num_component2_elements) {
			// first type has the fewest entities
			for (auto entity_id : m_packed.at(component1_id)) {
				if (HasComponent(component2_id, m_entities[entity_id])) {
					auto* c1 = GetComponent<Component1>(m_entities[entity_id]);
					auto* c2 = GetComponent<Component2>(m_entities[entity_id]);
					components.push_back(std::make_tuple(c1, c2));
				}
			}
		}
		else {
			for (auto entity_id : m_packed.at(component2_id)) {
				// second type has the fewest entities
				if (HasComponent(component1_id, m_entities[entity_id])) {
					auto* c1 = GetComponent<Component1>(m_entities[entity_id]);
					auto* c2 = GetComponent<Component2>(m_entities[entity_id]);
					components.push_back(std::make_tuple(c1, c2));
				}
			}
		}
		return components;
	}

	template <typename Component1, typename Component2, typename Component3>
	std::vector<std::tuple<Component1*, Component2*, Component3*>> GetComponents() {
		/* Gets the intersection of entities which have all three components and returns a vector of tuples of 
		*  pointers to the components.
		*/
		auto component1_id = GetID<Component1>();
		auto component2_id = GetID<Component2>();
		auto component3_id = GetID<Component3>();

		auto num_component1_elements = m_packed.at(component1_id).size();
		auto num_component2_elements = m_packed.at(component2_id).size();
		auto num_component3_elements = m_packed.at(component3_id).size();

		std::vector<std::tuple<Component1*, Component2*, Component3*>> components;

		if (num_component1_elements <= num_component2_elements &&
			num_component1_elements <= num_component3_elements) {
			// first type has the fewest entities
			for (auto entity_id : m_packed.at(component1_id)) {
				if (HasComponent(component2_id, m_entities[entity_id]) && HasComponent(component3_id, m_entities[entity_id])) {
					auto* c1 = GetComponent<Component1>(m_entities[entity_id]);
					auto* c2 = GetComponent<Component2>(m_entities[entity_id]);
					auto* c3 = GetComponent<Component3>(m_entities[entity_id]);
					components.push_back(std::make_tuple(c1, c2, c3));
				}
			}
		}
		else if (num_component2_elements <= num_component1_elements &&
			num_component2_elements <= num_component3_elements) {
			// second type has the fewest entities
			for (auto entity_id : m_packed.at(component2_id)) {
				if (HasComponent(component1_id, m_entities[entity_id]) && HasComponent(component3_id, m_entities[entity_id])) {
					auto* c1 = GetComponent<Component1>(m_entities[entity_id]);
					auto* c2 = GetComponent<Component2>(m_entities[entity_id]);
					auto* c3 = GetComponent<Component3>(m_entities[entity_id]);
					components.push_back(std::make_tuple(c1, c2, c3));
				}
			}
		}
		else {
			// third type has the fewest entities
			for (auto entity_id : m_packed.at(component3_id)) {
				if (HasComponent(component1_id, m_entities[entity_id]) && HasComponent(component2_id, m_entities[entity_id])) {
					auto* c1 = GetComponent<Component1>(m_entities[entity_id]);
					auto* c2 = GetComponent<Component2>(m_entities[entity_id]);
					auto* c3 = GetComponent<Component3>(m_entities[entity_id]);
					components.push_back(std::make_tuple(c1, c2, c3));
				}
			}
		}
		return components;
	}

	template <typename Component>
	void Serialise(std::ofstream& file) {
		// serialise component-type specific data (component pool, sparse array and packed array)
		auto id = GetID<Component>();

		auto& pool = m_component_pools[id];
		pool.get()->serialise<Component>(file);

		auto& _sparse = m_sparse.at(id);
		for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
			utils::serialiseUint32(file, static_cast<uint32_t>(_sparse.get()[i]));
		}
		auto& _packed = m_packed.at(id);
		utils::serialiseUint32(file, static_cast<uint32_t>(_packed.size()));
		std::for_each(_packed.begin(), _packed.end(), [&file](uint16_t e) {utils::serialiseUint32(file, static_cast<uint32_t>(e)); });
	}

	template <typename Component>
	void Deserialise(const char* buffer, size_t& offset) {
		// serialise component-type specific data (component pool, sparse array and packed array)
		auto id = GetID<Component>();

		auto& pool = m_component_pools[id];
		pool.get()->deserialise<Component>(buffer, offset);

		auto* _sparse = m_sparse.at(id).get();
		for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
			_sparse[i] = static_cast<uint16_t>(utils::deserialiseUint32(buffer, offset));
		}

		auto& _packed = m_packed.at(id);
		_packed.clear();
		auto num_packed_elements = utils::deserialiseUint32(buffer, offset);
		for (uint32_t i = 0; i < num_packed_elements; i++) {
			_packed.push_back(static_cast<uint16_t>(utils::deserialiseUint32(buffer, offset)));
		}

	}

	void Serialise(std::ofstream& file) {
		// serialise the component-type independent data
		utils::serialiseUint32(file, static_cast<uint32_t>(m_entity_counter));
		utils::serialiseUint32(file, static_cast<uint32_t>(m_free_entities.size()));
		std::for_each(m_free_entities.begin(), m_free_entities.end(), [&file](uint32_t x) {utils::serialiseUint32(file, x); });

		for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
			utils::serialiseUint32(file, m_entities[i]);
		}
	}

	void Deserialise(const char* buffer, size_t& offset) {
		// deserialise the component-type independent data
		m_entity_counter = static_cast<uint16_t>(utils::deserialiseUint32(buffer, offset));
		auto _num_free_entities = utils::deserialiseUint32(buffer, offset);
		for (uint32_t i = 0; i < _num_free_entities; i++) {
			m_free_entities.push_back(utils::deserialiseUint32(buffer, offset));
		}

		for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
			m_entities[i] = utils::deserialiseUint32(buffer, offset);
		}
	}
};
