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
	camera.mWorld.SetRotateX(0.0f); camera.mWorld.SetRotateY(0.0f); camera.mWorld.SetRotateZ(0.0f);

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
		Node space;
			solar_system_node.add_child(&space);


	//
	// Set up the nodes and other related attributes
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

	// space
	space.set_geometry(sphere);
	space.scale(glm::vec3(30.0f, 30.0f, 30.0f));
	GLuint const space_texture = bonobo::loadTexture2D("stars.png");
	space.add_texture("diffuse_texture", space_texture, GL_TEXTURE_2D);


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

		// fix camera position near earth
		glm::mat4 earth_pos_world = earth_pivot.get_transform()*earth_center.get_transform()*earth_axis.get_transform();
		printf("\033c");
		printf("earth center:\n[%f, %f, %f, %f]\n \n",earth_pos_world[3][0],earth_pos_world[3][1],earth_pos_world[3][1],earth_pos_world[3][3]);
		float zt = 3.0f;
		camera.mWorld.SetTranslate(glm::vec3(earth_pos_world[3][0]/earth_pos_world[3][3] - zt,
			-zt,
			earth_pos_world[3][2]/earth_pos_world[3][3]) + zt);
		glm::mat4 camo = camera.mWorld.GetMatrix();
		printf("cam rot:\n[%f, %f, %f\n%f, %f, %f\n%f, %f, %f]\n \n", camo[0][0], camo[0][1], camo[0][2],
			camo[1][0],camo[1][1], camo[1][2],
			camo[2][0], camo[2][1], camo[2][2]);
		printf("cam pos:\n[%f, %f, %f, %f]\n \n", camo[3][0], camo[3][1], camo[3][2], camo[3][3]);
		

		//
		// Traverse the scene graph and render all nodes
		//
		std::stack<Node const*> node_stack({ &solar_system_node });
		std::stack<glm::mat4> matrix_stack({ glm::mat4(1.0f) });

		Node const* current = NULL;

		while (!node_stack.empty()) {
			current = node_stack.top();
			node_stack.pop();
			// Render if leaf
			if (current->get_children_nb() == 0) {
				current->render(camera.GetWorldToClipMatrix(), matrix_stack.top(), shader, [](GLuint /*program*/) {});
				matrix_stack.pop();
			}
			else {
				glm::mat4 parent_transform = matrix_stack.top();
				matrix_stack.pop();
				// Add children to stack
				for ( int i = 0; i < current->get_children_nb(); i++){
					node_stack.push(current->get_child(i));
					matrix_stack.push(parent_transform * current->get_child(i)->get_transform());
				}
			}

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
