# ECS

This is a templated, header only Entity-Component-System implementation (minus the system part) to support composition over inheritance for games.

Entities are represented by an unsigned 64 bit integer containing a unique ID (the uppermost 16 bits), a version number (for systems to validate against) (another 16 bits)
and a 32-bit component mask (for a maximum of 32 components). 

The entity unique ID is used to index into the component array allowing the entity to be tied to it's associated components.

The `<World>` class is the sole arbiter of 'truth' regarding entities and associated components.

## Example Usage

To use this ECS implementation, include the World.h header file in your project and create an instance.

    World world;

With regards to components, ensure they inherit from `<IComponent>` (no methods defined) and don't create a constructor. 
<del> I intend to implement forwarding of arguments to allow components to be fully constructed whilst adding it to an entity. At the moment, it has to be done after the empty component is added to the entity. </del> 

Arguments can now be supplied 
when adding the component.

    struct YourComponent : IComponent
    {
      ...
    };

To create an entity, use the `<CreateEntity>` method. This will either create a totally fresh entity, or it will recycle an entity id from a pool of discarded ids.

    uint64_t entity = world.CreateEntity();
    
Component types are registered on first use, and components can be added thusly.

    world.AddComponent<YourComponent>(entity); // uses default constructor - will requires setting the data separately.
    world.AddComponent<YourComponent>(entity, arg1, arg2, arg3, ...); // forwards the supplied arguments and uses the defined constructor.

Currently, components can only be added signularly. If you would like to remove a component, it's done the same as above, but using the `<RemoveComponent<YourComponent>(entity)>`
method.

To retrieve components, it can be done in one of two ways. One is to use the `<HasComponent>` method which checks the bit field for the
supplied component and return true/false. The other is to directly attempt to the retrieve the component pointer and if the entity doesn't have this component added, a nullptr will 
be returned.

    if (world.HasComponent<YourComponent>(entity){
      ...
    }
    
or
    
    YourComponent* your_component_pointer = world.GetComponent<YourComponent>(entity);
   
Finally, if you would like get a list of all the entities with given components, this can be achieved by the `<GetEntitiesWith<YourComponent>(std::vector<uint64_t>& entities)>` method.
The method takes an empty `<std::vector<uint64_t>>` and fills it with those entities that have `<YourComponent>`. Up to three components can be requested in one call.

    std::vector<uint64_t> entities;
    world.GetEntitiesWith<YourComponent>(entities);

    std::vector<uint64_t> multi_component_entities;
    world.GetEntitiesWith<YourComponent1, YourComponent2, YourComponent3>(multi_component_entities);
    
 Finally, once you're finished with an entity, your can 'kill' it which adds it's unique ID to a free pool and zeros all of its associated components.
 
    world.KillEntity(entity);
 
 And that's it! It's a simple implementation and won't be winning any prizes for speed, but it's a workable system that's pretty easy to use.
 
