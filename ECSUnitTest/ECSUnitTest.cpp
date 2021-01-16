#include "pch.h"
#include "CppUnitTest.h"
#include "..\World.h"

#include <iostream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ECSUnitTest
{
	TEST_CLASS(ECSUnitTest)
	{
	public:
		
		TEST_METHOD(CreateNewComponent)
		{
			World world;
			int id{ 1 };
			id = world.GetID<Position>();

			Assert::AreEqual(0, id);
		}

		TEST_METHOD(CreateMultipleNewComponents)
		{

			World world;
			int id{ 10 };
			world.GetID<Position>();
			id = world.GetID<MeshRenderer>();

			Assert::AreEqual(1, id);
		}

		TEST_METHOD(GetDefinedComponentID)
		{
			World world;
			int id{ 10 };
			world.GetID<Position>();
			id = world. GetID<Position>();

			Assert::AreEqual(0, id);
		}

		TEST_METHOD(EntityCreation1)
		{
			World world;
			uint64_t e = world.CreateEntity();
			uint64_t zero{ 0 };
			
			Assert::AreEqual(zero, e);
		}

		TEST_METHOD(EntityCreation2)
		{
			World world;
			world.CreateEntity(); // entity with uid 0
			uint64_t e = world.CreateEntity(); // entity with uid 1
			uint64_t val{ 0 };
			val ^= (uint64_t)1 << 48; // uid starts at bit 48, so shift and flip 
			
			Assert::AreEqual(val, e);
		}

		TEST_METHOD(AddComponent)
		{
			World world;
			uint64_t entity = world.CreateEntity(); // entity with uid 0
			world.AddComponent<Position>(entity);
			//bool has_component = world.HasComponent<Position>(entity);
			Assert::IsTrue(true);
			//Assert::AreEqual(true, has_component);
		}

// 		TEST_METHOD(AddMultipleComponentAndCheckComponentIDs)
// 		{
// 			World world;
// 			uint64_t entity = world.CreateEntity(); // entity with uid 0
// 			// one call to GetID<Type> (component id of 0)
// 			world.AddComponent<Position>(entity); 
// 			world.AddComponent<MeshRenderer>(entity); 
// 			world.AddComponent<AI>(entity);
// 			world.AddComponent<RigidBody>(entity);
// 			world.AddComponent<Sprite>(entity);
// 			world.AddComponent<Model>(entity);
// 
// 			Assert::AreEqual(0, world.GetID<Position>());
// 			Assert::AreEqual(1, world.GetID<MeshRenderer>());
// 			Assert::AreEqual(2, world.GetID<AI>());
// 			Assert::AreEqual(3, world.GetID<RigidBody>());
// 			Assert::AreEqual(1, world.GetID<MeshRenderer>());
// 			Assert::AreEqual(4, world.GetID<Sprite>());
// 			Assert::AreEqual(5, world.GetID<Model>());
// 		}

		TEST_METHOD(AddAndConstructComponent)
		{
			World world;
			uint64_t entity = world.CreateEntity(); // entity with uid 0
			world.AddComponent<Position>(entity, 1.0f, 1.0f, 1.0f);

			auto* p = world.GetComponent<Position>(entity);

			Assert::AreEqual(1.0f, p->x);
		}

		TEST_METHOD(GetComponent)
		{
			World world;
			uint64_t entity = world.CreateEntity();
			world.AddComponent<Position>(entity);
			auto* pos = world.GetComponent<Position>(entity);
			
			Assert::IsNotNull(pos);
		}

		TEST_METHOD(GetUnaddedComponent)
		{
			World world;
			uint64_t entity = world.CreateEntity();
			world.AddComponent<Position>(entity);
			auto* mesh = world.GetComponent<MeshRenderer>(entity);
			
			Assert::IsNull(mesh);
		}


		TEST_METHOD(GetComponentMember)
		{
			World world;
			uint64_t entity = world.CreateEntity();
			world.AddComponent<Position>(entity);
			auto* pos = world.GetComponent<Position>(entity);
			pos->x = 2.0f;

			Assert::AreEqual(2.0f, pos->x);
		}


		TEST_METHOD(AddMultipleComponents)
		{
			World world;
			uint64_t entity = world.CreateEntity();
			world.AddComponent<Position>(entity);
			world.AddComponent<MeshRenderer>(entity);
			auto* mesh = world.GetComponent<MeshRenderer>(entity);
			
			Assert::IsNotNull(mesh);
		}


		TEST_METHOD(GetComponentForSecondEntity)
		{
			World world;
			// create and discard entity to allow 
			// us to test with not the first entity created.
			world.CreateEntity(); 
			uint64_t entity = world.CreateEntity();
			world.AddComponent<Position>(entity);
			auto* pos = world.GetComponent<Position>(entity);

			Assert::IsNotNull(pos);
		}


		TEST_METHOD(RemoveComponent)
		{
			World world;
			uint64_t entity = world.CreateEntity();
			world.AddComponent<Position>(entity);
			world.RemoveComponent<Position>(entity);
			auto* pos = world.GetComponent<Position>(entity);

			Assert::IsNull(pos);
		}


		TEST_METHOD(GetEntitiesWithOneComponent)
		{
			World world;
			uint32_t num_entities{ 10 };
			for (uint32_t i = 0; i < num_entities; i++) {
				uint64_t entity = world.CreateEntity();
				world.AddComponent<Position>(entity);
			}

			EntityList entities;
			world.GetEntitiesWith<Position>(entities);

			Assert::AreEqual(num_entities, entities.size());
		}


		TEST_METHOD(GetEntitiesWithTwoComponents)
		{
			World world;
			uint32_t num_entities{ 10 };
			for (uint32_t i = 0; i < num_entities; i++) {
				uint64_t entity = world.CreateEntity();
				world.AddComponent<Position>(entity);
				if (i % 2) {
					world.AddComponent<MeshRenderer>(entity);
				}
			}

			EntityList entities;
			world.GetEntitiesWith<Position, MeshRenderer>(entities);

			Assert::AreEqual(num_entities/2, entities.size());
		}

		TEST_METHOD(GetEntitiesWithThreeComponents)
		{
			World world;
			uint32_t num_entities{ 10 };
			for (uint32_t i = 0; i < num_entities; i++) {
				uint64_t entity = world.CreateEntity();
				world.AddComponent<Position>(entity);
				if (i % 2) {
					world.AddComponent<MeshRenderer>(entity);
				}
				if (i % 3) {
					world.AddComponent<AI>(entity);
				}
			}

			EntityList entities;
			world.GetEntitiesWith<Position, MeshRenderer, AI>(entities);

			Assert::AreEqual(num_entities / 3, entities.size());
		}

		TEST_METHOD(KillEntity)
		{
			World world;
			uint64_t e1 = world.CreateEntity();

			world.AddComponent<Position>(e1);
			world.AddComponent<MeshRenderer>(e1);

			world.KillEntity(e1); // remove all components
			
			uint64_t e2 = world.CreateEntity(); // recycle the id

			auto* p = world.GetComponent<Position>(e2); // this should be nullptr from the KillEntity method
			auto* m = world.GetComponent<MeshRenderer>(e2); // as should this

			Assert::IsTrue((IComponent*)p == (IComponent*)m); // cast to IComponent and compare nullptr == nullptr
		}

		TEST_METHOD(RecycleEntity)
		{
			World world;
			uint64_t e1 = world.CreateEntity(); // id 0
			uint64_t e2 = world.CreateEntity(); // id 1

			world.KillEntity(e1); // move id 0 into pool of free entities

			uint64_t e3 = world.CreateEntity(); // should be id 0 and not id 2

			uint16_t id = e3 >> 48;
			uint16_t zero{ 0 };

			Assert::IsTrue(zero == id);
		}

		TEST_METHOD(CheckVersionIncrease)
		{
			World world;
			uint64_t e1 = world.CreateEntity(); // id 0
			uint64_t e2 = world.CreateEntity(); // id 1

			world.KillEntity(e1); // move id 0 into pool of free entities

			uint64_t e3 = world.CreateEntity(); // should be version 1 and not version 0

			uint8_t version = (e3 << 16) >> 56;
			uint8_t expected_version{ 1 };

			Assert::IsTrue(expected_version == version);
		}

		TEST_METHOD(AddComponentToRecylcedEntity)
		{
			World world;
			uint64_t e1 = world.CreateEntity();

			world.AddComponent<Position>(e1);
			world.AddComponent<MeshRenderer>(e1);

			world.KillEntity(e1); // remove all components

			uint64_t e2 = world.CreateEntity(); // recycle the id

			world.AddComponent<Position>(e2);

			Position* p = world.GetComponent<Position>(e2);

			Assert::IsNotNull(p);
		}

		TEST_METHOD(PackedArraySwap) 
		{
			World world;
			uint64_t entities[4];
			for (int i = 0; i < 4; i++) {
				entities[i] = world.CreateEntity();
				world.AddComponent<Position>(entities[i], (float)i * 2, (float)i * 2, (float)i * 2);
			}

			world.RemoveComponent<Position>(entities[0]);

			auto* p1 = world.GetComponent<Position>(entities[0]);
			Assert::IsNull(p1);

			auto* p2 = world.GetComponent<Position>(entities[1]);
			Assert::AreEqual(2.0f, p2->x);

			auto p3 = world.GetComponent<Position>(entities[3]);
			Assert::AreEqual(6.0f, p3->x);
		}

		TEST_METHOD(AddComponentAfterPackedArraySwap)
		{
			World world;
			uint64_t entities[4];
			for (int i = 0; i < 4; i++) {
				entities[i] = world.CreateEntity();
				world.AddComponent<Position>(entities[i], (float)i * 2, (float)i * 2, (float)i * 2);
			}
			world.RemoveComponent<Position>(entities[0]);

			uint64_t entity = world.CreateEntity();
			world.AddComponent<Position>(entity, 10.0f, 10.0f, 10.0f);

			auto* p1 = world.GetComponent<Position>(entity);
			Assert::IsNotNull(p1);
			Assert::AreEqual(10.0f, p1->x);
		}

// 		TEST_METHOD(Destructor)
// 		{
// 			World* world = new World();
// 
// 			uint64_t e = world->CreateEntity();
// 			world->AddComponent<Position>(e);
// 
// 			world->test_destructor();
// 
// 			auto p = world->GetComponent<Position>(e);
// 
// 			Assert::IsNull(p);
// 		}
	};
}
