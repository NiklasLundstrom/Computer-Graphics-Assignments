#include "assignment4.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/ShaderProgramManager.hpp"
#include "core/node.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <external/imgui_impl_glfw_gl3.h>
#include <glm/gtc/type_ptr.hpp>
#include <tinyfiledialogs.h>

#include <cstdlib>
#include <stdexcept>

enum class polygon_mode_t : unsigned int {
	fill = 0u,
	line,
	point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

edaf80::Assignment4::Assignment4() :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(), window(nullptr)
{
	Log::View::Init();

	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateWindow("EDAF80: Assignment 4", window_datum, config::msaa_rate);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

edaf80::Assignment4::~Assignment4()
{
	Log::View::Destroy();
}

void
edaf80::Assignment4::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 10.0f, 130.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025f;

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/fallback.vert" },
	                                           { ShaderType::fragment, "EDAF80/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	//
	// Todo: Insert the creation of other shader programs.
	//       (Check how it was done in assignment 3.)
	//
	GLuint skybox = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/skybox.vert" },
											   { ShaderType::fragment, "EDAF80/skybox.frag" } },
		skybox);
	if (skybox == 0u) {
		LogError("Failed to load skybox");
	}

	GLuint water = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/water.vert" },
											   { ShaderType::fragment, "EDAF80/water.frag" } },
		water);
	if (water == 0u) {
		LogError("Failed to load water");
	}

	//
	// Todo: Load your geometry
	//
	auto const quad = parametric_shapes::createQuad(500, 500);
	if (quad.vao == 0u) {
		LogError("Failed to load quad");
	}

	auto const sphere = parametric_shapes::createSphere(40, 40, 0.25);
	if (sphere.vao == 0u) {
		LogError("Failed to load sphere");
	}

	auto light_position = glm::vec3(-32.0f, 64.0f, 32.0f);
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto amplitude = glm::vec2(0.192f, 0.067f);
	auto direction = glm::vec4(-1.0f, 0.0f
							, -0.7f, 0.7f);
	auto phase = glm::vec2(0.157f, 0.784f);
	auto frequency = glm::vec2(3.373f, 1.961f);
	auto sharpness = glm::vec2(2.0f, 2.0f);
	float time = 0.0f;
	float dt = 0.1f;


	auto const set_uniforms = [&light_position, &camera_position, &amplitude, &direction, &phase, &frequency, &sharpness, &time](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform2fv(glGetUniformLocation(program, "amplitude"), 1, glm::value_ptr(amplitude));
		glUniform4fv(glGetUniformLocation(program, "direction"), 1, glm::value_ptr(direction));
		glUniform2fv(glGetUniformLocation(program, "phase"), 1, glm::value_ptr(phase));
		glUniform2fv(glGetUniformLocation(program, "frequency"), 1, glm::value_ptr(frequency));
		glUniform2fv(glGetUniformLocation(program, "sharpness"), 1, glm::value_ptr(sharpness));
		glUniform1f(glGetUniformLocation(program, "time"), time);
	};

	auto quadNode = Node();
	quadNode.set_geometry(quad);
	quadNode.set_program(&water, set_uniforms);

	quadNode.scale(glm::vec3(50, 50, 50));
	quadNode.set_translation(glm::vec3(0, -4, 0));

	// normal mapping
	GLuint const normal_texture = bonobo::loadTexture2D("waves.png");
	quadNode.add_texture("normal_texture", normal_texture, GL_TEXTURE_2D);

	// cubemapping
	auto my_cube_map_id = bonobo::loadTextureCubeMap("cloudyhills/posx.png", "cloudyhills/negx.png",
		"cloudyhills/posy.png", "cloudyhills/negy.png",
		"cloudyhills/posz.png", "cloudyhills/negz.png", true);
	quadNode.add_texture("my_cube_map", my_cube_map_id, GL_TEXTURE_CUBE_MAP);


	auto sky = Node();
	sky.set_geometry(sphere);
	sky.scale(glm::vec3(1000.0, 1000.0, 1000.0));
	sky.set_program(&skybox, set_uniforms);
	sky.add_texture("my_cube_map", my_cube_map_id, GL_TEXTURE_CUBE_MAP);

	auto polygon_mode = polygon_mode_t::fill;
	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;

	while (!glfwWindowShouldClose(window)) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(ddeltatime, inputHandler);

		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}

		ImGui_ImplGlfwGL3_NewFrame();

		// Todo: If you need to handle inputs, you can do it here
		//

		// set up z-press
		if (inputHandler.GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
			polygon_mode = get_next_mode(polygon_mode);
		}
		switch (polygon_mode) {
		case polygon_mode_t::fill:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case polygon_mode_t::line:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case polygon_mode_t::point:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		}

		camera_position = mCamera.mWorld.GetTranslation();

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		if (!shader_reload_failed) {
			quadNode.render(mCamera.GetWorldToClipMatrix(), quadNode.get_transform());
			sky.render(mCamera.GetWorldToClipMatrix(), sky.get_transform());
		}

		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(400, 1000), -1.0f, 0);
		if (opened) {
			ImGui::SliderFloat("Amplitude Wave 1", &amplitude[0], 0.0f, 1.0f);
			ImGui::SliderFloat("Amplitude Wave 2", &amplitude[1], 0.0f, 1.0f);
			ImGui::SliderFloat("Frequency Wave 1", &frequency[0], 0.0f, 20.0f);
			ImGui::SliderFloat("Frequency Wave 2", &frequency[1], 0.0f, 20.0f);
			ImGui::SliderFloat("Phase Wave 1", &phase[0], 0.0f, 20.0f);
			ImGui::SliderFloat("Phase Wave 2", &phase[1], 0.0f, 20.0f);
			ImGui::SliderFloat("Sharpness Wave 1", &sharpness[0], 0.0f, 20.0f);
			ImGui::SliderFloat("Sharpness Wave 2", &sharpness[1], 0.0f, 20.0f);
		}
		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		if (show_logs)
			Log::View::Render();
		if (show_gui)
			ImGui::Render();

		glfwSwapBuffers(window);
		lastTime = nowTime;

		time += dt;

	}
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment4 assignment4;
		assignment4.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
