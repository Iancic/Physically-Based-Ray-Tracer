#include "precomp.h"
#include "Scene.h"

Scene::Scene()
{
	SetTime(0);
	Init();
}

void Scene::Init()
{
	// 1. Load Models 
	// arg1: location of model - arg2:  - arg3: texture extension - arg4: if shared texture look into ResourceManager
	/* 0 */ AddModel(modelsPath + "SciFiHelmet/SciFiHelmet.gltf", "SciFiHelmet", ".png");

	// 2. Build BVH's (Each Model One BVH)
	for (auto model: models) bvh.push_back(model->modelBVH);

	// 3. Use Serialized JSON's to populate scene with GameObjects and BLASES
	FindSerialized(gameObjectsPath, ".json", 0);

	// 4. Build TLAS
	BuildTLAS();

	// 5. Use Serialized JSON's to populate scene with lights
	FindSerialized(dirPrefabPath, ".json", 2); // dirLight
	FindSerialized(spotPrefabPath, ".json", 3); // spotLight
}

Scene::~Scene()
{

}

void Scene::SetTime(float t)
{
	animTime = t;
	//float tm = 1 - sqrf(fmodf(animTime, 2.0f) - 1);
}

bool Scene::IsOccluded(tinybvh::Ray& ray) const
{
	if (tlas.IsOccluded(ray)) return true; 
	return false;
}

float3 Scene::GetGeometryNormal(tinybvh::Ray& ray)
{
	float3 faceNormal = models[gameobjects[ray.hit.inst]->modelIndex]->faceNormals[ray.hit.prim];

	mat4 matrix;
	for (int i = 0; i < 16; i++)
		matrix.cell[i] = blases[ray.hit.inst].transform[i];

	faceNormal = tinybvh::tinybvh_transform_vector(faceNormal, matrix.Inverted().Transposed().cell);

	return faceNormal;
}

float3 Scene::GetShadingNormal(tinybvh::Ray& ray)
{
	int whatModel = gameobjects[ray.hit.inst]->modelIndex;

	float u = ray.hit.u;
	float v = ray.hit.v;
	float w = 1.0f - u - v;

	float3 normalColor;
	int triangleV0 = ray.hit.prim * 3;
	int triangleV1 = triangleV0 + 1;
	int triangleV2 = triangleV0 + 2;

	if (models[whatModel]->normalTexture != nullptr && Renderer::getInstance()->NORMALMAPPED)
	{
		float2 uv = v * models[whatModel]->fixedTextureCoords[triangleV2]
			+ u * models[whatModel]->fixedTextureCoords[triangleV1]
			+ w * models[whatModel]->fixedTextureCoords[triangleV0];

		int texWidth = models[whatModel]->albedoTexture->width;
		int texHeight = models[whatModel]->albedoTexture->height;

		int iu = static_cast<int>((uv.x * texWidth)) % texWidth;
		int iv = static_cast<int>((uv.y * texHeight)) % texHeight;

		uint normalTexel = models[whatModel]->normalTexture->pixels[iu + iv * texWidth];
		normalColor = MakeNormalFromTexel(normalTexel);

		int triangleIndex = triangleV0; // Convert primitive index to vertex index
		int i0 = models[whatModel]->indices[triangleIndex];
		int i1 = models[whatModel]->indices[triangleIndex + 1];
		int i2 = models[whatModel]->indices[triangleIndex + 2];

		float3 edge1 = models[whatModel]->vertices[i1] - models[whatModel]->vertices[i0];
		float3 edge2 = models[whatModel]->vertices[i2] - models[whatModel]->vertices[i0];

		float2 deltaUV1 = models[whatModel]->fixedTextureCoords[triangleV1] - models[whatModel]->fixedTextureCoords[triangleV0];
		float2 deltaUV2 = models[whatModel]->fixedTextureCoords[triangleV2] - models[whatModel]->fixedTextureCoords[triangleV0];

		float det = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
		float invDet = 1.0f / det;

		float3 T = normalize(invDet * (deltaUV2.y * edge1 - deltaUV1.y * edge2));
		float3 B = normalize(invDet * (-deltaUV2.x * edge1 + deltaUV1.x * edge2));

		float3 faceNormalUnmodified = models[whatModel]->fixedNormals[triangleV0] * w
			+ models[whatModel]->fixedNormals[triangleV1] * u
			+ models[whatModel]->fixedNormals[triangleV2] * v;

		mat4 matrix;
		for (int i = 0; i < 16; i++)
			matrix.cell[i] = blases[ray.hit.inst].transform[i];
		
		faceNormalUnmodified = tinybvh::tinybvh_transform_vector(faceNormalUnmodified, matrix.Inverted().Transposed().cell);

		float3 N = normalize(faceNormalUnmodified);

		glm::mat3 TBN = glm::mat3(glm::vec3{ T.x, T.y, T.z }, glm::vec3{ B.x, B.y, B.z }, glm::vec3{ N.x, N.y, N.z });
		glm::vec3 colorNorm = glm::vec3{ normalColor.x, normalColor.y, normalColor.z };
		glm::vec3 finalunconverted = normalize(colorNorm * transpose(TBN));
		return float3{ finalunconverted.x, finalunconverted.y, finalunconverted.z };
	}

	else
	{
		// Smooth Shaded
		float3 interpolated = models[whatModel]->fixedNormals[triangleV0] * w
			+ models[whatModel]->fixedNormals[triangleV1] * u
			+ models[whatModel]->fixedNormals[triangleV2] * v;

		mat4 matrix;
		for (int i = 0; i < 16; i++)
			matrix.cell[i] = blases[ray.hit.inst].transform[i];

		interpolated = tinybvh::tinybvh_transform_vector(interpolated, matrix.Inverted().Transposed().cell);

		return interpolated;
	}
}

MaterialProperties Scene::GetMaterialBRDF(tinybvh::Ray& ray) const
{
	int whatModel = gameobjects[ray.hit.inst]->modelIndex;
	Model* modelPtr = models[whatModel];

	float metalness{}, roughness{};
	float3 base{}, emission{};

	float u = ray.hit.u;
	float v = ray.hit.v;
	float w = 1.0f - u - v;

	int triangleV0 = ray.hit.prim * 3;
	int triangleV1 = triangleV0 + 1;
	int triangleV2 = triangleV0 + 2;

	float2 uv = v * modelPtr->fixedTextureCoords[triangleV2]
		+ u * modelPtr->fixedTextureCoords[triangleV1]
		+ w * modelPtr->fixedTextureCoords[triangleV0];
	
	int texWidth = modelPtr->albedoTexture->width;
	int texHeight = modelPtr->albedoTexture->height;

	int iu = (int)(uv.x * texWidth) % texWidth;
	int iv = (int)(uv.y * texHeight) % texHeight;
	int whatPixel = iu + iv * texWidth;

	// Base Color (First Texture)
	if (modelPtr->albedoTexture != nullptr)
	{
		uint basecolorTexel = modelPtr->albedoTexture->pixels[whatPixel];
		base = SrgbToLinear(MakeColorFromTexel(basecolorTexel));
	}

	// RMA
	if (modelPtr->metalnessTexture != nullptr)
	{
		uint rmaTexel = modelPtr->metalnessTexture->pixels[whatPixel];

		roughness = ExtractChannel(channel::GREEN, rmaTexel);
		metalness = ExtractChannel(channel::BLUE, rmaTexel);
	}

	// Emmision (Fourth Texture)
	if (modelPtr->emissionTexture != nullptr)
	{
		uint emissionTexel = modelPtr->emissionTexture->pixels[whatPixel];
		emission = MakeColorFromTexel(emissionTexel);
	}

	MaterialProperties result;
	
	// Dielectrics:
	if (gameobjects[ray.hit.inst]->modelIndex == -1) // *Nothing* // Instead of -1 place model index.
	{
		result.baseColor = base;
		result.transmissivness = 1.0f;
	}

	// Perfect Mirrors:
	if (gameobjects[ray.hit.inst]->modelIndex == -1) // *Nothing* // Instead of -1 Place model index.
	{
		result.baseColor = base;
		result.metalness = 1.f;
		result.roughness = 0.f;
	}
	// Normal Objects (Data From Textures) 
	else
	{
		result.baseColor = base;
		result.metalness = metalness;
		result.emissive = emission;
		result.roughness = roughness;
		result.reflectance = 0.5f;

	}
	
	return result;
}

void Scene::BuildTLAS()
{
	tlas.Build(blases.data(), static_cast<uint32_t>(blases.size()), bvh.data(), static_cast<uint32_t>(bvh.size()));
}

float3 Scene::MakeColorFromTexel(const uint c) const
{
	constexpr float s = 1.0f / 255.0f;
	return float3{ ((c >> 16) & 0xFF) * s, ((c >> 8) & 0xFF) * s, (c & 0xFF) * s };
}

float3 Scene::MakeNormalFromTexel(const uint c) const
{
	constexpr float s = 2.0f / 255.0f;
	return float3 { ((c >> 16) & 0xFF) * s - 1.0f, ((c >> 8) & 0xFF) * s - 1.0f, (c & 0xFF) * s - 1.0f };
}

float Scene::ExtractChannel(const channel type, const uint c) const
{
	float s = 1.0f / 255.0f;

	switch (type)
	{
	case channel::RED:
		return ((c >> 16) & 255) * s;
	case channel::GREEN:
		return ((c >> 8) & 255) * s;
	case channel::BLUE:
		return (c & 255) * s;
	case channel::ALPHA:
        return ((c >> 24) & 255) * s;
	default:
		return 0.0f; // Return a safe default value
	}
}

float3 Scene::SrgbToLinear(const float3 c) const
{
	return float3{
		(c.x <= 0.04045f) ? (c.x / 12.92f) : pow((c.x + 0.055f) / 1.055f, 2.4f),
		(c.y <= 0.04045f) ? (c.y / 12.92f) : pow((c.y + 0.055f) / 1.055f, 2.4f),
		(c.z <= 0.04045f) ? (c.z / 12.92f) : pow((c.z + 0.055f) / 1.055f, 2.4f)
	};
}

void Scene::applyRandomImpulse(btRigidBody* body)
{
	float impulseMagnitude = RandomFloat() * 5.0f;

	btVector3 impulse(0.0f, 0.0f, -impulseMagnitude);
	body->applyCentralImpulse(impulse);
}

void Scene::applyImpulse(btRigidBody* body, const float Impulse)
{
	btVector3 impulse(0.0f, 0.0f, -Impulse);
	body->applyCentralImpulse(impulse);
}

void Scene::FindSerialized(const std::string wherePath, const std::string whatExtension, const int jsonType) // jsonType: (0) = GameObject, (1) = pointLight, (2) = dirLight, (3) = spotLight
{
	// https://stackoverflow.com/questions/10681112/extracting-a-string-in-between-symbols
	// https://www.geeksforgeeks.org/cpp-program-to-get-the-list-of-files-in-a-directory/

	for (const auto& entry : filesystem::directory_iterator(wherePath))
	{
		std::filesystem::path outfilename = entry.path();
		std::string outfilename_str = outfilename.string();

		std::string result = std::filesystem::path(entry).filename().string();
		std::string::size_type start_position = 0;
		std::string::size_type end_position = 0;
		if (result.find(whatExtension) != std::string::npos)
		{
			start_position = result.find("""");
			if (start_position != std::string::npos)
			{
				++start_position;
				end_position = result.find(whatExtension + "");
				if (end_position != std::string::npos)
				{
					if (jsonType == 0)
					{
						AddGameObjects(gameObjectsPath + result.substr(0, end_position) + ".json");

						AddBLAS(gameobjects[gameobjects.size() - 1]->modelIndex);
					}
					else if (jsonType == 1)
						AddLight("pointlight", true);
					else if (jsonType == 2)
						AddLight("directionallight", true);
					else if (jsonType == 3)
						AddLight("spotlight", true);
				}
			}
		}
	}
}

void Scene::AddLight(std::string lightType, bool exists)
{
	if (lightType == "directionallight")
		directionalLights.push_back(new DirectionalLight("directionallight", "scene1", exists));
	if (lightType == "spotlight")
		spotlights.push_back(new SpotLight("spotlight", "scene1", exists));
}

void Scene::AddBLAS(int index)
{
	blases.push_back(tinybvh::BLASInstance(index));
}

void Scene::AddModel(std::string fullPath, std::string name, const std::string textureExtension, std::string sharedTexture)
{
	models.push_back(new Model(fullPath, name, textureExtension, sharedTexture));
}

void Scene::AddGameObjects(std::string fullPath)
{
	gameobjects.push_back(new GameObject(fullPath));
}