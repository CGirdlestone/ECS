# ECS

This is a templated, header only Entity-Component-System implementation to support composition over inheritance for games.

Entities are represented by an unsigned 32 bit integer containing a unique ID (the uppermost 16 bits), a version number (for systems to validate against) (another 8 bits)
with 8 bits currently unused.

This implementation uses a sparse array for each component type (created upon first adding a component) which is MAX_ENTITIES wide and is used to determine whether an entity has a component or not. If an entity has a component, the value in the sparse array is the index of that entities component in the packed component array. Mirroring the packed component array is a packed array of the same size where the value is the entity id (this makes rearranging the packed component array easier when an entity has a component removed. When a component is first added, a `<Pool>` for it is created which allocated exactly enough memory for exactly MAX_ENTITIES of that component. The pools are stored in a vector, so there is no limit (within your hardware bounds anyway) to the amout of components that can be used. 

The `<World>` class is the sole arbiter of 'truth' regarding entities and associated components.

## Example Usage

To use this ECS implementation, include the World.h header file in your project and create an instance.

    World world;

Components are not required to inherit from any common interface. When adding a component, you can supply arguments which will be forwarded to the component's constructor.
To create an entity, use the `<CreateEntity>` method. This will either create a totally fresh entity, or it will recycle an entity id from a pool of discarded ids.

    auto entity = world.CreateEntity();
    
Component types are registered on first use, and components can be added thusly.

    world.AddComponent<YourComponent>(entity); // uses default constructor - will requires setting the data separately.
    world.AddComponent<YourComponent>(entity, arg1, arg2, arg3, ...); // forwards the supplied arguments and uses the defined constructor.

Currently, components can only be added signularly. If you would like to remove a component, it's done the same as above, but using the `<RemoveComponent<YourComponent>(entity)>`
method.

To retrieve components, use the `<GetComponent<YourComponent>>` method to the retrieve the component pointer. If the entity doesn't have this component added, a nullptr will 
be returned.

    auto* your_component_pointer = world.GetComponent<YourComponent>(entity);
   
Finally, if you would like get a list of all the entities with given components, this can be achieved by the `<GetEntitiesWith<YourComponent>(std::vector<uint32_t>& entities)>` method. The method takes an empty `<std::vector<uint32_t>>` and fills it with those entities that have `<YourComponent>`. Up to three components can be requested in one call.

    std::vector<uint32_t> entities;
    world.GetEntitiesWith<YourComponent>(entities);

    std::vector<uint32_t> multi_component_entities;
    world.GetEntitiesWith<YourComponent1, YourComponent2, YourComponent3>(multi_component_entities);
    
 Finally, once you're finished with an entity, your can 'kill' it which adds it's unique ID to a free pool and zeros all of its associated components.
 
    world.KillEntity(entity);
 
 And that's it!
 
