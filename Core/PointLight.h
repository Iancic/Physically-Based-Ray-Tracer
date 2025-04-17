#pragma once
#include "LightTransform.h"

class PointLight
{
public:
	PointLight(std::string fileName, std::string sceneName, bool exists = false);
	~PointLight();

	std::string name;
	std::string finalPath;
	std::string resourcesPath;
	std::string prefabName;

	LightTransform* transform;

	void DeleteData();
	void Reset();
	void Update();

private:
};