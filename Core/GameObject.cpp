#include "precomp.h"
#include "GameObject.h"

GameObject::GameObject(std::string fullPath)
{
	// Read data from given path and interpret.
	jsonPath = fullPath;

	std::ifstream jsonIn(jsonPath);
	json data = json::parse(jsonIn);

	// Store Model Name/Type
	modelIndex = data["modelIndex"].get<int>();

	// Store Model Physics Type
	physicsType = data["physicsType"].get<string>();
}

GameObject::~GameObject()
{
	Update();
}

void GameObject::Update()
{
    // Read the existing JSON data
    json data;
    std::ifstream jsonIn(jsonPath);
    if (jsonIn.is_open()) {
        jsonIn >> data;  // Parse the existing JSON
        jsonIn.close();

        // Modify the relevant fields
        data["modelIndex"] = modelIndex;  // Update modelIndex
        data["physicsType"] = physicsType;  // Update physicsType

        // Write the updated data back to the JSON file
        std::ofstream jsonOut(jsonPath);
        if (jsonOut.is_open()) {
            jsonOut << std::setw(4) << data;  // Pretty print the updated data
            jsonOut.close();
        }
        else {
            //std::cerr << "Failed to open JSON file for writing: " << jsonPath << std::endl;
        }
    }
    else {
        //std::cerr << "Failed to open JSON file for reading: " << jsonPath << std::endl;
    }

}

void GameObject::Synchronise(tinybvh::BLASInstance* instance)
{
	// Translate
	mat4 transform = mat4::Identity();
	transform = transform * mat4::Translate(float3(position));

	// Rotate
	glm::quat glmQuat(glm::vec3(float(rotation.x), float(rotation.y), float(rotation.z)));
	quat templateQuat(glmQuat.x, glmQuat.y, glmQuat.z, glmQuat.w);
	transform = transform * templateQuat.toMatrix();

	transform = transform * mat4::Scale(scale);

	memcpy(&instance->transform, &transform, sizeof(float) * 16);
}