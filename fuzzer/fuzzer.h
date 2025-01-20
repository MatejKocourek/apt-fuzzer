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
#include <charconv>
#include <iterator>

// Undefine to capture stdout from running progarm
//#define CAPTURE_STDOUT

#ifdef _MSC_VER
#define UNREACHABLE __assume(0)
#else
#define UNREACHABLE __builtin_unreachable()
#endif

//thread_local std::random_device rd;  // Seed for the random number engine
/*thread_local */std::mt19937 gen/*(rd())*/; // Mersenne Twister engine seeded with `rd`


/// <summary>
/// All random string generators
/// </summary>
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
    bool randomBool()
    {
        std::uniform_int_distribution<int> res(0, 1);
        return (bool)res(gen);
    }
    float randomFloat()
    {
        std::uniform_real_distribution<float> res(0, 1);
        return res(gen);
    }
    int randomInt(int max)
    {
        std::uniform_int_distribution<int> res(0, max-1);
        return res(gen);
    }
    char randomASCII()
    {
        std::uniform_int_distribution<int> res(32, 126);
        return res(gen);
    }
    char randomDigit()
    {
        std::uniform_int_distribution<int> res('0', '9');
        return res(gen);
    }

    /// <summary>
    /// Provides random seeds for fuzzers. Sometimes generates string, sometimes numbers.
    /// </summary>
    /// <returns></returns>
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
}

/// <summary>
/// Escape a character as a unicode symbol (for unsupported characters)
/// </summary>
/// <param name="out">Output stream to escape to</param>
/// <param name="c">Char to escape</param>
/// <returns>Output stream from argument</returns>
static std::ostream& escapeUnicode(std::ostream& out, char c)
{
    // save default formatting
    std::ios init(NULL);
    init.copyfmt(out);

    out << "\\u00"
        << std::hex << std::uppercase // Use uppercase for letters in hex
        << std::setw(2) << std::setfill('0') // Ensures 2-digit output
        << static_cast<int>(static_cast<unsigned char>(c));

    out.copyfmt(init);

    return out;
}

/// <summary>
/// Escape a char to JSON format
/// </summary>
/// <param name="out">Output stream to escape to</param>
/// <param name="c">Character to escape</param>
/// <returns>Output stream from argument</returns>
static std::ostream& escape(std::ostream& out, char c)
{
    unsigned char uchar = static_cast<unsigned char>(c);
    if (uchar > 126 || uchar < 8 || (uchar < 32 && uchar > 13))
        return escapeUnicode(out, c);

    switch (c)
    {
    case '\v':
        escapeUnicode(out, c);
        break;
    case '\b':
        out << "\\b";
        break;
    case '\t':
        out << "\\t";
        break;
    case '\n':
        out << "\\n";
        break;
    case '\f':
        out << "\\f";
        break;
    case '\r':
        out << "\\r";
        break;
    case '\\':
    case '"':
        out << '\\';
        [[fallthrough]];
    default:
        out << c;
        break;
    }
    return out;
}

/// <summary>
/// Escape a string to JSON format
/// </summary>
/// <param name="out">Output stream to escape to</param>
/// <param name="str">String to escape</param>
/// <returns>Output stream from argument</returns>
static std::ostream& escape(std::ostream& out, const std::string_view& str)
{
    for (const auto& c : str)
    {
        escape(out, c);
    }
    return out;
}

bool isJsonAllowedOrEscapeable(char c)
{
    unsigned char uchar = static_cast<unsigned char>(c);
    return !(uchar > 126 || uchar < 8 || uchar == 11 || (uchar < 32 && uchar > 13));
}

/// <summary>
/// Mutators that can change existing strings
/// </summary>
namespace mutators {
    /// <summary>
    /// Deletes block of random size from random location at the string
    /// </summary>
    void deleteBlock(std::string& str)
    {
        if (str.size() <= 1) [[unlikely]]
            return;

        std::exponential_distribution<float> distLen(1.0);
        int blockSize = 1 + round(distLen(gen));

        if ((int)str.size() - blockSize <= 0) [[unlikely]]
            return;//Don't generate empty strings

        std::uniform_int_distribution<int> distStart(0, str.size() - 2);
        int start = distStart(gen);

        blockSize = std::min(blockSize, (int)str.size() - start);

        str.erase(start, blockSize);
    }

    /// <summary>
    /// Inserts new block of random size to random location at the string
    /// </summary>
    void insertBlock(std::string& input)
    {
        std::exponential_distribution<float> distLen(1.0);
        size_t blockLen = 1 + round(distLen(gen));
        std::uniform_int_distribution<size_t> distStart(0, input.size());
        size_t blockStart = distStart(gen);

        input.insert(blockStart, generators::generateRandomString(blockLen, 32, 126));
    }

    /// <summary>
    /// Insert random digit somewhere random in the string
    /// </summary>
    void insertDigit(std::string& input)
    {
        std::uniform_int_distribution<int> distChar(0, 9);

        std::uniform_int_distribution<size_t> distStart(0, input.size());
        size_t blockStart = distStart(gen);

        input.insert(input.begin() + blockStart, (char)(distChar(gen) + '0'));
    }

    /// <summary>
    /// Insert '\n' somewhere random in the string
    /// </summary>
    void insertNewline(std::string& input)
    {
        std::uniform_int_distribution<size_t> distStart(0, input.size());
        size_t blockStart = distStart(gen);

        input.insert(input.begin() + blockStart, '\n');
    }

    /// <summary>
    /// Insert '\n' at the end of the string
    /// </summary>
    void appendNewline(std::string& input)
    {
        input += '\n';
    }

    /// <summary>
    /// Join two strings together
    /// </summary>
    /// <param name="input">First string, will increase in size</param>
    /// <param name="input2">Second string, will be added to the first one</param>
    void concat(std::string& input, const std::string& input2)
    {
        input.reserve(input.size() + input2.size());
        input += input2;
    }

    /// <summary>
    /// If seed is a number, slightly change it
    /// </summary>
    void changeNum(std::string& input)
    {
        if (input.size() >= 19) // Cannot work with too long strings
            return;

        for (const auto& c : input)
        {
            if (!isdigit(c)) // Only change strings that are fully numeric
                return;
        }
        auto num = std::stoll(input);

        std::exponential_distribution<float> distLen(0.25);

        std::uniform_int_distribution<int> negative(0, 1);

        int val = 1 + round(distLen(gen));
        val *= 2 * negative(gen) - 1;

        num += val;

        input = std::to_string(num);
    }

    /// <summary>
    /// Flip random bit in random byte in a string, so that it remains an ASCII character
    /// </summary>
    void flipBitASCII(std::string& input)
    {
        std::uniform_int_distribution<size_t> distPos(0, input.size() - 1);
        std::uniform_int_distribution<int> distBit(0, 6);

        input[distPos(gen)] ^= (1 << distBit(gen));

        if (input[distPos(gen)] < 32)
            input[distPos(gen)] += 32;
    }

    /// <summary>
    /// Add random number to random byte in a string, so that it remains an ASCII character
    /// </summary>
    void addASCII(std::string& input)
    {
        std::uniform_int_distribution<size_t> distPos(0, input.size() - 1);
        std::exponential_distribution<float> distVal(1);

        std::uniform_int_distribution<int> negative(0, 1);

        int val = 1 + round(distVal(gen));
        val *= 2 * negative(gen) - 1;

        input[distPos(gen)] += val;
        input[distPos(gen)] &= 0b01111111;

        if (input[distPos(gen)] < 32)
            input[distPos(gen)] += 32;
    }

    /// <summary>
    /// Selects random mutator and applies the mutation
    /// </summary>
    /// <param name="input1">Mutation will be applied to this string</param>
    /// <param name="input2">Some mutators will use this second string to read from</param>
    void randomMutant(std::string& input)
    {
        switch (generators::randomInt(6))
        {
        case 0:
            return deleteBlock(input);
        case 1:
            return insertBlock(input);
        case 2:
            return changeNum(input);
        case 3:
            return insertDigit(input);
        case 4:
            return addASCII(input);
        case 5:
            return flipBitASCII(input);
        default:
            UNREACHABLE;
        }
    }

    /// <summary>
    /// Perform several random mutations
    /// </summary>
    /// <param name="input1">String to mutate</param>
    void randomNumberOfRandomMutants(std::string& input)
    {
        std::exponential_distribution<float> distVal(1);

        for (size_t i = 1 + round(distVal(gen)); i != 0; i--)
            randomMutant(input);
    }
}

static size_t currentAsanOffset = 0; // Workaround for coverage tool skewing the line numbers
static const std::regex errorTypeRegex("ERROR: AddressSanitizer: (.*) on address");
static const std::regex locationRegex("(main.c):(\\d+)");

/// <summary>
/// Represents the base class for black/grey box fuzzing
/// </summary>
struct fuzzer {
    std::atomic<size_t> nb_before_min = 0;
    std::atomic<size_t> nb_failed_runs = 0;
    std::atomic<size_t> nb_hanged_runs = 0;

    StatisticsMemory<double> statisticsExecution;
    StatisticsMemory<double> statisticsMinimization;
    StatisticsMemory<uint32_t> statisticsMinimizationSteps;

    /// <summary>
    /// This bool will be turned to false when fuzzer should terminate
    /// </summary>
    std::atomic<bool> keepRunning = true;

    /// <summary>
    /// Result of one execution
    /// </summary>
    struct ExecutionResult {
        int return_code;
#ifdef CAPTURE_STDOUT
        std::string stdout_output;
#endif
        std::string stderr_output;
        bool timed_out;
        std::chrono::duration<double, std::milli> execution_time;

        bool operator==(const ExecutionResult&) const = default;
    };

    /// <summary>
    /// Base class for execution inputs
    /// </summary>
    struct ExecutionInput
    {
        ExecutionInput(std::filesystem::path executablePath, std::chrono::milliseconds timeout) : executablePath(std::move(executablePath)), timeout(std::move(timeout)) {}

        virtual std::vector<std::string> getArguments() const = 0;
        virtual std::string_view getCin() const = 0;

        virtual void setInput(const std::string_view& input) = 0;

        const std::filesystem::path executablePath;
        const std::chrono::milliseconds timeout;

        virtual ~ExecutionInput() = default;
    };

    /// <summary>
    /// Execution input in form of a file content
    /// </summary>
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

        virtual ~FileInput() = default;
    private:
        const std::string path;
    };

    /// <summary>
    /// Execution input in form of a standard input
    /// </summary>
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

        virtual ~CinInput() = default;

    private:
        std::string_view cinInput;
    };

    /// <summary>
    /// Execute program in the system with a timeout and return its results
    /// </summary>
    /// <param name="executionInput">What to execute and how</param>
    /// <returns>Result of the executions</returns>
    ExecutionResult execute_with_timeout(const ExecutionInput& executionInput) {
        using namespace boost::process;

#ifdef CAPTURE_STDOUT
        ipstream stdout_stream;  // To capture standard output
#endif
        ipstream stderr_stream;  // To capture standard error
        opstream stdin_stream;   // To provide input

        auto start = std::chrono::high_resolution_clock::now();
        child process(
            executionInput.executablePath.c_str(),
            executionInput.getArguments(),
#ifdef CAPTURE_STDOUT
            std_out > stdout_stream,
#else
            std_out > boost::process::null,
#endif
            std_err > stderr_stream,
            std_in < stdin_stream
        );

        // Feed the process's standard input.
        stdin_stream << executionInput.getCin();
        stdin_stream.flush();
        stdin_stream.close();
        stdin_stream.pipe().close();  // Close stdin to signal end of input

        // Read all content from ipstream after the process finishes
        auto read_stream = [](ipstream& stream) {
            std::ostringstream oss;
            oss << stream.rdbuf(); // Efficiently reads the entire stream buffer
            return oss.str();
        };
        
        // Wait for process completion with a timeout.
        bool finished_in_time = process.wait_for(executionInput.timeout);
        if (!finished_in_time) {
            process.terminate();  // Kill the process if it times out
            auto duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - start);
            statisticsExecution.addNumber(duration.count());
            nb_hanged_runs.fetch_add(1, std::memory_order_relaxed);
            return {
                -1,
#ifdef CAPTURE_STDOUT
                read_stream(stdout_stream),
#endif
                read_stream(stderr_stream),
                true,
                duration 
           };  // Indicate a timeout occurred
        }

        auto duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - start);
        statisticsExecution.addNumber(duration.count());
        if (process.exit_code() != 0)
            nb_failed_runs.fetch_add(1, std::memory_order_relaxed);

        // Retrieve the outputs and return code
        return { 
            std::move(process.exit_code()),
#ifdef CAPTURE_STDOUT
            read_stream(stdout_stream),
#endif
            read_stream(stderr_stream),
            false,
            duration
        };
    }

    /// <summary>
    /// Base class for various errors that the oracle catches
    /// </summary>
    struct DetectedError
    {
        /// <summary>
        /// Checks whether this error was present in the output
        /// </summary>
        virtual bool isErrorEncountered(const ExecutionResult& executionResult) const = 0;

        /// <summary>
        /// Name of the oracle, as it should be in JSON export
        /// </summary>
        virtual const char* errorName() const = 0;

        /// <summary>
        /// Folder that the oracle should export to
        /// </summary>
        virtual const char* folder() const = 0;

        /// <summary>
        /// Export info about this bug to a stream
        /// </summary>
        virtual std::ostream& bugInfo(std::ostream& os) const = 0;

        /// <summary>
        /// Compares if two errors are the same for deduplication purposes
        /// </summary>
        virtual bool operator == (const DetectedError& Other) const = 0;

        virtual ~DetectedError() = default;
    };

    /// <summary>
    /// Oracle that catches a non-zero return code
    /// </summary>
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

        virtual bool isErrorEncountered(const ExecutionResult& executionResult) const override
        {
            auto tmp = tryDetectError(executionResult);
            if (tmp.has_value() && ((*tmp) == (*this)))
                return true;

            return false;
        }

        virtual const char* errorName() const override
        {
            return "return_code";
        }

        virtual const char* folder() const override
        {
            return "crashes";
        }

        virtual std::ostream& bugInfo(std::ostream& os) const override
        {
            os << returnCode;
            return os;
        }

        virtual ~ReturnCodeError() = default;

        const int returnCode;
    };

    /// <summary>
    /// Oracle that catches hangs of the program
    /// </summary>
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

        virtual ~TimeoutError() = default;

        std::chrono::duration<double, std::milli> timeout;
    };

    virtual size_t asanOffset() const = 0;

    /// <summary>
    /// Oracle that catches address sanitizer errors
    /// </summary>
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

            if (executionResult.return_code == 1)
            {
                std::smatch match;
                if (std::regex_search(executionResult.stderr_output, match, errorTypeRegex))
                {
                    //std::cerr << "Caught ASAN " << match[1] << " in " << std::endl << executionResult.stderr_output << std::endl;

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

                        res.emplace(std::move(asan), match2[1], std::to_string(std::stoi(match2[2]) - currentAsanOffset));
                    }
                }

            }

            return res;
        }

        virtual bool isErrorEncountered(const ExecutionResult& executionResult) const override
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

        virtual ~AddressSanitizerError() = default;
    };

    /// <summary>
    /// Catch error from runner output
    /// </summary>
    /// <param name="result">Output from the runner</param>
    /// <returns>Error present in this output. Nullptr if no error present.</returns>
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

    /// <summary>
    /// Minimize input in a way that the same error still persists, but the input is shortest as possible
    /// </summary>
    /// <param name="input">Unminimized input string that throws error</param>
    /// <param name="prevResult">Error that should be hit</param>
    /// <param name="executionInput">Where to test all the inputs</param>
    /// <param name="totalRuns">How many times it runs (statistic purposes)</param>
    /// <returns>Minimized string</returns>
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

    /// <summary>
    /// JSON crash report
    /// </summary>
    struct CrashReport
    {
        std::string input;
        DetectedError* detectedError;
        std::chrono::duration<double, std::milli> execution_time;
        size_t unminimized_size;
        size_t nb_steps;
        std::chrono::duration<double, std::milli> minimization_time;
    };

    /// <summary>
    /// Export information same for all fuzzers about a crash into a stream
    /// </summary>
    static void exportReportCommon(const CrashReport& report, std::ostream& output)
    {
        output <<

            "\"input\":"            "\""  ; escape(output, report.input) << "\","
            "\"oracle\":"           "\"" << report.detectedError->errorName() << "\","
            "\"bug_info\":";                report.detectedError->bugInfo(output) << ","
            "\"execution_time\":" << report.execution_time.count() <<
            ",\"minimization\":{"
            "\"unminimized_size\":" << report.unminimized_size << ","
            "\"nb_steps\":" << report.nb_steps << ","
            "\"execution_time\":" << report.minimization_time.count() << ""
            "}"
            ;
    }

    /// <summary>
    /// Export information about the report into a stream
    /// </summary>
    virtual void exportReport(const CrashReport& report, std::ostream& output) const = 0;

    /// <summary>
    /// Save the report about an error into a file
    /// </summary>
    /// <param name="report">Report to save</param>
    /// <param name="name">Name of the file to save to</param>
    /// <param name="resultFolder">Where to save it</param>
    void saveReport(const CrashReport& report, const std::string& name, const std::filesystem::path& resultFolder)
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

    /// <summary>
    /// Export statistics same for both fuzzers into a stream
    /// </summary>
    void exportStatisticsCommon(std::ostream& output)
    {
        std::lock_guard guard(m);
        output <<
            //"{"
                "\"fuzzer_name\":"              "\"kocoumat\","
                "\"fuzzed_program\":"           "\"" ; escape(output, FUZZED_PROG.string())  << "\","
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

    /// <summary>
    /// Export all statistics into a stream
    /// </summary>
    /// <param name="output"></param>
    virtual void exportStatistics(std::ostream& output) = 0;

    /// <summary>
    /// Save stats into a file
    /// </summary>
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
    }

    /// <summary>
    /// Checks whether error is present in runner output and do appropriate actions with it
    /// </summary>
    /// <param name="input">Input string that was ran by the runner</param>
    /// <param name="result">Result of the runner</param>
    /// <param name="executionInput">Where to test inputs</param>
    /// <param name="fromMin">Is this random or from minimization</param>
    /// <returns>Pointer to erorr if occured. Owned by result vector. Do not free!</returns>
    DetectedError* dealWithResult(const std::string_view& input, ExecutionResult result, ExecutionInput& executionInput, bool fromMin = false)
    {
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
        report.unminimized_size = input.size();

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

    /// <summary>
    /// Run the fuzzer (blocking call)
    /// </summary>
    void run()
    {
        currentAsanOffset = asanOffset();

        std::atomic<bool> threadsRunning = true;

        // Thread for updating the stats in real-time
        std::jthread updateStats([&]() {
            uint8_t counter = 0;
            while (threadsRunning)
            {
                if (counter++ == 10)
                {
                    counter = 0;
                    saveStatistics();
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cerr << "Program can end, writing one last statistics report and exiting..." << std::endl;
            saveStatistics();
            });

        // Thread for terminating the process if it runs for too long
        std::jthread timeoutThread([&]() {
            const auto start = std::chrono::high_resolution_clock::now();
            const auto timeout = TIMEOUT - std::chrono::seconds(1);
            while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start) < timeout && keepRunning)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for the set time
            }
            
            keepRunning = false;
            std::cerr << "Timeout reached or everything found, ending." << std::endl;
            });

        try
        {
            // Run the actual fuzzing. Ready for multiple threads of execution, but not implemented.

            std::vector<std::jthread> threads;
            auto threadCount = 1;// std::thread::hardware_concurrency();

            std::cerr << "Running " << threadCount << " fuzzers" << std::endl;

            //threads.reserve(threadCount - 1);
            //for (size_t i = 0; i < threadCount - 1; i++)
            //    threads.emplace_back(fuzz, this);

            fuzz();
        }
        catch (const std::exception& e)
        {
            std::cerr << "ERROR: " << e.what() << std::endl;
        }

        std::cerr << "All fuzzers done, ready to exit" << std::endl;
        threadsRunning = false;
    }

    void requestStop()
    {
        keepRunning = false;
    }

};

/// <summary>
/// Fuzzer that does not have any information about the file it is fuzzing
/// </summary>
struct fuzzer_blackbox : public fuzzer
{
    /// <summary>
    /// Fuzzer that does not have any information about the file it is fuzzing
    /// </summary>
    /// <param name="FUZZED_PROG">Path to the executable the fuzzer will be running</param>
    /// <param name="RESULT_FUZZ">Path to a folder where crashes will be saved</param>
    /// <param name="MINIMIZE">If the fuzzer should minimize crashing inputs</param>
    /// <param name="INPUT">Input method for given executable. "stdin" if through standard input, file name otherwise.</param>
    /// <param name="TIMEOUT">Time passed before the fuzzer is stopped</param>
    /// <param name="NB_KNOWN_BUGS">After the fuzzer finds this many bugs, it terminates.</param>
    fuzzer_blackbox(std::filesystem::path FUZZED_PROG, std::filesystem::path RESULT_FUZZ, bool MINIMIZE, std::string_view INPUT, std::chrono::seconds TIMEOUT, size_t NB_KNOWN_BUGS/*, size_t minSize = 1, size_t maxSize = 1024*/) : fuzzer(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(INPUT), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS))//, minSize(minSize), maxSize(maxSize)
    {

    }

    virtual size_t asanOffset() const override
    {
        return 0;
    }

    virtual void fuzz() override
    {
        while (keepRunning)
        {
            auto input = generators::generateRandomInput();

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

    virtual void exportReport(const CrashReport& report, std::ostream& out) const override
    {
        out << '{';
        exportReportCommon(report, out);
        out << '}';
    }
};

/// <summary>
/// Load the whole file into a string
/// </summary>
/// <param name="path">Path to the file</param>
/// <returns>Contents of given file</returns>
static std::string loadFile(const std::filesystem::path& path)
{
    auto file = std::ifstream(path);

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

/// <summary>
/// Fuzzer that works with source code of fuzzing file
/// </summary>
struct fuzzer_greybox : public fuzzer
{
    typedef std::vector<bool> coveragePath;

    virtual size_t asanOffset() const override
    {
        return 4; // Our tool has this offset
    }

    enum class POWER_SCHEDULE_T : uint8_t
    {
        simple,
        boosted
    };

    /// <summary>
    /// Seed that fuzzer keeps in queue
    /// </summary>
    struct seed
    {
        seed(std::string input) : input(std::move(input)) {};
        const std::string input;

        /// <summary>
        /// Increment the counter for this seed counting how many times it was selected (if needed)
        /// </summary>
        virtual void incrementSelected() = 0;
        /// <summary>
        /// Increment the counter for this seed counting how many times it improved the coverage (if needed)
        /// </summary>
        virtual void incrementImproved() = 0;
        /// <summary>
        /// Update the energy of this seed from set information.
        /// </summary>
        virtual void update() = 0;
        virtual ~seed() = default;
    };

    struct seedSimple : public seed
    {
        double e; //energy

        const double T; // execution time
        size_t nm; // how many times it was already selected to be mutated
        size_t nc; // how many times it led to an increase in coverage

        /// <summary>
        /// Seed for the simple power method
        /// </summary>
        /// <param name="input">String that should be associated with this seed</param>
        /// <param name="T">Time it took to execute this seed</param>
        /// <param name="nm">How many times it was already selected to be mutated</param>
        /// <param name="nc">How many times it led to an increase in coverage</param>
        seedSimple(std::string input, double T, size_t nm = 1, size_t nc = 1) : seed(std::move(input)), T(T), nm(nm), nc(nc)
        {
            e = power();
        }

        bool operator< (const seedSimple& other) const
        {
            return e > other.e;
        }

        double power() const
        {
            return 1.0 / (T * input.size() * nm / nc);
        }

        virtual void incrementSelected() override final
        {
            nm++;
        }
        virtual void incrementImproved() override final
        {
            nc++;
        }
        virtual void update() override final
        {
            e = power();
        }
        virtual ~seedSimple() = default;
    };

    struct seedBoosted : public seed
    {
        const coveragePath& h; //hash of the output

        /// <summary>
        /// Seed for the boosted power method
        /// </summary>
        /// <param name="input">String that should be associated with this seed</param>
        /// <param name="h">Path that is executed when this seed is run</param>
        seedBoosted(std::string input, const coveragePath& h) : seed(std::move(input)), h(h)
        {
        }

        double power(const std::unordered_map<coveragePath, size_t>& hashmap) const
        {
            return 1.0 / std::pow(hashmap.at(h), 5);
        }

        virtual void incrementSelected() override final
        {
            // Do nothing
        }

        virtual void incrementImproved() override final
        {
            // Do nothing
        }

        virtual void update() override final
        {
            // Do nothing (will be done when selecting)
        }
        virtual ~seedBoosted() = default;
    };

    struct powerStructure
    {
        /// <summary>
        /// Add new seed into the queue
        /// </summary>
        /// <param name="input">String that was fuzzed and should be recorded</param>
        /// <param name="h">Associated output path from the program</param>
        /// <param name="T">Runtime for this input</param>
        /// <param name="nm">How many times it was selected</param>
        /// <param name="nc">How many times it led to increased coverage</param>
        virtual void add(std::string input, const coveragePath& h, double T, size_t nm = 1, size_t nc = 1) = 0;

        /// <summary>
        /// Size of the queue
        /// </summary>
        virtual size_t size() const = 0;

        /// <summary>
        /// Retrives seed at given index
        /// </summary>
        /// <param name="n">Index</param>
        /// <returns>Constant reference to the element</returns>
        virtual const seed& at(size_t n) = 0;

        /// <summary>
        /// Perform a weighted random choice and borrow the seed, expecting to return it
        /// </summary>
        /// <returns>Reference to the selected seed</returns>
        virtual seed& weightedRandomChoiceBorrow() = 0;

        /// <summary>
        /// Return borrowed seed (must be done before borrowing a new one!)
        /// </summary>
        virtual void weightedRandomChoiceReturn() = 0;

        virtual ~powerStructure() = default;

        std::unordered_map<coveragePath, size_t> hashmap;
    };

    struct powerSimple : public powerStructure
    {
        // Currently borrowed seed. Store in in this way to avoid copying.
        std::pair<std::multiset<seedSimple>::node_type, std::multiset<seedSimple>::const_iterator> borrowed;

    public:
        void add(std::string input, double T, size_t nm = 1, size_t nc = 1)
        {
            queue.emplace(std::move(input), T, nm, nc);
        }
        virtual void add(std::string input, const coveragePath& h, double T, size_t nm = 1, size_t nc = 1) override
        {
            add(std::move(input), T, nm, nc);
        }
        virtual size_t size() const override
        {
            return queue.size();
        }
        virtual const seed& at(size_t n) override
        {
            // Cannot access directly, needs to jump over LL
            auto it = queue.cbegin();
            std::advance(it, n);
            return *it;
        }

        virtual seed& weightedRandomChoiceBorrow() override
        {
            if (!borrowed.first.empty()) [[unlikely]]
                throw std::runtime_error("Attempted to borrow another seed without returning previous one!");

            // Store the random choice in this structure, and give reference to it
            borrowed = weightedRandomChoiceExtract();
            return borrowed.first.value();
        }
        virtual void weightedRandomChoiceReturn() override
        {
            queue.insert(borrowed.second, std::move(borrowed.first));
        }
        virtual ~powerSimple() = default;

        /// <summary>
        /// Weighted random choice of a seed, where some portion of best seeds will be given 50% choice be selected
        /// </summary>
        /// <returns>Selected seed extracted from the queue</returns>
        std::pair<std::multiset<seedSimple>::node_type, std::multiset<seedSimple>::const_iterator> weightedRandomChoiceExtract()
        {
            if (queue.empty()) [[unlikely]]
                throw std::runtime_error("Queue is empty, cannot choose");
            else if (queue.size() == 1)
                return { queue.extract(queue.cbegin()), std::move(queue.cend()) };

            const size_t firstTenPercent = queue.size() * 0.1f;
            const double coefficientGood = 0.5 / firstTenPercent;
            const double coefficientWorse = 0.5 / (queue.size() - firstTenPercent);

            std::uniform_real_distribution<> dis(0.0, 1.0); // Range [0, totalWeight)

            // Generate a random number
            double randomValue = dis(gen);

            // Find the chosen option based on the random value
            double cumulativeWeight = 0.0;

            size_t i = 0;
            for (auto it = queue.cbegin(); it != queue.cend(); it++) {
                if (i <= firstTenPercent)
                    cumulativeWeight += coefficientGood;// Better score
                else
                    cumulativeWeight += coefficientWorse;// Worse score

                if (randomValue <= cumulativeWeight) {
                    auto nextIt = it++;
                    return { queue.extract(it), std::move(nextIt) };
                }
                i++;
            }

            // If we get here, there was an issue (e.g., no options, weights not positive)
            throw std::runtime_error("Failed to select a weighted random choice.");
        }

        std::multiset<seedSimple> queue;
    };

    struct powerBoosted : public powerStructure
    {
        bool isBorrowed = false;

        void add(std::string input, const coveragePath& h)
        {
            if(isBorrowed) [[unlikely]]
                throw std::logic_error("Cannot add to queue with borrowed element"); // Else it could cause reallocation of vector memory and this damned bug would be so difficult to find that you would spend your whole day finding it and not doing anythine else ask me how I know
            queue.emplace_back(std::move(input), h);
        }
        virtual void add(std::string input, const coveragePath& h, double T, size_t nm = 1, size_t nc = 1) override
        {
            add(std::move(input), h);
        }
        virtual size_t size() const override
        {
            return queue.size();
        }

        virtual const seed& at(size_t n) override
        {
            return queue.at(n);
        }

        virtual seed& weightedRandomChoiceBorrow() override
        {
            isBorrowed = true;
            return weightedRandomChoice();
        }

        virtual void weightedRandomChoiceReturn() override
        {
            isBorrowed = false;
        }
        virtual ~powerBoosted() = default;

        /// <summary>
        /// Weighted random choice of a seed
        /// </summary>
        /// <returns>Reference to the selected seed</returns>
        virtual seed& weightedRandomChoice()
        {
            if (queue.empty()) [[unlikely]]
                throw std::runtime_error("Queue is empty, cannot choose");

            // Calculate the total weight
            double totalWeight = 0.0;
            for (const auto& option : queue)
                totalWeight += option.power(hashmap);

            std::uniform_real_distribution<> dis(0.0, totalWeight); // Range [0, totalWeight)

            // Generate a random number
            double randomValue = dis(gen);

            // Find the chosen option based on the random value
            double cumulativeWeight = 0.0;

            for (auto& i : queue)
            {
                cumulativeWeight += i.power(hashmap);
                if (randomValue <= cumulativeWeight)
                    return i;
            }

            // If we get here, there was an issue (e.g., no options, weights not positive)
            throw std::runtime_error("Failed to select a weighted random choice.");
        }

        std::vector<seedBoosted> queue;
    };

    /// <summary>
    /// Joins random number of seeds together, possible with delimiters
    /// </summary>
    /// <param name="input"></param>
    void createMashups(std::string& input)
    {
        std::exponential_distribution<float> distVal(0.5);
        size_t mashups = 1 + round(distVal(gen));
        for (size_t i = 0; i < mashups; i++)
        {
            switch (generators::randomInt(6))
            {
            case 0:
                input += '\n';
                break;
            case 1:
                input += generators::randomDigit();
                break;
            case 2:
                input += generators::randomASCII();
                break;
            case 3:
            case 4:
            case 5:
            {
                input += queue->at(generators::randomInt(queue->size())).input;
            } break;

            default:
                UNREACHABLE;
            }
        }
    }

    /// <summary>
    /// Perform generation of a new mutant. Given the chance from the constructor, also splice/join multiple existing seeds together.
    /// </summary>
    /// <param name="input"></param>
    /// <returns></returns>
    std::string randomNumberOfRandomMutants(std::string input)
    {
        if (generators::randomFloat() < concatenatedness)
            createMashups(input); // Perform joining
        else
            mutators::randomNumberOfRandomMutants(input); // Perform mutation

        return input;
    }

    virtual void exportStatistics(std::ostream& out) override
    {
        out << '{';
        exportStatisticsCommon(out);
        out << ",\"nb_queued_seed\":" << queue->size() << ",";
        out << "\"coverage\":" << bestCoverage * 100 << ",";
        out << "\"nb_unique_hash\":" << queue->hashmap.size();
        out << '}';
    }
    virtual void exportReport(const CrashReport& report, std::ostream& out) const override
    {
        out << '{';
        exportReportCommon(report, out);
        out << ",\"coverage\":" << bestCoverage * 100;
        out << '}';
    }

    /// <summary>
    /// Get line from the input, split by the delimiter
    /// </summary>
    /// <param name="str">String to read from. Will be eaten</param>
    /// <param name="delimiter">Delimiter to read line to. Will be removed from input</param>
    /// <returns>Read line, without delimiter</returns>
    static std::string_view getLine(std::string_view& str, char delimiter='\n')
    {
        size_t i = 0;
        for (; i < str.size(); ++i)
        {
            if (str[i] == delimiter)
                break;
        }
        auto res = str.substr(0, i);
        if (i >= str.size())
            str = "";
        else
            str = str.substr(i + 1);
        return res;
    }

    /// <summary>
    /// Reads coverage percentage from a input
    /// </summary>
    static std::pair<double, coveragePath> coverage(const std::string& lcov)
    {
        std::pair<double, coveragePath> res;
        std::string_view str(lcov);
        size_t covered = 0;

        //Read lines until name of the file is present. (This is probably just for our coverage tool)
        while (true)
        {
            while (!getLine(str).starts_with("SF:"))
            {
                if (str.empty())
                    goto foundEverything;
            }

            while (str.starts_with("DA:"))
            {
                getLine(str, ','); // Eat up to the comma
                std::string_view numStr = getLine(str, '\n'); // Get the number
                size_t countHit;
                std::from_chars(numStr.data(), numStr.data() + numStr.size(), countHit); // Parse number
                res.second.push_back(countHit > 0);
                if (countHit > 0)
                    covered++;
            }
        }
        foundEverything:

        size_t total = res.second.size();

        res.first = static_cast<double>(covered) / total;

        return res;
    }

    /// <summary>
    /// Try to run a seed, and reward it if it succeeds
    /// </summary>
    /// <param name="parent">Seed that the mutant was taken from, nullptr if orphan (initial seed)</param>
    /// <param name="mutant">Mutant to run on</param>
    /// <typeparam name="alwaysInsert">Always insert in the queue, even if no improvement occurs</param>
    template <bool alwaysInsert = false>
    void trySeed(seed * parent, std::string mutant)
    {
        // Prepare input for execution
        executionInput->setInput(mutant);

        // Execute the actual program
        auto res = execute_with_timeout(*executionInput);

        // Check whether error occured, record it and minimize
        auto error = dealWithResult(mutant, res, *executionInput, false);

        // Load coverage from file
        double executedCoveragePercent = 0;
        coveragePath executedCoveragePath;
        if (std::filesystem::exists(COVERAGE_FILE)) //Sometimes no coverage file is available (error etc.)
        {
            auto lcov = loadFile(COVERAGE_FILE);
            std::filesystem::remove(COVERAGE_FILE);

            auto tmp = coverage(lcov);

            executedCoveragePercent = std::move(tmp.first);
            executedCoveragePath = std::move(tmp.second);
        }
        // else use empty path (for errors)

        // Insert it into hashtable
        auto it = queue->hashmap.emplace(std::move(executedCoveragePath), 0);
        it.first->second++;;
        const auto& recordedCoveragePath = it.first->first;
        bool foundNewPath = it.second; // If this created a new element in the table

        // If this mutant has a parent
        if (parent != nullptr)
        {
            // Reward parent for finding a new path
            if (foundNewPath)
                parent->incrementImproved();

            // Update and return the original seed back to the queue
            parent->update();
            queue->weightedRandomChoiceReturn();
        }

        // Add new interesting seed (crashing)
        if (alwaysInsert || foundNewPath)
            queue->add(std::move(mutant), recordedCoveragePath, res.execution_time.count(), 1, 1);

        // Improve coverage score (if it wasn't error)
        if (!error && executedCoveragePercent > bestCoverage)
        {
            std::cerr << "Just improved coverage! From " << bestCoverage << " to " << executedCoveragePercent << ". nb_runs=" << statisticsExecution.count() << std::endl;
            bestCoverage = executedCoveragePercent;
        }
    }

    std::unique_ptr<powerStructure> queue;
    double bestCoverage = 0;

    virtual void fuzz() override
    {
        // Run for initial seeds without mutating
        std::cerr << "Executing on empty input to set a coverage" << std::endl;
        executionInput->setInput("");
        execute_with_timeout(*executionInput);
        if (std::filesystem::exists(COVERAGE_FILE))
        {
            auto lcov = loadFile(COVERAGE_FILE);
            std::filesystem::remove(COVERAGE_FILE);
            bestCoverage = coverage(lcov).first;
        }
        std::cerr << "Initial coverage set to " << bestCoverage << std::endl;

        std::cerr << "Executing initial seeds..." << std::endl;
        for (const auto& i : std::filesystem::directory_iterator(INPUT_SEEDS))
        {
            if (!keepRunning)
                break;
            if (i.is_regular_file())
            {
                // Run directly on this seed
                auto input = loadFile(i.path());

                // Check if input does not contain control characters (we don't support those)
                for (const auto& c : input)
                    if (!isJsonAllowedOrEscapeable(c))
                        goto containsEscapes;

                trySeed<true>(nullptr, std::move(input));

            containsEscapes:
                while (0);
            }
        }
        std::cerr << "Loaded " << queue->size() << " seeds." << std::endl;

        std::cerr << "Mutating..." << std::endl;
        while (keepRunning)
        {
            // Make this a hybrid between greybox and blackbox fuzzing. Sometimes, instead of a mutating existing seed, test random input - if working, add it as seed.
            if (generators::randomFloat() < greyness)
            {
                trySeed(nullptr, generators::generateRandomInput());
            }
            else
            {
                auto& selected = queue->weightedRandomChoiceBorrow();
                selected.incrementImproved();
                trySeed(&selected, randomNumberOfRandomMutants(selected.input));
            }
        }
    }

    /// <summary>
    /// Populate a folder with random seeds
    /// </summary>
    void populateWithMySeeds(size_t loadSize = 1000)
    {
        std::cerr << "Populating folder with my seed" << std::endl;
        std::filesystem::create_directories(INPUT_SEEDS);

        for (size_t i = 0; i < loadSize; i++)
        {
            std::filesystem::path path = INPUT_SEEDS / (std::to_string(i) + ".txt");
            std::ofstream outFile(path);
            outFile << generators::generateRandomInput();
        }
    }

    fuzzer_greybox(std::filesystem::path FUZZED_PROG, std::filesystem::path RESULT_FUZZ, bool MINIMIZE, std::string_view INPUT, std::chrono::seconds TIMEOUT, size_t NB_KNOWN_BUGS, POWER_SCHEDULE_T POWER_SCHEDULE, std::filesystem::path COVERAGE_FILE, float greyness, float concatenatedness, std::filesystem::path INPUT_SEEDS) : fuzzer(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(INPUT), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS)), POWER_SCHEDULE(std::move(POWER_SCHEDULE)), COVERAGE_FILE(std::move(COVERAGE_FILE)), INPUT_SEEDS(std::move(INPUT_SEEDS)), greyness(std::move(greyness)), concatenatedness(std::move(concatenatedness))
    {
        switch (POWER_SCHEDULE)
        {
        case fuzzer_greybox::POWER_SCHEDULE_T::simple:
            queue = std::make_unique<powerSimple>();
            break;
        case fuzzer_greybox::POWER_SCHEDULE_T::boosted:
            queue = std::make_unique<powerBoosted>();
            break;
        default:
            UNREACHABLE;
        }
    }

    fuzzer_greybox(std::filesystem::path FUZZED_PROG, std::filesystem::path RESULT_FUZZ, bool MINIMIZE, std::string_view INPUT, std::chrono::seconds TIMEOUT, size_t NB_KNOWN_BUGS, POWER_SCHEDULE_T POWER_SCHEDULE, std::filesystem::path COVERAGE_FILE, float greyness, float concatenatedness) : fuzzer_greybox(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(INPUT), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS), std::move(POWER_SCHEDULE), std::move(COVERAGE_FILE), std::move(greyness), std::move(concatenatedness), "MY_SEED")
    {
        populateWithMySeeds();
    }

    const POWER_SCHEDULE_T POWER_SCHEDULE;
    const std::filesystem::path INPUT_SEEDS;
    const std::filesystem::path COVERAGE_FILE;

    const float greyness;
    const float concatenatedness;
};