#pragma once
#include "LightTransform.h"

class SpotLight
{
public:
	SpotLight(std::string fileName, std::string sceneName, bool exists = false);
	~SpotLight();

	float radius;

	std::string name;
	std::string finalPath;
	std::string resourcesPath;
	std::string prefabName;

	LightTransform* transform;

	void Reset();
	void DeleteData();
	void Update();

private:
};