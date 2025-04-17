#pragma once

class PhysicsObject
{
public:

	PhysicsObject(GameObject* targetArg, std::string fullPath, btScalar massArg, bool isSphere = false, bool isHull = false);
	~PhysicsObject();

	std::string jsonPath;

	const btScalar radius = 0.02f;
	btScalar mass;
	btDefaultMotionState* motionState = nullptr;
	btVector3 localInertia;
	btRigidBody* body = nullptr;
	btCollisionShape* convexHullShape = nullptr;

	GameObject* targetGameObject = nullptr;

	float3 rotationImgui { 0.f };
	float3 translationImgui { 0.f };

	btVector3 currentTorque;

	void SetMass(float massArg);

	void Move(const btVector3& translation);
	void Rotate(const btQuaternion& rotation);

	void ApplyTorque(btVector3 initialRotation, btVector3 targetRotation, float deltaTime, float lerp);

	void Update();
	void Synchronise();

};

