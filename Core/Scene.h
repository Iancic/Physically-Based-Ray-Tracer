#pragma once
#include "GameObject.h"
#include "SpotLight.h"
#include "AreaLight.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "BRDF.h"
#include "ResourceManager.h"
#include "PhysicsObject.h"
#include "Trigger.h"

class Scene
{

public:
	Scene();
	~Scene();

	std::vector <tinybvh::BVHBase*> bvh = { };      

	std::vector<tinybvh::BLASInstance> blases = { };

	std::vector<PhysicsObject*> physicsobjects = { };

	std::vector<Model*> models = { };

	std::vector<GameObject*> gameobjects = { };

	tinybvh::BVH tlas;

	// Lights
	std::vector<DirectionalLight*> directionalLights;
	std::vector<SpotLight*> spotlights;

	// Paths
	const std::string modelsPath = "../assets/prefabs/models/";
	const std::string gameObjectsPath = "../assets/scene1/";
	const std::string pointPrefabPath = "../assets/scene1/pointlights/";
	const std::string dirPrefabPath = "../assets/scene1/directionallights/";
	const std::string spotPrefabPath = "../assets/scene1/spotlights/";
	const std::string lightPath = "../assets/scene1/";

	void Init();

	// Time
	float animTime = 0;
	void SetTime(float t);

	// Tracing Rays:
	bool IsOccluded(tinybvh::Ray& ray) const;
	float3 GetGeometryNormal(tinybvh::Ray& ray);
	float3 GetShadingNormal(tinybvh::Ray& ray);
	MaterialProperties GetMaterialBRDF(tinybvh::Ray& ray) const;

	void BuildTLAS();

	void AddLight(std::string lightType, bool exists);
	void AddBLAS(int index);
	void AddModel(std::string fullPath, std::string name, const std::string textureExtension, std::string sharedTexture = "null");
	void AddGameObjects(std::string fullPath);
	void FindSerialized(const std::string wherePath, const std::string whatExtension, const int jsonType);

	enum class channel
	{
		RED,
		GREEN,
		BLUE,
		ALPHA
	};
	float ExtractChannel(const channel type, const uint c) const;
	float3 SrgbToLinear(const float3 c) const;
	float3 MakeColorFromTexel(const uint c) const;
	float3 MakeNormalFromTexel(const uint c) const;

	void applyRandomImpulse(btRigidBody* body);
	void applyImpulse(btRigidBody* body, const float Impulse);

private:

};