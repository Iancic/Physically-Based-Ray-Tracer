#pragma once
#include <fstream>
#include <json.hpp>
#include <iostream>
#include <filesystem>
using json = nlohmann::json;

class Transform
{
public:
	Transform(const std::string path);
	~Transform();

	float3 defaultPosition{ 0.f, 0.f, 0.f };
	float3 defaultRotation{ 0.f, 0.f, 0.f };
	float3 defaultScale{ 1.f, 1.f , 1.f };

	float3 position{ defaultPosition };
	float3 rotation{ defaultRotation };
	float3 scale{ defaultScale };

	std::string jsonPath;

	void Update();
	void Reset();

private:
};