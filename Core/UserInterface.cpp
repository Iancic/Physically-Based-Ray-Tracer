#include "precomp.h"
#include "UserInterface.h"

UserInterface::UserInterface()
{
	items[0] = "DamagedHelmet";
	items[1] = "SciFiHelmet";
	item_current = 0;

	lights[0] = "pointlight";
	lights[1] = "directionallight";
	lights_current = 0;
}

UserInterface::~UserInterface()
{
	
}

void UserInterface::UI()
{
	Style();
	Performance();

	if (ImGui::Checkbox("Accumulate", &Renderer::getInstance()->accumulates)) {  };
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	static Renderer::RENDER_STATES& renderingMode = Renderer::getInstance()->renderingMode;

	const char* renderStateNames[] = { "Combined", "Base Color", "Geometry Normal", "Shading Normal", "Metal", "Roughness", "Emissive" };

	int currentMode = static_cast<int>(renderingMode);

	if (ImGui::Combo("##25", &currentMode, renderStateNames, IM_ARRAYSIZE(renderStateNames)))
	{
		Renderer::getInstance()->renderingMode = static_cast<Renderer::RENDER_STATES>(currentMode);
	}
		
	ImGui::Dummy(ImVec2(0.0f, 15.0f));

	if (ImGui::BeginTabBar("Tabs"))
	{
		if (ImGui::BeginTabItem("Rendering"))
		{
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			Rendering();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Camera"))
		{
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			Camera();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Light"))
		{
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			LightsHierarchy();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Physics Objects"))
		{
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			PhysicsObjectsHierarchy();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Debugger"))
		{
			ImGui::Dummy(ImVec2(0.0f, 10.0f));
			Debug();
			ImGui::EndTabItem();
		}
		else
		{
			Renderer::getInstance()->DEBUG = false;
		}

		ImGui::EndTabBar();
	}
}

void UserInterface::Performance()
{
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Text("%5.2f ms", Renderer::getInstance()->avg); ImGui::SameLine();
	ImGui::Text("%.1f fps", Renderer::getInstance()->fps); ImGui::SameLine();
	ImGui::Text("%.1f Mrays/s", Renderer::getInstance()->rps / 1000);

	ImGui::Dummy(ImVec2(0.0f, 5.0f));
}

void UserInterface::Rendering()
{
	ImGui::Text("Secondary Bounces: "); ImGui::SameLine();
	float sliderWidth = 50.0f;
	ImGui::PushItemWidth(sliderWidth);
	ImGui::DragInt("##", &Renderer::getInstance()->bounces, 1, 0, 10);
	ImGui::PopItemWidth();

	ImGui::Checkbox("Stochastic Lighting", &Renderer::getInstance()->isStochastic);

	ImGui::Checkbox("Anti Aliasing", &Renderer::getInstance()->AA);

	ImGui::Checkbox("Gamma Correction", &Renderer::getInstance()->GAMMACORRECTED);

	ImGui::Checkbox("Disable Lights", &Renderer::getInstance()->LIGHTED);

	ImGui::Checkbox("Enable Normal Mapping", &Renderer::getInstance()->NORMALMAPPED);

	ImGui::Checkbox("Skybox", &Renderer::getInstance()->SKYBOX);

	ImGui::Checkbox("Enable Bullet Colliders", &Renderer::getInstance()->COLLIDERS);

	ImVec2 textSize = ImGui::CalcTextSize("WASD - Move Camera");
	float windowWidth = ImGui::GetWindowSize().x;
	float textPosX = (windowWidth - textSize.x) * 0.5f; // Centering formula

	ImGui::SetCursorPosX(textPosX); // Set horizontal position
	ImGui::Text("%s", "WASD - Move Camera");

	textSize = ImGui::CalcTextSize("RF - Up and Down");
	windowWidth = ImGui::GetWindowSize().x;
	textPosX = (windowWidth - textSize.x) * 0.5f; // Centering formula

	ImGui::SetCursorPosX(textPosX); // Set horizontal position
	ImGui::Text("%s", "RF - Up and Down");

	textSize = ImGui::CalcTextSize("Arrows - Rotate Camera");
	windowWidth = ImGui::GetWindowSize().x;
	textPosX = (windowWidth - textSize.x) * 0.5f; // Centering formula

	ImGui::SetCursorPosX(textPosX); // Set horizontal position
	ImGui::Text("%s", "Arrows - Rotate Camera");

	ImGui::Dummy(ImVec2(0.0f, 10.0f));
}

void UserInterface::Debug()
{
#pragma warning ( push )
#pragma warning ( disable: 4456 )
	Renderer::getInstance()->DEBUG = true;

	const int gridSize = 6;
	const int pixelOffset = -1; // Adjust to sample nearby pixels
	const float buttonSize = 20.0f; // Adjust for desired button size
	const float spacing = 4.0f; // Space between buttons

	// Calculate total grid width
	float totalWidth = gridSize * (buttonSize + spacing) - spacing;

	// Get the available space in the ImGui window
	float windowWidth = ImGui::GetContentRegionAvail().x;


	int px = Renderer::getInstance()->mousePos.x;
	int py = Renderer::getInstance()->mousePos.y;

	uint pixel = 0xFF00FF; // Default magenta
	int red = -1, green = -1, blue = -1;

	if (px >= 0 && px < SCRWIDTH && py >= 0 && py < SCRHEIGHT)
	{
		pixel = Renderer::getInstance()->screen->pixels[px + py * SCRWIDTH];
		red = (pixel & 0xFF0000) >> 16;
		green = (pixel & 0x00FF00) >> 8;
		blue = pixel & 0x0000FF;
	}

	ImVec4 color = (red >= 0) ? ImVec4(red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f)
		: ImVec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta fallback

	ImGui::Text("Cursor on: %d, %d", px, py); ImGui::SameLine();
	ImGui::ColorButton("Color", color, 0, ImVec2(buttonSize, buttonSize));

	// Compute horizontal centering offset
	float offsetX = (windowWidth - totalWidth) / 2.0f;
	if (offsetX < 0) offsetX = 0; // Prevent negative offset

	for (int y = 0; y < gridSize; y++)
	{
		// Reset cursor position at the start of each row to ensure centering
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

		for (int x = 0; x < gridSize; x++)
		{
			int px = Renderer::getInstance()->mousePos.x + (x - gridSize / 2) * pixelOffset;
			int py = Renderer::getInstance()->mousePos.y + (y - gridSize / 2) * pixelOffset;

			uint pixel = 0xFF00FF;
			int red = -1, green = -1, blue = -1;

			if (px >= 0 && px < SCRWIDTH && py >= 0 && py < SCRHEIGHT)
			{
				pixel = Renderer::getInstance()->screen->pixels[px + py * SCRWIDTH];
				red = (pixel & 0xFF0000) >> 16;
				green = (pixel & 0x00FF00) >> 8;
				blue = pixel & 0x0000FF;
			}

			ImVec4 color = (red >= 0) ? ImVec4(red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f)
				: ImVec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta fallback

			std::string label = "Color" + std::to_string(x) + std::to_string(y);

			// Draw a crosshair on the center pixel
			if (x == gridSize / 2 && y == gridSize / 2)
			{
				ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 255)); // White border
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
				ImGui::ColorButton(label.c_str(), color, ImGuiColorEditFlags_None, ImVec2(buttonSize, buttonSize));
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::ColorButton(label.c_str(), color, 0, ImVec2(buttonSize, buttonSize));
			}

			if (x < gridSize - 1)
				ImGui::SameLine(0, spacing);
		}
	}

	ImVec2 textSize = ImGui::CalcTextSize("Press F to Debug Break Ray");
	windowWidth = ImGui::GetWindowSize().x;
	float textPosX = (windowWidth - textSize.x) * 0.5f; // Centering formula

	ImGui::SetCursorPosX(textPosX); // Set horizontal position
	ImGui::Text("%s", "Press F to Debug Break Ray");

#pragma warning ( pop )
}

void Tmpl8::UserInterface::Camera()
{
	Renderer* renderer = Renderer::getInstance();

	ImGui::Checkbox("Free Cam", &Renderer::getInstance()->camera.isMovable);
	if (!Renderer::getInstance()->camera.isMovable)
	{
		ImGui::DragFloat3("Target", &renderer->camera.camTarget.x, 0.1f);
		ImGui::DragFloat3("Position", &renderer->camera.camPos.x, 0.1f);
	}

	// Set the desired width for sliders (adjust as needed)
	float sliderWidth = 150.0f;  // You can tweak this value
	ImGui::Checkbox("Enable Post Processing", &Renderer::getInstance()->isPostProcessed);
	if(Renderer::getInstance()->isPostProcessed)
	{
		ImGui::PushItemWidth(sliderWidth);
		ImGui::DragFloat3("Color Grading", &renderer->camera.colorGrading.x, 0.1f);
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(sliderWidth);
		ImGui::DragInt("Chromatic Aberration", &renderer->camera.abberationIntensity, 1, -5, 5);
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(sliderWidth);
		ImGui::DragFloat("Field Of View", &renderer->camera.fov, 1.0f, 0, 150);
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(sliderWidth);
		ImGui::DragFloat("Panini Distortion", &renderer->camera.distortion, 0.5f, 0, 150);
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(sliderWidth);
		ImGui::DragFloat("Vignette Intensity", &renderer->camera.vignetteIntensity, 0.5f, 0, 150);
		ImGui::PopItemWidth();

		ImGui::PushItemWidth(sliderWidth);
		ImGui::DragFloat("Vignette Radius", &renderer->camera.vignetteRadius, 0.1f, 0, 5);
		ImGui::PopItemWidth();
		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		float windowWidth = ImGui::GetContentRegionAvail().x;
		float buttonWidth = 120.0f;  // Adjust the button width as needed
		float offset = (windowWidth - buttonWidth) * 0.5f;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
		if (ImGui::Button("Preset 1", ImVec2(buttonWidth, 0)))
		{
			renderer->camera.colorGrading = renderer->camera.P1_colorGrading;
			renderer->camera.vignetteIntensity = renderer->camera.P1_vignetteIntensity;
			renderer->camera.vignetteRadius = renderer->camera.vignetteRadius;
			renderer->camera.distortion = renderer->camera.P1_distortion;
			renderer->camera.fov = renderer->camera.P1_fov;
		}

		ImGui::Spacing();

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
		if (ImGui::Button("Reset", ImVec2(buttonWidth, 0)))
		{
			renderer->camera.colorGrading = renderer->camera.P2_colorGrading;
			renderer->camera.vignetteIntensity = renderer->camera.P2_vignetteIntensity;
			renderer->camera.vignetteRadius = renderer->camera.vignetteRadius;
			renderer->camera.distortion = renderer->camera.P2_distortion;
			renderer->camera.fov = renderer->camera.P2_fov;

		}
	}

	float windowWidth = ImGui::GetContentRegionAvail().x;
	float buttonWidth = 120.0f;  // Adjust the button width as needed
	float offset = (windowWidth - buttonWidth) * 0.5f;
	ImGui::Spacing();

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
	if (ImGui::Button("Capture", ImVec2(buttonWidth, 0)))
	{
		Renderer::getInstance()->CAPTURE = true;
		std::cout << "Capture button clicked!" << std::endl;
	}
}

void UserInterface::LightsHierarchy()
{
	for (size_t i = 0; i < POINTLIGHTS; ++i)
	{
		if (ImGui::TreeNode(("Point Light " + std::to_string(i + 1)).c_str()))
		{
			// Position controls
			ImGui::Text("P ");
			ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("##1", &Renderer::getInstance()->posX.f[i], 0.1f);
			ImGui::SameLine();
			ImGui::DragFloat("##2", &Renderer::getInstance()->posY.f[i], 0.1f);
			ImGui::SameLine();
			ImGui::DragFloat("##3", &Renderer::getInstance()->posZ.f[i], 0.1f);
			ImGui::PopItemWidth();

			ImGui::Dummy(ImVec2(0.0f, 3.0f));

			// Color controls
			ImGui::Text("C ");
			ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("##4", &Renderer::getInstance()->colorX.f[i], 0.1f);
			ImGui::SameLine();
			ImGui::DragFloat("##5", &Renderer::getInstance()->colorY.f[i], 0.1f);
			ImGui::SameLine();
			ImGui::DragFloat("##6", &Renderer::getInstance()->colorZ.f[i], 0.1f);
			ImGui::PopItemWidth();

			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			// Close the tree node for this point light
			ImGui::TreePop();
		}
	}

	// Hierarchy for directional lights
	for (size_t i = 0; i < Renderer::getInstance()->scene.directionalLights.size(); ++i)
	{
		if (ImGui::TreeNode(("Directional Light " + std::to_string(i + 1)).c_str()))
		{
			ImGui::Text("P "); ImGui::SameLine();
			if (ImGui::DragFloat3("##1", &Renderer::getInstance()->scene.directionalLights[i]->transform->position.x, 0.1f))
				Renderer::getInstance()->scene.directionalLights[i]->Update();
			ImGui::Dummy(ImVec2(0.0f, 3.0f));
			ImGui::Text("C "); ImGui::SameLine();
			if (ImGui::DragFloat3("##2", &Renderer::getInstance()->scene.directionalLights[i]->transform->color.x))
				Renderer::getInstance()->scene.directionalLights[i]->Update();
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			if (ImGui::Button("Delete"))
			{
				Renderer::getInstance()->scene.directionalLights[i]->DeleteData();
				Renderer::getInstance()->scene.directionalLights.erase(Renderer::getInstance()->scene.directionalLights.begin() + i);
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset"))
			{
				Renderer::getInstance()->scene.directionalLights[i]->Reset();
				Renderer::getInstance()->scene.directionalLights[i]->Update();
			}

			ImGui::TreePop();
		}
	}

	// Hierarchy for spot lights
	for (size_t i = 0; i < Renderer::getInstance()->scene.spotlights.size(); ++i)
	{
		if (ImGui::TreeNode(("Spot Light " + std::to_string(i + 1)).c_str()))
		{
			ImGui::Text("P "); ImGui::SameLine();
			if (ImGui::DragFloat3("##1", &Renderer::getInstance()->scene.spotlights[i]->transform->position.x, 0.1f))
				Renderer::getInstance()->scene.spotlights[i]->Update();
			ImGui::Dummy(ImVec2(0.0f, 3.0f));
			ImGui::Text("C "); ImGui::SameLine();
			if (ImGui::DragFloat3("##2", &Renderer::getInstance()->scene.spotlights[i]->transform->color.x))
				Renderer::getInstance()->scene.spotlights[i]->Update();
			ImGui::Dummy(ImVec2(0.0f, 5.0f));
			ImGui::Text("R "); ImGui::SameLine();
			if (ImGui::DragFloat3("##3", &Renderer::getInstance()->scene.spotlights[i]->transform->rotation.x, 0.1f))
				Renderer::getInstance()->scene.spotlights[i]->Update();
			ImGui::Dummy(ImVec2(0.0f, 3.0f));
			if (ImGui::Button("Delete"))
			{
				Renderer::getInstance()->scene.spotlights[i]->DeleteData();
				Renderer::getInstance()->scene.spotlights.erase(Renderer::getInstance()->scene.spotlights.begin() + i);
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset"))
			{
				Renderer::getInstance()->scene.spotlights[i]->Reset();
				Renderer::getInstance()->scene.spotlights[i]->Update();
			}

			ImGui::TreePop();
		}
	}
}

void UserInterface::PhysicsObjectsHierarchy()
{
	for (int i = 0; i < Renderer::getInstance()->scene.physicsobjects.size(); ++i)
	{
		if (ImGui::TreeNode(("Game Object " + std::to_string(i + 1)).c_str()))
		{
			ImGui::Text("P "); ImGui::SameLine(); if (ImGui::Button("Reset Position")) { Renderer::getInstance()->scene.physicsobjects[i]->translationImgui = 0.f; Renderer::getInstance()->scene.physicsobjects[i]->Update(); } ImGui::SameLine();
			if (ImGui::DragFloat3("##1", &Renderer::getInstance()->scene.physicsobjects[i]->translationImgui.x, 0.025f, -50, 50))
			{
				Renderer::getInstance()->scene.physicsobjects[i]->Update();
				Renderer::getInstance()->scene.physicsobjects[i]->body->clearForces();
			}

			ImGui::Text("R "); ImGui::SameLine(); if (ImGui::Button("Reset Rotation")) { Renderer::getInstance()->scene.physicsobjects[i]->rotationImgui = 0.f; Renderer::getInstance()->scene.physicsobjects[i]->Update(); } ImGui::SameLine();
			if (ImGui::DragFloat3("##2", &Renderer::getInstance()->scene.physicsobjects[i]->rotationImgui.x, 0.5f, -360, 360))
			{
				Renderer::getInstance()->scene.physicsobjects[i]->Update();
				Renderer::getInstance()->scene.physicsobjects[i]->body->clearForces(); 
			}
			ImGui::Dummy(ImVec2(0.0f, 5.0f));

			ImGui::TreePop();
		}
	}
}

void UserInterface::Style()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;
	
#if  GAMETYPE == GAME
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 2.5f;
#endif

	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.95f);
	colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

	colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

	colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);

	colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);

	ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(255, 255, 255, 255));
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(255, 255, 255, 255));
	ImGui::PushStyleColor(ImGuiCol_Tab, IM_COL32(40, 40, 40, 255));            // Dark gray for inactive tabs
	ImGui::PushStyleColor(ImGuiCol_TabHovered, IM_COL32(80, 80, 80, 255));     // Lighter gray when hovered
	ImGui::PushStyleColor(ImGuiCol_TabActive, IM_COL32(70, 70, 70, 255));      // Dark gray for active tab
	ImGui::PushStyleColor(ImGuiCol_TabUnfocused, IM_COL32(30, 30, 30, 255));  // Even darker when unfocused
	ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, IM_COL32(50, 50, 50, 255));
}