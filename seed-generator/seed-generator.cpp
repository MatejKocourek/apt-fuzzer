#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <filesystem>
#include "seed-generator.h"

/// <summary>
/// Load the whole file into a string
/// </summary>
/// <param name="path">Path to the file</param>
/// <returns>Contents of given file</returns>
static std::string loadFile(const std::filesystem::path& path)
{
	auto file = std::ifstream(path, std::ios::binary);

	if (!file.is_open()) [[unlikely]]
		throw std::runtime_error("Cannot open file: " + path.string());

	std::string res;
	file.seekg(0, std::ios::end);
	res.resize(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(&res[0], res.size());

	if (file.fail()) [[unlikely]]
		throw std::runtime_error("Error reading file: " + path.string());

	return res;
}

int main(int argc, char* argv[]) {
	if (argc < 3) [[unlikely]]
	{
		std::cerr << "Provide path to source folder and output folder" << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		std::cerr << "Creating seeds from constants in files from " << argv[1] << " and placing them in " << argv[2] << std::endl;

		std::filesystem::path path(argv[1]);

		if (!std::filesystem::is_directory(path)) [[unlikely]]
			throw std::runtime_error("Source directory does not exist");

		SeedGenerator seedGenerator;
		for (const auto& i : std::filesystem::directory_iterator(path))
		{
			try
			{
				if (i.path().extension() == ".c" || i.path().extension() == ".h")
				{
					seedGenerator.parseSource(loadFile(i.path()));
					std::cerr << "Loaded file " << i.path() << std::endl;
				}
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error parsing file " << i.path() << ": " << e.what() << std::endl;
			}
		}

		std::cerr << "Creating seeds from parsed files" << std::endl;
		seedGenerator.createSeeds(argv[2]);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
