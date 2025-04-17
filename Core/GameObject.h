#pragma once
#include "btBulletDynamicsCommon.h"
#include "Model.h"

#include <string>
#include <fstream>
#include <json.hpp>
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using json = nlohmann::json;

class GameObject
{
public:
	GameObject(std::string fullPath);
	~GameObject();

	std::string physicsType, jsonPath;

	float3 position;
	float3 rotation;
	float scale = 1.f;
	uint modelIndex = 99;

	void Update();
	void Synchronise(tinybvh::BLASInstance* instance);

private:

};