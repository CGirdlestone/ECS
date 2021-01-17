#pragma once

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
	
	virtual ~MeshRenderer() {};
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

