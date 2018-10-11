#include "assignment5.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <external/imgui_impl_glfw_gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tinyfiledialogs.h>

#include <cstdlib>
#include <stdexcept>

edaf80::Assignment5::Assignment5() :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(), window(nullptr)
{
	Log::View::Init();

	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

edaf80::Assignment5::~Assignment5()
{
	Log::View::Destroy();
}

void
edaf80::Assignment5::run()
{	//
	// Set up the camera
	//
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 30.0f, 70.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025f;

	//
	// Create the shader programs
	//
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/fallback.vert" },
	                                           { ShaderType::fragment, "EDAF80/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/phong.vert" },
											   { ShaderType::fragment, "EDAF80/phong.frag" } },
		phong_shader);
	if (phong_shader == 0u) {
		LogError("Failed to load phong shader");
	}

	//
	// set up height
	//
	float ground_height = 0.0;

	//
	// set up uniform variables
	//
	auto light_position = glm::vec3(-32.0f, 64.0f, 32.0f);
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.2f, 0.1f, 0.1f);
	auto diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 15.0f;
	auto const phong_set_uniforms = [&light_position, &camera_position, &ambient, &diffuse, &specular, &shininess](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
	};
	
	//
	// Load the geometries
	//

		// teapot
	std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects("utah-teapot.obj");
	if (objects.empty()) {
		LogError("Failed to load the box geometry: exiting.");
		Log::View::Destroy();
		Log::Destroy();
		return;
	}
	bonobo::mesh_data const& teapot = objects.front();

		// quad
	auto const quad = parametric_shapes::createQuad(500, 500);
	if (quad.vao == 0u) {
		LogError("Failed to load quad");
	}

	//
	// Set up node tree
	//
	// TODO set upp node tree
	Node world = Node();
		Node car = Node();
			world.add_child(&car);
			Node car_geometry = Node();
				car.add_child(&car_geometry);
		Node ground = Node();
			world.add_child(&ground);

	//
	// set up nodes
	//
	// TODO set up nodes

	// car
	glm::vec3 car_pos = glm::vec3(0, ground_height, 0);
	car.set_translation(car_pos);
	glm::vec3 car_dir = glm::vec3(0, 0, -1);
	float rot_speed = glm::pi<float>()/32;

	// car geometry
	
	car_geometry.set_geometry(teapot);
	car_geometry.set_scaling(glm::vec3(1, 1, 1));
	car_geometry.set_rotation_y(glm::half_pi<float>());
	car_geometry.set_translation(glm::vec3(0, 9, 0)); // TODO can we get the objects size?
	car_geometry.set_program(&phong_shader, phong_set_uniforms);

	// ground
	ground.set_geometry(quad);
	ground.set_scaling(glm::vec3(30, 30, 30));
	ground.set_translation(glm::vec3(0, ground_height, 0));
	ground.set_program(&phong_shader, phong_set_uniforms);


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

		//
		// Todo: If you need to handle inputs, you can do it here
		//
		glm::mat4 rot = glm::mat4(1.0f);
		if (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & PRESSED) {
			rot = glm::rotate(rot, -rot_speed, glm::vec3(0, 1, 0));
			car_dir = (rot * glm::vec4(car_dir, 1));
			car_dir = glm::normalize(glm::vec3(car_dir.x, car_dir.y, car_dir.z));
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & PRESSED) {
			rot = glm::rotate(rot, rot_speed, glm::vec3(0, 1, 0));
			car_dir = (rot * glm::vec4(car_dir, 1));
			car_dir = glm::normalize(glm::vec3(car_dir.x, car_dir.y, car_dir.z));
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_UP) & PRESSED)
			car_pos += glm::normalize(car_dir);
		if (inputHandler.GetKeycodeState(GLFW_KEY_DOWN) & PRESSED)
			car_pos -= glm::normalize(car_dir);;

		//
		// update car pos
		//
		car.set_translation(car_pos);

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		if (!shader_reload_failed) {
			//
			// Todo: Render properly, not explicit for all nodes
			//
			car_geometry.render(mCamera.GetWorldToClipMatrix(), car.get_transform()*car_geometry.get_transform());
			ground.render(mCamera.GetWorldToClipMatrix(), ground.get_transform());

		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//

		if (show_logs)
			Log::View::Render();
		if (show_gui)
			ImGui::Render();

		glfwSwapBuffers(window);
		lastTime = nowTime;
	}
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment5 assignment5;
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
