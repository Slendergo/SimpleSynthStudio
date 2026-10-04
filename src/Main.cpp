#include "pch.h"

#include "SimpleSynthStudio/SynthApp.h"

int main(int argc, char** argv)
{
	SimpleSynthStudio::SynthApp app;
	if (!app.Initialize()) {
		return 1;
	}

	std::filesystem::path project;
	if (argc >= 2 && std::filesystem::path(argv[1]).extension() == ".psynth") {
		project = argv[1];
	}

	app.Run(project);
	return 0;
}
