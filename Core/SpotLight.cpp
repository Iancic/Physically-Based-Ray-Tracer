#include "precomp.h"
#include "SpotLight.h"

SpotLight::SpotLight(std::string fileName, std::string sceneName, bool exists)
{
    if (!exists)
    {
        prefabName = fileName;
        name = fileName;

        finalPath = "../assets/" + sceneName + "/spotlights/" + name;
        resourcesPath = "../assets/prefabs/lights/spotlights/" + prefabName;

        while (filesystem::exists(finalPath + ".json"))
        {
            name += "Copy";
            finalPath = "../assets/" + sceneName + "/spotlights/" + name;
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
        finalPath = "../assets/" + sceneName + "/spotlights/" + name;

        transform = new LightTransform(finalPath + ".json");
    }
}

SpotLight::~SpotLight()
{

}

void SpotLight::Reset()
{
    transform->position = transform->defaultPosition;
    transform->rotation = transform->defaultRotation;
    transform->color = transform->defaultColor;
}

void SpotLight::DeleteData()
{
    filesystem::remove(finalPath + ".json");
}

void SpotLight::Update()
{
    transform->Update();
}