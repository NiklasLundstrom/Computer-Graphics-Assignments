#include "assignment3.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
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

enum class polygon_mode_t : unsigned int {
	fill = 0u,
	line,
	point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

edaf80::Assignment3::Assignment3() :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(), window(nullptr)
{
	Log::View::Init();

	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateWindow("EDAF80: Assignment 3", window_datum, config::msaa_rate);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

edaf80::Assignment3::~Assignment3()
{
	Log::View::Destroy();
}

void
edaf80::Assignment3::run()
{
	// Load the sphere geometry
	//auto sphere = parametric_shapes::createSphere(4u, 60u, 1.0f);
	std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects("car.obj");
	bonobo::mesh_data const& teapot_shape = objects[1];
	if (teapot_shape.vao == 0u) {
		LogError("Failed to retrieve the teapot");
		return;
	}
	auto const sphere = parametric_shapes::createSphere(40, 40, 0.25);
	if (sphere.vao == 0u) {
		LogError("Failed to load sphere");
	}
	std::vector<bonobo::mesh_data> const objects2 = bonobo::loadObjects("box.obj");
	bonobo::mesh_data const& cube = objects2.front();
	if (cube.vao == 0u) {
		LogError("Failed to retrieve the cube");
		return;
	}
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;

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

	GLuint diffuse_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/diffuse.vert" },
	                                           { ShaderType::fragment, "EDAF80/diffuse.frag" } },
	                                         diffuse_shader);
	if (diffuse_shader == 0u)
		LogError("Failed to load diffuse shader");

	GLuint normal_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/normal.vert" },
	                                           { ShaderType::fragment, "EDAF80/normal.frag" } },
	                                         normal_shader);
	if (normal_shader == 0u)
		LogError("Failed to load normal shader");

	GLuint texcoord_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/texcoord.vert" },
	                                           { ShaderType::fragment, "EDAF80/texcoord.frag" } },
	                                         texcoord_shader);
	if (texcoord_shader == 0u)
		LogError("Failed to load texcoord shader");

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/phong.vert" },
											   { ShaderType::fragment, "EDAF80/phong.frag" } },
											phong_shader);
	if (phong_shader == 0u) {
		LogError("Failed to load phong shader");
	}
	GLuint phong_shader_w_texture = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/phong_w_texture.vert" },
											   { ShaderType::fragment, "EDAF80/phong_w_texture.frag" } },
		phong_shader_w_texture);
	if (phong_shader_w_texture == 0u) {
		LogError("Failed to load phong shader with texture");
	}
	GLuint cubemapping = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/cubemapping.vert" },
											   { ShaderType::fragment, "EDAF80/cubemapping.frag" } },
		cubemapping);
	if (cubemapping == 0u) {
		LogError("Failed to load cube mapping");
	}
	GLuint normal_mapping = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/normal_mapping.vert" },
											   { ShaderType::fragment, "EDAF80/normal_mapping.frag" } },
		normal_mapping);
	if (normal_mapping == 0u) {
		LogError("Failed to load normal mapping");
	}
	GLuint skybox = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/skybox.vert" },
											   { ShaderType::fragment, "EDAF80/skybox.frag" } },
		skybox);
	if (skybox == 0u) {
		LogError("Failed to load skybox");
	}
	GLuint reflective = 0u;
	program_manager.CreateAndRegisterProgram({ { ShaderType::vertex, "EDAF80/reflective.vert" },
											   { ShaderType::fragment, "EDAF80/reflective.frag" } },
		reflective);
	if (reflective == 0u) {
		LogError("Failed to load reflective");
	}

	auto light_position = glm::vec3(-32.0f, 64.0f, 32.0f);

	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.2f, 0.2f, 0.2f);
	auto diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 1.0f;
	auto const phong_set_uniforms = [&light_position,&camera_position,&ambient,&diffuse,&specular,&shininess](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
	};

	auto polygon_mode = polygon_mode_t::fill;

	auto teapot = Node();
	//teapot.set_geometry(sphere);
	//teapot.scale(glm::vec3(100.0,100.0,100.0));
	teapot.set_geometry(teapot_shape);
	teapot.set_program(&fallback_shader, set_uniforms);
//	glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, width, height, 0, size, pixels);

	// texture
	GLuint const image_texture = bonobo::loadTexture2D("car_out_d.png");
	teapot.add_texture("diffuse_texture", image_texture, GL_TEXTURE_2D);

	// normal mapping
	GLuint const normal_texture = bonobo::loadTexture2D("holes_bump.png");
	teapot.add_texture("normal_texture", normal_texture, GL_TEXTURE_2D);

	// cubemapping
	auto my_cube_map_id = bonobo::loadTextureCubeMap("sunset_sky/posx.png", "sunset_sky/negx.png",
		"sunset_sky/posy.png", "sunset_sky/negy.png",
		"sunset_sky/posz.png", "sunset_sky/negz.png", true);
	teapot.add_texture("my_cube_map", my_cube_map_id, GL_TEXTURE_CUBE_MAP);

	// specular map
	GLuint const reflect_texture = bonobo::loadTexture2D("TerrainRock_0016_Specular.png");
	teapot.add_texture("reflect_texture", reflect_texture, GL_TEXTURE_2D);


	auto sky = Node();
	sky.set_geometry(sphere);
	sky.scale(glm::vec3(1000.0, 1000.0, 1000.0));
	sky.set_program(&skybox, set_uniforms);
	sky.add_texture("my_cube_map", my_cube_map_id, GL_TEXTURE_CUBE_MAP);


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

		ImGui_ImplGlfwGL3_NewFrame();

		if (inputHandler.GetKeycodeState(GLFW_KEY_1) & JUST_PRESSED) {
			teapot.set_program(&fallback_shader, set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED) {
			teapot.set_program(&diffuse_shader, set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED) {
			teapot.set_program(&normal_shader, set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED) {
			teapot.set_program(&texcoord_shader, set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_5) & JUST_PRESSED) {
			teapot.set_program(&phong_shader, phong_set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_6) & JUST_PRESSED) {
			teapot.set_program(&phong_shader_w_texture, phong_set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_7) & JUST_PRESSED) {
			teapot.set_program(&cubemapping, phong_set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_8) & JUST_PRESSED) {
			teapot.set_program(&normal_mapping, phong_set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_9) & JUST_PRESSED) {
			teapot.set_program(&reflective, phong_set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
			polygon_mode = get_next_mode(polygon_mode);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
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

		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		teapot.render(mCamera.GetWorldToClipMatrix(), teapot.get_transform());
		sky.render(mCamera.GetWorldToClipMatrix(), sky.get_transform());

		bool opened = ImGui::Begin("Scene Control", &opened, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient));
			ImGui::ColorEdit3("Diffuse", glm::value_ptr(diffuse));
			ImGui::ColorEdit3("Specular", glm::value_ptr(specular));
			ImGui::SliderFloat("Shininess", &shininess, 0.0f, 500.0f);
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text("%.3f ms", ddeltatime);
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
		edaf80::Assignment3 assignment3;
		assignment3.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
