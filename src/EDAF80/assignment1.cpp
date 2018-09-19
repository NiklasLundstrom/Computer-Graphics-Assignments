#include "config.hpp"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"
#include "core/WindowManager.hpp"

#include <imgui.h>
#include <external/imgui_impl_glfw_gl3.h>

#include <stack>

#include <cstdlib>


int main()
{
	//
	// Set up the logging system
	//
	Log::Init();
	Log::View::Init();

	//
	// Set up the camera
	//
	InputHandler input_handler;
	FPSCameraf camera(0.5f * glm::half_pi<float>(),
	                  static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                  0.01f, 1000.0f);
	//camera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	camera.mMouseSensitivity = 0.003f;
	camera.mMovementSpeed = 0.25f * 12.0f;

	//
	// Set up the windowing system and create the window
	//
	WindowManager window_manager;
	WindowManager::WindowDatum window_datum{ input_handler, camera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};
	GLFWwindow* window = window_manager.CreateWindow("EDAF80: Assignment 1", window_datum, config::msaa_rate);
	if (window == nullptr) {
		LogError("Failed to get a window: exiting.");

		Log::View::Destroy();
		Log::Destroy();

		return EXIT_FAILURE;
	}

	//
	// Load the sphere geometry
	//
	std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects("sphere.obj");
	if (objects.empty()) {
		LogError("Failed to load the sphere geometry: exiting.");

		Log::View::Destroy();
		Log::Destroy();

		return EXIT_FAILURE;
	}
	bonobo::mesh_data const& sphere = objects.front();


	//
	// Create the shader program
	//
	ShaderProgramManager program_manager;
	GLuint shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/default.vert" },
	                                           { ShaderType::fragment, "EDAF80/default.frag" } },
	                                         shader);
	if (shader == 0u) {
		LogError("Failed to generate the shader program: exiting.");

		Log::View::Destroy();
		Log::Destroy();

		return EXIT_FAILURE;
	}

	// set up the solar system nodes
	Node solar_system_node;
		Node sun_node;
		solar_system_node.add_child(&sun_node);
		Node earth_moon_pivot_node;
		solar_system_node.add_child(&earth_moon_pivot_node);
			Node earth_moon_translate_node;
			earth_moon_pivot_node.add_child(&earth_moon_translate_node);
				Node earth_node;
				earth_moon_translate_node.add_child(&earth_node);
				Node moon_pivot_node;
				earth_moon_translate_node.add_child(&moon_pivot_node);
					Node moon_node;
					moon_pivot_node.add_child(&moon_node);


	sun_node.set_geometry(sphere);
	earth_node.set_geometry(sphere);
	moon_node.set_geometry(sphere);
	GLuint const sun_texture = bonobo::loadTexture2D("sunmap.png");
	GLuint const earth_texture = bonobo::loadTexture2D("earth_diffuse.png");
	GLuint const moon_texture = bonobo::loadTexture2D("noise.png");
	sun_node.add_texture("diffuse_texture", sun_texture, GL_TEXTURE_2D);
	earth_node.add_texture("diffuse_texture", earth_texture, GL_TEXTURE_2D);
	moon_node.add_texture("diffuse_texture", moon_texture, GL_TEXTURE_2D);
	float const sun_spin_speed = glm::two_pi<float>() / 18.0f; // Full rotation in twelwe seconds
	float const earth_moon_pivot_spin_speed = glm::two_pi<float>() / 12.0f; //Full rotation in six seconds
	float const earth_moon_translate_compensate_spin_speed = -earth_moon_pivot_spin_speed;
	float const earth_spin_speed = glm::two_pi<float>() / 3.0f;
	float const moon_pivot_spin_speed = glm::two_pi<float>() / 2.0f; //Full rotation in two seconds
	earth_moon_translate_node.set_translation(glm::vec3(3.0f, 0.0f, 0.0f));
	moon_node.set_translation(glm::vec3(0.5f, 0.0f, 0.0f));
	earth_node.set_scaling(glm::vec3(0.2f, 0.2f, 0.2f));
	moon_node.set_scaling(glm::vec3(0.05f, 0.05f, 0.05f));
	earth_node.rotate_z(2.0f/4.0f);
	


	glViewport(0, 0, config::resolution_x, config::resolution_y);
	glClearDepthf(1.0f);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glEnable(GL_DEPTH_TEST);


	size_t fpsSamples = 0;
	double lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;


	bool show_logs = true;
	bool show_gui = true;

	while (!glfwWindowShouldClose(window)) {
		//
		// Compute timings information
		//
		double const nowTime = GetTimeSeconds();
		double const delta_time = nowTime - lastTime;
		lastTime = nowTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1.0;
			fpsSamples = 0;
		}
		++fpsSamples;


		//
		// Process inputs
		//
		glfwPollEvents();

		ImGuiIO const& io = ImGui::GetIO();
		input_handler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);
		input_handler.Advance();
		camera.Update(delta_time, input_handler);

		if (input_handler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (input_handler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;


		//
		// Start a new frame for Dear ImGui
		//
		ImGui_ImplGlfwGL3_NewFrame();


		//
		// Clear the screen
		//
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


		//
		// Update the transforms
		//
		sun_node.rotate_y(sun_spin_speed * delta_time);
		earth_moon_pivot_node.rotate_y(earth_moon_pivot_spin_speed * delta_time);
		earth_node.rotate_y(earth_spin_speed * delta_time);
		moon_pivot_node.rotate_y(moon_pivot_spin_speed * delta_time);
		earth_moon_translate_node.rotate_y(earth_moon_translate_compensate_spin_speed * delta_time);

		glm::mat4 earth_pos_world = earth_moon_pivot_node.get_transform()*earth_moon_translate_node.get_transform()*earth_node.get_transform();
		//std::cout << '[' << earth_pos_world[3][0] << ", " << earth_pos_world[3][1] << ", " << earth_pos_world[3][1] << ", " << earth_pos_world[3][3] << ']' << std::endl;
		camera.mWorld.SetTranslate(glm::vec3(earth_pos_world[3][0] / earth_pos_world[3][3],
			earth_pos_world[3][1] / earth_pos_world[3][3],
			earth_pos_world[3][2] / earth_pos_world[3][3]) + 1.0f);

		

		//
		// Traverse the scene graph and render all nodes
		//
		std::stack<Node const*> node_stack({ &solar_system_node });
		std::stack<glm::mat4> matrix_stack({ glm::mat4(1.0f) });
		while (!node_stack.empty()) {
			Node const* top_node = node_stack.top();
			node_stack.pop();
			if (top_node->get_children_nb() == 0) {
				top_node->render(camera.GetWorldToClipMatrix(), matrix_stack.top(), shader, [](GLuint /*program*/) {});
				matrix_stack.pop();
			}
			else {
				glm::mat4 top_matrix = matrix_stack.top();
				matrix_stack.pop();
				for (int i = 0; i < top_node->get_children_nb(); i++) {
					node_stack.push(top_node->get_child(i));
					matrix_stack.push(top_matrix*top_node->get_child(i)->get_transform());
				}
			}


		}


		// TODO: Replace this explicit rendering of the Sun with a
		// traversal of the scene graph and rendering of all its nodes.
		//sun_node.render(camera.GetWorldToClipMatrix(), sun_node.get_transform(), shader, [](GLuint /*program*/){});
		//earth_node.render(camera.GetWorldToClipMatrix(), earth_moon_pivot_node.get_transform()*earth_moon_translate_node.get_transform()*earth_node.get_transform(), shader, [](GLuint /*program*/) {});
		//moon_node.render(camera.GetWorldToClipMatrix(), earth_moon_pivot_node.get_transform()*earth_moon_translate_node.get_transform()*moon_pivot_node.get_transform()*moon_node.get_transform(), shader, [](GLuint /*program*/) {});
		// Display Dear ImGui windows
		//
		if (show_logs)
			Log::View::Render();
		if (show_gui)
			ImGui::Render();


		//
		// Queue the computed frame for display on screen
		//
		glfwSwapBuffers(window);
	}

	glDeleteTextures(1, &sun_texture);


	Log::View::Destroy();
	Log::Destroy();

	return EXIT_SUCCESS;
}
