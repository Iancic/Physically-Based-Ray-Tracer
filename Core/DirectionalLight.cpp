#include "precomp.h"
#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(std::string fileName, std::string sceneName, bool exists)
{
    if (!exists)
    {
        prefabName = fileName;
        name = fileName;

        finalPath = "../assets/" + sceneName + "/directionallights/" + name;
        resourcesPath = "../assets/prefabs/lights/directionallights/" + prefabName;

        while (filesystem::exists(finalPath + ".json"))
        {
            name += "Copy";
            finalPath = "../assets/" + sceneName + "/directionallights/" + name;
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
        finalPath = "../assets/" + sceneName + "/directionallights/" + name;

        transform = new LightTransform(finalPath + ".json");
    }
}

DirectionalLight::~DirectionalLight()
{

}

void DirectionalLight::Reset()
{
    transform->position = transform->defaultPosition;
    transform->rotation = transform->defaultRotation;
    transform->color = transform->defaultColor;
    transform->Update();
}

void DirectionalLight::DeleteData()
{
    filesystem::remove(finalPath + ".json");
}

void DirectionalLight::Update()
{
    transform->Update();
}
