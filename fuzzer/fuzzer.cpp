#include "fuzzer.h"
#include <optional>

fuzzer* myFuzzer;


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

    //std::cerr << "Provided arguments: " << argc - 1 << std::endl;
    //for (size_t i = 1; i < argc; i++)
    //{
    //    std::cerr << argv[i] << std::endl;
    //}

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
    size_t currentArg = 1;

    std::filesystem::path FUZZED_PROG = argv[currentArg++];
    std::filesystem::path RESULT_FUZZ = argv[currentArg++];
    bool MINIMIZE = std::atoi(argv[currentArg++]) != 0;
    std::string_view fuzzInputType = argv[currentArg++];
    std::chrono::seconds TIMEOUT = std::chrono::seconds(std::atoi(argv[currentArg++]));
    size_t NB_KNOWN_BUGS = std::atoi(argv[currentArg++]);

    std::cerr << "Fuzzing program " << FUZZED_PROG << ", placing results into folder " << RESULT_FUZZ << ", minimize=" << MINIMIZE << ", type=" << fuzzInputType << ", timeout=" << TIMEOUT.count() << ", known_bugs=" << NB_KNOWN_BUGS << std::endl;

    if (NB_KNOWN_BUGS == 0)
        NB_KNOWN_BUGS = std::numeric_limits<size_t>::max();

    try
    {

        if (argc <= currentArg)
        {
            //BLACKBOX

            std::cerr << "Blackbox" << std::endl;

            fuzzer_blackbox blackbox(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(fuzzInputType), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS));
            myFuzzer = &blackbox;
            blackbox.run();
        }
        else
        {
            // GREYBOX
            std::cerr << "Greybox" << std::endl;

            std::string_view POWER_SCHEDULE = argv[currentArg++];
            fuzzer_greybox::POWER_SCHEDULE_T schedule = POWER_SCHEDULE == "simple" ? fuzzer_greybox::POWER_SCHEDULE_T::simple : fuzzer_greybox::POWER_SCHEDULE_T::boosted;
            std::cerr << "schedule=" << POWER_SCHEDULE << "=" << (int)schedule << ", ";

            std::filesystem::path COVERAGE_FILE = argv[currentArg++];
            std::cerr << "COVERAGE_FILE=" << COVERAGE_FILE << std::endl;

            float GREYNESS = std::atoi(argv[currentArg++]) / 100.0f;
            std::cerr << "GREYNESS=" << GREYNESS << std::endl;

            float CONCATENATEDNESS = std::atoi(argv[currentArg++]) / 100.0f;
            std::cerr << "CONCATENATEDNESS=" << CONCATENATEDNESS << std::endl;

            if (argc <= currentArg)
            {
                std::cerr << "Seed directory not provided" << std::endl;
                fuzzer_greybox greybox(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(fuzzInputType), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS), schedule, std::move(COVERAGE_FILE), GREYNESS, CONCATENATEDNESS);
                myFuzzer = &greybox;
                greybox.run();
            }
            else
            {
                std::filesystem::path INPUT_SEEDS = argv[currentArg++];
                std::cerr << "Seed directory provided: " << INPUT_SEEDS << std::endl;
                fuzzer_greybox greybox(std::move(FUZZED_PROG), std::move(RESULT_FUZZ), std::move(MINIMIZE), std::move(fuzzInputType), std::move(TIMEOUT), std::move(NB_KNOWN_BUGS), schedule, std::move(COVERAGE_FILE), GREYNESS, CONCATENATEDNESS, std::move(INPUT_SEEDS));
                myFuzzer = &greybox;
                greybox.run();
            }
        }

    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
