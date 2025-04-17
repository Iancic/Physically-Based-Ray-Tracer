#include "precomp.h"
#include "PhysicsObject.h"

PhysicsObject::PhysicsObject(GameObject* targetArg, std::string fullPath, btScalar massArg, bool isSphere, bool isHull)
{
	mass = massArg;
	localInertia = btVector3(0, 0, 0);
	targetGameObject = targetArg;

	// Check if the target game object represents a sphere or a convex hull
	if (isSphere)  // Assume isSphere is a property indicating if the object is a sphere
	{
		convexHullShape = new btSphereShape(radius);
	}
	else
	{
		if (isHull)
		// Create a convex hull shape (the original behavior)
			convexHullShape = Renderer::getInstance()->scene.models[targetGameObject->modelIndex]->convexHullShape;
		else
			convexHullShape = Renderer::getInstance()->scene.models[targetGameObject->modelIndex]->triangleMeshBVH;
	}

	convexHullShape->calculateLocalInertia(mass, localInertia);

	// Read data from the given path and interpret.
	jsonPath = fullPath;

	std::ifstream jsonIn(jsonPath);
	json data = json::parse(jsonIn);

	// Apply the position from the JSON to translationImgui
	translationImgui.x = data["positionX"].get<float>();
	translationImgui.y = data["positionY"].get<float>();
	translationImgui.z = data["positionZ"].get<float>();

	// Apply the rotation from the JSON to rotationImgui
	rotationImgui.x = data["rotationX"].get<float>();
	rotationImgui.y = data["rotationY"].get<float>();
	rotationImgui.z = data["rotationZ"].get<float>();

	// Apply the position and rotation to the targetGameObject as well
	targetGameObject->position = float3(translationImgui.x, translationImgui.y, translationImgui.z);
	targetGameObject->rotation = float3(rotationImgui.x, rotationImgui.y, rotationImgui.z);

	// Create the motion state with the correct translation and rotation from JSON
	btQuaternion initialRotation = btQuaternion(rotationImgui.x * PI / 180.0f, rotationImgui.y * PI / 180.0f, rotationImgui.z * PI / 180.0f);
	btTransform initialTransform(initialRotation, btVector3(translationImgui.x, translationImgui.y, translationImgui.z));
	motionState = new btDefaultMotionState(initialTransform);

	// Create the rigid body construction info with the appropriate shape
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, convexHullShape, localInertia);
	body = new btRigidBody(rigidBodyCI);

	body->setSleepingThresholds(0, 0);

	Update();
}

PhysicsObject::~PhysicsObject()
{
	delete body;
	delete motionState;
	delete targetGameObject;
	delete convexHullShape;
}

void PhysicsObject::SetMass(float massArg)
{
	mass = massArg;

	// Recalculate inertia based on the new mass
	convexHullShape->calculateLocalInertia(mass, localInertia);

	// Update the rigid body with the new mass and inertia
	body->setMassProps(mass, localInertia);  // Update mass properties (inertia and mass)
}

void PhysicsObject::Move(const btVector3& translation)
{
	// Get current transform
	btTransform transform;
	body->getMotionState()->getWorldTransform(transform);

	// Apply translation
	transform.setOrigin(transform.getOrigin() + translation);

	// Update the transform
	body->getMotionState()->setWorldTransform(transform);
}

void PhysicsObject::Rotate(const btQuaternion& rotation)
{
	// Get current transform
	btTransform transform;
	body->getMotionState()->getWorldTransform(transform);

	// Apply rotation (increasing current rotation)
	transform.setRotation(transform.getRotation() * rotation);

	// Update the transform
	body->getMotionState()->setWorldTransform(transform);
}

void PhysicsObject::ApplyTorque(btVector3 initialRotation, btVector3 targetRotation, float deltaTime, float lerp)
{
	UNREFERENCED_PARAMETER(initialRotation);
	float torqueSpeed = 10.f;

	currentTorque = currentTorque.lerp(targetRotation, lerp * deltaTime * torqueSpeed);

	body->applyTorque(currentTorque);
}

void PhysicsObject::Update()
{
	btVector3 currentTranslation = btVector3(translationImgui.x, translationImgui.y, translationImgui.z);
	Move(currentTranslation);

	// Convert degrees to radians before applying the rotation
	btQuaternion newRotation;
	newRotation.setEulerZYX(rotationImgui.x * PI / 180.0f, rotationImgui.y * PI / 180.0f, rotationImgui.z * PI / 180.0f);
	Rotate(newRotation);

	// After applying the translation and rotation, update the motion state and rigid body
	// Get the current transform
	btTransform transform;
	body->getMotionState()->getWorldTransform(transform);

	// Apply the new translation and rotation
	transform.setOrigin(currentTranslation);
	transform.setRotation(newRotation);

	// Set the new transform in the rigid body's motion state
	body->getMotionState()->setWorldTransform(transform);

	// If the mass or inertia changes, you would need to update the rigid body as well
	// For example, recalculate the inertia (only needed if mass or shape changes)
	convexHullShape->calculateLocalInertia(mass, localInertia);
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, convexHullShape, localInertia);
	body->setWorldTransform(transform);  // Update the rigid body's transform in the physics world
	body->setMassProps(mass, localInertia);  // Update the mass properties (if needed)

	json data;
	std::ifstream jsonIn(jsonPath);
	if (jsonIn.is_open()) {
		jsonIn >> data;  // Parse the existing JSON
		jsonIn.close();

		// Modify the data
		data["positionX"] = currentTranslation.x();
		data["positionY"] = currentTranslation.y();
		data["positionZ"] = currentTranslation.z();
		data["rotationX"] = rotationImgui.x;
		data["rotationY"] = rotationImgui.y;
		data["rotationZ"] = rotationImgui.z;

		// Write the updated data back to the JSON file
		std::ofstream jsonOut(jsonPath);
		if (jsonOut.is_open()) {
			jsonOut << std::setw(4) << data;  // Pretty print the updated data
			jsonOut.close();
		}
		else {
			//std::cerr << "Failed to open JSON file for writing: " << jsonPath << std::endl;
		}
	}
	else {
		//std::cerr << "Failed to open JSON file for reading: " << jsonPath << std::endl;
	}
}

void PhysicsObject::Synchronise()
{
	// Synchronise Position
	btTransform worldTransform = body->getWorldTransform();
	btVector3 position = worldTransform.getOrigin();
	targetGameObject->position = float3(position.x(), position.y(), position.z());

	// Synchronise Rotation
	btQuaternion rotation = worldTransform.getRotation(); 	// Get the rotation (quaternion) of the rigid body
	btScalar yaw, pitch, roll; 	// Convert quaternion to Euler angles (yaw, pitch, roll)
	rotation.getEulerZYX(yaw, pitch, roll);
	//rotationImgui = float3(roll * -180.0f / PI, pitch * 180.0f / PI, yaw * -180.0f / PI);
	targetGameObject->rotation = float3(-yaw, pitch, -roll);
}
