#pragma once
#include "Transform.h"
#include "tinyBVH.h"

class Camera
{
public:
	Camera();
	~Camera();

	float4 P1_colorGrading = { 1.f, 1.f, 1.2f, 0.f };
	float4 colorGrading = { 1.f };
	float4 P2_colorGrading = colorGrading;

	float3 camPos, camTarget;
	float3 topLeft, topRight, bottomLeft;
	float3 right, up, ahead;
	float3 tmpUp{ 0, 1, 0 };

	const float P1_fov = 90.f, P1_distortion = 2.f, P1_vignetteIntensity = 5.5f, P1_vignetteRadius = 0.8f;
	const float aspect = (float)SCRWIDTH / (float)SCRHEIGHT;
	const float P2_fov = fov, P2_distortion = distortion, P2_vignetteIntensity = vignetteIntensity, P2_vignetteRadius = vignetteRadius;
	float fov = 40.f, distortion = 40.f, vignetteIntensity = 20.f, vignetteRadius = 0.3f;
	float* skyPixels;

	int skyWidth, skyHeight, skyBpp;
	int abberationIntensity = 0;
	const int P1_abberationIntensity = -1;
	const int P2_abberationIntensity = abberationIntensity;

	bool isMovable = true;
	
	float3 SampleSkybox(tinybvh::Ray ray);
	float3 Panini(float2 ndc);
	tinybvh::Ray GetPrimaryRay(const float x, const float y);
	bool HandleInput(const float t);

};