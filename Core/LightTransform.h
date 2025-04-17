#pragma once
#include <fstream>
#include <json.hpp>
#include <iostream>
#include <filesystem>
using json = nlohmann::json;

class LightTransform
{
public:
	LightTransform(const std::string path);
	~LightTransform();

	float3 defaultPosition{ 0.f, 0.f, 0.f };
	float3 defaultRotation{ 0.f, 0.f, 0.f };
	float3 defaultColor{ 0.0f, 0.0f , 0.0f };

	float3 position{ defaultPosition };
	float3 rotation{ defaultRotation };
	float3 color{ defaultColor };

	std::string jsonPath;

	void Update();
	void Reset();

private:
};

