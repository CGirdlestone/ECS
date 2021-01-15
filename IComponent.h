#pragma once



struct IComponent
{
	virtual ~IComponent() {};
};

struct Position : IComponent
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

struct MeshRenderer : IComponent
{
	
	virtual ~MeshRenderer() {};
};

struct AI : IComponent
{

	virtual ~AI() {};
};



