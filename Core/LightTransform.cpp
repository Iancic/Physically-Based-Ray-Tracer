#include "precomp.h"
#include "LightTransform.h"

LightTransform::LightTransform(const std::string path)
{
	// Read and parse Transform Data
	// Warning: Invalid JSON files throw exceptions
	jsonPath = path;

	std::ifstream jsonIn(jsonPath);
	json data = json::parse(jsonIn);

	// Store Position
	position = float3{ data["pX"].get<float>(), data["pY"].get<float>(), data["pZ"].get<float>() };

	// Store Color
	color = float3{ data["cX"].get<float>(), data["cY"].get<float>(), data["cZ"].get<float>() };

	// Store Rotation
	rotation = float3{ data["rX"].get<float>(), data["rY"].get<float>(), data["rZ"].get<float>() };

	Update();
}

LightTransform::~LightTransform()
{

}

void LightTransform::Update()
{
	json newData;

	// New Positions
	newData["pX"] = position.x;
	newData["pY"] = position.y;
	newData["pZ"] = position.z;

	// New Color
	newData["cX"] = color.x;
	newData["cY"] = color.y;
	newData["cZ"] = color.z;

	// New Scale
	newData["rX"] = rotation.x;
	newData["rY"] = rotation.y;
	newData["rZ"] = rotation.z;

	std::ofstream jsonOut(jsonPath);
	jsonOut << std::setw(4) << newData;
}

void LightTransform::Reset()
{
	position = defaultPosition;
	rotation = defaultRotation;
	color = defaultColor;

	Update();
}