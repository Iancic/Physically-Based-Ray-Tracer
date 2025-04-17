#pragma once
#include "precomp.h"
#include "Camera.h"
#include "stb_image.h"

Camera::Camera()
{
	// Load HDR skydome
	skyPixels = stbi_loadf("../assets/skydomes/workshop3.hdr", &skyWidth, &skyHeight, &skyBpp, 0);

#if  GAMETYPE == DEBUGMODE
	// Load position and target view from JSON
	std::ifstream jsonIn("../assets/prefabs/camera.json");
	json data = json::parse(jsonIn);
	camPos = float3(data["pX"].get<float>(), data["pY"].get<float>(), data["pZ"].get<float>());
	camTarget = float3(data["tX"].get<float>(), data["tY"].get<float>(), data["tZ"].get<float>());
#else
	colorGrading = P1_colorGrading;
	vignetteIntensity = P1_vignetteIntensity;
	vignetteRadius = vignetteRadius;
	distortion = P1_distortion;
	fov = P1_fov;
	abberationIntensity = P1_abberationIntensity;
	isMovable = false;
	camPos = float3{ -0.40299490094184875,  -0.9010030627250671, 0.7681850075721741 };
	camTarget = float3{ -0.368676096200943, -1.2066869735717773, 0.014384759590029716 };
#endif

	ahead = normalize(camTarget - camPos);
	camTarget = camPos + ahead;
	right = normalize(cross(ahead, tmpUp));
	up = normalize(cross(right, ahead));

	topLeft = camPos + ahead * 2.0f - aspect * right + up;
	topRight = camPos + ahead * 2.0f + aspect * right + up;
	bottomLeft = camPos + ahead * 2.0f - aspect * right - up;
}

Camera::~Camera()
{
}

float3 Camera::SampleSkybox(tinybvh::Ray ray)
{
	float u = 0.5f + (atan2f(ray.D.z, ray.D.x) / (2.0f * PI));
	float v = acosf(ray.D.y) / PI;

	float uTex = u * skyWidth;
	float vTex = v * skyHeight;

	uint u0 = static_cast<uint>(floor(uTex)) % skyWidth;
	uint v0 = static_cast<uint>(floor(vTex)) % skyHeight;
	uint u1 = (u0 + 1) % skyWidth;
	uint v1 = (v0 + 1) % skyHeight;

	float du = uTex - u0;
	float dv = vTex - v0;

	uint idx00 = (u0 + v0 * skyWidth) * 3;
	uint idx01 = (u1 + v0 * skyWidth) * 3;
	uint idx10 = (u0 + v1 * skyWidth) * 3;
	uint idx11 = (u1 + v1 * skyWidth) * 3;

	float3 color00 = float3(skyPixels[idx00], skyPixels[idx00 + 1], skyPixels[idx00 + 2]);
	float3 color01 = float3(skyPixels[idx01], skyPixels[idx01 + 1], skyPixels[idx01 + 2]);
	float3 color10 = float3(skyPixels[idx10], skyPixels[idx10 + 1], skyPixels[idx10 + 2]);
	float3 color11 = float3(skyPixels[idx11], skyPixels[idx11 + 1], skyPixels[idx11 + 2]);

	float3 interpU0 = color00 + du * (color01 - color00);
	float3 interpU1 = color10 + du * (color11 - color10);
	float3 finalColor = interpU0 + dv * (interpU1 - interpU0);

	return finalColor;
}

// perspective/rectilinear (0 distortion) vs panini
// 90 degree angle before distortion / up to 140 degress with less distortion
// straight only / vertical lines remain straight
// strong edge distortion / low edge distortion
// small to medium fov / wide angle views
float3 Camera::Panini(float2 ndc)
{
	// Field Of View
	float fo = PI / 2 - fov * 0.5f;
	float f = cos(fo) / sin(fo) * 2.0f;
	float f2 = f * f;

	// Horizontal Scaling
	float d2 = distortion * distortion;
	float b = (sqrt(std::max(0.0f, (distortion + d2) * (distortion + d2) * (f2 + f2 * f2))) - (distortion * f + f)) / (d2 + d2 * f2 - 1.0f);
	ndc *= b;
	float h = ndc.x, v = ndc.y;

	float h2 = h * h;
	float k = h2 / ((distortion + 1.0f) * (distortion + 1.0f));
	float k2 = k * k;

	// Corrected Horizontal Angle
	float discr = std::max(0.0f, k2 * d2 - (k + 1.0f) * (k * d2 - 1.0f));
	float cosPhi = (-k * distortion + sqrt(discr)) / (k + 1.0f);

	// Corrected Vertical Angle
	float S = (distortion + 1.0f) / (distortion + cosPhi);
	float tanTheta = v / S;

	// Final Direction Given 
	float sinPhi = sqrt(std::max(0.0f, 1.0f - cosPhi * cosPhi));
	if (ndc.x < 0.0f) sinPhi *= -1.0f;
	float s = 1.0f / sqrt(1.0f + tanTheta * tanTheta);
	return float3(sinPhi, tanTheta, cosPhi) * s;
}

tinybvh::Ray Camera::GetPrimaryRay(const float x, const float y)
{
	// calculate pixel position on virtual screen plane
	const float u = (float)x * (1.0f / SCRWIDTH); // [0,1]
	const float v = (float)y * (1.0f / SCRHEIGHT);

	const float2 ndc{ (2.0f * u) - 1.0f, 1.0f - (2.0f * v) }; // [-1,1]
	const float3 P = topLeft + u * (topRight - topLeft) + v * (bottomLeft - topLeft);

	const float3 rayDir = normalize(P - camPos);  // Initial ray direction

	if (Renderer::getInstance()->isPostProcessed)
	{
		// Apply Panini projection to adjust the ray direction
		const float3 paniniDir = Panini(ndc);
		const float3 correctedRay = paniniDir * length(P - camPos); // Preserve original magnitude

		// Transform the corrected ray into world space
		const float3 worldRay = normalize(
			right * correctedRay.x +
			up * correctedRay.y +
			ahead * correctedRay.z);
		return tinybvh::Ray(camPos, worldRay); // Return the ray with the panini camera projection
	}
	else
		return tinybvh::Ray(camPos, rayDir);
}

bool Camera::HandleInput(const float t)
{
	if (!WindowHasFocus()) return false;
	float speed = 0.0025f * t;
	ahead = normalize(camTarget - camPos);
	right = normalize(cross(ahead, tmpUp)); // Not tmpUp first!
	up    = normalize(cross(right, ahead));
	bool changed = false;

	if (isMovable)
	{
		if (IsKeyDown(GLFW_KEY_A)) camPos -= speed * 2 * right, changed = true;
		if (IsKeyDown(GLFW_KEY_D)) camPos += speed * 2 * right, changed = true;
		if (IsKeyDown(GLFW_KEY_W)) camPos += speed * 2 * ahead, changed = true;
		if (IsKeyDown(GLFW_KEY_S)) camPos -= speed * 2 * ahead, changed = true;
		if (IsKeyDown(GLFW_KEY_R)) camPos += speed * 2 * up, changed = true;
		if (IsKeyDown(GLFW_KEY_F)) camPos -= speed * 2 * up, changed = true;
	}

	camTarget = camPos + ahead;

	if (isMovable)
	{
		if (IsKeyDown(GLFW_KEY_UP)) camTarget += speed * up, changed = true;
		if (IsKeyDown(GLFW_KEY_DOWN)) camTarget -= speed * up, changed = true;
		if (IsKeyDown(GLFW_KEY_LEFT)) camTarget -= speed * right, changed = true;
		if (IsKeyDown(GLFW_KEY_RIGHT)) camTarget += speed * right, changed = true;
	}

	if (!changed) return false;
	ahead = normalize(camTarget - camPos);
	up = normalize(cross(ahead, right));
	up = normalize(cross(right, ahead));
	topLeft = camPos + ahead * 2.0f - aspect * right + up;
	topRight = camPos + ahead * 2.0f + aspect * right + up;
	bottomLeft = camPos + ahead * 2.0f - aspect * right - up;

	json newData;

	// New Positions
	newData["pX"] = camPos.x;
	newData["pY"] = camPos.y;
	newData["pZ"] = camPos.z;

	// New Target
	newData["tX"] = camTarget.x;
	newData["tY"] = camTarget.y;
	newData["tZ"] = camTarget.z;

	std::ofstream jsonOut("../assets/prefabs/camera.json");
	jsonOut << std::setw(4) << newData;

	return true;
}