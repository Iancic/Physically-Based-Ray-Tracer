#pragma once
#include "LightTransform.h"

class DirectionalLight
{
public:
	DirectionalLight(std::string fileName, std::string sceneName, bool exists = false);
	~DirectionalLight();

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