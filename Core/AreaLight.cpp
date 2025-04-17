#include "precomp.h"
#include "AreaLight.h"


AreaLight::AreaLight(std::string fileName, std::string sceneName, bool exists)
{
    if (!exists)
    {
        prefabName = fileName;
        name = fileName;

        finalPath = "../assets/" + sceneName + "/arealights/" + name;
        resourcesPath = "../assets/prefabs/lights/arealights/" + prefabName;

        while (filesystem::exists(finalPath + ".json"))
        {
            name += "Copy";
            finalPath = "../assets/" + sceneName + "/arealights/" + name;
        }

        // Create New JSON
        std::ofstream jsonOUT(finalPath + ".json");

        // Override JSON with Prefab JSON
        filesystem::copy_file(resourcesPath + ".json",
            finalPath + ".json", filesystem::copy_options::overwrite_existing);

        transform = new LightTransform(finalPath + ".json");
    }

    else
    {
        name = fileName;
        finalPath = "../assets/" + sceneName + "/arealights/" + name;

        transform = new LightTransform(finalPath + ".json");
    }
}

AreaLight::~AreaLight()
{

}

float3 AreaLight::RandomPointOnLight()
{
	float3 finalSample{};

	for (int i = 0; i <= samples; i++)
	{
		finalSample += float3(RandomFloat() - 1, 3, RandomFloat() - 1);
	}

	return finalSample / static_cast<float>(samples);
}

void AreaLight::Reset()
{
    transform->position = transform->defaultPosition;
    transform->rotation = transform->defaultRotation;
    transform->color = transform->defaultColor;
    transform->Update();
}

void AreaLight::DeleteData()
{
	filesystem::remove(finalPath + ".json");
}

void AreaLight::Update()
{
    transform->Update();
}