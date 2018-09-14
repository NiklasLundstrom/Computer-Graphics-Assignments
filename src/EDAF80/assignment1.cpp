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
	camera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
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

	//
	// Create node tree
	//
	Node solar_system_node;
		Node sun_node;
			solar_system_node.add_child(&sun_node);
		Node earth_pivot;
			solar_system_node.add_child(&earth_pivot);
			Node earth_center;
				earth_pivot.add_child(&earth_center);
				Node earth_axis;
					earth_center.add_child(&earth_axis);
					Node earth_node;
						earth_axis.add_child(&earth_node);
				Node moon_pivot;
					earth_center.add_child(&moon_pivot);
					Node moon_node;
						moon_pivot.add_child(&moon_node);



	//
	// Set up the sun node and other related attributes
	//
	// Sun
	sun_node.set_geometry(sphere);
	sun_node.set_scaling(glm::vec3(1.0f, 1.0f, 1.0f));// Sun scaling
	float const sun_spin_speed = glm::two_pi<float>() / 2.0f; // Full rotation in six seconds
	GLuint const sun_texture = bonobo::loadTexture2D("sunmap.png");
	sun_node.add_texture("diffuse_texture", sun_texture, GL_TEXTURE_2D);
	
	// earth pivot
	float const earth_pivot_spin_speed = glm::two_pi<float>() / 20.0f;

	// earth center
	earth_center.set_translation(glm::vec3(4.0f, 0.0f, 0.0f));
	float const earth_center_spin_speed = -earth_pivot_spin_speed;

	// earth axis
	earth_axis.set_rotation_z(glm::pi<float>() / 6);

	// earth
	earth_node.set_geometry(sphere);
	earth_node.scale(glm::vec3(0.5f, 0.5f, 0.5f));
	float const earth_spin_speed = glm::two_pi<float>() / 1.0f;
	GLuint const earth_texture = bonobo::loadTexture2D("earthmap.png");
	earth_node.add_texture("diffuse_texture", earth_texture, GL_TEXTURE_2D);
	

	// moon pivot
	float const moon_pivot_spin_speed = glm::two_pi<float>() / 6.0f;

	// moon
	moon_node.set_geometry(sphere);
	moon_node.scale(glm::vec3(0.15f, 0.15f, 0.15f));
	moon_node.set_translation(glm::vec3(1.0f, 0.0f, 0.0f));
	GLuint const moon_texture = bonobo::loadTexture2D("moonmap.png");
	moon_node.add_texture("diffuse_texture", moon_texture, GL_TEXTURE_2D);

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

		earth_pivot.rotate_y(earth_pivot_spin_speed * delta_time);

		earth_center.rotate_y(earth_center_spin_speed * delta_time);

		earth_node.rotate_y(earth_spin_speed * delta_time);

		moon_pivot.rotate_y(moon_pivot_spin_speed * delta_time);



		//
		// Traverse the scene graph and render all nodes
		//
		std::stack<Node const*> node_stack({ &solar_system_node });
		std::stack<glm::mat4> matrix_stack({ glm::mat4(1.0f) });
		std::stack<int> childnbr({ -1 });
		Node const* current = NULL;

		while (!node_stack.empty()) {
			// set current to next child if there is one
			if (((int)node_stack.top()->get_children_nb() - 1) > childnbr.top()) {
				childnbr.top() += 1;
				current = node_stack.top()->get_child(childnbr.top());
			}
			// go further down the tree as long as it is possible
			while (current != NULL) {
				node_stack.push(current);
				childnbr.push(-1);
				matrix_stack.push( matrix_stack.top() * current->get_transform());
				if (node_stack.top()->get_children_nb() != 0) {
					childnbr.top() += 1;
					current = node_stack.top()->get_child(childnbr.top());
				}else{
					current = NULL;
				}
			}
			// render leafs only
			if (node_stack.top()->get_children_nb() == 0) {
				node_stack.top()->render(camera.GetWorldToClipMatrix(), matrix_stack.top(), shader, [](GLuint /*program*/) {});
			}
			// pop node
			childnbr.pop();
			node_stack.pop();
			matrix_stack.pop();
		}

		
		//
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
