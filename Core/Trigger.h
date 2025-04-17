#pragma once

class Trigger
{
public:
	Trigger(std::string fullPath);
	~Trigger();

	std::string jsonPath;

	btVector3 extents;
	btScalar mass = 0.f;
	btVector3 localInertia;
	btDefaultMotionState* motionState = nullptr;
	btBoxShape* boxShape = nullptr;
	btTransform* capstuleTransform = nullptr;
	btRigidBody* body = nullptr;

	float3 rotationImgui{ 0.f };
	float3 translationImgui{ 0.f };
	float3 extentsImgui{ 0.f };

	void Update();
	void Move(const btVector3& translation);
	void Rotate(const btQuaternion& rotation);
	void Scale(const btVector3& scale);
	void ModifyBox();

private:
};