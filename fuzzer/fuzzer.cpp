#include <iostream>
#include <random>
#include <string>
#include <stdexcept>
#include <boost/process.hpp>
#include <chrono>
#include <optional>
#include <thread>
#include <future>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <stack>
#include <memory>
#include <regex>
#include <csignal>


#include "median.h"


thread_local std::random_device rd;  // Seed for the random number engine
thread_local std::mt19937 gen(rd()); // Mersenne Twister engine seeded with `rd`

static std::atomic<size_t> nb_before_min = 0;
static std::atomic<size_t> nb_failed_runs = 0;
static std::atomic<size_t> nb_hanged_runs = 0;

StatisticsMemory<double> statisticsExecution;
StatisticsMemory<double> statisticsMinimization;
StatisticsMemory<uint32_t> statisticsMinimizationSteps;

std::atomic<bool> keepRunning = true;


std::string generateRandomAlphaNum(std::size_t size) {
    constexpr char alphaNumerical[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z' };
    std::uniform_int_distribution<int> dist(0, sizeof(alphaNumerical)-1);

    std::string randomString;
    randomString.reserve(size);

    for (std::size_t i = 0; i < size; ++i) {
        char tmp = alphaNumerical[dist(gen)];

        randomString += tmp; // Generate a random character
    }

    return randomString;
}

std::string generateRandomString(std::size_t size, int minChar, int maxChar) {
    if (minChar > maxChar) [[unlikely]]
        throw std::invalid_argument("min must be less than or equal to max");
  
    std::uniform_int_distribution<int> dist(minChar, maxChar);

    std::string randomString;
    randomString.reserve(size);

    for (std::size_t i = 0; i < size; ++i) {
        char tmp = dist(gen);
        while (tmp == '\\' || tmp == '"')
            tmp = dist(gen);

        randomString += tmp; // Generate a random character
    }

    return randomString;
}

std::string generateRandomNum(int min, int max) {
    if (min > max) [[unlikely]]
        throw std::invalid_argument("min must be less than or equal to max");

    std::uniform_int_distribution<int> dist(min, max);

    return std::to_string(dist(gen));
}

struct ExecutionResult {
    int return_code;
    std::string stdout_output;
    std::string stderr_output;
    bool timed_out;
    std::chrono::duration<double, std::milli> execution_time;

    //ExecutionResult() = default;

    //ExecutionResult(const ExecutionResult&) = default;
    //ExecutionResult(ExecutionResult&&) = default;

    bool operator==(const ExecutionResult&) const = default;

    bool isOk() const
    {
        return return_code == 0 && timed_out == false;
    }
};

struct ExecutionInput
{
    ExecutionInput(std::string executablePath, std::chrono::milliseconds timeout) : executablePath(std::move(executablePath)), timeout(std::move(timeout))
    {

    }

    virtual std::vector<std::string> getArguments() const = 0;
    virtual std::string_view getCin() const = 0;

    virtual void setInput(const std::string_view& input) = 0;
    virtual void setInput(std::string_view&& input)
    {
        setInput(input);
    }

    const std::string executablePath;
    const std::chrono::milliseconds timeout;

};

struct FileInput final : public ExecutionInput
{
    FileInput(std::string executablePath, std::chrono::milliseconds timeout, std::string path) : ExecutionInput(std::move(executablePath), std::move(timeout)), path(std::move(path))
    {

    }

    virtual std::vector<std::string> getArguments() const final
    {
        return { path };
    }
    virtual std::string_view getCin() const final
    {
        return "";
    }

    virtual void setInput(const std::string_view& input) final
    {
        std::ofstream fuzzInput(path);
        fuzzInput << input;
    }
private:
    const std::string path;
};

struct CinInput final : public ExecutionInput
{
    CinInput(std::string executablePath, std::chrono::milliseconds timeout) : ExecutionInput(std::move(executablePath), std::move(timeout))
    {

    }

    virtual std::vector<std::string> getArguments() const final
    {
        return {};
    }
    virtual std::string_view getCin() const final
    {
        return cinInput;
    }

    virtual void setInput(const std::string_view& input) final
    {
        cinInput = input;
    }
    virtual void setInput(std::string_view&& input) final
    {
        cinInput = std::move(input);
    }
private:
    std::string_view cinInput;
};

ExecutionResult execute_with_timeout(const ExecutionInput& executionInput) {
    using namespace boost::process;
    //namespace asio = boost::asio;

    ipstream stdout_stream;  // To capture standard output
    ipstream stderr_stream;  // To capture standard error
    opstream stdin_stream;   // To provide input

    //std::vector<std::string> arguments;

    //if (!executionInput.getArgument().empty())
    //    arguments.push_back(std::string(executionInput.getArgument()).c_str());

    auto start = std::chrono::high_resolution_clock::now();
    child process(executionInput.executablePath, executionInput.getArguments(), std_out > stdout_stream, std_err > stderr_stream, std_in < stdin_stream);

    // Feed the process's standard input.
    stdin_stream << executionInput.getCin();
    stdin_stream.close();
    //stdin_stream.flush();
    stdin_stream.pipe().close();  // Close stdin to signal end of input

    std::future<std::string> stdout_future = std::async(std::launch::async, [&]() {
        std::ostringstream oss;
        std::string line;
        while (stdout_stream && std::getline(stdout_stream, line))
            oss << line << '\n';
        return oss.str();
        });

    std::future<std::string> stderr_future = std::async(std::launch::async, [&]() {
        std::ostringstream oss;
        std::string line;
        while (stderr_stream && std::getline(stderr_stream, line))
            oss << line << '\n';
        return oss.str();
        });


    // Wait for process completion with a timeout.
    bool finished_in_time = process.wait_for(executionInput.timeout);
    if (!finished_in_time) {
        process.terminate();  // Kill the process if it times out
        return { -1, "", "", true, std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - start) };  // Indicate a timeout occurred
    }

    // Retrieve the outputs and return code
    return { std::move(process.exit_code()), std::move(stdout_future.get()), std::move(stderr_future.get()), false, std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - start) };
}

struct DetectedError
{
    //bool operator==(const DetectedError&) const = default;

    virtual bool isErrorEncountered(const ExecutionResult& executionResult) const = 0;

    virtual const char* errorName() const = 0;

    virtual const char* folder() const = 0;

    virtual std::ostream& bugInfo(std::ostream& os) const = 0;

    virtual void incrementCounter() const = 0;

    virtual bool operator == (const DetectedError& Other) const = 0;
};

struct ReturnCodeError : public DetectedError
{
    ReturnCodeError(int returnCode) : returnCode(std::move(returnCode)) {}

    bool operator==(const ReturnCodeError& other) const
    {
        return returnCode == other.returnCode;
    }
    virtual bool operator == (const DetectedError& Other) const override {
        if (typeid(Other) == typeid(*this))
        {
            const ReturnCodeError& other = static_cast<const ReturnCodeError&>(Other);
            
            return *this == other;
        }
        else
            return false;
    }

    ReturnCodeError(ReturnCodeError&&) = default;
    ReturnCodeError(const ReturnCodeError&) = default;

    static std::optional<ReturnCodeError> tryDetectError(const ExecutionResult& executionResult)
    {
        std::optional<ReturnCodeError> res;

        if (executionResult.return_code != EXIT_SUCCESS)
            res.emplace(executionResult.return_code);

        return res;
    }

    virtual bool isErrorEncountered(const ExecutionResult& executionResult) const
    {
        auto tmp = tryDetectError(executionResult);
        if (tmp.has_value() && ((*tmp) == (*this)))
            return true;

        return false;
    }

    virtual const char* errorName() const
    {
        return "return_code";
    }

    virtual const char* folder() const
    {
        return "crashes";
    }

    virtual std::ostream& bugInfo(std::ostream& os) const override
    {
        os << returnCode;
        return os;
    }

    virtual void incrementCounter() const override
    {
        nb_failed_runs.fetch_add(1, std::memory_order_relaxed);
    }

    const int returnCode;
};

struct TimeoutError final : public DetectedError
{
    bool operator==(const TimeoutError& other) const
    {
        return true;// timeout == other.timeout;
    }

    TimeoutError(std::chrono::duration<double, std::milli> timeout) : timeout(std::move(timeout)){}
    TimeoutError(TimeoutError&&) = default;
    TimeoutError(const TimeoutError&) = default;

    virtual bool operator == (const DetectedError& Other) const override {
        if (typeid(Other) == typeid(*this))
        {
            const TimeoutError& other = static_cast<const TimeoutError&>(Other);

            return *this == other;
        }
        else
            return false;
    }

    static std::optional<TimeoutError> tryDetectError(const ExecutionResult& executionResult)
    {
        std::optional<TimeoutError> res;

        if (executionResult.timed_out)
            res.emplace(executionResult.execution_time);

        return res;
    }

    virtual bool isErrorEncountered(const ExecutionResult& executionResult) const final
    {
        auto tmp = tryDetectError(executionResult);
        if (tmp.has_value() && ((*tmp) == (*this)))
            return true;

        return false;
    }

    virtual const char* errorName() const final
    {
        return "timeout";
    }

    virtual const char* folder() const final
    {
        return "hangs";
    }

    virtual std::ostream& bugInfo(std::ostream& os) const override
    {
        os << timeout.count();
        return os;
    }

    virtual void incrementCounter() const override
    {
        nb_hanged_runs.fetch_add(1, std::memory_order_relaxed);
    }

    std::chrono::duration<double, std::milli> timeout;
};

static const std::regex errorTypeRegex("ERROR: AddressSanitizer: (.*) on address");
static const std::regex locationRegex("(main.c):(\\d+)");//"(at 0x[0-9A-Fa-f]+.*:(\\d+))");

struct AddressSanitizerError : public ReturnCodeError
{
    AddressSanitizerError(std::string asanType, std::string file, std::string line) : asanType(std::move(asanType)), file(std::move(file)), line(std::move(line)), ReturnCodeError(1) {}

    AddressSanitizerError(AddressSanitizerError&&) = default;
    AddressSanitizerError(const AddressSanitizerError&) = default;

    bool operator==(const AddressSanitizerError& other) const
    {
        return other.asanType == asanType && other.file == file && other.line == line;
    }

    virtual bool operator == (const DetectedError& Other) const override {
        if (typeid(Other) == typeid(*this))
        {
            const AddressSanitizerError& other = static_cast<const AddressSanitizerError&>(Other);

            return *this == other;
        }
        else
            return false;
    }

    virtual const char* errorName() const final
    {
        return "asan";
    }

    static std::optional<AddressSanitizerError> tryDetectError(const ExecutionResult& executionResult)
    {
        std::optional<AddressSanitizerError> res;
        //const std::string test = "==22836==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffe6d166b08 at pc\n#3 0x64161615 in main tests/minimization/main.c:17\n#4 0x6456564...";

        if (executionResult.return_code == 1)
        {
            std::smatch match;
            if (std::regex_search(executionResult.stderr_output, match, errorTypeRegex))
            {
                //std::cerr << "Caught ASAN " << match[1] << " in " << std::endl;
                //std::cerr << executionResult.stderr_output << std::endl;

                std::smatch match2;

                if (std::regex_search(executionResult.stderr_output, match2, locationRegex)) {
                    std::string asan;
                    if (match[1] == "heap-buffer-overflow")
                        asan = "heap";
                    else if (match[1] == "stack-buffer-overflow")
                        asan = "stack";
                    else
                        asan = match[1];
                    //std::filesystem::path(std::string(match2[1])).filename().string()
                    res.emplace(std::move(asan), match2[1], match2[2]);
                    //std::cerr << "File: " << match2[1] << ", line: " << match2[2] << std::endl;
                }
            }

        }

        return res;
    }

    virtual bool isErrorEncountered(const ExecutionResult& executionResult) const
    {
        auto tmp = tryDetectError(executionResult);
        if (tmp.has_value() && ((*tmp) == (*this)))
            return true;

        return false;
    }

    virtual std::ostream& bugInfo(std::ostream& os) const override
    {
        os << "{"
                "\"file\":\"" << file << "\","
                "\"line\":" << line << ","
                "\"kind\":\"" << asanType << "\""
              "}"
        ;
        return os;
    }

    const std::string asanType;
    const std::string file;
    const std::string line;
};

//struct HeapBufferError final : AddressSanitizerError
//{
//    HeapBufferError(int line) : AddressSanitizerError(std::move(line)) {}
//
//    HeapBufferError(HeapBufferError&&) = default;
//    HeapBufferError(const HeapBufferError&) = default;
//
//    static std::optional<HeapBufferError> tryDetectError(const ExecutionResult& executionResult)
//    {
//        std::optional<HeapBufferError> res;
//
//        if (executionResult.return_code == 1)
//        {
//            std::smatch match;
//            if (std::regex_search(executionResult.stderr_output, match, errorTypeRegex))
//            {
//                if (match[1] == "heap-buffer-overflow")
//                {
//                    if (std::regex_search(output, match, locationRegex)) {
//                        std::string lineNumber = match[1];
//                        std::cout << "Error Location: Line " << lineNumber << std::endl;
//                    }
//                }
//            }
//
//        }
//
//        return res;
//    }
//
//    virtual bool isErrorEncountered(const ExecutionResult& executionResult) const final
//    {
//        auto tmp = tryDetectError(executionResult);
//        if (tmp.has_value() && ((*tmp) == (*this)))
//            return true;
//
//        return false;
//    }
//
//    bool operator==(const HeapBufferError&) const = default;
//};
//
//struct StackBufferError final : AddressSanitizerError
//{
//    StackBufferError(int line) : line(std::move(line)) {}
//
//    StackBufferError(StackBufferError&&) = default;
//    StackBufferError(const StackBufferError&) = default;
//
//    static std::optional<StackBufferError> tryDetectError(const ExecutionResult& executionResult)
//    {
//        std::optional<StackBufferError> res;
//
//        //TODO
//
//        return res;
//    }
//
//    virtual bool isErrorEncountered(const ExecutionResult& executionResult) const final
//    {
//        auto tmp = tryDetectError(executionResult);
//        if (tmp.has_value() && ((*tmp) == (*this)))
//            return true;
//
//        return false;
//    }
//
//    bool operator==(const StackBufferError&) const = default;
//
//    const int line;
//};

std::unique_ptr<DetectedError> detectError(const ExecutionResult& result)
{
    {
        auto tmp = TimeoutError::tryDetectError(result);
        if (tmp.has_value())
            return std::make_unique<TimeoutError>(std::move(*tmp));
    }
    {
        auto tmp = AddressSanitizerError::tryDetectError(result);
        if (tmp.has_value())
            return std::make_unique<AddressSanitizerError>(std::move(*tmp));
    }

    //{
    //    auto tmp = HeapBufferError::tryDetectError(result);
    //    if (tmp.has_value())
    //        return std::make_unique<HeapBufferError>(std::move(*tmp));
    //}
    //{
    //    auto tmp = StackBufferError::tryDetectError(result);
    //    if (tmp.has_value())
    //        return std::make_unique<StackBufferError>(std::move(*tmp));
    //}
    {
        auto tmp = ReturnCodeError::tryDetectError(result);
        if (tmp.has_value())
            return std::make_unique<ReturnCodeError>(std::move(*tmp));
    }

    return std::unique_ptr<DetectedError>();
}

std::string minimizeInput(const std::string_view& input, const DetectedError& prevResult, ExecutionInput& executionInput, size_t& totalRuns, std::stack<std::pair<std::string, ExecutionResult>>& unprocessedErrors)
{
    constexpr size_t divisionsStepStart = 2;
    size_t divisionStep = divisionsStepStart;

    while (true)
    {
        //std::string_view originalInput = input;
        size_t step = input.length() / divisionStep;

        if (step < 1)
            return std::string(input);

        //Step 1
        for (size_t i = 0; i < input.length(); i += step)
        {
            auto cropped = input.substr(i, step);
            executionInput.setInput(cropped);
            //input = originalInput.substr(i, step);

            totalRuns+=1;
            auto result = execute_with_timeout(executionInput);
            if (prevResult.isErrorEncountered(result))
            {
                return minimizeInput(cropped, prevResult, executionInput, totalRuns, unprocessedErrors);
            }
            else
            {
                if (!result.isOk())
                    unprocessedErrors.emplace(input, std::move(result));// Minimization discovered a different bug, remember for later
            }
        }

        //Step 2
        for (size_t i = 0; i < input.length(); i += step)
        {
            std::string complement;
            complement.reserve(input.size() - step);
            complement += input.substr(0, i);
            if(i+step < input.length())
                complement += input.substr(i + step);

            executionInput.setInput(complement);
            //input = complement;

            totalRuns += 1;
            auto result = execute_with_timeout(executionInput);
            if (prevResult.isErrorEncountered(result))
            {
                return minimizeInput(complement, prevResult, executionInput, totalRuns, unprocessedErrors);
            }
            else
            {
                if (!result.isOk())
                    unprocessedErrors.emplace(input, std::move(result));// Minimization discovered a different bug, remember for later
            }
        }

        //Step 3

        //If it arrives here, it means we can't minimize at this granularity

        //input = originalInput;
        divisionStep++;//*= 2; // Minimize with more steps
    }
}



struct CrashReport
{
    std::string input;
    DetectedError* detectedError;
    std::chrono::duration<double,std::milli> execution_time;
    size_t unminimized_size;
    size_t nb_steps;
    std::chrono::duration<double, std::milli> minimization_time;
};


void exportReport(const CrashReport& report, std::ostream& output)
{
    output <<
     "{"
        "\"input\":"            "\"" << report.input << "\","
        "\"oracle\":"           "\"" << report.detectedError->errorName() << "\","
        "\"bug_info\":";                report.detectedError->bugInfo(output) << ","
        "\"execution_time\":"        << report.execution_time.count() << 
        ",\"minimization\":{"
            "\"unminimized_size\":" << report.unminimized_size << ","
            "\"nb_steps\":" << report.nb_steps << ","
            "\"execution_time\":" << report.minimization_time.count() << ""
        "}"
     "}";
}
static void saveReport(const CrashReport& report, const std::string& name, const std::filesystem::path& resultFolder)
{
    std::filesystem::path resultFile;

    resultFile = resultFolder / report.detectedError->folder();

    std::filesystem::create_directories(resultFile);

    resultFile /= name; 
    //const auto now = std::chrono::system_clock::now();
    //std::cout << std::format("{:%Y-%m-%d-%H-%M-%OS}", now) << '\n';


    std::ofstream output(resultFile);

    exportReport(report, output);

    std::cout << "New error report: \n";
    exportReport(report, std::cout);
    std::cout << std::endl;

    if (!output)
        std::cerr << "Error saving report!" << std::endl;
}

static std::string FUZZED_PROG;
static std::filesystem::path RESULT_FUZZ;
static bool MINIMIZE;
static std::string_view fuzzInputType;
static std::chrono::seconds TIMEOUT;
static size_t NB_KNOWN_BUGS;


static std::mutex m;
static std::vector<std::unique_ptr<DetectedError>> uniqueResults;

static void exportStatistics(std::ostream& output)
{
    std::lock_guard guard(m);
    output <<
    "{"
        "\"fuzzer_name\":"              "\"kocoumat\","
        "\"fuzzed_program\":"           "\"" << FUZZED_PROG <<"\","
        "\"nb_runs\":"                   << statisticsExecution.count() << ","
        "\"nb_failed_runs\":"            << nb_failed_runs.load(std::memory_order_relaxed) << ","
        "\"nb_hanged_runs\":"            << nb_hanged_runs.load(std::memory_order_relaxed) << ","
        "\"execution_time\": {"
            "\"average\":"               << statisticsExecution.avg() << ","
            "\"median\":"                << statisticsExecution.median() << ","
            "\"min\":"                   << statisticsExecution.min() << ","
            "\"max\":"                   << statisticsExecution.max() <<
        "},"
        "\"nb_unique_failures\":"        << uniqueResults.size() << ","
        "\"minimization\": {"
            "\"before\":"                << nb_before_min.load(std::memory_order_relaxed) << ","
            "\"avg_steps\":"             << std::lround(statisticsMinimizationSteps.avg()) << ","
            "\"execution_time\": {"
                "\"average\":"           << statisticsMinimization.avg() << ","
                "\"median\":"            << statisticsMinimization.median() << ","
                "\"min\":"               << statisticsMinimization.min() << ","
                "\"max\":"               << statisticsMinimization.max() <<
            "}"
        "}"
    "}"
     ;
}

static void saveStatistics()
{
    auto path = RESULT_FUZZ / "stats.json";
    //std::cerr << "Saving statistics to " << path << std::endl;

    std::ofstream output(path);
    exportStatistics(output);

    std::cout << "Current stats: \n";
    exportStatistics(std::cout);
    std::cout << std::endl;

    if (!output)
        std::cerr << "Error saving statistics!" << std::endl;
    //else
      //  std::cerr << "Stats saved." << std::endl;
}

#ifndef _MSC_VER

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cerr << "Caught SIGINT (Ctrl+C). Exiting gracefully...\n";
        keepRunning = false;
    }
    else if (signal == SIGTERM) {
        std::cerr << "Caught SIGTERM. Exiting gracefully...\n";
        keepRunning = false;
    }

    if (!keepRunning)
    {
        saveStatistics();
        std::cerr << "Ready for exit." << std::endl;
    }
}

void setupSignalHandlers() {
    struct sigaction sa;
    sa.sa_handler = signalHandler; // Specify the handler function
    sa.sa_flags = 0;              // No special flags
    sigemptyset(&sa.sa_mask);     // Clear all signals from the mask during the handler

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        perror("Error setting up SIGINT handler");
        std::exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        perror("Error setting up SIGTERM handler");
        std::exit(EXIT_FAILURE);
    }
}
#else
#include <windows.h> 

BOOL WINAPI consoleHandler(DWORD signal) {

    if (signal == CTRL_C_EVENT)
    {
        keepRunning = false;
        return TRUE;
    }

    return FALSE;
}
#endif


void fuzz()
{
    constexpr size_t minSize = 1;
    constexpr size_t maxSize = 2048;
    constexpr std::chrono::milliseconds timeout = std::chrono::seconds(5);

    std::uniform_int_distribution<size_t> dist(minSize, maxSize);

    std::unique_ptr<ExecutionInput> executionInput;

    if (fuzzInputType == "stdin")
    {
        //std::cerr << "Using cin as input" << std::endl;
        executionInput = std::make_unique<CinInput>(FUZZED_PROG, timeout);
    }
    else
    {
        //std::cerr << "Using file as input" << std::endl;
        executionInput = std::make_unique<FileInput>(FUZZED_PROG, timeout, std::string(fuzzInputType));
    }

    std::stack<std::pair<std::string, ExecutionResult>> unprocessedErrors;
    
    while (keepRunning)
    {
        std::string input = (statisticsExecution.count() % 2 == 0) ? generateRandomString(dist(gen), 33, 126) : generateRandomNum(1, 1000000);
        //std::cerr << "generated random string:" << input << std::endl;
        //auto input = generateRandomNum(1, 1000000);

        executionInput->setInput(input);

        //auto start = std::chrono::high_resolution_clock::now();
        auto res = execute_with_timeout(*executionInput);
        //auto end = std::chrono::high_resolution_clock::now();

        //auto execution_time = std::chrono::duration_cast<std::chrono::duration<double,std::milli>>(end - start);
        statisticsExecution.addNumber(res.execution_time.count());

        if (!res.isOk())
        {
            unprocessedErrors.emplace(input, std::move(res));
        }

        while (!unprocessedErrors.empty() && keepRunning)
        {
            auto pop = std::move(unprocessedErrors.top());
            unprocessedErrors.pop();

            if (pop.first != input) // this was generated by random, not discovered while minimizing
                statisticsExecution.addNumber(pop.second.execution_time.count());

            CrashReport report;
            size_t errorCount;
            {
                auto err = detectError(pop.second);

                assert(err); //Should never be nullptr

                
                err->incrementCounter();

                {
                    std::unique_lock lock(m);
                    errorCount = uniqueResults.size();
                    for (size_t i = 0; i < uniqueResults.size(); i++)
                    {
                        if (*err == *uniqueResults[i])//Error already logged
                        {
                            //std::cerr << "This error was already found, nothing new" << std::endl;
                            goto alreadyFound;
                        }

                    }
                    //std::cerr << "Detected new error " << err->errorName() << " for input " << pop.first << std::endl;
                    uniqueResults.push_back(std::move(err));

                    if (uniqueResults.size() >= NB_KNOWN_BUGS)
                        keepRunning = false;
                }
            }

            if (pop.first == input) // this was generated by random, not discovered while minimizing
                nb_before_min.fetch_add(1, std::memory_order_relaxed);

            report.nb_steps = 0;
            report.unminimized_size = pop.first.size() + 1;

            report.execution_time = pop.second.execution_time;
            report.detectedError = uniqueResults[errorCount].get();

            report.minimization_time = std::chrono::milliseconds(0);

            if (MINIMIZE)
            {
                auto start = std::chrono::high_resolution_clock::now();
                report.input = minimizeInput(pop.first, *report.detectedError, *executionInput, report.nb_steps, unprocessedErrors);
                auto end = std::chrono::high_resolution_clock::now();

                report.minimization_time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - start);

                statisticsMinimization.addNumber(report.minimization_time.count());
                statisticsMinimizationSteps.addNumber(report.nb_steps);

                //std::cerr << "Minimized to: " << pop.first << std::endl;
            }
            else
                report.input = std::move(pop.first);


            saveReport(report, std::to_string(errorCount) + ".json", RESULT_FUZZ);

            alreadyFound:
            continue;
        }
    }
    std::cerr << "Exiting fuzzer" << std::endl;
}


int main(int argc, char* argv[])
{
    std::ios_base::sync_with_stdio(false);
    if (argc <= 6)
    {
        std::cerr << "Provide arguments" << std::endl;
        return 1;
    }

#ifndef _MSC_VER
    setupSignalHandlers();
#else
    if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        std::cerr << "ERROR: Could not set control handler" << std::endl;
        return 1;
    }
#endif

    FUZZED_PROG = argv[1];
    RESULT_FUZZ = argv[2];
    MINIMIZE = std::atoi(argv[3]) != 0;
    fuzzInputType = argv[4];
    TIMEOUT = std::chrono::seconds(std::atoi(argv[5]));
    NB_KNOWN_BUGS = std::atoi(argv[6]);

    std::cout << "Fuzzing program " << FUZZED_PROG << ", placing results into folder " << RESULT_FUZZ << ", minimize=" << MINIMIZE << ", type=" << fuzzInputType << ", timeout=" << TIMEOUT << ", known_bugs=" << NB_KNOWN_BUGS << std::endl;

    if (!std::filesystem::exists(FUZZED_PROG))
    {
        std::cerr << "Program to fuzz does not exist" << std::endl;
        return 1;
    }


    std::filesystem::create_directories(RESULT_FUZZ);
    std::filesystem::create_directories(RESULT_FUZZ / "crashes");
    std::filesystem::create_directories(RESULT_FUZZ / "hangs");

    std::atomic<bool> threadsRunning = true;
    std::jthread updateStats([&]() {
        while (threadsRunning)
        {
            saveStatistics();
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        std::cerr << "Program can end, writing one last statistics report and exiting..." << std::endl;
        saveStatistics();
        });
    {
        std::vector<std::jthread> threads;// (std::thread::hardware_concurrency(), std::jthread(&fuzz));
        auto threadCount = 1;// std::thread::hardware_concurrency();

        std::cerr << "Running " << threadCount << " fuzzers" << std::endl;

        threads.reserve(threadCount - 1);
        for (size_t i = 0; i < threadCount-1; i++)
            threads.emplace_back(fuzz);


        //std::jthread worker([&]() {
        //    std::this_thread::sleep_for(TIMEOUT); // Wait for the set time
        //    keepRunning = false;
        //    std::cerr << "Timeout reached, ending." << std::endl;
        //    });


        fuzz();

        //std::cin.get();
        //keepRunning = false;
    }
    threadsRunning = false;

	return 0;
}
