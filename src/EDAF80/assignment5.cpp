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
using namespace std;


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

float terrainHeight(std::vector<unsigned char> *image, float x, float z, int width, int height, float scale) {
	if (x > scale || x < -scale || z > scale || z < -scale)
		return 0u;
	//if (x < -50)
	//	int bp = 0;
	 //TODO interpolate between pixels

	float delta = 10.0f;
	int u_1 = glm::clamp( ((-(x-delta) / scale) + 1)/2, 0.0f, 1.0f )*(width - 1);
	int u = ((-(x) / scale) + 1) *(height - 1) / 2;
	int u1 = glm::clamp( ((-(x + delta) / scale) + 1) / 2, 0.0f, 1.0f)*(width - 1);
	int v_1 = glm::clamp( (((z-delta) / scale) + 1) / 2, 0.0f, 1.0f)*(height - 1);
	int v = ((z / scale) + 1)*(width - 1) / 2;
	int v1 = glm::clamp( (((z+delta) / scale) + 1) / 2, 0.0f, 1.0f)*(height - 1);

	//int v = ((x / scale) + 1)*(height - 1) / 2;
	//int u = ((z / scale) + 1)*(width - 1) / 2;

	//int p_0_0 = image.at(u + v*height);
	//float v = (z/scale + 1)/2; 
	//float u = (-x/scale + 1)/2;
	//int p_0_0 = image.at((height - 1)*u*width + (width - 1)*v);
	//printf("\033c");
	//printf("index:, %f, v:, %f, u:, %f, p_0_0:, %d", (height - 1)*u*width + (width - 1)*v, v, u, p_0_0);

	//int offset = 100;
	int p_1_1 = image->at(height*u_1 + v_1);
	int p_1_0 = image->at(height*u_1 + v);
	int p_11 = image->at(height*u_1 + v1);
	int p_0_1 = image->at(height*u + v_1);
	int p_0_0 = image->at(u*width + v);
	int p_01 = image->at(height*u + v1);
	int p1_1 = image->at(height*u1 + v_1);
	int p1_0 = image->at(height*u1 + v);
	int p11 = image->at(height*u1 + v1);

	int mean = (p_1_1 + p_1_0 + p_11 +
				p_0_1 + p_0_0 + p_01 +
				p1_1 + p1_0 + p11) / 9;
	
	return ((float) mean)/255.0f;// image.at(height*u + v)*offset / 255;
}

glm::vec3 normalDir(std::vector<glm::vec3>* image, float x, float z, int width, int height, float scale) {
	if (x > scale || x < -scale || z > scale || z < -scale) {
		return glm::vec3(0.0f,0.0f,0.0f);
	}

	//int u = ((-(x) / scale) + 1) *(height - 1) / 2;
	//int v = ((z / scale) + 1)*(width - 1) / 2;
	float delta = 30.0f;
	int u_1 = glm::clamp(((-(x - delta) / scale) + 1) / 2, 0.0f, 1.0f)*(width - 1);
	int u = ((-(x) / scale) + 1) *(height - 1) / 2;
	int u1 = glm::clamp(((-(x + delta) / scale) + 1) / 2, 0.0f, 1.0f)*(width - 1);
	int v_1 = glm::clamp((((z - delta) / scale) + 1) / 2, 0.0f, 1.0f)*(height - 1);
	int v = ((z / scale) + 1)*(width - 1) / 2;
	int v1 = glm::clamp((((z + delta) / scale) + 1) / 2, 0.0f, 1.0f)*(height - 1);

	glm::vec3 p_1_1 = image->at(height*u_1 + v_1);
	glm::vec3 p_1_0 = image->at(height*u_1 + v);
	glm::vec3 p_11 = image->at(height*u_1 + v1);
	glm::vec3 p_0_1 = image->at(height*u + v_1);
	glm::vec3 p_0_0 = image->at(u*width + v);
	glm::vec3 p_01 = image->at(height*u + v1);
	glm::vec3 p1_1 = image->at(height*u1 + v_1);
	glm::vec3 p1_0 = image->at(height*u1 + v);
	glm::vec3 p11 = image->at(height*u1 + v1);

	glm::vec3 mean = (p_1_1 + p_1_0 + p_11 +
		p_0_1 + p_0_0 + p_01 +
		p1_1 + p1_0 + p11) / 9.0f;

	
	auto returnValue = image->at(u*width + v);
	return returnValue;
}

glm::vec3 normalDirTest(std::vector<unsigned char> image, float x, float z, int width, int height, float scale) {
	if (x > scale || x < -scale || z > scale || z < -scale) {
		return glm::vec3(0.0f, 0.0f, 0.0f);
	}

	int u = ((-(x) / scale) + 1) *(height - 1) / 2;
	int v = ((z / scale) + 1)*(width - 1) / 2;

	int r = image.at(u*width + v);
	int g = image.at(u*width + v+1);
	int b = image.at(u*width + v+2);

	return glm::vec3(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);

}

void
edaf80::Assignment5::run()
{
	// Set scale of world
	float ground_scale = 4000.0f;
	float world_scale = ground_scale / 1000.0f;
	//
	// Set up the camera
	//
	mCamera.mWorld.SetTranslate(glm::vec3(100.0f, 200.0f, 200.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f;
	mCamera.SetProjection(mCamera.GetFov(), mCamera.GetAspect(), 25.0f*world_scale, 700.0f*world_scale);

	int chosen_cam = 2;

	
	//
	// Create the shader programs
	//
	ShaderProgramManager program_manager;

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

	GLuint car_window_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/car_window.vert" },
											   { ShaderType::fragment, "EDAF80/car_window.frag" } },
		car_window_shader);
	if (car_window_shader == 0u) {
		LogError("Failed to load car window shader");
	}

	GLuint goal_ball_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/goal_ball.vert" },
											   { ShaderType::fragment, "EDAF80/goal_ball.frag" } },
		goal_ball_shader);
	if (goal_ball_shader == 0u) {
		LogError("Failed to load goal ball shader");
	}

	//
	// set up height
	//
	//float ground_height = 0.0;
	//
	// load height map
	//
	std::string landscape_filename = "terrain/terrain_heightmap.png";
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
	std::string road_filename = "terrain/road_alpha.png";
	u32 width_road, height_road;
	auto const road_path = config::resources_path("textures/" + road_filename);
	std::vector<unsigned char> road;
	if (lodepng::decode(road, width_road, height_road, road_path, LCT_GREY) != 0) {
		LogWarning("Couldn't load or decode image file %s", road_path.c_str());
		return;
	}

	//
	// load normal map
	//
	std::string normal_filename = "terrain/terrain_normal.png";
	u32 width_normal, height_normal;
	auto const normal_path = config::resources_path("textures/" + normal_filename);
	std::vector<unsigned char> normal;
	if (lodepng::decode(normal, width_normal, height_normal, normal_path, LCT_RGB) != 0) {
		LogWarning("Couldn't load or decode image file %s", normal_path.c_str());
		return;
	}

	// convert normal vector to vector where each element is an RGB vector
	int rows = width_normal * height_normal;
	int cols = 3;
	int tracker = 0;
	std::vector< glm::vec3 > normalVec(rows, glm::vec3(cols));
	for (int i = 0; i < width_normal * height_normal; i++) {
		normalVec[i][0] = normal[tracker++]; 
		normalVec[i][1] = normal[tracker++];
		normalVec[i][2] = normal[tracker++];
		/*printf("v[i][0]: %f, v[i][1]: %f, v[i][2]: %f, tracker: %d\n", v[i][0], v[i][1], v[i][2], tracker);
		system("PAUSE");*/
	}


	                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
	//printf("width: %d, height: %d\nsize: %d\n", width_normal, height_normal, normal.size());

    //glm::vec3 vec = normalDir(v, 3900,3900, width_normal, height_normal, ground_scale);
	//printf("vec[0]: %f, vec[1]: %f, vec[2]: %f", vec[0], vec[1], vec[2]);

	//
	// set up uniform variables
	//
	auto light_position = glm::vec3(1000.0f, 1000.0f, 1000.0f)*world_scale;
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.2f, 0.1f, 0.1f);
	auto diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 15.0f;
	auto y_scale = 0.37f;
	auto time = 0.0f;
	glm::vec3 sky_color = glm::vec3(0.5, 0.5, 0.5);
	auto const phong_set_uniforms = [&light_position, &camera_position, &ambient, &diffuse, &specular, &shininess, &ground_scale, &y_scale, &time, &sky_color](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
		glUniform1f(glGetUniformLocation(program, "ground_scale"), ground_scale);
		glUniform1f(glGetUniformLocation(program, "y_scale"), y_scale);
		glUniform1f(glGetUniformLocation(program, "time"), time);
		glUniform3fv(glGetUniformLocation(program, "sky_color"), 1, glm::value_ptr(sky_color));
	};
	
	//
	// Load the geometries
	//

	// car
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
	auto const quad = parametric_shapes::createQuad(800, 800);

	if (quad.vao == 0u) {
		LogError("Failed to load quad");
	}

	// sphere
	auto const sphere = parametric_shapes::createSphere(50, 50, 1);
	if (quad.vao == 0u) {
		LogError("Failed to load sphere");
	}

	//
	// Set up node tree
	//
	Node world = Node();
		Node car = Node();
			world.add_child(&car);
			Node car_rot = Node();
				car.add_child(&car_rot);
				Node car_rot_XZ = Node();
					car_rot.add_child(&car_rot_XZ);
					Node car_ext = Node();
						car_rot_XZ.add_child(&car_ext);
					Node car_int = Node();
						car_rot_XZ.add_child(&car_int);
					Node car_gl = Node();
						car_rot_XZ.add_child(&car_gl);
					Node car_cam = Node();
						car_rot_XZ.add_child(&car_cam);
			Node world_cam = Node();
				car.add_child(&world_cam);
		Node ground = Node();
			world.add_child(&ground);
		Node goal_sphere = Node();
			world.add_child(&goal_sphere);
		

	//
	// set up nodes
	//

	// car
	glm::vec2 start_pos_xz = glm::vec2(-520, -265)*world_scale;
	float ground_height = y_scale*terrainHeight(&landscape, start_pos_xz.x, start_pos_xz.y, width_landscape, height_landscape, ground_scale);
	glm::vec3 start_pos = glm::vec3(start_pos_xz.x, ground_scale*ground_height, start_pos_xz.y);
	glm::vec3 car_pos = start_pos; // TODO vary car_pos.y according to height map
	car.set_translation(car_pos);
	float car_speed = 0.0f;
	bool finished_race = false;
	bool has_passed_keypoint = false;


	// car rotation
	glm::vec3 car_dir = glm::vec3(0, 0, -1);
	float car_rot_speed = glm::pi<float>();


	// car geometry
	// car exterior
	car_ext.set_geometry(car_exterior);
	car_ext.set_scaling(glm::vec3(10, 10, 10));
	car_ext.set_rotation_y(glm::pi<float>());
	car_ext.set_translation(glm::vec3(0, 2, 0));
	car_ext.set_program(&car_shader, phong_set_uniforms);
	GLuint const car_ext_texture = bonobo::loadTexture2D("car_out_d.png");
	car_ext.add_texture("diffuse_texture", car_ext_texture, GL_TEXTURE_2D);

	// car interior
	car_int.set_geometry(car_interior);
	car_int.set_scaling(glm::vec3(10, 10, 10));
	car_int.set_rotation_y(glm::pi<float>());
	car_int.set_translation(glm::vec3(0, 2, 0)); 
	car_int.set_program(&car_shader, phong_set_uniforms);
	GLuint const car_in_texture = bonobo::loadTexture2D("car_in_d.png");
	car_int.add_texture("diffuse_texture", car_in_texture, GL_TEXTURE_2D);

	// car glass
	car_gl.set_geometry(car_glass);
	car_gl.set_scaling(glm::vec3(10, 10, 10));
	car_gl.set_rotation_y(glm::pi<float>());
	car_gl.set_translation(glm::vec3(0, 2, 0)); 
	car_gl.set_program(&car_window_shader, phong_set_uniforms);
	car_gl.add_texture("diffuse_texture", car_ext_texture, GL_TEXTURE_2D);


	GLuint const height_map = bonobo::loadTexture2D(landscape_filename);
	
	// car camera ("first person"), chosen camera: 2
	car_cam.set_translation(glm::vec3(0, 100, 250));

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
			// heightmap
	ground.add_texture("height_map", height_map, GL_TEXTURE_2D);
			// diffuse rock
	GLuint const ground_diffuse = bonobo::loadTexture2D("terrain/terrain_color_hr.png");
	ground.add_texture("diffuse_tex", ground_diffuse, GL_TEXTURE_2D);
			// road alpha
	//GLuint const road_alpha = bonobo::loadTexture2D(road_filename);
	//ground.add_texture("road_alpha", road_alpha, GL_TEXTURE_2D);
	//GLuint const ground_normal = bonobo::loadTexture2D("terrain/terrain_normal.png");
	//ground.add_texture("normal_map", ground_normal, GL_TEXTURE_2D);
			// normal map
	GLuint const ground_normal_hr = bonobo::loadTexture2D("terrain/terrain_normal_hr.png");
	ground.add_texture("normal_map_hr", ground_normal_hr, GL_TEXTURE_2D);
			// water alpha
	GLuint const water_alpha = bonobo::loadTexture2D("terrain/water_alpha_hr.png");
	ground.add_texture("water_alpha", water_alpha, GL_TEXTURE_2D);
			// cube map
	std::string scene = "snow/";
	auto my_cube_map_id = bonobo::loadTextureCubeMap(scene + "posx.png", scene + "negx.png",
		scene + "posy.png", scene + "negy.png",
		scene + "posz.png", scene + "negz.png", true);
	ground.add_texture("my_cube_map", my_cube_map_id, GL_TEXTURE_CUBE_MAP);
			// water normal waves
	GLuint const water_normal_texture = bonobo::loadTexture2D("waves2.png");
	ground.add_texture("water_normal", water_normal_texture, GL_TEXTURE_2D);
			// grass texture
	GLuint const grass_texture = bonobo::loadTexture2D("grass.png");
	ground.add_texture("grass_texture", grass_texture, GL_TEXTURE_2D);

	// goal sphere
	goal_sphere.set_geometry(sphere);
	glm::vec3 goal_pos = glm::vec3(-520*world_scale, ground_scale*ground_height, -227*world_scale);
	float goal_radius = 75 * world_scale;
	goal_sphere.set_scaling(glm::vec3(1.0)*goal_radius);
	goal_sphere.set_translation(goal_pos);
	goal_sphere.set_program(&goal_ball_shader, phong_set_uniforms);

	// sky
	glEnable(GL_DEPTH_TEST);
	

	// Enable face culling to improve performance:
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	glCullFace(GL_BACK);
	//glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	int currFPS = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;
	double startTime = GetTimeMilliseconds();
	double finishTime = 0.0;

	bool has_started = false;

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
		if (!has_started && inputHandler.GetKeycodeState(GLFW_KEY_UP) & PRESSED) {
			has_started = true;
			startTime = nowTime;
		}

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
		if (has_started && inputHandler.GetKeycodeState(GLFW_KEY_UP) & PRESSED)
			car_speed = glm::clamp(car_speed + 400.0f*(float)ddeltatime*0.001f, -800.0f, 800.0f);
		if (has_started && inputHandler.GetKeycodeState(GLFW_KEY_DOWN) & PRESSED)
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
		// 
		// friction
		car_speed = glm::sign(car_speed)
			* glm::max(glm::abs(car_speed) - 200.0f*(float)ddeltatime*0.001f, 0.0f);
		// outside road
		float on_road = ((float)terrainHeight(&road, car_pos.x, car_pos.z, width_road, height_road, ground_scale));
		if (0.5 > on_road) {
			if (glm::abs(car_speed) > 400.0f) {
				car_speed = glm::sign(car_speed) * (glm::abs(car_speed) - 750.0f*(float)ddeltatime*0.001f);
			}else if (glm::abs(car_speed) > 4000.0f*(float)ddeltatime*0.001f) {
				car_speed = glm::sign(car_speed) * (glm::abs(car_speed) - 500.0f*(float)ddeltatime*0.001f);
			}
		}
		// move car_pos
		car_pos += glm::normalize(car_dir) * (float)(ddeltatime * car_speed*0.001);
		car_pos.y = y_scale*terrainHeight(&landscape, car_pos.x, car_pos.z, width_landscape, height_landscape, ground_scale)*ground_scale;
		// rotate car
		glm::vec3 n = normalDir(&normalVec, car_pos.x, car_pos.z, width_landscape, height_landscape, ground_scale);
		n = 2.0f * n/255.0f - 1.0f;
		n = glm::normalize(glm::vec3(n.g, n.b, n.r));
		float cosPhi = 1/glm::length(glm::vec3(car_dir.x, -glm::dot(n, car_dir) / n.y, car_dir.z));
		float phi = glm::acos(cosPhi);
		float rotSign = glm::sign(-glm::dot(n, car_dir) / n.y);
		car_rot_XZ.set_rotation_x(rotSign*phi);

		// update car
		car.set_translation(car_pos);
		car_rot.rotate_y(theta);
		// has passed keypoint
		if (car_pos.x > 197 * world_scale) {
			has_passed_keypoint = true;
		}
		// finished race
		if (!finished_race && has_passed_keypoint && glm::distance(car_pos, goal_pos) < goal_radius) {
			finished_race = true;
			finishTime = nowTime - startTime;
		}

		// print car pos
		//printf("\033c");
		//printf("node center:\n[%f, %f, %f]\n \n", car_pos.x, car_pos.y, car_pos.z);
		//printf("")

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(sky_color.r, sky_color.g, sky_color.b, 1.0f);
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
			
			car_ext.render(mCamera.GetWorldToClipMatrix(), car.get_transform() * car_rot.get_transform() * car_rot_XZ.get_transform() * car_ext.get_transform());
			car_int.render(mCamera.GetWorldToClipMatrix(), car.get_transform() * car_rot.get_transform() * car_rot_XZ.get_transform() * car_int.get_transform());
			car_gl.render(mCamera.GetWorldToClipMatrix(), car.get_transform() * car_rot.get_transform()  * car_rot_XZ.get_transform() * car_gl.get_transform());
			ground.render(mCamera.GetWorldToClipMatrix(), ground.get_transform());
			goal_sphere.render(mCamera.GetWorldToClipMatrix(), goal_sphere.get_transform());
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
		bool opened2 = ImGui::Begin("Scene Control", &opened2, ImVec2(500, 50), -5.0f, 0);
		if (opened2) {
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -8000.0f, 8000.0f);
			ImGui::SliderFloat("y_scale", &y_scale, 0.0f, 2.0f);
		}
		bool opened3 = ImGui::Begin("Info", &opened3, ImVec2(200, 100), -2.0f, 0);
		if (opened3) {
			ImGui::Text(has_started ? ("Elapsed time: " + std::to_string((nowTime - startTime)/1000.0) + " s").c_str() : "Press UP key to start!");
			ImGui::Text((has_passed_keypoint ? "OK" : "not passed keypoint yet"));
			ImGui::Text(("Car speed: " + std::to_string(car_speed)).c_str());
			std::string ifFinished = "FINISHED! time: " + std::to_string(finishTime / 1000.0) + " s";
			ImGui::Text((finished_race ? ifFinished.c_str() : ""));
		}
		bool opened4 = ImGui::Begin("Car position", &opened4, ImVec2(200, 100), -2.0f, 0);
		if (opened3) {
			ImGui::Text((std::to_string(car_pos.x) + ", " + std::to_string(car_pos.y) + ", " + std::to_string(car_pos.z) ).c_str());
		}
		ImGui::End();
		if (show_logs)
			Log::View::Render();
		if (show_gui)
			ImGui::Render();

		glfwSwapBuffers(window);
		lastTime = nowTime;
		time += 60.0f*(float)ddeltatime*0.001f;
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
