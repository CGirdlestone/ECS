#pragma once



struct IComponent
{
	virtual ~IComponent() {};
};

struct Position : IComponent
{
	float x{ 0.0f }, y{ 0.0f }, z{ 0.0f };

	virtual ~Position() {};
};

struct MeshRenderer : IComponent
{
	
	virtual ~MeshRenderer() {};
};

struct AI : IComponent
{

	virtual ~AI() {};
};



