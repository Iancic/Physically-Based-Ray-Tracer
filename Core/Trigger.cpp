#include "precomp.h"
#include "Trigger.h"

Trigger::Trigger(std::string fullPath)
{
	// Read data from the given path and interpret.
	jsonPath = fullPath;
	std::ifstream jsonIn(jsonPath);
	json data = json::parse(jsonIn);

    rotationImgui = float3(data["rX"].get<float>() * PI / 180.0f, data["rY"].get<float>() * PI / 180.0f, data["rZ"].get<float>() * PI / 180.0f);
    translationImgui = float3(data["pX"].get<float>(), data["pY"].get<float>(), data["pZ"].get<float>());
    extentsImgui = float3(data["sX"].get<float>(), data["sY"].get<float>(), data["sZ"].get<float>());

	// Box Extents
	extents = btVector3(data["sX"].get<float>(), data["sY"].get<float>(), data["sZ"].get<float>());
	boxShape = new btBoxShape(extents);

	btVector3 serializedPosition = btVector3{ data["pX"].get<float>() , data["pY"].get<float>(), data["pZ"].get<float>() };
	btQuaternion serializedRotation = btQuaternion{ data["rX"].get<float>() * PI / 180.0f , data["rY"].get<float>() * PI / 180.0f, data["rZ"].get<float>() * PI / 180.0f };

	btQuaternion initialRotation = serializedRotation;
	btTransform initialTransform(initialRotation, serializedPosition);
	motionState = new btDefaultMotionState(initialTransform);

	// Create the rigid body construction info with the appropriate shape
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, boxShape, localInertia);
	body = new btRigidBody(rigidBodyCI);

	body->setSleepingThresholds(0, 0);
	// For Making The Object Trigger
	body->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
}

Trigger::~Trigger()
{
}

void Trigger::Update()
{
    ModifyBox();

    json data;
    std::ifstream jsonIn(jsonPath);
    if (jsonIn.is_open()) {
        jsonIn >> data;  // Parse the existing JSON
        jsonIn.close();

        btTransform worldTransform;
        body->getMotionState()->getWorldTransform(worldTransform);

        btVector3 position = worldTransform.getOrigin();
        btQuaternion rotation = worldTransform.getRotation();
        btVector3 extentsLocal = boxShape->getHalfExtentsWithMargin();
        btVector3 fullExtents = extentsLocal * 2.0f;

        // Update the JSON data with the new transform values
        data["pX"] = translationImgui.x;
        data["pY"] = translationImgui.y;
        data["pZ"] = translationImgui.z;

        data["rX"] = rotationImgui.x;
        data["rY"] = rotationImgui.y;
        data["rZ"] = rotationImgui.z;

        data["sX"] = extentsImgui.x;
        data["sY"] = extentsImgui.y;
        data["sZ"] = extentsImgui.z;

        // Write the updated data back to the JSON file
        std::ofstream jsonOut(jsonPath);
        if (jsonOut.is_open()) {
            jsonOut << std::setw(4) << data;  // Pretty print the updated data
            jsonOut.close();
        }
        else {
            std::cerr << "Failed to open JSON file for writing: " << jsonPath << std::endl;
        }
    }
    else {
        std::cerr << "Failed to open JSON file for reading: " << jsonPath << std::endl;
    }
}

void Trigger::Move(const btVector3& translation)
{
	// Get current transform
	btTransform transform;
	body->getMotionState()->getWorldTransform(transform);

	// Apply translation
	transform.setOrigin(transform.getOrigin() + translation);

	// Update the transform
	body->getMotionState()->setWorldTransform(transform);
}

void Trigger::Rotate(const btQuaternion& rotation)
{
	// Get current transform
	btTransform transform;
	body->getMotionState()->getWorldTransform(transform);

	// Apply rotation (increasing current rotation)
	transform.setRotation(transform.getRotation() * rotation);

	// Update the transform
	body->getMotionState()->setWorldTransform(transform);
}

void Trigger::Scale(const btVector3& scale)
{
	
	// Apply scaling to the box shape
	btVector3 newHalfExtents = extents * scale;  // Multiply current extents by the scale factors

	// Update the box shape with the new scaled extents
	//boxShape->set;

	// Also, update the extents member variable
	extents = newHalfExtents;  // Update the internal extents to reflect the scaling
	
}

void Trigger::ModifyBox()
{
    // Step 1: Recreate the Box Shape with new extents (scaling)
    btVector3 newHalfExtents(extentsImgui.x, extentsImgui.y, extentsImgui.z);
    btBoxShape* newBoxShape = new btBoxShape(newHalfExtents);

    // Step 2: Apply translation and rotation (we'll use the provided translation and rotation values)
    btQuaternion newRotation;
    newRotation.setEulerZYX(rotationImgui.x * PI / 180.0f, rotationImgui.y * PI / 180.0f, rotationImgui.z * PI / 180.0f);  // Convert degrees to radians

    btVector3 newPosition(translationImgui.x, translationImgui.y, translationImgui.z);
    btTransform newTransform(newRotation, newPosition);

    // Step 3: Recalculate mass and inertia
    btVector3 newLocalInertia(0, 0, 0);  // Local inertia should be calculated based on mass and shape
    newBoxShape->calculateLocalInertia(mass, newLocalInertia);

    // Step 4: Create a new rigid body with the new shape, and set the motion state with the new transform
    btRigidBody::btRigidBodyConstructionInfo newRigidBodyCI(mass, new btDefaultMotionState(newTransform), newBoxShape, newLocalInertia);
    btRigidBody* newRigidBody = new btRigidBody(newRigidBodyCI);

    // Step 5: Set collision flags (optional, as needed)
    newRigidBody->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
    newRigidBody->setSleepingThresholds(0, 0);

    // Step 6: Get the current dynamics world and replace the old rigid body with the new one
    btDynamicsWorld* dynamicsWorld = Renderer::getInstance()->dynamicsWorld; // Get the world from the old body
    dynamicsWorld->removeRigidBody(body);  // Remove old body
    dynamicsWorld->addRigidBody(newRigidBody);  // Add the new rigid body

    // Step 7: Update the class's body pointer and extents
    body = newRigidBody;
    extents = newHalfExtents;  // Update the internal extents to reflect the scaling
}