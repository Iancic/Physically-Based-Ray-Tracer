#pragma once
#include "Renderer.h"
#include "Scene.h"

namespace Tmpl8
{
	class UserInterface
	{
	public:
		UserInterface();
		~UserInterface();

		void UI();

		ImGuiWindowFlags HUD_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground;
		bool open = true;

		ImFont* customFont;

	private:
		int selectedTexture = 0;

		const char* items[2];
		int item_current;

		const char* lights[2];
		int lights_current;

		void Performance();
		void Rendering();
		void Debug();
		void Camera();
		void LightsHierarchy();
		void PhysicsObjectsHierarchy();
		void Style();
	};
}