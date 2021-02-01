#pragma once

#include <fstream>;
#include "Utils.hpp"

struct ISerializeable {
	virtual void serialise(std::ofstream& file) = 0;
	virtual void deserialise(const char* buffer, size_t& offset) = 0;
};

struct Position : public ISerializeable
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

	virtual void serialise(std::ofstream& file) override;
	virtual void deserialise(const char* buffer, size_t& offset) override;
};

struct MeshRenderer : public ISerializeable
{
	MeshRenderer() {};
	MeshRenderer(unsigned int _id) : id(_id) {};
	virtual ~MeshRenderer() {};
	unsigned int id;

	virtual void serialise(std::ofstream& file) override;
	virtual void deserialise(const char* buffer, size_t& offset) override;
};

struct AI : public ISerializeable
{
	virtual ~AI() {};
	virtual void serialise(std::ofstream& file) override;
	virtual void deserialise(const char* buffer, size_t& offset) override;
};

struct RigidBody : public ISerializeable
{
	virtual ~RigidBody() {};
	virtual void serialise(std::ofstream& file) override;
	virtual void deserialise(const char* buffer, size_t& offset) override;
};

struct Sprite : public ISerializeable
{
	virtual ~Sprite() {};
	virtual void serialise(std::ofstream& file) override;
	virtual void deserialise(const char* buffer, size_t& offset) override;
};

struct Model : public ISerializeable
{
	virtual ~Model() {};
	virtual void serialise(std::ofstream& file) override;
	virtual void deserialise(const char* buffer, size_t& offset) override;
};


