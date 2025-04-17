#pragma once

class ResourceManager
{
private:
	// Singleton
	static ResourceManager* resources_Instance;

	ResourceManager();

public:

	// Prevent copying or assignment 
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;

	// Singleton
	static ResourceManager* getInstance()
	{
		if (resources_Instance == nullptr)
		{
			resources_Instance = new ResourceManager();
		}

		return resources_Instance;
	}

	enum class textureType
	{
		ALBEDO, 
		NORMAL,
		METALNESS
	};

	Surface* PinballMachine_normal = nullptr;
	Surface* PinballMachine_albedo = nullptr;
	Surface* PinballMachine_rma = nullptr;

	Surface* PinballBoard_normal = nullptr;
	Surface* PinballBoard_albedo = nullptr;
	Surface* PinballBoard_rma = nullptr;

	Surface* getSurface(std::string textureName, textureType whatType);

};