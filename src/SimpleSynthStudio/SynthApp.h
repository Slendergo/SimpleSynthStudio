#pragma once

struct GLFWwindow;

namespace SimpleSynthStudio
{
	class SynthApp
	{
	public:
		~SynthApp();

		bool Initialize();
		void Run(const std::filesystem::path& project);

	private:
		void Shutdown();

	private:
		GLFWwindow* _window = nullptr;
		bool _imguiReady = false;
	};
}
