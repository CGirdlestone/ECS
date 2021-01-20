#pragma once

#include <fstream>;

struct Position
{
	Position()
	{
	};
	Position(float _x, float _y, float _z) :
		x(_x), y(_y), z(_z)
	{
	};
	virtual ~Position() 
	{
	};

	float x{ 0.0f }, y{ 0.0f }, z{ 0.0f };
};

struct MeshRenderer
{
	MeshRenderer() {};
	MeshRenderer(unsigned int _id) : id(_id) {};
	virtual ~MeshRenderer() {};
	unsigned int id;
};

struct AI
{
	virtual ~AI() {};
};

struct RigidBody
{
	virtual ~RigidBody() {};
};

struct Sprite
{
	virtual ~Sprite() {};
};

struct Model
{
	virtual ~Model() {};
};


