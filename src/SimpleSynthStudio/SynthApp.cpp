#include "pch.h"
#include "SimpleSynthStudio/SynthApp.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "SimpleSynthStudio/SynthEditor.h"

namespace SimpleSynthStudio
{
	SynthApp::~SynthApp()
	{
		Shutdown();
	}

	bool SynthApp::Initialize()
	{
		if (!glfwInit()) {
			LOG_ERROR("Failed to initialize GLFW.");
			return false;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		_window = glfwCreateWindow(1280, 800, "Synth Tool", nullptr, nullptr);
		if (!_window) {
			LOG_ERROR("[Synth Tool] Failed to create the window.");
			return false;
		}

		glfwMakeContextCurrent(_window);
		glfwSwapInterval(1);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().IniFilename = nullptr;
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(_window, true);
		ImGui_ImplOpenGL3_Init("#version 330");
		_imguiReady = true;
		return true;
	}

	void SynthApp::Run(const std::filesystem::path& project)
	{
		SynthEditor editor;
		if (!editor.Initialize()) {
			LOG_ERROR("[Synth Tool] Audio is unavailable.");
			return;
		}

		if (!project.empty()) {
			editor.OpenProject(project);
		}

		std::string title;
		double lastTime = glfwGetTime();
		while (true) {
			glfwPollEvents();

			if (glfwWindowShouldClose(_window)) {
				glfwSetWindowShouldClose(_window, GLFW_FALSE);
				editor.RequestExit();
			}
			if (editor.IsExitConfirmed()) {
				break;
			}

			const std::string currentTitle = editor.GetTitle();
			if (currentTitle != title) {
				title = currentTitle;
				glfwSetWindowTitle(_window, title.c_str());
			}

			const double now = glfwGetTime();
			const float dt = static_cast<float>(now - lastTime);
			lastTime = now;

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			editor.Update(dt);
			editor.Draw();

			ImGui::Render();

			int width = 0;
			int height = 0;
			glfwGetFramebufferSize(_window, &width, &height);
			glViewport(0, 0, width, height);
			glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(_window);
		}
	}

	void SynthApp::Shutdown()
	{
		if (_imguiReady) {
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			_imguiReady = false;
		}

		if (_window) {
			glfwDestroyWindow(_window);
			_window = nullptr;
		}
		glfwTerminate();
	}
}
