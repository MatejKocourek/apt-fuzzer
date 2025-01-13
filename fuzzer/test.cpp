#include <gtest/gtest.h>
#include "fuzzer.h"
#include <optional>
#include <string_view>

TEST(InputGenerator, generateRandomAlphaNum) {
    auto tmp = fuzzer::generateRandomAlphaNum(42);

    EXPECT_EQ(tmp.size(), 42);

	for (const auto& c : tmp)
		EXPECT_TRUE(isalpha(c) || isalnum(c));
}

TEST(InputGenerator, generateRandomString) {
	auto tmp = fuzzer::generateRandomString(42,42,123);

	EXPECT_EQ(tmp.size(), 42);

	for (const auto& c : tmp)
		EXPECT_TRUE(c >= 42 && c<=123);
}

TEST(InputGenerator, generateRandomNum) {
	auto tmp = fuzzer::generateRandomNum(42,123);

	for (const auto& c : tmp)
		EXPECT_TRUE(isalnum(c));

	auto num = std::stoi(tmp);
	EXPECT_TRUE(num >= 42 && num <= 123);
}

class FuzzerCat : public ::testing::Test {
protected:
	std::optional<fuzzer> fuzz;

	void SetUp() override {
		fuzz.emplace("/bin/cat", "/tmp/kocoumat-fuzzer/", true, "stdin", std::chrono::seconds(1), 1);
	}

	void TearDown() override {
		fuzz.reset();
	}
};

class FuzzerEcho : public ::testing::Test {
protected:
	std::optional<fuzzer> fuzz;

	void SetUp() override {
		fuzz.emplace("/bin/echo", "/tmp/kocoumat-fuzzer/", true, "/bin/echo", std::chrono::seconds(1), 1);
	}

	void TearDown() override {
		fuzz.reset();
	}
};

class FuzzerSleep : public ::testing::Test {
protected:
	std::optional<fuzzer> fuzz;

	void SetUp() override {
		fuzz.emplace("/bin/sleep", "/tmp/kocoumat-fuzzer/", true, "/bin/sleep", std::chrono::seconds(1), 1);
	}

	void TearDown() override {
		fuzz.reset();
	}
};

TEST_F(FuzzerCat, execute_running_cin) {
	try
	{
		fuzzer::CinInput input("/bin/cat", std::chrono::seconds(1));
		input.setInput("test");
		fuzzer::ExecutionResult res = fuzz->execute_with_timeout(input);
		
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

TEST_F(FuzzerEcho, execute_running_arg) {
	try
	{
		fuzzer::FileInput input("/bin/echo", std::chrono::seconds(1),"test");
		fuzzer::ExecutionResult res = fuzz->execute_with_timeout(input);

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
		fuzzer::FileInput input("/bin/sleep", std::chrono::seconds(1), "5");
		fuzzer::ExecutionResult res = fuzz->execute_with_timeout(input);

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
		fuzzer::FileInput input("/bin/sleep", std::chrono::seconds(1), "");
		fuzzer::ExecutionResult res = fuzz->execute_with_timeout(input);

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
	fuzzer::ExecutionResult res{ num, "", "", false, std::chrono::milliseconds(1) };
	auto err = fuzzer::detectError(res);

	EXPECT_EQ(std::string_view(err->errorName()), "return_code");
	EXPECT_EQ(std::string_view(err->folder()), "crashes");
	EXPECT_TRUE(err->isErrorEncountered(res));

	std::string_view bugInfo =
		"42";

	std::stringstream ss;
	err->bugInfo(ss);
	EXPECT_EQ(ss.str(), bugInfo);

	ASSERT_EQ(typeid(*err.get()), typeid(fuzzer::ReturnCodeError));

	const fuzzer::ReturnCodeError& error = static_cast<const fuzzer::ReturnCodeError&>(*err);

	EXPECT_EQ(error.returnCode, num);
}

TEST(Oracle, detectErrorTimeout) {
	fuzzer::ExecutionResult res{ -1, "", "", true, std::chrono::seconds(1) };
	auto err = fuzzer::detectError(res);

	EXPECT_EQ(std::string_view(err->errorName()), "timeout");
	EXPECT_EQ(std::string_view(err->folder()), "hangs");
	EXPECT_TRUE(err->isErrorEncountered(res));


	std::stringstream ss;
	err->bugInfo(ss);
	EXPECT_EQ(ss.str(), "1000");

	ASSERT_EQ(typeid(*err.get()), typeid(fuzzer::TimeoutError));

	const fuzzer::TimeoutError& error = static_cast<const fuzzer::TimeoutError&>(*err);

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
	fuzzer::ExecutionResult res{ 1, "", output, false, std::chrono::milliseconds(1) };
	auto err = fuzzer::detectError(res);

	
	EXPECT_EQ(std::string_view(err->errorName()), "asan");
	EXPECT_EQ(std::string_view(err->folder()), "crashes");
	EXPECT_TRUE(err->isErrorEncountered(res));

	std::string_view bugInfo =
		"{\"file\":\"main.c\",\"line\":30,\"kind\":\"heap\"}";

	std::stringstream ss;
	err->bugInfo(ss);
	EXPECT_EQ(ss.str(), bugInfo);

	ASSERT_EQ(typeid(*err.get()), typeid(fuzzer::AddressSanitizerError));

	const fuzzer::AddressSanitizerError& error = static_cast<const fuzzer::AddressSanitizerError&>(*err);

	EXPECT_EQ(error.asanType, "heap");
	EXPECT_EQ(error.file, "main.c");
	EXPECT_EQ(error.line, "30");
}

TEST(Results, exports) {
	fuzzer::AddressSanitizerError err("heap", "main.c", "30");

	fuzzer::CrashReport report;
	report.input = "test";
	report.detectedError = &err;
	report.execution_time = std::chrono::duration<double, std::milli>(1.25);
	report.unminimized_size = 42;
	report.nb_steps = 123;
	report.minimization_time = std::chrono::duration<double, std::milli>(12.75);

	std::stringstream ss;
	fuzzer::exportReport(report, ss);

	EXPECT_EQ(ss.str(), "{\"input\":\"test\",\"oracle\":\"asan\",\"bug_info\":{\"file\":\"main.c\",\"line\":30,\"kind\":\"heap\"},\"execution_time\":1.25,\"minimization\":{\"unminimized_size\":42,\"nb_steps\":123,\"execution_time\":12.75}}");


	fuzzer::saveReport(report, "export.json", "/tmp/fuzzer/");

	std::ifstream createdFile("/tmp/fuzzer/crashes/export.json");
	EXPECT_TRUE(createdFile.good());

	std::string read;
	createdFile >> read;

	EXPECT_EQ(read, "{\"input\":\"test\",\"oracle\":\"asan\",\"bug_info\":{\"file\":\"main.c\",\"line\":30,\"kind\":\"heap\"},\"execution_time\":1.25,\"minimization\":{\"unminimized_size\":42,\"nb_steps\":123,\"execution_time\":12.75}}");
	EXPECT_TRUE(createdFile.eof());
}

class FuzzerFalse : public ::testing::Test {
protected:
	std::optional<fuzzer> fuzz;

	void SetUp() override {
		fuzz.emplace("/bin/false", "/tmp/fuzzer/", true, "stdin", std::chrono::seconds(1), 1);
	}

	void TearDown() override {
		fuzz.reset();
	}
};

TEST_F(FuzzerFalse, fuzzer_fuzz) {
	try
	{
		fuzz->run();
		std::ifstream createdFile("/tmp/fuzzer/crashes/0.json");
		EXPECT_TRUE(createdFile.good());

		std::string read;
		createdFile >> read;

		EXPECT_EQ(read.substr(0, 48), "{\"input\":\"k\",\"oracle\":\"return_code\",\"bug_info\":1");

		read.clear();

		std::ifstream createdFileStats("/tmp/fuzzer/stats.json");
		createdFileStats >> read;

		EXPECT_EQ(read.substr(0, 105), "{\"fuzzer_name\":\"kocoumat\",\"fuzzed_program\":\"/bin/false\",\"nb_runs\":6,\"nb_failed_runs\":6,\"nb_hanged_runs\":0");
	}
	catch (const std::exception& e)
	{
		EXPECT_NO_THROW(throw e);
	}
}