#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "FileInstrument.h"
#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

/// <summary>
/// Load the whole file into a string
/// </summary>
/// <param name="path">Path to the file</param>
/// <returns>Contents of given file</returns>
std::string loadFile(const char* path)
{
	auto file = std::ifstream(path, std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("Cannot open file");

	std::string res;
	file.seekg(0, std::ios::end);
	res.resize(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(&res[0], res.size());

	if (file.fail())
		throw std::runtime_error("Error reading file");

	return res;
}

int main(int argc, char* argv[]) {
	if (argc <= 1) [[unlikely]]
	{
		std::cerr << "Provide path(s) to source file(s)" << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		std::vector<FileInstrument> fileInstruments;

		for (int fileId = 1; fileId < argc; ++fileId)
		{
			try
			{
				fileInstruments.emplace_back(loadFile(argv[fileId]), argv[fileId], fileId-1);
				std::cerr << "Loaded file " << argv[fileId] << std::endl;
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error parsing file " << argv[fileId] << ": " << e.what() << std::endl;
			}
		}

		std::cerr << "Loaded " << fileInstruments.size() << " files." << std::endl;

		for (const auto& i : fileInstruments)
		{
			std::ofstream outFile(STR(i.fileId << "_instrumented_main.c"));

			if (i.thisIsMainFile)
				instrumentHeaderMain(outFile, fileInstruments);
			else
				instrumentHeaderExtern(outFile, i);

			i.instrument(outFile);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what();
	}

	return 0;
}