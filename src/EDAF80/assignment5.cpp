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

float terrainHeight(std::vector<unsigned char> image, float x, float z, int width, int height, float scale) {
	if (x > scale || x < -scale || z > scale || z < -scale)
		return 0u;
	//if (x < -50)
	//	int bp = 0;
	 //TODO interpolate between pixels

	float delta = 4.0f;
	int u_1 = glm::clamp( ((-(x-delta) / scale) + 1)/2, 0.0f, 1.0f )*(width - 1);
	int u = ((-(x) / scale) + 1)*(width - 1) / 2;
	int u1 = glm::clamp( ((-(x + delta) / scale) + 1) / 2, 0.0f, 1.0f)*(width - 1);
	int v_1 = glm::clamp( (((z-delta) / scale) + 1) / 2, 0.0f, 1.0f)*(height - 1);
	int v = ((z / scale) + 1)*(height - 1) / 2;
	int v1 = glm::clamp( (((z+delta) / scale) + 1) / 2, 0.0f, 1.0f)*(height - 1);

	//int v = ((x / scale) + 1)*(height - 1) / 2;
	//int u = ((z / scale) + 1)*(width - 1) / 2;

	//int p_0_0 = image.at(u + v*height);

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
	
	return ((float) p_0_0)/255.0f;// image.at(height*u + v)*offset / 255;
}

void
edaf80::Assignment5::run()
{
	// Set scale of world
	float ground_scale = 1000.0f;
	float world_scale = ground_scale / 1000.0f;
	//
	// Set up the camera
	//
	mCamera.mWorld.SetTranslate(glm::vec3(100.0f, 200.0f, 200.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f;
	mCamera.SetProjection(mCamera.GetFov(), mCamera.GetAspect(), 1.0f, 2000.0f*world_scale);

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
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/phong_w_texture.vert" },
											   { ShaderType::fragment, "EDAF80/phong_w_texture.frag" } },
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
	std::string landscape_filename = "test_orient2.png";
	u32 width_landscape, height_landscape;
	auto const landscape_path = config::resources_path("textures/" + landscape_filename);
	std::vector<unsigned char> landscape;
	if (lodepng::decode(landscape, width_landscape, height_landscape, landscape_path, LCT_GREY) != 0) {
		LogWarning("Couldn't load or decode image file %s", landscape_path.c_str());
		return;
	}

	//
	// load road alpha
	//
	std::string road_filename = "road_alpha_2.png";
	u32 width_road, height_road;
	auto const road_path = config::resources_path("textures/" + road_filename);
	std::vector<unsigned char> road;
	if (lodepng::decode(road, width_road, height_road, road_path, LCT_GREY) != 0) {
		LogWarning("Couldn't load or decode image file %s", road_path.c_str());
		return;
	}

	//printf("width: %d, height: %d\nsize: %d\n", width, height, image.size());
	//printf("0,0: %f\n", terrainHeight(image, 0, 0, width, height, ground_scale));

	//
	// set up uniform variables
	//
	auto light_position = glm::vec3(-100.0f, 200.0f, 30.0f)*world_scale;
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.2f, 0.1f, 0.1f);
	auto diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 15.0f;
	auto const phong_set_uniforms = [&light_position, &camera_position, &ambient, &diffuse, &specular, &shininess, &ground_scale](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
		glUniform1f(glGetUniformLocation(program, "ground_scale"), ground_scale);
	};
	
	//
	// Load the geometries
	//

		// teapot
	std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects("car.obj");
	if (objects.empty()) {
		LogError("Failed to load the box geometry: exiting.");
		Log::View::Destroy();
		Log::Destroy();
		return;
	}
	bonobo::mesh_data const& car_exterior = objects[0];
	bonobo::mesh_data const& car_interior = objects[1];
	bonobo::mesh_data const& car_glass = objects[2];

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
				Node car_ext = Node();
					car_rot.add_child(&car_ext);
				Node car_int = Node();
					car_rot.add_child(&car_int);
				Node car_gl = Node();
					car_rot.add_child(&car_gl);
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
	int ground_height =	terrainHeight(landscape, 0, 0, width_landscape, height_landscape, ground_scale);
	glm::vec3 car_pos = glm::vec3(0, ground_scale*ground_height, 0)*world_scale; // TODO vary car_pos.y according to height map
	car.set_translation(car_pos);
	float car_speed = 0.0f;


	// car rotation
	glm::vec3 car_dir = glm::vec3(0, 0, -1);
	float car_rot_speed = glm::pi<float>();

	// car geometry
	// car exterior
	car_ext.set_geometry(car_exterior);
	car_ext.set_scaling(glm::vec3(10, 10, 10));
	car_ext.set_rotation_y(glm::pi<float>());
	car_ext.set_translation(glm::vec3(0, 9, 0)); // TODO can we get the objects size?
	car_ext.set_program(&phong_shader, phong_set_uniforms);
	GLuint const car_ext_texture = bonobo::loadTexture2D("car_out_d.png");
	car_ext.add_texture("diffuse_texture", car_ext_texture, GL_TEXTURE_2D);

	// car interior
	car_int.set_geometry(car_interior);
	car_int.set_scaling(glm::vec3(10, 10, 10));
	car_int.set_rotation_y(glm::pi<float>());
	car_int.set_translation(glm::vec3(0, 9, 0)); // TODO can we get the objects size?
	car_int.set_program(&phong_shader, phong_set_uniforms);
	GLuint const car_in_texture = bonobo::loadTexture2D("car_in_d.png");
	car_int.add_texture("diffuse_texture", car_in_texture, GL_TEXTURE_2D);

	// car glass
	car_gl.set_geometry(car_glass);
	car_gl.set_scaling(glm::vec3(10, 10, 10));
	car_gl.set_rotation_y(glm::pi<float>());
	car_gl.set_translation(glm::vec3(0, 9, 0)); // TODO can we get the objects size?
	car_gl.set_program(&phong_shader, phong_set_uniforms);
	car_gl.add_texture("diffuse_texture", car_ext_texture, GL_TEXTURE_2D);


	GLuint const height_map = bonobo::loadTexture2D(landscape_filename);
	
	// car camera ("first person"), chosen camera: 2
	car_cam.set_translation(glm::vec3(0, 50, 150));

	// world camera ("third person"), chosen camera: 1
	world_cam.set_translation(glm::vec3(0, 50, 100)*world_scale);
	float wcam_car_dist = 300.0*world_scale;
	float wcam_height_angle = glm::pi<float>()/3;
	float wcam_y_angle = 0.0;

	// ground
	ground.set_geometry(quad);
	ground.set_scaling(glm::vec3(1, 1, 1)*ground_scale);
	ground.set_translation(glm::vec3(0, 0, 0));
	ground.set_program(&terrain_shader, phong_set_uniforms);
	ground.add_texture("height_map", height_map, GL_TEXTURE_2D);
	GLuint const ground_diffuse = bonobo::loadTexture2D("checkers.png");
	ground.add_texture("diffuse_tex", ground_diffuse, GL_TEXTURE_2D);
	GLuint const road_alpha = bonobo::loadTexture2D(road_filename);
	ground.add_texture("road_alpha", road_alpha, GL_TEXTURE_2D);


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

	bool show_logs = false;
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
		glm::mat4 rot = glm::mat4(1.0f);
		float theta = 0.0;
		if (inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & PRESSED) {
			theta = -car_rot_speed * 0.001f * ddeltatime;
			rot = glm::rotate(rot, theta, glm::vec3(0, 1, 0));
			car_dir = (rot * glm::vec4(car_dir, 1));
			car_dir = glm::normalize(glm::vec3(car_dir.x, car_dir.y, car_dir.z));
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & PRESSED) {
			theta = car_rot_speed * 0.001f * ddeltatime;
			rot = glm::rotate(rot, theta, glm::vec3(0, 1, 0));
			car_dir = (rot * glm::vec4(car_dir, 1));
			car_dir = glm::normalize(glm::vec3(car_dir.x, car_dir.y, car_dir.z));
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_UP) & PRESSED)
			car_speed = glm::clamp(car_speed + 400.0f*(float)ddeltatime*0.001f, -800.0f, 800.0f);
		if (inputHandler.GetKeycodeState(GLFW_KEY_DOWN) & PRESSED)
			car_speed = glm::clamp(car_speed - 400.0f*(float)ddeltatime*0.001f, -800.0f, 800.0f);
		if (inputHandler.GetKeycodeState(GLFW_KEY_1) & PRESSED)
			chosen_cam = 1;
		if (inputHandler.GetKeycodeState(GLFW_KEY_2) & PRESSED) {
			chosen_cam = 2;
			glm::mat4 carT = car.get_transform();
			mCamera.mWorld.LookAt(glm::vec3(carT[3][0], carT[3][1], carT[3][2]), glm::vec3(0, 1, 0));
		}
		if (chosen_cam == 1) {
			if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED)
				wcam_car_dist = glm::clamp(wcam_car_dist - 5.0f*world_scale, 50.0f*world_scale, 600.0f*world_scale);
			if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED)
				wcam_car_dist = glm::clamp(wcam_car_dist + 5.0f*world_scale, 50.0f*world_scale, 600.0f*world_scale);
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
		// friction
		car_speed = glm::sign(car_speed)
			* glm::max(glm::abs(car_speed) - 200.0f*(float)ddeltatime*0.001f, 0.0f);
		// outside road
		float on_road = ((float)terrainHeight(road, car_pos.x, car_pos.z, width_road, height_road, ground_scale));
		if (0.5 > on_road) {
			if (glm::abs(car_speed) > 4000.0f*(float)ddeltatime*0.001f) {
				car_speed = glm::sign(car_speed) * (glm::abs(car_speed) - 500.0f*(float)ddeltatime*0.001f);
			}
		}
		// move car_pos
		car_pos += glm::normalize(car_dir) * (float)(ddeltatime * car_speed*0.001);
		car_pos.y = terrainHeight(landscape, car_pos.x, car_pos.z, width_landscape, height_landscape, ground_scale)*ground_scale;
		// update car
		car.set_translation(car_pos);
		car_rot.rotate_y(theta);

		// print car pos
		//printf("\033c");
		//printf("node center:\n[%f, %f, %f]\n \n", car_pos.x, car_pos.y, car_pos.z);

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
			//mCamera.mWorld.RotateY(theta);
			mCamera.mWorld.LookAt(car_pos + glm::vec3(0,40,0), glm::vec3(0, 1, 0));
			}

		// update value with current position
		camera_position = mCamera.mWorld.GetTranslation();


		//
		// Todo: Render properly, not explicit for all nodes
		//
		if (!shader_reload_failed) {
			
			car_ext.render(mCamera.GetWorldToClipMatrix(), car.get_transform() * car_rot.get_transform() * car_ext.get_transform());
			car_int.render(mCamera.GetWorldToClipMatrix(), car.get_transform() * car_rot.get_transform() * car_int.get_transform());
			car_gl.render(mCamera.GetWorldToClipMatrix(), car.get_transform() * car_rot.get_transform() * car_gl.get_transform());
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
