#include "precomp.h"
#include "Transform.h"

Transform::Transform(const std::string path)
{
	// Read and parse Transform Data
	// Warning: Invalid JSON files throw exceptions
	jsonPath = path;

	std::ifstream jsonIn(jsonPath);
	json data = json::parse(jsonIn);

	// Store Position
	position = float3{ data["pX"].get<float>(), data["pY"].get<float>(), data["pZ"].get<float>() };

	// Store Scale
	scale = float3{ data["sX"].get<float>(), data["sY"].get<float>(), data["sZ"].get<float>() };

	// Store Rotation
	rotation = float3{ data["rX"].get<float>(), data["rY"].get<float>(), data["rZ"].get<float>() };
}

Transform::~Transform()
{

}

void Transform::Update()
{
	json newData;

	// New Positions
	newData["pX"] = position.x;
	newData["pY"] = position.y;
	newData["pZ"] = position.z;

	// New Scale
	newData["sX"] = scale.x;
	newData["sY"] = scale.y;
	newData["sZ"] = scale.z;

	// New Scale
	newData["rX"] = rotation.x;
	newData["rY"] = rotation.y;
	newData["rZ"] = rotation.z;

	std::ofstream jsonOut(jsonPath);
	jsonOut << std::setw(4) << newData;
}

void Transform::Reset()
{
	position = defaultPosition;
	rotation = defaultRotation;
	scale = defaultScale;

	Update();
}
