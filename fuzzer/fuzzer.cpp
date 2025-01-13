#include "fuzzer.h"
#include <optional>

std::optional<fuzzer> myFuzzer;


#ifndef _MSC_VER

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cerr << "Caught SIGINT (Ctrl+C). Exiting gracefully...\n";
        myFuzzer->requestStop();
    }
    else if (signal == SIGTERM) {
        std::cerr << "Caught SIGTERM. Exiting gracefully...\n";
        myFuzzer->requestStop();
    }

    //if (!keepRunning)
    //{
    //    saveStatistics();
    //    std::cerr << "Ready for exit." << std::endl;
    //}
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
        myFuzzer->requestStop();
        return TRUE;
    }

    return FALSE;
}
#endif


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

    std::string FUZZED_PROG = argv[1];
    std::filesystem::path RESULT_FUZZ = argv[2];
    bool MINIMIZE = std::atoi(argv[3]) != 0;
    std::string_view fuzzInputType = argv[4];
    std::chrono::seconds TIMEOUT = std::chrono::seconds(std::atoi(argv[5]));
    size_t NB_KNOWN_BUGS = std::atoi(argv[6]);

    std::cout << "Fuzzing program " << FUZZED_PROG << ", placing results into folder " << RESULT_FUZZ << ", minimize=" << MINIMIZE << ", type=" << fuzzInputType << ", timeout=" << TIMEOUT << ", known_bugs=" << NB_KNOWN_BUGS << std::endl;

    try
    {
        myFuzzer.emplace(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(fuzzInputType), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS));
        myFuzzer->run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
