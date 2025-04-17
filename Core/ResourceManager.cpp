#include "precomp.h"
#include "ResourceManager.h"

ResourceManager* ResourceManager::resources_Instance = nullptr;

ResourceManager::ResourceManager()
{
	PinballMachine_normal = new Surface("../assets/prefabs/models/PinballMachine/Textures/PinballMachine_Normal.png");
	PinballMachine_albedo = new Surface("../assets/prefabs/models/PinballMachine/Textures/PinballMachine_AlbedoTransparency.png");
	PinballMachine_rma = new Surface("../assets/prefabs/models/PinballMachine/Textures/PinballMachine_MetallicSmoothness.png");

	PinballBoard_normal = new Surface("../assets/prefabs/models/PinballMachine/Textures/PinballBoard_Normal.png");
	PinballBoard_albedo = new Surface("../assets/prefabs/models/PinballMachine/Textures/PinballBoard_AlbedoTransparency.png");
	PinballBoard_rma = new Surface("../assets/prefabs/models/PinballMachine/Textures/PinballBoard_AlbedoTransparency.png");
}

Surface* ResourceManager::getSurface(std::string textureName, textureType whatType)
{
	if (textureName == "PinballMachine")
	{
		switch (whatType)
		{
		case textureType::ALBEDO:
			return PinballMachine_albedo;
			break;
		case textureType::METALNESS:
			return PinballMachine_rma;
			break;
		case textureType::NORMAL:
			return PinballMachine_normal;
			break;
		}
	}
	else if (textureName == "PinballBoard")
	{
		switch (whatType)
		{
		case textureType::ALBEDO:
			return PinballBoard_albedo;
			break;
		case textureType::METALNESS:
			return PinballBoard_rma;
			break;
		case textureType::NORMAL:
			return PinballBoard_normal;
			break;
		}
	}
	
	return nullptr;
}