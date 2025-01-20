#include <gtest/gtest.h>
#include "seed-generator.h"
#include <optional>
#include <string_view>
#include <sstream>
#include <filesystem>

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

const std::filesystem::path path("test-directory");

class SeedGeneratorTest : public ::testing::Test {
protected:
	std::optional<SeedGenerator> generator;

	void SetUp() override {
		std::filesystem::remove_all(path);
		generator.emplace();
	}

	void TearDown() override {
		generator.reset();
		std::filesystem::remove_all(path);
	}
};

TEST(Escaping, unescape)
{
	EXPECT_EQ(SeedGenerator::unEscapeString("\"\\a\""), "\a");
	EXPECT_EQ(SeedGenerator::unEscapeString("\"\\b\""), "\b");
	EXPECT_EQ(SeedGenerator::unEscapeString("\"\\n\""), "\n");
	EXPECT_EQ(SeedGenerator::unEscapeString("\"\\n\\a\\b\\f\\r\\t\\v\\\'\\\"\\\\\""), "\n\a\b\f\r\t\v\'\"\\");
	EXPECT_EQ(SeedGenerator::unEscapeString("\"test\""), "test");

}

TEST(StringSize, stringSize)
{
	std::stringstream sstream;
	SeedGenerator::writeStringOfSize(sstream, 10);

	EXPECT_EQ(sstream.str().size(), 10);
}

TEST_F(SeedGeneratorTest, structural)
{
	std::string_view code1 =
		"\
		#include \"should_not_be_generated.h\"\
		int main(){\
			int a = 42;\
			char b = 'x';\
			return a;\
		}\
		";
	generator->parseSource(code1);

	std::string_view code2 =
		"\
		#define RET 123\
		//comments are left out, even if they contain:\
		//numbers:\
		/*99*/\
		/*\"or strings\"*/\
		int main(){\
			char* c = \"hello\\nworld\";\
			float d = 3.14;\
			return RET;\
		}\
		";
	generator->parseSource(code2);

	generator->createSeeds(path);

	std::unordered_set<std::string> seeds;

	for (const auto& file : std::filesystem::directory_iterator(path))
		seeds.emplace(loadFile(file.path()));

	EXPECT_TRUE(seeds.contains("42"));
	EXPECT_TRUE(seeds.contains("x"));
	EXPECT_TRUE(seeds.contains("123"));
	EXPECT_TRUE(seeds.contains("hello\nworld"));
	EXPECT_TRUE(seeds.contains("3.14"));

	EXPECT_FALSE(seeds.contains("99"));
	EXPECT_FALSE(seeds.contains("or strings"));
	EXPECT_FALSE(seeds.contains("should_not_be_generated.h"));
}