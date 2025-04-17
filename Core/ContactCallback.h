#include "btBulletDynamicsCommon.h"

class CollisionCallback : public btCollisionWorld::ContactResultCallback
{

public:
    CollisionCallback(btRigidBody* body1, btRigidBody* body2)
        : rigidBody1(body1), rigidBody2(body2), collisionDetected(false) {
    }
    virtual btScalar addSingleResult(btManifoldPoint & cp, const btCollisionObjectWrapper * colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper * colObj1Wrap, int partId1, int index1) override
    {
        UNREFERENCED_PARAMETER(cp);
        UNREFERENCED_PARAMETER(partId0);
        UNREFERENCED_PARAMETER(index0);
        UNREFERENCED_PARAMETER(partId1);
        UNREFERENCED_PARAMETER(index1);

        // Check if the two objects are the ones we are interested in
        if ((colObj0Wrap->getCollisionObject() == rigidBody1 && colObj1Wrap->getCollisionObject() == rigidBody2) ||
            (colObj0Wrap->getCollisionObject() == rigidBody2 && colObj1Wrap->getCollisionObject() == rigidBody1)) {
            collisionDetected = true;
        }
        else
        {
            collisionDetected = false;
        }
        return 0;  // Returning 0 to continue processing
    }

    bool hasCollisionOccurred() const {
        return collisionDetected;
    }

    void resetCollisionFlag() {
        collisionDetected = false;
    }

    btRigidBody* rigidBody1;
    btRigidBody* rigidBody2;
    bool collisionDetected;
  

};