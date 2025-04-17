#pragma once
#include "LightTransform.h"

class AreaLight
{
public:
	AreaLight(std::string fileName, std::string sceneName, bool exists = false);
	~AreaLight();

	int samples = 5;

	std::string name;
	std::string finalPath;
	std::string resourcesPath;
	std::string prefabName;

	LightTransform* transform;

	float3 RandomPointOnLight();

	void Reset();
	void DeleteData();
	void Update();

private:
};