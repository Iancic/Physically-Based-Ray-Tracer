#pragma once

class CollisionInfo
{
public:
	CollisionInfo() = default;
	~CollisionInfo() = default;

	string name = "default";
	void* classType = nullptr;
};