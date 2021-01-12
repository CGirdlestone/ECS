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
			int id{ 1 };
			id = GetID<Position>();

			Assert::AreEqual(0, id);
		}

		TEST_METHOD(CreateMultipleNewComponents)
		{
			int id{ 10 };
			GetID<Position>();
			id = GetID<MeshRenderer>();

			Assert::AreEqual(1, id);
		}

		TEST_METHOD(GetDefinedComponentID)
		{
			int id{ 10 };
			GetID<Position>();
			id = GetID<Position>();

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
			bool has_component = world.HasComponent<Position>(entity);

			Assert::AreEqual(true, has_component);
		}

		TEST_METHOD(GetComponent)
		{
			World world;
			uint64_t entity = world.CreateEntity();
			world.AddComponent<Position>(entity);
			auto *pos = world.GetComponent<Position>(entity);
			
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
			auto mesh = world.GetComponent<MeshRenderer>(entity);
			
			Assert::IsNotNull(mesh);
		}


		TEST_METHOD(GetComponentForSecondEntity)
		{
			World world;
			world.CreateEntity(); // create and discard entity to allow us to test with not the first entity created.
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

			EntityList entities = world.GetEntitiesWith<Position>();

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

			EntityList entities = world.GetEntitiesWith<Position, MeshRenderer>();

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

			EntityList entities = world.GetEntitiesWith<Position, MeshRenderer, AI>();

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

			Position* p = world.GetComponent<Position>(e2); // this should be nullptr from the KillEntity method
			MeshRenderer* m = world.GetComponent<MeshRenderer>(e2); // as should this

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

		TEST_METHOD(Destructor)
		{
			World* world = new World();

			uint64_t e = world->CreateEntity();
			world->AddComponent<Position>(e);

			world->test_destructor();

			auto p = world->GetComponent<Position>(e);

			Assert::IsNull(p);
		}
	};
}
