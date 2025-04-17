#include "precomp.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Renderer* Renderer::renderer_Instance = nullptr;

void Renderer::Init()
{   
	userInterface = new UserInterface();

	accumulator = (float4*)MALLOC64(SCRWIDTH * SCRHEIGHT * 16);
	std::memset(accumulator, 0, SCRWIDTH * SCRHEIGHT * 16);

	InitLights();
	InitPhysics();
}

void Tmpl8::Renderer::Shutdown()
{
}

void Renderer::Tick(float deltaTime)
{
	Timer t;
	dT = deltaTime;

	// Step Physics World
	float timeStep = 1.0f / 30.0f;
	int maxSubSteps = 5;
	dynamicsWorld->stepSimulation(timeStep, maxSubSteps);

	// Synchronise GameObjects with Physics Objects
	for (int i = 0; i < scene.physicsobjects.size(); i++)
		scene.physicsobjects[i]->Synchronise();

	// Synchronise BLASES with GameObjects
	for (int i = 0; i < scene.gameobjects.size(); i++)
		scene.gameobjects[i]->Synchronise(&scene.blases[i]);

	// Rebuild TLAS
	scene.BuildTLAS();

#pragma omp parallel for schedule(dynamic)
	for (int y = 0; y < SCRHEIGHT; y++)
	{
		int pixelHeight = y * SCRWIDTH;
		for (int x = 0; x < SCRWIDTH; x++)
		{	
			if (callDebugBreak)
				DebugBreak();

			if (DEBUG && !callDebugBreak) screen->pixels[x + pixelHeight] = screen->pixels[x + pixelHeight];
			else
			{
				float3 traceResult;
#pragma warning ( push )
#pragma warning ( disable: 4244 )
				tinybvh::Ray r1 = camera.GetPrimaryRay(x, y);
				if (AA)
				{
					tinybvh::Ray r2 = camera.GetPrimaryRay(x + RandomFloat(), y + RandomFloat());
#pragma warning ( pop )
					float3 sample1 = Trace(r1);
					float3 sample2 = Trace(r2);
					traceResult = 0.5f * (sample1 + sample2);
				}
				else
				{
					float3 sample1 = Trace(r1);
					traceResult = sample1;
				}

				if (GAMMACORRECTED)
				{
					//traceResult = aces(traceResult);
					traceResult.x = sqrtf(traceResult.x);
					traceResult.y = sqrtf(traceResult.y);
					traceResult.z = sqrtf(traceResult.z);
				}
				
				float4 average;
				if (accumulates)
				{
					if (abs(distances[x + pixelHeight] - r1.hit.t) < EPSILON)
					{
						samplesPerPixel[x + pixelHeight]++;

						accumulator[x + pixelHeight] += traceResult;
						average = accumulator[x + pixelHeight] * (1.f / samplesPerPixel[x + pixelHeight]);
					}
					else
					{
						samplesPerPixel[x + pixelHeight] = 1;
						accumulator[x + pixelHeight] = traceResult;
						average = accumulator[x + pixelHeight];
					}

					distances[x + pixelHeight] = r1.hit.t;
				}
				else
				{
					accumulator[x + pixelHeight] = traceResult;
					average = accumulator[x + pixelHeight];
				}

				
				if (isPostProcessed)
				{
					// Chromatic Aberration
					float4 aberratedColor = average;
					if (camera.abberationIntensity != 0) // Avoid calculations
					{
						int shiftedX_R = clamp(x + camera.abberationIntensity, 0, SCRWIDTH - 1);
						int shiftedX_B = clamp(x - camera.abberationIntensity, 0, SCRWIDTH - 1);
						float4 color_R = accumulator[shiftedX_R + pixelHeight] * (1.f / samplesPerPixel[x + pixelHeight]);
						float4 color_B = accumulator[shiftedX_B + pixelHeight] * (1.f / samplesPerPixel[x + pixelHeight]);
						float red = 0.75f * average.x + 0.25f * color_R.x; // Blend red shift
						float g = average.y; // Keep green stable
						float b = 0.75f * average.z + 0.25f * color_B.z; // Blend blue shift
						aberratedColor = float4(red, g, b, average.w);
					}

					// Vignette
					float2 uv = { x / (float)SCRWIDTH, y / (float)SCRHEIGHT };
					uv *= 1.0f - uv;
					float vig = uv.x * uv.y * camera.vignetteIntensity;
					vig = pow(vig, camera.vignetteRadius);

					// Color Grading
					aberratedColor *= camera.colorGrading;
#pragma warning ( push )
#pragma warning ( disable: 4238 )
					screen->pixels[x + pixelHeight] = RGBF32_to_RGB8(&(aberratedColor * vig));
				}
				
				else
					screen->pixels[x + pixelHeight] = RGBF32_to_RGB8(&average);
#pragma warning ( pop )
			}
		}
	}

	if (CAPTURE) Capture();

	Debug(t);

	if (camera.HandleInput(deltaTime) || !accumulates) std::memset(accumulator, 0, SCRWIDTH * SCRHEIGHT * 16);
}

float3 Tmpl8::Renderer::Trace(tinybvh::Ray& ray, int recursionCap)
{
	if (recursionCap >= bounces) return float3{ 0.f };

	float3 result{ 0.f };
	float3 throughput{ 1.f };

	scene.tlas.IntersectTLAS(ray);

	if (ray.hit.t >= BVH_FAR) if (SKYBOX) return camera.SampleSkybox(ray); else return float3{ 0.f };

	float3 I = ray.IntersectionPoint();
	float3 V = -ray.D; // Direction

	float3 geometryNormal = scene.GetGeometryNormal(ray); 
	float3 shadingNormal = scene.GetShadingNormal(ray);

	MaterialProperties hitMaterial = scene.GetMaterialBRDF(ray); 

	// A. Debug Views to see each texture separately
	switch (renderingMode)
	{
		case RENDER_STATES::BASECOLOR:
			return float3(hitMaterial.baseColor);
			break;
		case RENDER_STATES::METAL:
			return float3(hitMaterial.metalness);
			break;
		case RENDER_STATES::ROUGHNESS:
			return float3(hitMaterial.roughness);
			break;
		case RENDER_STATES::EMMISIVE:
			return float3(hitMaterial.emissive);
			break;
		case RENDER_STATES::GEOMETRYNORMAL:
			return (geometryNormal + 1) * 0.5f;
			break;
		case RENDER_STATES::SHADINGNORMAL:
			return (shadingNormal + 1) * 0.5f;
			break;
		case RENDER_STATES::BRDF:
			break;
		default:
			break;
	}

	result += throughput * hitMaterial.emissive;

	if (isStochastic)
	{
		float3 illumination{};
		float3 pointLightContribution{ 0.f, 0.f, 0.f };
		float3 directionalLightContribution{ 0.f, 0.f, 0.f };
		float3 spotLightContribution{ 0.f, 0.f, 0.f };

		float pointLightProbability = 0.3f;  // 30%
		float directionalLightProbability = 0.5f;  // 50%
		float spotLightProbability = 0.2f;  // 20%

		// Stochasticly Pick Which Light Type Should Be Sampled: Point Lights or Directional
		float stochasticSeed = RandomFloat();
		int pick = -1;
		if (stochasticSeed < pointLightProbability) pick = 0;  // Point light
		else if (stochasticSeed < pointLightProbability + directionalLightProbability) pick = 1;  // Directional light
		else pick = 2;  // Spot light

		if (pick == 0) // A. Point Lights
		{
			// 1. Light Calculations
			// Convert intersection to SIMD intrinsic
			__m128 Ix = _mm_set1_ps(I.x);
			__m128 Iy = _mm_set1_ps(I.y);
			__m128 Iz = _mm_set1_ps(I.z);

			// 1. Lenght Of Vector
			__m128 Lx = _mm_sub_ps(posX.vec, Ix);
			__m128 Ly = _mm_sub_ps(posY.vec, Iy);
			__m128 Lz = _mm_sub_ps(posZ.vec, Iz);

			// 2. Distance
			__m128 distSq = _mm_add_ps(
				_mm_add_ps(_mm_mul_ps(Lx, Lx), _mm_mul_ps(Ly, Ly)),
				_mm_mul_ps(Lz, Lz)
			);
			__m128 distanceSIMD = _mm_sqrt_ps(distSq);

			// 3. Normalization. Multiply with reciprocal
			__m128 invDist = _mm_rcp_ps(distanceSIMD);
			Lx = _mm_mul_ps(Lx, invDist);
			Ly = _mm_mul_ps(Ly, invDist);
			Lz = _mm_mul_ps(Lz, invDist);

			// 4. cosa = dot(N, L)
			__m128 cosaSIMD = _mm_add_ps(
				_mm_add_ps(_mm_mul_ps(_mm_set1_ps(shadingNormal.x), Lx),
					_mm_mul_ps(_mm_set1_ps(shadingNormal.y), Ly)),
				_mm_mul_ps(_mm_set1_ps(shadingNormal.z), Lz)
			);

			cosaSIMD = _mm_max_ps(cosaSIMD, _mm_set1_ps(0.0f));

			__m128 finalCalculationX = _mm_mul_ps(colorX.vec, _mm_mul_ps(invDist, cosaSIMD));
			__m128 finalCalculationY = _mm_mul_ps(colorY.vec, _mm_mul_ps(invDist, cosaSIMD));
			__m128 finalCalculationZ = _mm_mul_ps(colorZ.vec, _mm_mul_ps(invDist, cosaSIMD));

			for (int i = 0; i < 4; i++)
			{
				tinybvh::Ray shadowRay(I + float3{ Lx.m128_f32[i], Ly.m128_f32[i], Lz.m128_f32[i] } *EPSILON, float3{ Lx.m128_f32[i], Ly.m128_f32[i], Lz.m128_f32[i] }, distSq.m128_f32[i] - EPSILON);

				if (!scene.IsOccluded(shadowRay))
					pointLightContribution += float3{ finalCalculationX.m128_f32[i], finalCalculationY.m128_f32[i], finalCalculationZ.m128_f32[i] };
			}

			// Proper stochastic: This is to ensure that the final result is unbiased and correctly weighted by the probability.
			pointLightContribution /= pointLightProbability;

			// 2. Final Illumination 
			int whichLight = (int)(RandomFloat() * 10) % 4; // Pick what light source should be evaluated for specular
			result += throughput * (LIGHTED ? (BRDF::getInstance()->evalCombinedBRDF(shadingNormal, float3{ Lx.m128_f32[whichLight], Ly.m128_f32[whichLight], Lz.m128_f32[whichLight] }, V, hitMaterial) * pointLightContribution) : float3{});
		}
		else if (pick == 1)// B. Directional Light
		{
			// 1. Light Calculations
			float3 L = scene.directionalLights[0]->transform->position - I;
			float distance = length(L);
			L = L / distance;
			float cosa = max(0.0f, dot(shadingNormal, L));
			tinybvh::Ray shadowRay(I + L * EPSILON, L, distance - EPSILON);
			if (!scene.IsOccluded(shadowRay))
				directionalLightContribution = scene.directionalLights[0]->transform->color * cosa;

			// Proper stochastic: This is to ensure that the final result is unbiased and correctly weighted by the probability.
			directionalLightContribution /= directionalLightProbability;

			// 2. Final Illumination 
			result += throughput * (LIGHTED ? (BRDF::getInstance()->evalCombinedBRDF(shadingNormal, L, V, hitMaterial) * directionalLightContribution) : float3{});
		}
		else if (pick == 2)// B. Spot Light
		{
			// 1. Light Calculations
			float3 L = scene.spotlights[0]->transform->position - I;
			float distance = length(L);
			L = L / distance;
			float cosa = max(0.0f, dot(shadingNormal, L));

			float factor = dot(L, scene.spotlights[0]->transform->rotation);

			tinybvh::Ray shadowRay(I + L * EPSILON, L, distance - EPSILON);

			if (!scene.IsOccluded(shadowRay))
			{
				if (factor > 0.9) spotLightContribution = scene.spotlights[0]->transform->color * (1 / (distance * distance)) * cosa;
				else spotLightContribution = float3(0.f);
			}

			// Proper stochastic: This is to ensure that the final result is unbiased and correctly weighted by the probability.
			spotLightContribution /= spotLightProbability;

			// 2. Final Illumination 
			result += throughput * (LIGHTED ? (BRDF::getInstance()->evalCombinedBRDF(shadingNormal, L, V, hitMaterial) * spotLightContribution) : float3{});
		}
	}
	else
	{
		float3 directionalLightContribution;
		// 1. Light Calculations
		float3 L = scene.directionalLights[0]->transform->position - I;
		float distance = length(L);
		L = L / distance;
		float cosa = max(0.0f, dot(shadingNormal, L));
		tinybvh::Ray shadowRay(I + L * EPSILON, L, distance - EPSILON);
		if (!scene.IsOccluded(shadowRay))
			directionalLightContribution = scene.directionalLights[0]->transform->color * cosa;

		// 2. Final Illumination 
		result += throughput * (LIGHTED ? (BRDF::getInstance()->evalCombinedBRDF(shadingNormal, L, V, hitMaterial) * directionalLightContribution) : float3{});
	}

	int brdfType = 1; // Diffuse by default
	if (recursionCap == bounces - 1) return result;
	// Fast path for dielectrics
	if (hitMaterial.transmissivness == 1)
	{
		// Albedo for dielectric (we'll assume it is color-neutral here)
		float3 albedo = float{ 1.f };

		// Refractive indices: assuming air (n1 = 1.0) and glass (n2 = 1.46)
		float n1 = 1.0f;   // Air
		float n2 = 1.46f;  // Glass

		// Calculate cosine of the angle between the ray direction and the normal
		float cosTheta = clamp(-dot(ray.D, shadingNormal), 0.0f, 1.0f);

		// Compute the reflection direction (using the reflection formula)
		float3 reflectionDir = reflect(ray.D, shadingNormal);

		// Trace the reflection ray
		tinybvh::Ray reflectionRay(I + shadingNormal * EPSILON, reflectionDir);
		float3 reflected = Trace(reflectionRay, recursionCap + 1);

		// Calculate refraction direction using Snell's Law
		float eta = n1 / n2; // Ratio of refractive indices
		float k = 1.0f - eta * eta * (1.0f - cosTheta * cosTheta); // Term to check if total internal reflection occurs

		float3 refracted(0.0f);
		if (k > 0.0f)
		{
			// Refract the ray: Snell's law (refracted direction)
			float3 refractedDir = refract(ray.D, shadingNormal, eta);
			tinybvh::Ray refractionRay(I - shadingNormal * EPSILON, refractedDir);
			refracted = Trace(refractionRay, recursionCap + 1);
		}

		// Fresnel approximation (Schlick's formula)
		float R0 = ((n1 - n2) / (n1 + n2)) * ((n1 - n2) / (n1 + n2)); // Fresnel term at normal incidence
		float fresnel = R0 + (1.0f - R0) * pow(1.0f - cosTheta, 5.0f); // Fresnel term for non-normal incidence

		// Total internal reflection handling (if k < 0, no refraction)
		if (k <= 0.0f) fresnel = 1.0f;  // Total internal reflection occurs when k <= 0

		// Return the final color after blending reflection and refraction
		return albedo * (fresnel * reflected + (1.0f - fresnel) * refracted);
	}
	else
	{
		// Fast path for perfect mirrors
		if (hitMaterial.metalness == 1.0f && hitMaterial.roughness == 0.0f)
			brdfType = SPECULAR_TYPE;
		else
		{
			float brdfProbability = BRDF::getInstance()->getBrdfProbability(hitMaterial, V, shadingNormal);

			if (RandomFloat() < brdfProbability)
			{
				brdfType = SPECULAR_TYPE;
				throughput /= brdfProbability;
			}
			else
			{
				brdfType = DIFFUSE_TYPE;
				throughput /= (1.0f - brdfProbability);
			}
		}
	}
	
	float3 brdfWeight{ 1.f }, rayDirection;
	float2 u = float2(RandomFloat(), RandomFloat());

	if (!BRDF::getInstance()->evalIndirectCombinedBRDF(u, shadingNormal, geometryNormal, V, hitMaterial, brdfType, rayDirection, brdfWeight))
		return result;

	throughput *= brdfWeight;
#pragma warning ( push )
#pragma warning ( disable: 4239 )
	return result + Trace(tinybvh::Ray(I + rayDirection * EPSILON, rayDirection), recursionCap + 1) * throughput;
#pragma warning ( pop )
}

void Tmpl8::Renderer::InitLights()
{
	posX.f[0] = pXLights[0]; posX.f[1] = pXLights[1]; posX.f[2] = pXLights[2]; posX.f[3] = pXLights[3];
	posY.f[0] = pYLights[0]; posY.f[1] = pYLights[1]; posY.f[2] = pYLights[2]; posY.f[3] = pYLights[3];
	posZ.f[0] = pZLights[0]; posZ.f[1] = pZLights[1]; posZ.f[2] = pZLights[2]; posZ.f[3] = pZLights[3];

	colorX.f[0] = cXLights[0]; colorX.f[1] = cXLights[1]; colorX.f[2] = cXLights[2]; colorX.f[3] = cXLights[3];
	colorY.f[0] = cYLights[0]; colorY.f[1] = cYLights[1]; colorY.f[2] = cYLights[2]; colorY.f[3] = cYLights[3];
	colorZ.f[0] = cZLights[0]; colorZ.f[1] = cZLights[1]; colorZ.f[2] = cZLights[2]; colorZ.f[3] = cZLights[3];
}

void Tmpl8::Renderer::InitPhysics()
{
	// Bullet Physics World Initialization
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfig);
	broadphase = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver();
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
	dynamicsWorld->setGravity(btVector3(0, GRAVITY, 0));

	Spaceship = new PhysicsObject(scene.gameobjects[0], "../assets/scene1/XShip.json", 0, false);
	dynamicsWorld->addRigidBody(Spaceship->body);
	scene.physicsobjects.push_back(Spaceship);

	dynamicsWorld->setDebugDrawer(debugger);
	debugger->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawContactPoints);
}

void Tmpl8::Renderer::Capture()
{
	auto start = std::chrono::system_clock::now();
	auto end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end - start;
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);

	// stbi write png takes RGB but screen pixels is uint format (4 byte integers)
	// the function expects raw 8 bit rgbrgbrgb... not 32 bit integers
	// convert from 32 bit form (argb) to rgb
	uint8_t* rgbPixels = new uint8_t[SCRWIDTH * SCRHEIGHT * 3];
	for (int i = 0; i < SCRWIDTH * SCRHEIGHT; i++) {
		uint32_t pixel = screen->pixels[i];
		uint8_t r = (pixel >> 16) & 0xFF;
		uint8_t g = (pixel >> 8) & 0xFF;
		uint8_t b = pixel & 0xFF;

		rgbPixels[i * 3 + 0] = r;
		rgbPixels[i * 3 + 1] = g;
		rgbPixels[i * 3 + 2] = b;
	}
	std::ostringstream ss;
	ss << "../assets/captures/capture_" << std::put_time(std::localtime(&end_time), "%Y-%m-%d_%H-%M-%S") << ".png";

	stbi_write_png(ss.str().c_str(), SCRWIDTH, SCRHEIGHT, 3, rgbPixels, SCRWIDTH * 3);
	delete[] rgbPixels;
	CAPTURE = false;
}

void Tmpl8::Renderer::Debug(Timer t)
{
	avg = 10, alpha = 1;
	avg = (1 - alpha) * avg + alpha * t.elapsed() * 1000;
	if (alpha > 0.05f) alpha *= 0.5f;
	fps = 1000.0f / avg;
	rps = (SCRWIDTH * SCRHEIGHT) / avg;
}

void Renderer::UI()
{
#if  GAMETYPE == DEBUGMODE
	userInterface->UI();
#else

#endif

}

void Tmpl8::Renderer::MouseUp(int button)
{
	UNREFERENCED_PARAMETER(button);
}

void Tmpl8::Renderer::MouseDown(int button)
{
	UNREFERENCED_PARAMETER(button);
}

void Tmpl8::Renderer::MouseMove(int x, int y)
{
	mousePos.x = x, mousePos.y = y;
}

void Tmpl8::Renderer::MouseWheel(float y)
{
	UNREFERENCED_PARAMETER(y);
}

void Tmpl8::Renderer::KeyUp(int key)
{
	if (key == GLFW_KEY_SPACE)
	{
	}
}

void Tmpl8::Renderer::KeyDown(int key)
{
	if (key == GLFW_KEY_F)
	{
		if (DEBUG)
			callDebugBreak = true;
	}
}

float3 Tmpl8::Renderer::refract(const float3& incidentDirection, const float3& normal, float eta)
{
	float cosi = clamp(dot(incidentDirection, normal), -1.0f, 1.0f);
	float etai = 1.0f;  // air (outside)
	float etat = eta;  // material

	// If the ray is going into the material
	if (cosi > 0.0f) 
	{
		std::swap(etai, etat);  // Swap if the ray is entering the material
	}

	float etaRatio = etai / etat;
	float cosTheta = fabs(cosi);  // Angle of incidence

	// Snell's Law
	float k = 1.0f - etaRatio * etaRatio * (1.0f - cosTheta * cosTheta);
	if (k < 0.0f) 
	{
		// Total internal reflection, return no refraction (just reflect)
		return float3(0.0f, 0.0f, 0.0f);
	}
	else 
	{
		// Calculate refraction direction
		float3 refractedDir = etaRatio * (incidentDirection - normal * cosTheta) - normal * sqrt(k);
		return refractedDir;
	}
}