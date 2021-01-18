#pragma once

#include <fstream>;

struct SerializeableComponent
{
	virtual ~SerializeableComponent() {};
	virtual void Serialize(std::ofstream & file) = 0;
	virtual int Deserialize(const char* buffer, int offset) = 0;
};

struct Position : SerializeableComponent
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
	void Serialize(std::ofstream& file) {

	}
	int Deserialize(const char* buffer, int offset) {
		return offset;
	}

	float x{ 0.0f }, y{ 0.0f }, z{ 0.0f };
};

struct MeshRenderer
{
	void Serialize(std::ofstream& file) {

	}
	int Deserialize(const char* buffer, int offset) {
		return offset;
	}
	virtual ~MeshRenderer() {};
};

struct AI
{
	void Serialize(std::ofstream& file) {

	}
	int Deserialize(const char* buffer, int offset) {
		return offset;
	}
	virtual ~AI() {};
};

struct RigidBody
{
	void Serialize(std::ofstream& file) {

	}
	int Deserialize(const char* buffer, int offset) {
		return offset;
	}
	virtual ~RigidBody() {};
};

struct Sprite
{
	void Serialize(std::ofstream& file) {

	}
	int Deserialize(const char* buffer, int offset) {
		return offset;
	}
	virtual ~Sprite() {};
};

struct Model
{
	void Serialize(std::ofstream& file) {

	}
	int Deserialize(const char* buffer, int offset) {
		return offset;
	}
	virtual ~Model() {};
};


