#include "precomp.h"
#include "PointLight.h"

PointLight::PointLight(std::string fileName, std::string sceneName, bool exists)
{
    if (!exists)
    {
        prefabName = fileName;
        name = fileName;

        finalPath = "../assets/" + sceneName + "/pointlights/" + name;
        resourcesPath = "../assets/prefabs/lights/pointlights/" + prefabName;

        while (filesystem::exists(finalPath + ".json"))
        {
            name += "Copy";
            finalPath = "../assets/" + sceneName + "/pointlights/" + name;
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
        finalPath = "../assets/" + sceneName + "/pointlights/" + name;

        transform = new LightTransform(finalPath + ".json");
    }
}

PointLight::~PointLight()
{

}

void PointLight::DeleteData()
{
    filesystem::remove(finalPath + ".json");
}

void PointLight::Reset()
{
    transform->position = transform->defaultPosition;
    transform->color = transform->defaultColor;
    transform->Update();
}

void PointLight::Update()
{
    transform->Update();
}