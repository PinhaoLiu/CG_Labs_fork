#include "project.hpp"
#include "EDAF80/parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/helpers.hpp"
#include "core/opengl.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tinyfiledialogs.h>

#include <array>
#include <clocale>
#include <cstdlib>
#include <stdexcept>

#include "fluidSystem.hpp"


edan35::Project::Project(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
		static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
		0.01f, 1000.f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera,
		config::resolution_x, config::resolution_y,
		0, 0, 0, 0
	};

	window = mWindowManager.CreateGLFWWindow("EDAN35: Project",	window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edan35::Project::~Project()
{
	bonobo::deinit();
}

void
edan35::Project::run()
{
	constexpr int pointCounts = 100 * 100 * 100;

	FluidSystem fluidSystem;
	fluidSystem.init(pointCounts, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 50.0f),
		glm::vec3(30.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 20.0f), glm::vec3(0.0f, -9.8f, 0.0f));

	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(-50.0f, 20.0f, 50.0f));
	mCamera.mWorld.LookTowards(glm::vec3(1.0f, 0.0f, .0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f);	// 3 m/s => 10.8 km/h

	// particle-based water shader
	ShaderProgramManager program_manager;
	GLuint shader = 0u;
	program_manager.CreateAndRegisterProgram("water",
		{ { ShaderType::vertex, "project/shader.vert" },
		  { ShaderType::fragment, "project/shader.frag" } },
		shader);
	if (shader == 0u) {
		LogError("Failed to load particle-based water shader");
		return;
	}

	// Create particles
	auto const shape = parametric_shapes::createSphere(0.5f, 10u, 10u);
	if (shape.vao == 0u)
		return;

	auto points = fluidSystem.getPointBuf();
	auto point_counts = fluidSystem.getPointCounts(); std::cout << point_counts << '\n';
	auto velocity = new glm::vec3[point_counts];
	auto position = new glm::vec3[point_counts];
	for (int i = 0; i < point_counts; i++) {
		velocity[i] = points[i].velocity;
		position[i] = points[i].pos;

	}

	


	auto nodes = new Node[point_counts]();
	for (int i = 0; i < point_counts; i++) {
		nodes[i].set_geometry(shape);
		nodes[i].set_program(&shader);
		auto point = fluidSystem.getPointBuf() + i;
		auto pos = point->pos;
		nodes[i].get_transform().SetTranslate(pos);
	}


	// Initialize buffer
	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	auto lastTime = std::chrono::high_resolution_clock::now();

	std::int32_t program_index = 0;
	float elapsed_time_s = 0.0f;
	auto cull_mode = bonobo::cull_mode_t::disabled;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool show_logs = true;
	bool show_gui = true;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;

	changeCullMode(cull_mode);



	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);
		auto const deltaTime = std::chrono::duration<float>(deltaTimeUs).count();
		elapsed_time_s += deltaTime;

		// Change window layout

		// F2: show or hide gui
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;

		// F3: show or hide log
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;

		// F11: toggle full screen status for window or not
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);

		// TODO: Add more keys to change other layouts

		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);

		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		bonobo::changePolygonMode(polygon_mode);

		//
		fluidSystem.tick();

		for (int i = 0; i < point_counts; i++) {
			auto point = fluidSystem.getPointBuf() + i;
			auto position = point->pos;
			nodes[i].get_transform().SetTranslate(position);
			nodes[i].render(mCamera.GetWorldToClipMatrix());
		}


		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
		if (opened) {

			// Select polygon mode.
			bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);

			// Separator
			ImGui::Separator();
		}

		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	srand((int)time(0));

	try {
		edan35::Project project(framework.GetWindowManager());
		project.run();
	}
	catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
