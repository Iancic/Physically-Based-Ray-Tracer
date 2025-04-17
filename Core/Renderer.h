#pragma once
#include "Scene.h"
#include "UserInterface.h"
#include "btBulletDynamicsCommon.h"
#include "DebugDrawer.h"
#include "ContactCallback.h"
#include <Audio/Sound.hpp>

namespace Tmpl8
{

class Renderer : public TheApp
{
private:
	static Renderer* renderer_Instance;

	Renderer() {};
public:
	~Renderer() {};

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	static Renderer* getInstance()
	{
		if (renderer_Instance == nullptr)
		{
			renderer_Instance = new Renderer();
		}
		return renderer_Instance;
	}

	bool accumulates = true;

	// IMGUI Rendering Options
	int bounces = 2;
	enum class RENDER_STATES
	{
		BRDF,
		BASECOLOR,
		GEOMETRYNORMAL,
		SHADINGNORMAL,
		METAL,
		ROUGHNESS,
		EMMISIVE,
	};
	RENDER_STATES renderingMode = RENDER_STATES::BRDF;
	bool AUDIOPLAYING = true, LIGHTED = true, GAMMACORRECTED = true, NORMALMAPPED = true, SKYBOX = true, COLLIDERS = false, CAPTURE = false, AA = true, isPostProcessed = false, isStochastic = true;
	bool DEBUG = false, callDebugBreak = false;
	
	// Bullet Physics
	DebugDrawer* debugger = new DebugDrawer();
	btDefaultCollisionConfiguration* collisionConfig;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* broadphase;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;

	int2 mousePos;

	int samplesPerPixel[1280 * 720] = { 1 };
	float distances[1280 * 720] = { -1.f };
	float4* accumulator;

	float dT; //deltaTime
	float avg = 10, alpha = 1, fps, rps;
	
	Scene scene;
	Camera camera;
	UserInterface* userInterface;

	PhysicsObject* Spaceship = nullptr;

	// SIMD Lights
	union Vec4f {
		__m128 vec;
		float f[4];
	};

	Vec4f posX, posY, posZ, posXL, posYL, posZL;
	Vec4f colorX, colorY, colorZ;
	bool activeLights[POINTLIGHTS] = { false };
	float pXLights[POINTLIGHTS] = { 0.f };
	float pYLights[POINTLIGHTS] = { 0.f };
	float pZLights[POINTLIGHTS] = { 0.f };
	float cXLights[POINTLIGHTS] = { 0.f };
	float cYLights[POINTLIGHTS] = { 0.f };
	float cZLights[POINTLIGHTS] = { 0.f };

	void Init();
	void Tick(float deltaTime);
	void Shutdown();
	float3 Trace(tinybvh::Ray& ray, int recursionCap = 0);

	// Utilities
	void InitLights();
	void InitPhysics();
	void Capture();
	void Debug(Timer t);
	void UI();

	// Input
	void MouseUp(int button);
	void MouseDown(int button);
	void MouseMove(int x, int y);
	void MouseWheel(float y);
	void KeyUp(int key);
	void KeyDown(int key);

	float3 refract(const float3& incidentDirection, const float3& normal, float eta);

};
}