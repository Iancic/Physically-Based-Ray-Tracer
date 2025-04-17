#include "precomp.h"
#include "Model.h"

Model::Model(std::string fullPath, std::string name, const std::string textureExtension, std::string sharedTexture)
{
    if (!convexHullShape) {
        convexHullShape = new btConvexHullShape();
    }

    modelName = name;
    locationPath = fullPath;
    Load(locationPath, textureExtension, sharedTexture);
    ProcessBVHTriangles();

    modelBVH = new tinybvh::BVH8_CPU();
    modelBVH->BuildHQ(triangles.data(), static_cast<uint32_t>(triangles.size() / 3));
}

Model::~Model()
{
    delete convexHullShape;
    delete modelBVH;
}

void Model::ProcessBVHTriangles()
{
    for (unsigned int i = 0; i < faces.size(); ++i) 
    {
        const aiFace& face = faces[i];

        // Ensure the face has 3 indices (this assumes triangles, not polygons)
        if (face.mNumIndices == 3) 
        {
            for (int j = 0; j < 3; ++j) 
            {
                int vertexIndex = face.mIndices[j];  // Get the index of the vertex
                aiVector3D vertex = aiVector3D(vertices[vertexIndex].x, vertices[vertexIndex].y, vertices[vertexIndex].z);
                triangles.push_back(float4{ vertex.x, vertex.y, vertex.z, 0.0f });

                aiVector3D vertexNormal = aiVector3D(verticesNormals[vertexIndex].x, verticesNormals[vertexIndex].y, verticesNormals[vertexIndex].z);
                fixedNormals.push_back(float4{ vertexNormal.x, vertexNormal.y, vertexNormal.z, 0.0f });

                aiVector3D vertexTexCoord = aiVector3D(verticesTexCoords[vertexIndex].x, verticesTexCoords[vertexIndex].y, verticesTexCoords[vertexIndex].z);
                fixedTextureCoords.push_back(float2{ vertexTexCoord.x, vertexTexCoord.y });
            }
        }
    }
}

void Model::ProcessMesh(const aiMesh* mesh)
{
    // Extract vertices
    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D v = mesh->mVertices[i];  // Vertex position
        vertices.push_back(float3(v.x, v.y, v.z));
    }

    // Extract texture coordinates
    if (mesh->HasTextureCoords(0))
    {
        verticesTexCoords.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            aiVector3D texCoord = mesh->mTextureCoords[0][i];
            verticesTexCoords.push_back(float3(texCoord.x, texCoord.y, 0.0f));
        }
    }

    // Extract vertex normals
    if (mesh->HasNormals())
    {
        verticesNormals.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            aiVector3D normal = mesh->mNormals[i];
            verticesNormals.push_back(float3(normal.x, normal.y, normal.z));
        }
    }

    // Store faces & compute face normals/atangent/btangent
    faces.reserve(mesh->mNumFaces);
    faceNormals.reserve(mesh->mNumFaces); // Reserve space for face normals

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];  // Get the face at index i
        faces.push_back(face);  // Copy the face into the vector

        if (face.mNumIndices == 3) // Ensure it's a triangle
        {
            // Store Indices
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);

            int i0 = face.mIndices[0];
            int i1 = face.mIndices[1];
            int i2 = face.mIndices[2];

            float3 v0 = vertices[i0];
            float3 v1 = vertices[i1];
            float3 v2 = vertices[i2];

            float3 edge1 = v1 - v0;
            float3 edge2 = v2 - v0;

            // Compute face normal
            float3 normal = normalize(cross(edge1, edge2));

            // Store face normal
            faceNormals.push_back(normal);
        }
    }

    ProcessConvexMesh(mesh);
    ProcessTriangleMesh(mesh);
}

void Model::ProcessConvexMesh(const aiMesh* mesh)
{
    btQuaternion rotation(btVector3(0, 0, 1), 3.14f / 1); // 90 degrees around X-axis (pi/2)

    // Create a rotation matrix from the quaternion
    btMatrix3x3 rotationMatrix(rotation);

    // Loop through all the faces of the mesh
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        // Process the triangle face (make sure it's a triangle)
        if (face.mNumIndices == 3)
        {
            // Get the three vertex indices of the face
            aiVector3D aiVertex0 = mesh->mVertices[face.mIndices[0]];
            aiVector3D aiVertex1 = mesh->mVertices[face.mIndices[1]];
            aiVector3D aiVertex2 = mesh->mVertices[face.mIndices[2]];

            // Convert to btVector3 and apply the rotation
            btVector3 bulletVertex0(aiVertex0.x, aiVertex0.y, aiVertex0.z);
            btVector3 bulletVertex1(aiVertex1.x, aiVertex1.y, aiVertex1.z);
            btVector3 bulletVertex2(aiVertex2.x, aiVertex2.y, aiVertex2.z);

            // Apply the rotation matrix to each vertex
            bulletVertex0 = rotationMatrix * bulletVertex0;
            bulletVertex1 = rotationMatrix * bulletVertex1;
            bulletVertex2 = rotationMatrix * bulletVertex2;

            // Add these rotated vertices to the Bullet Convex Hull
            bulletVerticesHUll.push_back(bulletVertex0);
            bulletVerticesHUll.push_back(bulletVertex1);
            bulletVerticesHUll.push_back(bulletVertex2);
        }
    }

    // Add each vertex to the convex hull
    for (const auto& vertex : bulletVerticesHUll) {
        convexHullShape->addPoint(vertex);
    }
}

void Model::Load(std::string filename, const std::string ext, std::string sharedTexture)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return;
    }

    // Extract model directory and filename
    std::filesystem::path filePath(filename);
    std::string directory = filePath.parent_path().string();
    std::string modelNameLoad = filePath.stem().string();  // Model name without extension

    if (scene->HasMaterials())
        material = scene->mMaterials[scene->mMeshes[0]->mMaterialIndex];

    auto LoadTexture = [&](const std::string& type, Surface*& textureVar, const std::string ext)
        {
            std::string filePath = directory + "/" + modelNameLoad + "_" + type + ext;

            if (!std::filesystem::exists(filePath))
                filePath = directory + "/" + modelNameLoad + "_" + type + ext;

            if (std::filesystem::exists(filePath))
                textureVar = new Surface(filePath.c_str());
        };

    aiString str;
    if (sharedTexture == "null")
    {
        // Load textures based on convention
        LoadTexture("albedo", albedoTexture, ext);
        LoadTexture("normal", normalTexture, ext);
        LoadTexture("metalness", metalnessTexture, ext);
        LoadTexture("roughness", roughnessTexture, ext);
        LoadTexture("emission", emissionTexture, ext);
        LoadTexture("ao", aoTexture, ext);
    }
    else
    {
        albedoTexture = ResourceManager::getInstance()->getSurface(sharedTexture, ResourceManager::textureType::ALBEDO);
        normalTexture = ResourceManager::getInstance()->getSurface(sharedTexture, ResourceManager::textureType::NORMAL);
        metalnessTexture = ResourceManager::getInstance()->getSurface(sharedTexture, ResourceManager::textureType::METALNESS);
        LoadTexture("albedo", albedoTexture, ext);
        LoadTexture("normal", normalTexture, ext);
        LoadTexture("metalness", metalnessTexture, ext);
        //Surface* atlas = new Surface(100, 100)
    }

    if (scene->mNumMeshes > 0) 
        ProcessMesh(scene->mMeshes[0]);
}

void Model::ProcessTriangleMesh(const aiMesh* mesh)
{
    // Define a rotation quaternion (90 degrees around the X-axis, as an example)
    btQuaternion rotation(btVector3(0, 0, 1), 3.14f / 1); // 90 degrees around X-axis (pi/2)

    // Create a rotation matrix from the quaternion
    btMatrix3x3 rotationMatrix(rotation);

    // Create a Bullet triangle mesh
    

    // Loop through all the faces of the mesh
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        // Process only triangles (ensure face is a triangle)
        if (face.mNumIndices == 3)
        {
            // Get the three vertex indices of the face
            aiVector3D aiVertex0 = mesh->mVertices[face.mIndices[0]];
            aiVector3D aiVertex1 = mesh->mVertices[face.mIndices[1]];
            aiVector3D aiVertex2 = mesh->mVertices[face.mIndices[2]];

            // Convert to btVector3 (Bullet's vector type)
            btVector3 bulletVertex0(aiVertex0.x, aiVertex0.y, aiVertex0.z);
            btVector3 bulletVertex1(aiVertex1.x, aiVertex1.y, aiVertex1.z);
            btVector3 bulletVertex2(aiVertex2.x, aiVertex2.y, aiVertex2.z);

            // Apply the rotation to each vertex
            bulletVertex0 = rotationMatrix * bulletVertex0;
            bulletVertex1 = rotationMatrix * bulletVertex1;
            bulletVertex2 = rotationMatrix * bulletVertex2;

            // Add the rotated triangle to the Bullet triangle mesh
            triangleMesh->addTriangle(bulletVertex0, bulletVertex1, bulletVertex2, true);  // 'true' is for two-sided faces
        }
    }

    // Now you can use the triangle mesh to create a collision shape
    triangleMeshBVH = new btBvhTriangleMeshShape(triangleMesh, true); // true for two-sided faces
}