#include "Components.h"

void Position::serialise(std::ofstream& file)
{
	utils::serialiseUint32(file, static_cast<uint32_t>(x));
	utils::serialiseUint32(file, static_cast<uint32_t>(y));
}

void Position::deserialise(const char* buffer, size_t& offset)
{
	x = static_cast<int>(utils::deserialiseUint32(buffer, offset));
	y = static_cast<int>(utils::deserialiseUint32(buffer, offset));
}

void MeshRenderer::serialise(std::ofstream& file)
{

}

void MeshRenderer::deserialise(const char* buffer, size_t& offset)
{

}

void AI::serialise(std::ofstream& file)
{

}

void AI::deserialise(const char* buffer, size_t& offset)
{

}

void RigidBody::serialise(std::ofstream& file)
{

}

void RigidBody::deserialise(const char* buffer, size_t& offset)
{

}

void Sprite::serialise(std::ofstream& file)
{
}

void Sprite::deserialise(const char* buffer, size_t& offset)
{

}

void Model::serialise(std::ofstream& file)
{

}

void Model::deserialise(const char* buffer, size_t& offset)
{

}
