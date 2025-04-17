#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <fstream>

#include "tinyBVH.h"
#include "ResourceManager.h"

class Model
{
public:
	Model(std::string fullPath, std::string name, const std::string textureExtension, std::string sharedTexture = "null");
	~Model();

	// name + .glb / .obj / .fbx
	// name + _ + type + .jpeg
	std::string modelName; // Name used in conventions
	std::string locationPath; // Where it exists

	std::vector<aiFace> faces;

	// Model Data:
	aiMaterial* material = nullptr;

	Surface* albedoTexture = nullptr;
	Surface* normalTexture = nullptr;
	Surface* roughnessTexture = nullptr;
	Surface* metalnessTexture = nullptr;
	Surface* aoTexture = nullptr;
	Surface* emissionTexture = nullptr;
	Surface* rmaTexture = nullptr;

	std::vector<int>indices;
	std::vector<float3> vertices;
	std::vector<float3> verticesTexCoords;
	std::vector<float3> verticesNormals; // Normal per vertex
	std::vector<float3> aTangent;
	std::vector<float3> aBitangent;
	std::vector<float3> faceNormals; // Normal per face
	std::vector<float4> triangles; // Fat Triangles For Tinybvh
	std::vector<float4> fixedNormals; // Fat Triangles For Tinybvh
	std::vector<float2> fixedTextureCoords; // Fat Triangles For Tinybvh

	// Bullet:
	std::vector<btVector3> bulletVerticesHUll;
	btConvexHullShape* convexHullShape = new btConvexHullShape();
	btTriangleMeshShape* triangleShape = nullptr;
	btBvhTriangleMeshShape* triangleMeshBVH = nullptr;
	std::vector<unsigned int> bulletIndices;
	btTriangleMesh* triangleMesh = new btTriangleMesh();

	// BVH
	tinybvh::BVH8_CPU* modelBVH;

	void ProcessBVHTriangles();
	void ProcessMesh(const aiMesh* mesh);
	void ProcessConvexMesh(const aiMesh* mesh);
	void ProcessTriangleMesh(const aiMesh* mesh);
	void Load(std::string filename, const std::string ext, std::string sharedTexture = "null");

};