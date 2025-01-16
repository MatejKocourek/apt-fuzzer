#pragma once

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
#include <utility>
#include <set>

//std::unreachable();
#define UNREACHABLE assert(false)
//__builtin_unreachable()

//thread_local std::random_device rd;  // Seed for the random number engine
/*thread_local */std::mt19937 gen/*(rd())*/; // Mersenne Twister engine seeded with `rd`

static const std::regex errorTypeRegex("ERROR: AddressSanitizer: (.*) on address");
static const std::regex locationRegex("(main.c):(\\d+)");

namespace generators {
    std::string generateRandomAlphaNum(std::size_t size) {
        constexpr char alphaNumerical[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z' };
        std::uniform_int_distribution<int> dist(0, sizeof(alphaNumerical) - 1);

        std::string randomString;
        randomString.reserve(size);

        for (std::size_t i = 0; i < size; ++i) {
            char tmp = alphaNumerical[dist(gen)];

            randomString += tmp;
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
                while (tmp == '\\' || tmp == '"') // Trick to not having to escape in JSON
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
}


namespace mutants {
    void deleteBlock(std::string& str)
    {
        if (str.size() <= 1) [[unlikely]]
            return;

        std::exponential_distribution<double> distLen(1.0);
        int blockSize = 1 + lround(distLen(gen));

        if ((int)str.size() - blockSize <= 0) [[unlikely]]
            return;//Don't generate empty strings

        std::uniform_int_distribution<int> distStart(0, str.size() - 2);
        int start = distStart(gen);

        blockSize = std::min(blockSize, (int)str.size() - start);

        str.erase(start, blockSize);
    }
    void insertBlock(std::string& input)
    {
        std::exponential_distribution<double> distLen(1.0);
        size_t blockLen = 1 + round(distLen(gen));
        std::uniform_int_distribution<size_t> distStart(0, input.size());
        size_t blockStart = distStart(gen);

        input.insert(blockStart, generators::generateRandomString(blockLen,33,126));
    }
    void insertDigit(std::string& input)
    {
        std::uniform_int_distribution<size_t> distChar(0, 9);

        std::uniform_int_distribution<size_t> distStart(0, input.size());
        size_t blockStart = distStart(gen);

        input.insert(input.begin() + blockStart, (char)(distChar(gen) + '0'));
    }
    void duplicate(std::string& input)
    {
        input.insert(input.size(), input);
    }

    template<uint8_t maxBit>
    void flipBit(std::string& input)
    {
        std::uniform_int_distribution<size_t> distPos(0, input.size() - 1);
        std::uniform_int_distribution<int> distBit(0, maxBit-1);

        input[distPos(gen)] ^= (1 << distBit(gen));
    }
    void flipBitBIN(std::string& input)
    {
        return flipBit<8>(input);
    }
    void flipBitASCII(std::string& input)
    {
        return flipBit<7>(input);
    }
    void addASCII(std::string& input)
    {
        std::uniform_int_distribution<size_t> distPos(0, input.size() - 1);
        std::exponential_distribution<double> distVal(1);

        std::uniform_int_distribution<int> negative(0, 1);

        int val = 1 + round(distVal(gen));
        val *= 2 * negative(gen) - 1;

        input[distPos(gen)] += val;
        input[distPos(gen)] &= 0b01111111;
    }

    void randomMutant(std::string& input)
    {
        std::uniform_int_distribution<int> mutants(0, 5);
        switch (mutants(gen))
        {
        case 0:
            return deleteBlock(input);
        case 1:
            return insertBlock(input);
        case 2:
            return flipBitASCII(input);
        case 3:
            return addASCII(input);
        case 4:
            return insertDigit(input);
        case 5:
            return duplicate(input);

        default:
            UNREACHABLE;
        }
    }
    void randomNumberOfRandomMutants(std::string& input)
    {
        std::exponential_distribution<double> distVal(1);

        for (size_t i = 1 + round(distVal(gen)); i != 0; i--)
            randomMutant(input);
    }
}

struct fuzzer {
    std::atomic<size_t> nb_before_min = 0;
    std::atomic<size_t> nb_failed_runs = 0;
    std::atomic<size_t> nb_hanged_runs = 0;

    StatisticsMemory<double> statisticsExecution;
    StatisticsMemory<double> statisticsMinimization;
    StatisticsMemory<uint32_t> statisticsMinimizationSteps;

    std::atomic<bool> keepRunning = true;

    struct ExecutionResult {
        int return_code;
        std::string stdout_output;
        std::string stderr_output;
        bool timed_out;
        std::chrono::duration<double, std::milli> execution_time;

        bool operator==(const ExecutionResult&) const = default;
    };

    struct ExecutionInput
    {
        ExecutionInput(std::filesystem::path executablePath, std::chrono::milliseconds timeout) : executablePath(std::move(executablePath)), timeout(std::move(timeout)) {}

        virtual std::vector<std::string> getArguments() const = 0;
        virtual std::string_view getCin() const = 0;

        virtual void setInput(const std::string_view& input) = 0;

        const std::filesystem::path executablePath;
        const std::chrono::milliseconds timeout;
    };

    struct FileInput final : public ExecutionInput
    {
        FileInput(std::filesystem::path executablePath, std::chrono::milliseconds timeout, std::string path) : ExecutionInput(std::move(executablePath), std::move(timeout)), path(std::move(path)) {}

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
        CinInput(std::filesystem::path executablePath, std::chrono::milliseconds timeout) : ExecutionInput(std::move(executablePath), std::move(timeout)) {}

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

        ipstream stdout_stream;  // To capture standard output
        ipstream stderr_stream;  // To capture standard error
        opstream stdin_stream;   // To provide input

        //std::vector<std::string> arguments;

        //if (!executionInput.getArgument().empty())
        //    arguments.push_back(std::string(executionInput.getArgument()).c_str());

        auto start = std::chrono::high_resolution_clock::now();
        child process(executionInput.executablePath.c_str(), executionInput.getArguments(), std_out > stdout_stream, std_err > stderr_stream, std_in < stdin_stream);

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
            auto duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - start);
            statisticsExecution.addNumber(duration.count());
            nb_hanged_runs.fetch_add(1, std::memory_order_relaxed);
            return { -1, "", "", true, duration };  // Indicate a timeout occurred
        }

        auto duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - start);
        statisticsExecution.addNumber(duration.count());
        if (process.exit_code() != 0)
            nb_failed_runs.fetch_add(1, std::memory_order_relaxed);

        // Retrieve the outputs and return code
        return { std::move(process.exit_code()), std::move(stdout_future.get()), std::move(stderr_future.get()), false, duration };
    }

    struct DetectedError
    {
        virtual bool isErrorEncountered(const ExecutionResult& executionResult) const = 0;

        virtual const char* errorName() const = 0;

        virtual const char* folder() const = 0;

        virtual std::ostream& bugInfo(std::ostream& os) const = 0;

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

        const int returnCode;
    };

    struct TimeoutError final : public DetectedError
    {
        bool operator==(const TimeoutError& other) const
        {
            return true; //All timeouts are treated as equal
            // return timeout == other.timeout;
        }

        TimeoutError(std::chrono::duration<double, std::milli> timeout) : timeout(std::move(timeout)) {}
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

        std::chrono::duration<double, std::milli> timeout;
    };


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
                        else if (match[1] == "global-buffer-overflow")
                            asan = "global";
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

    static std::unique_ptr<DetectedError> detectError(const ExecutionResult& result)
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
        {
            auto tmp = ReturnCodeError::tryDetectError(result);
            if (tmp.has_value())
                return std::make_unique<ReturnCodeError>(std::move(*tmp));
        }

        return std::unique_ptr<DetectedError>();
    }

    std::string minimizeInput(const std::string_view& input, const DetectedError& prevResult, ExecutionInput& executionInput, size_t& totalRuns)
    {
        constexpr int divisionsStepStart = 2;
        int divisionStep = divisionsStepStart - 1;
        int prevStep = -1;

        while (true)
        {
            int step;

            do
            {
                step = input.length() / ++divisionStep;
            } while (step == prevStep);
            prevStep = step;

            if (step < 1)
                return std::string(input);

            //Step 1
            for (size_t i = 0; i < input.length(); i += step)
            {
                auto cropped = input.substr(i, step);
                executionInput.setInput(cropped);

                totalRuns += 1;
                auto result = execute_with_timeout(executionInput);
                if (prevResult.isErrorEncountered(result))
                {
                    return minimizeInput(cropped, prevResult, executionInput, totalRuns);
                }
                else
                {
                    dealWithResult(input, std::move(result), executionInput, true);// Minimization discovered a different bug, remember for later
                }
            }

            //Step 2
            for (size_t i = 0; i < input.length(); i += step)
            {
                std::string complement;
                complement.reserve(input.size() - step);
                complement += input.substr(0, i);
                if (i + step < input.length())
                    complement += input.substr(i + step);

                executionInput.setInput(complement);

                totalRuns += 1;
                auto result = execute_with_timeout(executionInput);
                if (prevResult.isErrorEncountered(result))
                {
                    return minimizeInput(complement, prevResult, executionInput, totalRuns);
                }
                else
                {
                    dealWithResult(input, std::move(result), executionInput, true);// Minimization discovered a different bug, remember for later
                }
            }

            //Step 3

            //If it arrives here, it means we can't minimize at this granularity
        }
    }



    struct CrashReport
    {
        std::string input;
        DetectedError* detectedError;
        std::chrono::duration<double, std::milli> execution_time;
        size_t unminimized_size;
        size_t nb_steps;
        std::chrono::duration<double, std::milli> minimization_time;
    };



    static void exportReport(const CrashReport& report, std::ostream& output)
    {
        output <<
            "{"
            "\"input\":"            "\"" << report.input << "\","
            "\"oracle\":"           "\"" << report.detectedError->errorName() << "\","
            "\"bug_info\":";                report.detectedError->bugInfo(output) << ","
            "\"execution_time\":" << report.execution_time.count() <<
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

        std::ofstream output(resultFile);

        exportReport(report, output);

        std::cout << "New error report: \n";
        exportReport(report, std::cout);
        std::cout << std::endl;

        if (!output)
            std::cerr << "Error saving report!" << std::endl;
    }

    const std::filesystem::path FUZZED_PROG;
    const std::filesystem::path RESULT_FUZZ;
    const bool MINIMIZE;
    const std::string_view fuzzInputType;
    const std::chrono::seconds TIMEOUT;
    const size_t NB_KNOWN_BUGS;


    std::mutex m;
    std::vector<std::unique_ptr<DetectedError>> uniqueResults;

    void exportStatisticsCommon(std::ostream& output)
    {
        std::lock_guard guard(m);
        output <<
            //"{"
                "\"fuzzer_name\":"              "\"kocoumat\","
                "\"fuzzed_program\":"           "\"" << FUZZED_PROG.string() << "\","
                "\"nb_runs\":" << statisticsExecution.count() << ","
                "\"nb_failed_runs\":" << nb_failed_runs.load(std::memory_order_relaxed) << ","
                "\"nb_hanged_runs\":" << nb_hanged_runs.load(std::memory_order_relaxed) << ","
                "\"execution_time\": {"
                    "\"average\":" << statisticsExecution.avg() << ","
                    "\"median\":" << statisticsExecution.median() << ","
                    "\"min\":" << statisticsExecution.min() << ","
                    "\"max\":" << statisticsExecution.max() <<
                "},"
                "\"nb_unique_failures\":" << uniqueResults.size() << ","
                "\"minimization\": {"
                    "\"before\":" << nb_before_min.load(std::memory_order_relaxed) << ","
                    "\"avg_steps\":" << std::lround(statisticsMinimizationSteps.avg()) << ","
                    "\"execution_time\": {"
                        "\"average\":" << statisticsMinimization.avg() << ","
                        "\"median\":" << statisticsMinimization.median() << ","
                        "\"min\":" << statisticsMinimization.min() << ","
                        "\"max\":" << statisticsMinimization.max() <<
                    "}"
                "}"
            //"}"
            ;
    }

    virtual void exportStatistics(std::ostream& output) = 0;

    void saveStatistics()
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

    DetectedError* dealWithResult(const std::string_view& input, ExecutionResult result, ExecutionInput& executionInput, bool fromMin)
    {
        if (!keepRunning)
            return nullptr;

        CrashReport report;
        size_t errorCount;
        {
            auto err = detectError(result);

            if (err == nullptr)
                return nullptr; //OK, no error

            {
                std::unique_lock lock(m);
                errorCount = uniqueResults.size();
                for (size_t i = 0; i < uniqueResults.size(); i++)
                {
                    if (*err == *uniqueResults[i])//Error already logged
                    {
                        //std::cerr << "This error was already found, nothing new" << std::endl;
                        return uniqueResults[i].get();
                    }

                }
                //std::cerr << "Detected new error " << err->errorName() << " for input " << pop.first << std::endl;
                uniqueResults.push_back(std::move(err));

                if (uniqueResults.size() >= NB_KNOWN_BUGS)
                    keepRunning = false;
            }
        }

        if (!fromMin) // this was generated by random, not discovered while minimizing
            nb_before_min.fetch_add(1, std::memory_order_relaxed);

        report.nb_steps = 0;
        report.unminimized_size = input.size();// + 1;

        report.execution_time = result.execution_time;
        report.detectedError = uniqueResults[errorCount].get();

        report.minimization_time = std::chrono::milliseconds(0);

        if (MINIMIZE)
        {
            auto start = std::chrono::high_resolution_clock::now();
            report.input = minimizeInput(input, *report.detectedError, executionInput, report.nb_steps);
            auto end = std::chrono::high_resolution_clock::now();

            report.minimization_time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end - start);

            statisticsMinimization.addNumber(report.minimization_time.count());
            statisticsMinimizationSteps.addNumber(report.nb_steps);
        }
        else
            report.input = input;


        saveReport(report, std::to_string(errorCount) + ".json", RESULT_FUZZ);
        return report.detectedError;
    }

    virtual void fuzz() = 0;


    std::unique_ptr<ExecutionInput> executionInput;

public:
    fuzzer(std::filesystem::path FUZZED_PROG, std::filesystem::path RESULT_FUZZ, bool MINIMIZE, std::string_view fuzzInputType, std::chrono::seconds TIMEOUT, size_t NB_KNOWN_BUGS) : FUZZED_PROG(FUZZED_PROG), RESULT_FUZZ(RESULT_FUZZ), MINIMIZE(MINIMIZE), fuzzInputType(fuzzInputType), TIMEOUT(TIMEOUT), NB_KNOWN_BUGS(NB_KNOWN_BUGS)//, minSize(minSize), maxSize(maxSize)
    {
        if (!std::filesystem::exists(this->FUZZED_PROG) || std::filesystem::is_directory(this->FUZZED_PROG))
            throw std::runtime_error("Program to fuzz does not exist");

        std::filesystem::create_directories(RESULT_FUZZ);
        std::filesystem::create_directories(RESULT_FUZZ / "crashes");
        std::filesystem::create_directories(RESULT_FUZZ / "hangs");

        constexpr std::chrono::milliseconds timeout = std::chrono::seconds(5);

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
    }

    void run()
    {
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

        //std::jthread worker([&]() {
        //    std::this_thread::sleep_for(TIMEOUT); // Wait for the set time
        //    keepRunning = false;
        //    std::cerr << "Timeout reached, ending." << std::endl;
        //    });

        {
            std::vector<std::jthread> threads;
            auto threadCount = 1;// std::thread::hardware_concurrency();

            std::cerr << "Running " << threadCount << " fuzzers" << std::endl;

            //threads.reserve(threadCount - 1);
            //for (size_t i = 0; i < threadCount - 1; i++)
            //    threads.emplace_back(fuzz, this);

            fuzz();
        }
        std::cerr << "All fuzzers done, ready to exit" << std::endl;
        threadsRunning = false;
    }

    void requestStop()
    {
        keepRunning = false;
    }


    static std::string generateRandomInput()
    {
        std::uniform_int_distribution<int> whichInput(0, 1);

        const size_t minSize = 1;
        const size_t maxSize = 1024;

        std::uniform_int_distribution<size_t> dist(minSize, maxSize);

        switch (whichInput(gen))
        {
        case 0:
            return generators::generateRandomString(dist(gen), 33, 126);
        case 1:
            return generators::generateRandomNum(1, 1000000);
        default:
            UNREACHABLE;
        }
    }
};

struct fuzzer_blackbox : public fuzzer
{
    fuzzer_blackbox(std::filesystem::path FUZZED_PROG, std::filesystem::path RESULT_FUZZ, bool MINIMIZE, std::string_view INPUT, std::chrono::seconds TIMEOUT, size_t NB_KNOWN_BUGS/*, size_t minSize = 1, size_t maxSize = 1024*/) : fuzzer(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(INPUT), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS))//, minSize(minSize), maxSize(maxSize)
    {

    }

    virtual void fuzz() override
    {
        while (keepRunning)
        {
            auto input = generateRandomInput();

            executionInput->setInput(input);

            auto res = execute_with_timeout(*executionInput);

            dealWithResult(input, std::move(res), *executionInput, false);
        }
        //std::cerr << "Exiting fuzzer" << std::endl;
    }

    virtual void exportStatistics(std::ostream& out) override
    {
        out << '{';
        exportStatisticsCommon(out);
        out << '}';
    }
};

static const std::regex linesFound("LF:(\\d+)");
static const std::regex linesHit("LH:(\\d+)");

static std::string loadFile(const std::filesystem::path& path)
{
    auto file = std::ifstream(path);

    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + path.string());

    std::string sourcecode = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

    if (file.fail())
        throw std::runtime_error("Error reading file: " + path.string());

    return sourcecode;
}

struct fuzzer_greybox : public fuzzer
{
    double powerSchedule(double time, size_t size, size_t nm, size_t nc, const std::string& hash)
    {
        switch (POWER_SCHEDULE)
        {
        case POWER_SCHEDULE_T::simple:
            return 1.0 / (time * size * nm / nc);
        case POWER_SCHEDULE_T::boosted:
            return 1.0 / std::pow(hashmap.at(hash), 5);
        default:
            UNREACHABLE;
        }
    }

    enum class POWER_SCHEDULE_T : uint8_t
    {
        simple,
        boosted
    };

    struct seed {
        seed(std::string&& input, const std::string& h, double T, double e, size_t nm = 1, size_t nc = 1) : input(std::move(input)), h(h), T(T), e(e), nm(nm), nc(nc)
        {

        }
        const std::string input;
        const std::string& h; //hash of the output
        //const double coverage; //coverage of the output
        double e; //energy

        const double T; // execution time
        //size_t s; // size in bytes
        size_t nm; // how many times it was already selected to be mutated
        size_t nc; // how many times it led to an increase in coverage

        bool operator< (const seed& other) const
        {
            return e > other.e;
        }
    };

    std::multiset<seed> queue;

    fuzzer_greybox(std::filesystem::path FUZZED_PROG, std::filesystem::path RESULT_FUZZ, bool MINIMIZE, std::string_view INPUT, std::chrono::seconds TIMEOUT, size_t NB_KNOWN_BUGS, POWER_SCHEDULE_T POWER_SCHEDULE, std::filesystem::path COVERAGE_FILE, std::filesystem::path INPUT_SEEDS) : fuzzer(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(INPUT), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS)), POWER_SCHEDULE(std::move(POWER_SCHEDULE)), COVERAGE_FILE(std::move(COVERAGE_FILE)), INPUT_SEEDS(std::move(INPUT_SEEDS))
    {

    }

    virtual void exportStatistics(std::ostream& out) override
    {
        out << '{';
        exportStatisticsCommon(out);
        out << ",\"nb_queued_seed\":" << queue.size() << ",";
        out << "\"coverage\":" << bestCoverage * 100 << ",";
        out << "\"nb_unique_hash\":" << hashmap.size();
        out << '}';
    }

    //double rankAFL(const seed& s)
    //{
    //    return s.T.count() * s.input.size() * s.nm / s.nc;
    //}
    //double rankExp(const seed& s)
    //{
    //    return 1.0 / std::pow(hashmap.at(*s.h), 5);
    //}

    //seed& weightedRandomChoiceAFL(std::vector<seed>& options)
    //{
    //    return weightedRandomChoice(options, rankAFL);
    //}
    //seed& weightedRandomChoiceExp(std::vector<seed>& options)
    //{
    //    return weightedRandomChoice(options, rankExp);
    //}

    double bestCoverage = 0;
    std::unordered_map<std::string, size_t> hashmap;


    static seed weightedRandomChoice(std::multiset<seed>& options, float percent) {
        // Calculate the total weight

        double totalWeight = 0;


        const size_t firstTenPercent = options.size() * percent;
        const double coefficientGood = 0.5 / firstTenPercent;
        const double coefficientWorse = 0.5 / (options.size() - firstTenPercent);

        size_t i = 0;
        for (auto it = options.begin(); it != options.end(); it++) {
            if (i <= firstTenPercent)
                totalWeight += coefficientGood;// totalWeight += it->e * coefficientGood; // Better score
            else
                totalWeight += coefficientWorse;// totalWeight += it->e * coefficientWorse; // Worse score
            i++;
        }

        
        std::uniform_real_distribution<> dis(0.0, totalWeight); // Range [0, totalWeight)

        // Generate a random number
        double randomValue = dis(gen);

        // Find the chosen option based on the random value
        double cumulativeWeight = 0.0;


        i = 0;
        for (auto it = options.begin(); it != options.end(); it++) {
            if (i <= firstTenPercent)
                cumulativeWeight += coefficientGood;// cumulativeWeight += it->e * coefficientGood; // Better score
            else
                cumulativeWeight += coefficientWorse;// cumulativeWeight += it->e * coefficientWorse; // Worse score

            if (randomValue <= cumulativeWeight) {
                return std::move(options.extract(it).value());
                //return option;
            }
            i++;
        }

        // If we get here, there was an issue (e.g., no options, weights not positive)
        throw std::runtime_error("Failed to select a weighted random choice.");
    }

    //static seed weightedRandomChoiceNormal(std::multiset<seed>& options) {
    //    // Calculate the total weight
    //    double totalWeight = 0.0;
    //    for (const auto& option : options) {
    //        totalWeight += option.e;
    //    }

    //    std::uniform_real_distribution<> dis(0.0, totalWeight); // Range [0, totalWeight)

    //    // Generate a random number
    //    double randomValue = dis(gen);

    //    // Find the chosen option based on the random value
    //    double cumulativeWeight = 0.0;

    //    size_t i = 0;
    //    for (auto it = options.begin(); it != options.end(); it++) {
    //        cumulativeWeight += it->e;

    //        if (randomValue <= cumulativeWeight) {
    //            return std::move(options.extract(it).value());
    //            //return option;
    //        }
    //        i++;
    //    }

    //    // If we get here, there was an issue (e.g., no options, weights not positive)
    //    throw std::runtime_error("Failed to select a weighted random choice.");
    //}

    static double coverage(const std::string& lcov)
    {
        size_t covered;
        size_t total;

        std::smatch match;
        if (!std::regex_search(lcov, match, linesHit))
            throw std::runtime_error("LCOV file not valid");

        covered = std::stoull(match[1]);

        if (!std::regex_search(lcov, match, linesFound))
            throw std::runtime_error("LCOV file not valid");

        total = std::stoull(match[1]);

        return static_cast<double>(covered) / total;
    }


    void fuzz()
    {
        // Run for initial seeds without mutating
        std::cerr << "Executing initial seeds..." << std::endl;
        for (const auto& i : std::filesystem::directory_iterator(INPUT_SEEDS))
        {
            if (i.is_regular_file())
            {
                // Run directly on this seed
                auto input = loadFile(i.path());
                executionInput->setInput(input);


                auto res = execute_with_timeout(*executionInput);
                
                auto error = dealWithResult(input, std::move(res), *executionInput, false);

                const std::string* executedCoverageOutput;

                if (error)
                {
                    // Use bug info as a hash
                    std::stringstream ss;
                    error->bugInfo(ss);
                    auto it = hashmap.emplace(ss.str(), 0);
                    it.first->second++;;
                    executedCoverageOutput = &it.first->first;
                }
                else
                {
                    auto lcov = loadFile(COVERAGE_FILE);
                    auto executedCoverage = coverage(lcov);

                    if (executedCoverage > bestCoverage)
                        bestCoverage = executedCoverage;

                    auto it = hashmap.emplace(std::move(lcov), 0);
                    it.first->second++;;
                    executedCoverageOutput = &it.first->first;
                }

                queue.emplace(std::move(input), *executedCoverageOutput, res.execution_time.count(), powerSchedule(res.execution_time.count(), input.size(), 1, 1, *executedCoverageOutput));
            }
        }
        std::cerr << "Loaded " << queue.size() << " seeds." << std::endl;
        std::cerr << "Mutating..." << std::endl;
        while (keepRunning)
        {
            float coefficient;
            switch (POWER_SCHEDULE)
            {
            case POWER_SCHEDULE_T::simple:
                coefficient = 0.1;
                break;
            case POWER_SCHEDULE_T::boosted:
                coefficient = 0;
                break;
            default:
                UNREACHABLE;
            }
            auto selected = weightedRandomChoice(queue, coefficient);

            //std::cerr << "Selected: " << std::endl << selected.input << std::endl;

            selected.nm++;

            std::string mutant = selected.input;

            mutants::randomNumberOfRandomMutants(mutant);

            //std::cerr << "Mutated to: " << std::endl << mutoString << std::endl;

            executionInput->setInput(mutant);

            auto res = execute_with_timeout(*executionInput);
            
            auto error = dealWithResult(mutant, res, *executionInput, false);

            if (error)
            {
                // Use bug info as a hash
                std::stringstream ss;
                error->bugInfo(ss);
                auto it = hashmap.emplace(ss.str(), 0);
                it.first->second++;;
                const auto& executedCoverageOutput = it.first->first;

                //selected.nc++;

                //Add new interesting seed (crashing)
                queue.emplace(std::move(mutant), executedCoverageOutput, res.execution_time.count(), powerSchedule(res.execution_time.count(), mutant.size(), 2, 2, executedCoverageOutput), 2, 2);
            }
            else
            {
                auto lcov = loadFile(COVERAGE_FILE);

                auto executedCoverage = coverage(lcov);

                auto it = hashmap.emplace(std::move(lcov), 0);
                it.first->second++;;
                const auto& executedCoverageOutput = it.first->first;

                if ((executedCoverage > bestCoverage) || (executedCoverage > bestCoverage && executedCoverageOutput != selected.h))
                {
                    bestCoverage = executedCoverage;
                    selected.nc++;

                    if (executedCoverage > bestCoverage)
                        std::cerr << "Just improved coverage! nb_runs=" <<statisticsExecution.count() << std::endl;
                    //Add new interesting seed (higher coverage)
                    queue.emplace(std::move(mutant), executedCoverageOutput, res.execution_time.count(), powerSchedule(res.execution_time.count(), mutant.size(), 1, 1, executedCoverageOutput));
                    //queue.emplace(std::move(mutant), executedCoverageOutput, 1);
                }
                //else if (executedCoverage == bestCoverage && executedCoverageOutput != selected.h)
                //{
                //    selected.nc++;

                //    //Add new interesting seed (different result)
                //    queue.emplace(std::move(mutant), executedCoverageOutput, res.execution_time.count(), powerSchedule(res.execution_time.count(), mutant.size(), 1, 1, executedCoverageOutput));
                //}
            }

            selected.e = powerSchedule(selected.T, selected.input.size(), selected.nm, selected.nc, selected.h);

            // Return original seed back
            queue.insert(std::move(selected));
        }
    }

    void populateWithMySeeds()
    {
        std::cerr << "Populating folder with my seeds" << std::endl;
        std::filesystem::create_directories(INPUT_SEEDS);

        for (size_t i = 0; i < 100; i++)
        {
            std::filesystem::path path = INPUT_SEEDS / (std::to_string(i) + ".txt");
            std::ofstream outFile(path);
            outFile << generateRandomInput();
        }
    }

    fuzzer_greybox(std::filesystem::path FUZZED_PROG, std::filesystem::path RESULT_FUZZ, bool MINIMIZE, std::string_view INPUT, std::chrono::seconds TIMEOUT, size_t NB_KNOWN_BUGS, POWER_SCHEDULE_T POWER_SCHEDULE, std::filesystem::path COVERAGE_FILE) : fuzzer_greybox(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(INPUT), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS), std::move(POWER_SCHEDULE), std::move(COVERAGE_FILE), "MY_SEED")
    {
        populateWithMySeeds();
    }

    const POWER_SCHEDULE_T POWER_SCHEDULE;
    const std::filesystem::path INPUT_SEEDS;
    const std::filesystem::path COVERAGE_FILE;
};