#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <utility>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif
#include <GLFW/glfw3.h>
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <opengl/Shader.h>
#include <opengl/Camera.h>
#include <opengl/Model.h>
#include <opengl/FileSystem.h>
#include <ModelLoader.h>

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mousedown_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void loadNewModel();

// settings
const unsigned int SCR_WIDTH = 1240;
const unsigned int SCR_HEIGHT = 800;
const unsigned int OUTPUT_WIDTH = (SCR_WIDTH * 0.7) - 4;
const unsigned int OUTPUT_HEIGHT = (SCR_HEIGHT * 0.82) - 8;
const unsigned int CONTROL_WIDTH = (SCR_WIDTH * 0.3) - 4;
const unsigned int CONTROL_HEIGHT = OUTPUT_HEIGHT;

float lastX = OUTPUT_WIDTH / 2.0f;
float lastY = OUTPUT_HEIGHT / 2.0f;

bool firstMouse = true;
bool isDragging = false;
bool modelLoading = false;
bool blinn = false;

std::string modelPath = "./resources/convexmesh.obj";

int mouseButton = GLFW_MOUSE_BUTTON_LEFT;

// camera
Camera camera(glm::vec3(0.0f, -0.4f, 3.0f));
Model previewModel;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

int main(int, char**)
{
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if __APPLE__
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

	// Load debug window
	ModelLoader::InitDebugConsole();

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LiveCam FIlter Viewer", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mousedown_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	glfwSwapInterval(1); // Enable vsync

	// Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
	bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
	bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
	bool err = gladLoadGL() == 0;
#else
	bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader modelShader("shaders/material.vert", "shaders/material.frag");
	Shader lampShader("shaders/lamp.vert", "shaders/lamp.frag");

	// load models
	// -----------
	previewModel.Load(modelPath);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// framebuffer configuration
	// -------------------------
	unsigned int framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	// create a color attachment texture
	unsigned int textureColorbuffer;
	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, OUTPUT_WIDTH, OUTPUT_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
	// second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, framebuffer);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Our state
	bool show_demo_window = false;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	modelShader.use();
	modelShader.setInt("diffuseTexture", 0);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 2. Show the model view window
		{
			ImGui::SetNextWindowPos(ImVec2(-1, -1), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(OUTPUT_WIDTH, OUTPUT_HEIGHT), ImGuiCond_FirstUseEver);

			ImGuiWindowFlags window_flags = 0;
			window_flags |= ImGuiWindowFlags_NoTitleBar;
			window_flags |= ImGuiWindowFlags_NoScrollbar;
			window_flags |= ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoResize;
			window_flags |= ImGuiWindowFlags_NoCollapse;
			window_flags |= ImGuiWindowFlags_NoNav;
			window_flags |= ImGuiWindowFlags_NoBackground;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
			ImGui::Begin("Model Viewport", NULL, window_flags);

			// render
			// ------
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glDisable(GL_CULL_FACE);

			// make sure we clear the framebuffer's content
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// don't forget to enable shader before setting uniforms
			modelShader.use();

			//glActiveTexture(GL_TEXTURE0);

			float specVal = (blinn) ? 0.8f : 0.1f;
			float shininess = (blinn) ? 32.0f : 5.0f;

			// set light uniforms
			modelShader.setInt("blinn", blinn);
			modelShader.setVec3("light.position", lightPos);
			modelShader.setVec3("viewPos", camera.Position);

			// light properties
			modelShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
			modelShader.setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
			modelShader.setVec3("light.specular", specVal, specVal, specVal);

			// material properties
			modelShader.setVec3("material.ambient", 0.4f, 0.4f, 0.4f);
			modelShader.setVec3("material.specular", 0.6f, 0.6f, 0.6f); // specular lighting doesn't have full effect on this object's material
			modelShader.setFloat("material.shininess", shininess);

			// view/projection transformations
			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)OUTPUT_WIDTH / (float)OUTPUT_HEIGHT, 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();
			modelShader.setMat4("projection", projection);
			modelShader.setMat4("view", view);

			// render the loaded model
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0.0f, -1.75f, 0.0f)); // translate it down so it's at the center of the scene
			model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));	// it's a bit too big for our scene, so scale it down
			model = glm::rotate(model, camera.SliderRotation.x, glm::vec3(0.1f, 0.0f, 0.0f));
			model = glm::rotate(model, camera.SliderRotation.y, glm::vec3(0.0f, 0.1f, 0.0f));
			model = glm::rotate(model, camera.SliderRotation.z, glm::vec3(0.0f, 0.0f, 0.1f));
			modelShader.setMat4("model", model);

			// draw the meshes
			previewModel.Draw(modelShader);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			/*glDisable(GL_DEPTH_TEST);

			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);*/

			// also draw the lamp object
			lampShader.use();
			lampShader.setMat4("projection", projection);
			lampShader.setMat4("view", view);
			model = glm::mat4(1.0f);
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
			lampShader.setMat4("model", model);

			glBindVertexArray(lightVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

			ImVec2 pos = ImGui::GetCursorScreenPos();

			ImGui::GetWindowDrawList()->AddImage(
				(void*)textureColorbuffer, ImVec2(ImGui::GetCursorScreenPos()),
				ImVec2(ImGui::GetCursorScreenPos().x + OUTPUT_WIDTH, ImGui::GetCursorScreenPos().y + SCR_HEIGHT), ImVec2(0, 1), ImVec2(1, 0));

			ImGui::End();
		}

		// 3. Show model controls
		{
			static int counter = 0;
			ImGui::SetNextWindowPos(ImVec2((SCR_WIDTH - CONTROL_WIDTH), 0), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(CONTROL_WIDTH, CONTROL_HEIGHT), ImGuiCond_FirstUseEver);

			ImGui::Begin("Model Controls");

			if (ImGui::Button("Load Model"))
			{
				loadNewModel();
			}

			ImGui::SameLine();
			ImGui::Text(modelPath.c_str());

			ImGui::Text("PITCH: %f", camera.Pitch);
			ImGui::Text("YAW: %f", camera.Yaw);
			ImGui::Text("ROLL: %f", camera.Roll);
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Text("Transform X: %f", camera.SliderTransform.x);
			ImGui::Text("Transform Y: %f", camera.SliderTransform.y);
			ImGui::Text("Transform R: %f", camera.SliderTransform.z);
			if (ImGui::Button("Reset Xt"))
				camera.SliderTransform.x = 0.0f;
			ImGui::SameLine();
			ImGui::SliderFloat("Xt", &camera.SliderTransform.x, -30.0f, 30.0f);

			if (ImGui::Button("Reset Yt"))
				camera.SliderTransform.y = 0.0f;
			ImGui::SameLine();
			ImGui::SliderFloat("Yt", &camera.SliderTransform.y, -30.0f, 30.0f);

			if (ImGui::Button("Reset Zt"))
				camera.SliderTransform.z = 0.0f;
			ImGui::SameLine();
			ImGui::SliderFloat("Zt", &camera.SliderTransform.z, -1.0f, 1.0f);

			ImGui::Spacing();
			ImGui::Text("Rotation X: %f", camera.SliderRotation.x);
			ImGui::Text("Rotation Y: %f", camera.SliderRotation.y);
			ImGui::Text("Rotation R: %f", camera.SliderRotation.z);

			if (ImGui::Button("Reset Xr")) 
				camera.SliderRotation.x = 0.0f;
			ImGui::SameLine();
			ImGui::SliderFloat("Xr", &camera.SliderRotation.x, -1.0f, 1.0f);

			if (ImGui::Button("Reset Yr"))
				camera.SliderRotation.y = 0.0f;
			ImGui::SameLine();
			ImGui::SliderFloat("Yr", &camera.SliderRotation.y, -1.0f, 1.0f);

			if (ImGui::Button("Reset Zr"))
				camera.SliderRotation.z = 0.0f;
			ImGui::SameLine();
			ImGui::SliderFloat("Zr", &camera.SliderRotation.z, -1.0f, 1.0f);

			ImGui::Spacing();

			if (ImGui::Button("Toggle Lighting"))
				blinn = !blinn;

			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void loadNewModel()
{
	camera.Reset();
	modelPath = ModelLoader::openfilename();
	previewModel.Load(modelPath);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (isDragging && mouseButton == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;

		lastX = xpos;
		lastY = ypos;

		camera.ProcessMouseMovement(xoffset, yoffset, false);
	}
}

void mousedown_callback(GLFWwindow* window, int button, int action, int mods)
{
	mouseButton = button;
	if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		isDragging = (action == GLFW_PRESS);
	}
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

