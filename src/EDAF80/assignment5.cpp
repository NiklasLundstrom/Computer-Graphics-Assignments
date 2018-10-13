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
#include <external/lodepng.h>
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

unsigned char terrainHeight(std::vector<unsigned char> image, float x, float z, int width, int height, float scale) {
	if (x > scale || x < -scale || z > scale || z < -scale)
		return 0u;
	if (x < -50)
		int bp = 0;
	// TODO interpolate between pixels
	//int u = ((-x / scale) + 1)*(width - 1) / 2;
	//int v = ((z / scale) + 1)*(height - 1) / 2;

	float delta = 4.0f;
	int u_1 = glm::clamp( ((-(x-delta) / scale) + 1)/2, 0.0f, 1.0f )*(width - 1);
	int u = ((-(x) / scale) + 1)*(width - 1) / 2;
	int u1 = glm::clamp( ((-(x + delta) / scale) + 1) / 2, 0.0f, 1.0f)*(width - 1);
	int v_1 = glm::clamp( (((z-delta) / scale) + 1) / 2, 0.0f, 1.0f)*(height - 1);
	int v = ((z / scale) + 1)*(height - 1) / 2;
	int v1 = glm::clamp( (((z+delta) / scale) + 1) / 2, 0.0f, 1.0f)*(height - 1);

	int offset = 100;
	int p_1_1 = image.at(height*u_1 + v_1);
	int p_1_0 = image.at(height*u_1 + v);
	int p_11 = image.at(height*u_1 + v1);
	int p_0_1 = image.at(height*u + v_1);
	int p_0_0 = image.at(height*u + v);
	int p_01 = image.at(height*u + v1);
	int p1_1 = image.at(height*u1 + v_1);
	int p1_0 = image.at(height*u1 + v);
	int p11 = image.at(height*u1 + v1);

	int mean = (p_1_1 + p_1_0 + p_11 +
				p_0_1 + p_0_0 + p_01 +
				p1_1 + p1_0 + p11) / 9;
	
	return mean * offset / 255;// image.at(height*u + v)*offset / 255;
}

void
edaf80::Assignment5::run()
{	//
	// Set up the camera
	//
	mCamera.mWorld.SetTranslate(glm::vec3(100.0f, 200.0f, 200.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f;

	int chosen_cam = 1;
	
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

	GLuint terrain_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/terrain.vert" },
											   { ShaderType::fragment, "EDAF80/terrain.frag" } },
		terrain_shader);
	if (terrain_shader == 0u) {
		LogError("Failed to load terrain shader");
	}

	GLuint car_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/car.vert" },
											   { ShaderType::fragment, "EDAF80/car.frag" } },
		car_shader);
	if (car_shader == 0u) {
		LogError("Failed to load car shader");
	}
	
	//
	// set up height
	//
	//float ground_height = 0.0;
	//
	// load height map
	//
	std::string landscape = "landscape.png";
	u32 width, height;
	auto const path = config::resources_path("textures/" + landscape);
	std::vector<unsigned char> image;
	if (lodepng::decode(image, width, height, path, LCT_GREY) != 0) {
		LogWarning("Couldn't load or decode image file %s", path.c_str());
		return;
	}
	float ground_scale = 300.0f;
	printf("width: %d, height: %d\nsize: %d\n", width, height, image.size());
	printf("0,0: %d\n", terrainHeight(image, 0, 0, width, height, ground_scale));
	printf("s,0: %d\n", terrainHeight(image, ground_scale, 0, width, height, ground_scale));
	printf("0,s: %d\n", terrainHeight(image, 0, ground_scale, width, height, ground_scale));
	printf("s,s: %d\n", terrainHeight(image, ground_scale, ground_scale, width, height, ground_scale));

	//
	// set up uniform variables
	//
	auto light_position = glm::vec3(-100.0f, 200.0f, 30.0f);
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
	Node world = Node();
		Node car = Node();
			world.add_child(&car);
			Node car_rot = Node();
				car.add_child(&car_rot);
				Node car_geometry = Node();
					car_rot.add_child(&car_geometry);
				Node car_cam = Node();
					car_rot.add_child(&car_cam);
			Node world_cam = Node();
				car.add_child(&world_cam);
		Node ground = Node();
			world.add_child(&ground);
		

	//
	// set up nodes
	//

	// car
	int ground_height =	terrainHeight(image, 0, 0, width, height, ground_scale);
	glm::vec3 car_pos = glm::vec3(0, 0, 0); // TODO vary car_pos.y according to height map
	car.set_translation(car_pos);
	float car_speed = 100.0;


	// car rotation
	glm::vec3 car_dir = glm::vec3(0, 0, -1);
	float car_rot_speed = glm::pi<float>();

	// car geometry
	
	car_geometry.set_geometry(teapot);
	car_geometry.set_scaling(glm::vec3(1, 1, 1));
	car_geometry.set_rotation_y(glm::half_pi<float>());
	car_geometry.set_translation(glm::vec3(0, 9, 0)); // TODO can we get the objects size?
	car_geometry.set_program(&car_shader, phong_set_uniforms);
	GLuint const height_map = bonobo::loadTexture2D(landscape);
	car_geometry.add_texture("height_map", height_map, GL_TEXTURE_2D);
	
	// car camera ("first person"), chosen camera: 2
	car_cam.set_translation(glm::vec3(0, 50, 100));

	// world camera ("third person"), chosen camera: 1
	world_cam.set_translation(glm::vec3(0, 50, 100));
	float wcam_car_dist = 300.0;
	float wcam_height_angle = glm::pi<float>()/3;
	float wcam_y_angle = 0.0;

	// ground
	ground.set_geometry(quad);
	ground.set_scaling(glm::vec3(1, 1, 1)*ground_scale);
	ground.set_translation(glm::vec3(0, 0, 0));
	ground.set_program(&terrain_shader, phong_set_uniforms);
	ground.add_texture("height_map", height_map, GL_TEXTURE_2D);


	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);


	f64 ddeltatime;
	size_t fpsSamples = 0;
	int currFPS = 0;
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
			currFPS = fpsSamples;
			fpsSamples = 0;
		}
		fpsSamples++;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		//mCamera.Update(ddeltatime, inputHandler);

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
		// Inputs, keyboard etc.
		//
		//handleInput();
		glm::mat4 rot = glm::mat4(1.0f);
		float theta = 0.0;
		if (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & PRESSED) {
			theta = -car_rot_speed*0.001f * ddeltatime;
			rot = glm::rotate(rot, theta, glm::vec3(0, 1, 0));
			car_dir = (rot * glm::vec4(car_dir, 1));
			car_dir = glm::normalize(glm::vec3(car_dir.x, car_dir.y, car_dir.z));
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & PRESSED) {
			theta = car_rot_speed*0.001f * ddeltatime;
			rot = glm::rotate(rot, theta, glm::vec3(0, 1, 0));
			car_dir = (rot * glm::vec4(car_dir, 1));
			car_dir = glm::normalize(glm::vec3(car_dir.x, car_dir.y, car_dir.z));
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_UP) & PRESSED)
			car_pos += glm::normalize(car_dir) * (float) (ddeltatime * car_speed*0.001);
		if (inputHandler.GetKeycodeState(GLFW_KEY_DOWN) & PRESSED)
			car_pos -= glm::normalize(car_dir) * (float)(ddeltatime * car_speed*0.001);
		if (inputHandler.GetKeycodeState(GLFW_KEY_1) & PRESSED)
			chosen_cam = 1;
		if (inputHandler.GetKeycodeState(GLFW_KEY_2) & PRESSED) {
			chosen_cam = 2;
			glm::mat4 carT = car.get_transform();
			mCamera.mWorld.LookAt(glm::vec3(carT[3][0], carT[3][1], carT[3][2]), glm::vec3(0, 1, 0));
		}
		if (chosen_cam == 1) {
			if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED)
				wcam_car_dist = glm::clamp(wcam_car_dist - 5.0f, 50.0f, 600.0f);
			if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED)
				wcam_car_dist = glm::clamp(wcam_car_dist + 5.0f, 50.0f, 600.0f);
			if (inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED)
				wcam_y_angle -= 0.025f;
			if (inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED)
				wcam_y_angle += 0.025f;
			if (inputHandler.GetKeycodeState(GLFW_KEY_E) & PRESSED)
				wcam_height_angle = glm::clamp(wcam_height_angle + 0.025f, 0.25f, glm::half_pi<float>());
			if (inputHandler.GetKeycodeState(GLFW_KEY_Q) & PRESSED)
				wcam_height_angle = glm::clamp(wcam_height_angle - 0.025f, 0.25f, glm::half_pi<float>());
		}

		//
		// update car pos
		//
		// TODO vary car.y according to height map
		//car_pos.y = terrainHeight(image, car_pos.x, car_pos.z, width, height, ground_scale);
		car.set_translation(car_pos);
		car_rot.rotate_y(theta);

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		//
		// Set camera position
		//
		if (chosen_cam == 1) {
			float dx = wcam_car_dist * glm::sin(wcam_y_angle);
			float dy = wcam_car_dist * glm::sin(wcam_height_angle);
			float dz = wcam_car_dist * glm::cos(wcam_y_angle);
			world_cam.set_translation(glm::vec3(dx, dy, dz));
			glm::mat4 new_cam_pos = world.get_transform() * car.get_transform()
							* world_cam.get_transform();
			mCamera.mWorld.SetTranslate(glm::vec3(new_cam_pos[3][0] / new_cam_pos[3][3],
				new_cam_pos[3][1] / new_cam_pos[3][3],
				new_cam_pos[3][2] / new_cam_pos[3][3]));
			mCamera.mWorld.LookAt(car_pos, glm::vec3(0,1,0));
		}
		if (chosen_cam == 2) {
			glm::mat4 new_cam_pos = world.get_transform() * car.get_transform()
							* car_rot.get_transform() * car_cam.get_transform();
			mCamera.mWorld.SetTranslate(glm::vec3(new_cam_pos[3][0] / new_cam_pos[3][3],
				new_cam_pos[3][1] / new_cam_pos[3][3],
				new_cam_pos[3][2] / new_cam_pos[3][3]));
			mCamera.mWorld.RotateY(theta);
			}

		// update value with current position
		camera_position = mCamera.mWorld.GetTranslation();

		//
		// Todo: Render properly, not explicit for all nodes
		//
		if (!shader_reload_failed) {
			
			car_geometry.render(mCamera.GetWorldToClipMatrix(), car.get_transform() * car_rot.get_transform() * car_geometry.get_transform());
			ground.render(mCamera.GetWorldToClipMatrix(), ground.get_transform());

		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Set up ImGui window
		//
		bool const opened = ImGui::Begin("Frames per second", nullptr, ImVec2(400, 100), -1.0f, 0);
		if (opened) {
			std::string FC = std::to_string(1000.0/ddeltatime);
			ImGui::Text(FC.c_str());
			ImGui::Text(std::to_string(currFPS).c_str());
		}
		ImGui::End();
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
