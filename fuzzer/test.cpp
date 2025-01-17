#include <gtest/gtest.h>

#define CAPTURE_STDOUT

#include "fuzzer.h"
#include <optional>
#include <string_view>

TEST(InputGenerator, generateRandomAlphaNum) {
    auto tmp = generators::generateRandomAlphaNum(42);

    EXPECT_EQ(tmp.size(), 42);

	for (const auto& c : tmp)
		EXPECT_TRUE(isalpha(c) || isalnum(c));
}

TEST(InputGenerator, generateRandomString) {
	auto tmp = generators::generateRandomString(42,42,123);

	EXPECT_EQ(tmp.size(), 42);

	for (const auto& c : tmp)
		EXPECT_TRUE(c >= 42 && c<=123);
}

TEST(InputGenerator, generateRandomNum) {
	auto tmp = generators::generateRandomNum(42,123);

	for (const auto& c : tmp)
		EXPECT_TRUE(isalnum(c));

	auto num = std::stoi(tmp);
	EXPECT_TRUE(num >= 42 && num <= 123);
}

class FuzzerCat : public ::testing::Test {
protected:
	std::optional<fuzzer_blackbox> fuzz;

	void SetUp() override {
		fuzz.emplace("/bin/cat", "/tmp/kocoumat-fuzzer/", true, "stdin", std::chrono::seconds(60), 1);
	}

	void TearDown() override {
		fuzz.reset();
	}
};

class FuzzerEcho : public ::testing::Test {
protected:
	std::optional<fuzzer_blackbox> fuzz;

	void SetUp() override {
		fuzz.emplace("/bin/echo", "/tmp/kocoumat-fuzzer/", true, "/bin/echo", std::chrono::seconds(60), 1);
	}

	void TearDown() override {
		fuzz.reset();
	}
};

class FuzzerSleep : public ::testing::Test {
protected:
	std::optional<fuzzer_blackbox> fuzz;

	void SetUp() override {
		fuzz.emplace("/bin/sleep", "/tmp/kocoumat-fuzzer/", true, "/bin/sleep", std::chrono::seconds(60), 1);
	}

	void TearDown() override {
		fuzz.reset();
	}
};

TEST_F(FuzzerCat, execute_running_cin) {
	try
	{
		fuzzer_blackbox::CinInput input("/bin/cat", std::chrono::seconds(1));
		input.setInput("test");
		fuzzer_blackbox::ExecutionResult res = fuzz->execute_with_timeout(input);
		
		EXPECT_EQ(res.stdout_output, "test");
		EXPECT_TRUE(res.stderr_output.empty());
		EXPECT_FALSE(res.timed_out);
		EXPECT_EQ(res.return_code, 0);
	}
	catch (const std::exception& e)
	{
		EXPECT_NO_THROW(throw e);
	}
}

TEST_F(FuzzerEcho, execute_running_arg) {
	try
	{
		fuzzer_blackbox::FileInput input("/bin/echo", std::chrono::seconds(1),"test");
		fuzzer_blackbox::ExecutionResult res = fuzz->execute_with_timeout(input);

		EXPECT_EQ(res.stdout_output, "test\n");
		EXPECT_TRUE(res.stderr_output.empty());
		EXPECT_FALSE(res.timed_out);
		EXPECT_EQ(res.return_code, 0);
	}
	catch (const std::exception& e)
	{
		EXPECT_NO_THROW(throw e);
	}
}

TEST_F(FuzzerSleep, execute_running_timeout) {
	try
	{
		fuzzer_blackbox::FileInput input("/bin/sleep", std::chrono::seconds(1), "5");
		fuzzer_blackbox::ExecutionResult res = fuzz->execute_with_timeout(input);

		EXPECT_TRUE(res.stdout_output.empty());
		EXPECT_TRUE(res.stderr_output.empty());
		EXPECT_TRUE(res.timed_out);
	}
	catch (const std::exception& e)
	{
		EXPECT_NO_THROW(throw e);
	}
}

TEST_F(FuzzerSleep, execute_running_error) {
	try
	{
		fuzzer_blackbox::FileInput input("/bin/sleep", std::chrono::seconds(1), "");
		fuzzer_blackbox::ExecutionResult res = fuzz->execute_with_timeout(input);

		EXPECT_FALSE(res.timed_out);
		EXPECT_NE(res.return_code, 0);
	}
	catch (const std::exception& e)
	{
		EXPECT_NO_THROW(throw e);
	}
}

TEST(Oracle, detectErrorNum) {
	const int num = 42;
	fuzzer_blackbox::ExecutionResult res{ num, "", "", false, std::chrono::milliseconds(1) };
	auto err = fuzzer_blackbox::detectError(res);

	EXPECT_EQ(std::string_view(err->errorName()), "return_code");
	EXPECT_EQ(std::string_view(err->folder()), "crashes");
	EXPECT_TRUE(err->isErrorEncountered(res));

	std::string_view bugInfo =
		"42";

	std::stringstream ss;
	err->bugInfo(ss);
	EXPECT_EQ(ss.str(), bugInfo);

	ASSERT_EQ(typeid(*err.get()), typeid(fuzzer_blackbox::ReturnCodeError));

	const fuzzer_blackbox::ReturnCodeError& error = static_cast<const fuzzer_blackbox::ReturnCodeError&>(*err);

	EXPECT_EQ(error.returnCode, num);
}

TEST(Oracle, detectErrorTimeout) {
	fuzzer_blackbox::ExecutionResult res{ -1, "", "", true, std::chrono::seconds(1) };
	auto err = fuzzer_blackbox::detectError(res);

	EXPECT_EQ(std::string_view(err->errorName()), "timeout");
	EXPECT_EQ(std::string_view(err->folder()), "hangs");
	EXPECT_TRUE(err->isErrorEncountered(res));


	std::stringstream ss;
	err->bugInfo(ss);
	EXPECT_EQ(ss.str(), "1000");

	ASSERT_EQ(typeid(*err.get()), typeid(fuzzer_blackbox::TimeoutError));

	const fuzzer_blackbox::TimeoutError& error = static_cast<const fuzzer_blackbox::TimeoutError&>(*err);

	EXPECT_EQ(error.timeout, std::chrono::seconds(1));
}

TEST(Oracle, detectErrorAsan) {
	auto output =
		"==26276==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000018 at pc 0x7fd27ebe8f89 bp 0x7ffd957169d0 sp 0x7ffd95716148\n"
		"READ of size 9 at 0x602000000018 thread T0\n"
		"    #0 0x7fd27ebe8f88 in printf_common ../../../../src/libsanitizer/sanitizer_common/sanitizer_common_interceptors_format.inc:553\n"
		"    #1 0x7fd27ebe96cc in __interceptor_vprintf ../../../../src/libsanitizer/sanitizer_common/sanitizer_common_interceptors.inc:1660\n"
		"    #2 0x7fd27ebe97c6 in __interceptor_printf ../../../../src/libsanitizer/sanitizer_common/sanitizer_common_interceptors.inc:1718\n"
		"    #3 0x55615b8345b2 in main /home/gaier/APT/fuzzer/src/test/resources/from-file/main.c:30\n"
		"    #4 0x7fd27e98bd8f in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58\n"
		"    #5 0x7fd27e98be3f in __libc_start_main_impl ../csu/libc-start.c:392\n"
		"    #6 0x55615b834284 in _start (/home/gaier/APT/fuzzer/src/test/resources/from-file/main+0x1284)\n";
	fuzzer_blackbox::ExecutionResult res{ 1, "", output, false, std::chrono::milliseconds(1) };
	auto err = fuzzer_blackbox::detectError(res);

	
	EXPECT_EQ(std::string_view(err->errorName()), "asan");
	EXPECT_EQ(std::string_view(err->folder()), "crashes");
	EXPECT_TRUE(err->isErrorEncountered(res));

	std::string_view bugInfo =
		"{\"file\":\"main.c\",\"line\":30,\"kind\":\"heap\"}";

	std::stringstream ss;
	err->bugInfo(ss);
	EXPECT_EQ(ss.str(), bugInfo);

	ASSERT_EQ(typeid(*err.get()), typeid(fuzzer_blackbox::AddressSanitizerError));

	const fuzzer_blackbox::AddressSanitizerError& error = static_cast<const fuzzer_blackbox::AddressSanitizerError&>(*err);

	EXPECT_EQ(error.asanType, "heap");
	EXPECT_EQ(error.file, "main.c");
	EXPECT_EQ(error.line, "30");
}

class FuzzerFalse : public ::testing::Test {
protected:
	std::optional<fuzzer_blackbox> fuzz;

	void SetUp() override {
		fuzz.emplace("/bin/false", "/tmp/fuzzer/", true, "stdin", std::chrono::seconds(60), 1);
	}

	void TearDown() override {
		fuzz.reset();
	}
};

TEST_F(FuzzerFalse, exports) {
	fuzzer_blackbox::AddressSanitizerError err("heap", "main.c", "30");

	fuzzer_blackbox::CrashReport report;
	report.input = "test";
	report.detectedError = &err;
	report.execution_time = std::chrono::duration<double, std::milli>(1.25);
	report.unminimized_size = 42;
	report.nb_steps = 123;
	report.minimization_time = std::chrono::duration<double, std::milli>(12.75);

	std::stringstream ss;
	fuzz->exportReport(report, ss);

	EXPECT_EQ(ss.str(), "{\"input\":\"test\",\"oracle\":\"asan\",\"bug_info\":{\"file\":\"main.c\",\"line\":30,\"kind\":\"heap\"},\"execution_time\":1.25,\"minimization\":{\"unminimized_size\":42,\"nb_steps\":123,\"execution_time\":12.75}}");


	fuzz->saveReport(report, "export.json", "/tmp/fuzzer/");

	std::ifstream createdFile("/tmp/fuzzer/crashes/export.json");
	EXPECT_TRUE(createdFile.good());

	std::string read;
	createdFile >> read;

	EXPECT_EQ(read, "{\"input\":\"test\",\"oracle\":\"asan\",\"bug_info\":{\"file\":\"main.c\",\"line\":30,\"kind\":\"heap\"},\"execution_time\":1.25,\"minimization\":{\"unminimized_size\":42,\"nb_steps\":123,\"execution_time\":12.75}}");
	EXPECT_TRUE(createdFile.eof());
}


TEST_F(FuzzerFalse, fuzzer_fuzz) {
	try
	{
		fuzz->run();
		std::ifstream createdFile("/tmp/fuzzer/crashes/0.json");
		EXPECT_TRUE(createdFile.good());

		std::string read;
		createdFile >> read;

		EXPECT_EQ(read.substr(0, 48), "{\"input\":\"$\",\"oracle\":\"return_code\",\"bug_info\":1");

		read.clear();

		std::ifstream createdFileStats("/tmp/fuzzer/stats.json");
		createdFileStats >> read;

		EXPECT_EQ(read.substr(0, 55), "{\"fuzzer_name\":\"kocoumat\",\"fuzzed_program\":\"/bin/false\"");
	}
	catch (const std::exception& e)
	{
		EXPECT_NO_THROW(throw e);
	}
}

TEST(Mutators, mutateBlocksDelete) {
	std::string tmp = "test";
	mutators::deleteBlock(tmp);
	EXPECT_LT(tmp.size(), 4);
}

TEST(Mutators, mutateBlocksInsert) {
	std::string tmp = "test";
	mutators::insertBlock(tmp);
	EXPECT_GT(tmp.size(), 4);
}

TEST(Mutators, mutateBlocksDigit) {
	std::string tmp = "test";
	mutators::insertDigit(tmp);
	EXPECT_EQ(tmp.size(), 5);
}

TEST(Mutators, mutateConcat) {
	std::string tmp1 = "test";
	std::string tmp2 = "ahoj";
	mutators::concat(tmp1, tmp2);
	EXPECT_EQ(tmp1, "testahoj");
}

TEST(Mutators, flipBit) {
	std::string tmp = "test";
	mutators::flipBitASCII(tmp);
	EXPECT_EQ(tmp.size(), 4);
}

TEST(Mutators, add) {
	std::string tmp = "test";
	mutators::addASCII(tmp);
	EXPECT_EQ(tmp.size(), 4);
}

TEST(Mutators, changeNum) {
	std::string tmp = "42";
	mutators::changeNum(tmp);
	EXPECT_NE(std::stoi(tmp), 42);
}

TEST(Power, weightedChoiceFavourite) {
	std::string input = "test";
	std::string hash = "hash";
	std::multiset<fuzzer_greybox::seed> options;
	options.emplace(std::move(input), hash, 1, 1, 1, 1);

	auto choice = fuzzer_greybox::weightedRandomChoiceFavourite<true>(options, 0.1);
	
	EXPECT_EQ(hash, choice.h);
	EXPECT_EQ(options.size(), 0);
}

TEST(Power, weightedChoiceNormal) {
	std::string input = "test";
	std::string hash = "hash";
	std::multiset<fuzzer_greybox::seed> options;
	options.emplace(std::move(input), hash, 1, 1, 1, 1);

	auto choice = fuzzer_greybox::weightedRandomChoiceNormal<true>(options);

	EXPECT_EQ(hash, choice.h);
	EXPECT_EQ(options.size(), 0);
}

TEST(Escape, escape) {

	for (size_t i = 0; i < 128; i++)
	{
		std::stringstream ss;
		escape(ss, (char)i);
		if(ss.str().size() > 0)
			EXPECT_GE(ss.str()[0], ' ');
	}
}


TEST(Coverage, lcov) {
	std::string input = "TN:test\n"
		"SF:filename\n"
		"DA:5,0\n"
		"DA:8,1\n"
		"DA:9,1\n"
		"DA:11,10\n"
		"DA:13,1\n"
		"DA:18,1\n"
		"DA:19,1\n"
		"LH:6\n"
		"LF:7\n"
		"end_of_record\n";

	auto coverage = fuzzer_greybox::coverage(input);

	EXPECT_EQ(coverage, 6.0/7.0);
}

class Greybox : public ::testing::Test {
protected:
	std::optional<fuzzer_greybox> fuzz;

	void SetUp() override {
		fuzz.emplace("/bin/false", "/tmp/fuzzer/", true, "stdin", std::chrono::seconds(60), 1, fuzzer_greybox::POWER_SCHEDULE_T::simple, "coverage.lcov");
	}

	void TearDown() override {
		fuzz.reset();
	}
};

TEST_F(Greybox, greybox_fuzz) {
	try
	{
		fuzz->run();
		std::ifstream createdFile("/tmp/fuzzer/crashes/0.json");
		EXPECT_TRUE(createdFile.good());

		std::string read;
		createdFile >> read;

		// Any input of len 1 can cause the fuzzer to find the result. If there is only 1 char, it can be any.
		auto eq1 = read.substr(0, 48);
		eq1[10] = '0';
		EXPECT_EQ(eq1, "{\"input\":\"0\",\"oracle\":\"return_code\",\"bug_info\":1");

		read.clear();

		std::ifstream createdFileStats("/tmp/fuzzer/stats.json");
		createdFileStats >> read;

		EXPECT_EQ(read.substr(0, 55), "{\"fuzzer_name\":\"kocoumat\",\"fuzzed_program\":\"/bin/false\"");
	}
	catch (const std::exception& e)
	{
		EXPECT_NO_THROW(throw e);
	}
}

TEST_F(Greybox, generateMySeeds) {
	fuzz->populateWithMySeeds(1234);

	size_t counter = 0;

	for (const auto& i : std::filesystem::directory_iterator(fuzz->INPUT_SEEDS))
		counter++;

	EXPECT_EQ(counter, 1234);
}