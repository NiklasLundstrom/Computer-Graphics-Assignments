#include "assignment2.hpp"
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
#include "external/imgui_impl_glfw_gl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

edaf80::Assignment2::Assignment2() :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(), window(nullptr)
{
	Log::View::Init();

	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateWindow("EDAF80: Assignment 2", window_datum, config::msaa_rate);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

edaf80::Assignment2::~Assignment2()
{
	Log::View::Destroy();
}

void
edaf80::Assignment2::run()
{
	// Load the sphere geometry
	//auto const shape = parametric_shapes::createCircleRing(4u, 60u, 1.0f, 2.0f);
	auto const sphere = parametric_shapes::createSphere(40, 40, 0.25);
	auto const torus = parametric_shapes::createTorus(40, 40, 1, 0.1);

	if (torus.vao == 0u || sphere.vao == 0u)
		return;

	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f * 12.0f;

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

	auto const light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	// Set the default tensions value; it can always be changed at runtime
	// through the "Scene Controls" window.
	float catmull_rom_tension = 0.0f;

	// Set whether the default interpolation algorithm should be the linear one;
	// it can always be changed at runtime through the "Scene Controls" window.
	bool use_linear = true;

	/*auto circle_rings = Node();
	circle_rings.set_geometry(shape);
	circle_rings.set_program(&fallback_shader, set_uniforms);
	*/

	auto polygon_mode = polygon_mode_t::fill;

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	glCullFace(GL_BACK);


	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;

	bool show_logs = true;
	bool show_gui = true;
	
	float x = 0.0f;
	float dx = 0.01f;

	// create key points
	glm::vec3 p0 = glm::vec3(-2, 2, 0);
	glm::vec3 p1 = glm::vec3(0, 0, 4);
	glm::vec3 p2 = glm::vec3(4, 4, 0);
	glm::vec3 p3 = glm::vec3(4, -2, -2);
	glm::vec3 p4 = glm::vec3(-2, -6, 0);
	glm::vec3 p5 = glm::vec3(-6, -4, -2);
	glm::vec3 p6 = glm::vec3(-9, -3, -5);
	glm::vec3 p7 = glm::vec3(-11, -3, -9);
	glm::vec3 p8 = glm::vec3(-9, -3, -15);
	glm::vec3 p9 = glm::vec3(-4, 0, -8);
	glm::vec3 p10 = glm::vec3(-3, 3, 2);

	std::vector<glm::vec3> points = std::vector<glm::vec3>(11);
	points[0] = p0;
	points[1] = p1;
	points[2] = p2;
	points[3] = p3;
	points[4] = p4;
	points[5] = p5;
	points[6] = p6;
	points[7] = p7;
	points[8] = p8;
	points[9] = p9;
	points[10] = p10;

	int nbrP = points.size();
	int index_prev = nbrP - 1;
	int index_current = 0;
	int index_next = index_current + 1;
	int index_next_next = index_next + 1;

	//Create toruses
	std::vector<Node> toruses = std::vector<Node>(nbrP);
	for (int i = 0; i < nbrP; i++) {
		toruses[i] = Node();
		toruses[i].set_geometry(torus);
		toruses[i].set_program(&fallback_shader, set_uniforms);
		toruses[i].set_translation(points[i]);
	}

	// Create ball
	Node ball = Node();
	ball.set_geometry(sphere);
	ball.set_program(&fallback_shader, set_uniforms);
	ball.set_translation(points[0]);


	while (!glfwWindowShouldClose(window)) {
		nowTime = GetTimeSeconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1.0;
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

		ImGui_ImplGlfwGL3_NewFrame();


		if (inputHandler.GetKeycodeState(GLFW_KEY_1) & JUST_PRESSED) {
			for (int i = 0; i < nbrP; i++) {
				toruses[i].set_program(&fallback_shader, set_uniforms);
			}
			ball.set_program(&fallback_shader, set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED) {
			for (int i = 0; i < nbrP; i++) {
				toruses[i].set_program(&diffuse_shader, set_uniforms);
			}
			ball.set_program(&diffuse_shader, set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED) {
			for (int i = 0; i < nbrP; i++) {
				toruses[i].set_program(&normal_shader, set_uniforms);
			}
			ball.set_program(&normal_shader, set_uniforms);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED) {
			for (int i = 0; i < nbrP; i++) {
				toruses[i].set_program(&texcoord_shader, set_uniforms);
			}
			ball.set_program(&texcoord_shader, set_uniforms);
		}
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



		// Interpolate the movement of a shape between various
		x += dx;
		if (x >= 1) {
			x = 0;
			// linear interpolation
			if (use_linear) {
				index_current = (points.size() + index_current + 1) % points.size();
				index_next = (points.size() + index_current + 1) % points.size();

			}
			// cubic interpolation
			else {
				index_current = (points.size() + index_current + 1) % points.size();
				index_prev = (points.size() + index_current - 1) % points.size();
				index_next = (points.size() + index_current + 1) % points.size();
				index_next_next = (points.size() + index_next + 1) % points.size();
			}
		}

		glm::vec3 res;
		if (use_linear) {
			res = interpolation::evalLERP(points[index_current], points[index_next], x);
		}
		else {
			res = interpolation::evalCatmullRom(points[index_prev], points[index_current],
			points[index_next], points[index_next_next], catmull_rom_tension, x);
		}
		//ball.set_translation(res);


		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		// render
		for (int i = 0; i < toruses.size(); i++){

			glm::mat4 prevM = toruses[(nbrP + i - 1) % nbrP].get_transform();
			glm::vec3 prevT = glm::vec3(prevM[3][0], prevM[3][1], prevM[3][2]);
			glm::mat4 nextM = toruses[(nbrP + i + 1) % nbrP].get_transform();
			auto nextT = glm::vec3(nextM[3][0], nextM[3][1], nextM[3][2]);
			auto f = glm::normalize(nextT - prevT);
			auto l = glm::cross(glm::vec3(0, 1, 0),f);
			auto u = glm::cross(f, l);
			auto R = glm::mat4(l[0], u[0], f[0], 0,
								l[1], u[1], f[1], 0,
								l[2], u[2], f[2], 0,
								0, 0, 0, 1);



			toruses[i].render(mCamera.GetWorldToClipMatrix(), toruses[i].get_transform() * R);
		}

		ball.render(mCamera.GetWorldToClipMatrix(), ball.get_transform());

		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::SliderFloat("Catmull-Rom tension", &catmull_rom_tension, 0.0f, 1.0f);
			ImGui::Checkbox("Use linear interpolation", &use_linear);
		}
		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
		edaf80::Assignment2 assignment2;
		assignment2.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
