
#include "IMBTDialog_imgui.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include <stdio.h>
#include "GL/gl3w.h"
#include "glfw/glfw3.h"


void IMBTDialog_imgui::Show()
{
	// Hiding console window. when debug needed, just comment it
//HWND hWndConsole = GetConsoleWindow();
//ShowWindow(hWndConsole, SW_HIDE);

	int displayW{};
	int displayH{};

	// GLFW Initialization
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* GUIWindow;
	GUIWindow = glfwCreateWindow(1200, 800, "Example", nullptr, nullptr);
	glfwMakeContextCurrent(GUIWindow);

	// GL3W Initialization
	gl3wInit();

	// ImGui Initialization
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = nullptr;

	ImGui_ImplGlfw_InitForOpenGL(GUIWindow, true);
	ImGui_ImplOpenGL3_Init("#version 430 core");

	while (!glfwWindowShouldClose(GUIWindow))
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Render whole components
		ImGui::Render();

		glfwMakeContextCurrent(GUIWindow);
		glfwGetFramebufferSize(GUIWindow, &displayW, &displayH);

		glViewport(0, 0, displayW, displayH);
		glClearColor(0.9f, 0.9f, 0.9f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(GUIWindow);

		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(GUIWindow);
	glfwTerminate();
}
