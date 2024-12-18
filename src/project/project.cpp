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
	/*fluidSystem.init(pointCounts, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(100.0f, 100.0f, 100.0f),
		glm::vec3(75.0f, 0.0f, 0.0f), glm::vec3(100.0f, 100.0f, 25.0f), glm::vec3(0.0f, -9.8f, 0.0f));*/

	fluidSystem.init(pointCounts, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 50.0f),
		glm::vec3(25.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 25.0f), glm::vec3(0.0f, -9.8f, 0.0f));

	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(-50.0f, 40.0f, 50.0f));
	mCamera.mWorld.LookTowards(glm::vec3(1.0f, 0.0f, 0.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f);	// 3 m/s => 10.8 km/h

	// particle-based water shader
	ShaderProgramManager program_manager;
	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram("phong",
		{ { ShaderType::vertex, "project/phong.vert"},
		  { ShaderType::fragment, "project/phong.frag" } },
		phong_shader);

	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Skybox",
		{ {	ShaderType::vertex, "EDAF80/skybox.vert" },
		  { ShaderType::fragment, "EDAF80/skybox.frag" } },
		skybox_shader);

	if (skybox_shader == 0u)
		LogError("Failed to load skybox shader");

	auto light_position = glm::vec3(-100.0f, 100.0f, 100.0f);
	auto const set_uniforms = [&light_position](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		};

	auto skybox_shape = parametric_shapes::createSphere(20.0f, 100u, 100u);
	if (skybox_shape.vao == 0u) {
		LogError("Failed to retrieve the mesh for the skybox");
		return;
	}

	GLuint cubeMap = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/NissiBeach2/posx.jpg"),
		config::resources_path("cubemaps/NissiBeach2/negx.jpg"),
		config::resources_path("cubemaps/NissiBeach2/posy.jpg"),
		config::resources_path("cubemaps/NissiBeach2/negy.jpg"),
		config::resources_path("cubemaps/NissiBeach2/posz.jpg"),
		config::resources_path("cubemaps/NissiBeach2/negz.jpg")
	);

	Node skybox;
	skybox.set_geometry(skybox_shape);
	skybox.set_program(&skybox_shader, set_uniforms);
	skybox.add_texture("cubemap", cubeMap, GL_TEXTURE_CUBE_MAP);


	// Create particles
	auto const shape = parametric_shapes::createSphere(0.8f, 2u, 2u);
	if (shape.vao == 0u)
		return;

	auto point_counts = fluidSystem.getPointCounts(); std::cout << point_counts << '\n';
	auto points = fluidSystem.getPointBuf();
	
	auto camera_position = mCamera.mWorld.GetTranslation();

	bonobo::material_data demo_material;
	demo_material.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
	demo_material.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
	demo_material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
	demo_material.shininess = 10.0f;

	auto nodes = new Node[point_counts]();
	for (int i = 0; i < point_counts; i++) {
		auto& velocity = points[i].velocity;

		auto const phong_set_uniforms = [&light_position, &camera_position, &velocity](GLuint program) {
			glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
			glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
			glUniform3fv(glGetUniformLocation(program, "velocity"), 1, glm::value_ptr(velocity));
			};

		nodes[i].set_geometry(shape);
		nodes[i].set_material_constants(demo_material);
		nodes[i].set_program(&phong_shader, phong_set_uniforms);
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
	auto cull_mode = bonobo::cull_mode_t::front_faces;
	auto polygon_mode = bonobo::polygon_mode_t::line;
	bool show_gui = true;

	changeCullMode(cull_mode);

	bool start = false;

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

		bonobo::changePolygonMode(bonobo::polygon_mode_t::fill);
		changeCullMode(bonobo::cull_mode_t::disabled);

		skybox.get_transform().SetTranslate(mCamera.mWorld.GetTranslation());
		glDisable(GL_DEPTH_TEST);
		skybox.render(mCamera.GetWorldToClipMatrix());
		glEnable(GL_DEPTH_TEST);

		bonobo::changePolygonMode(polygon_mode);
		changeCullMode(cull_mode);

		//
		if (start)
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
			ImGui::Checkbox("Start animation", &start);
		}

		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
