# Random fuzzer with test minimization - NI-APT task 2

After cloning, run:
`git submodule update --init --recursive`
This ensures that required libraries are available for the compilation process

### Bonuses that are implemented:

- Integer input generation
- Reading from environmental variables to speed up CI

## Testing

Line coverage: 83%

# Project annotation

The goal of this project is to create a random fuzzer that also minimizes the failure-inducing inputs it has found.
It will also establish an adequate architecture that will be reused in the next assignments:

- input generation
- runner
- oracle
- handling of results
- statistics
- minimizer

The fuzzer should be stopped with `ctrl+c` (SIGINT) or upon receiving SIGTERM and save the relevant information when being stopped.

## Input generation

You will add a random input generator wich performs random generation of C strings (with length of the string and a range of characters).

Optionally, you can also generate random array of bytes (with parameters the byte range, and length of the array) in the case of generating files. The only difference between the two lies into how they deal with `'\0'`.
In that case, randomly pick an input generator to generate each input.

Feel free to add more random input generators.

## Runner

The runner will be an external runner: it runs the program to be fuzzed.

<div class="admonition-block note">
<p>
You can <em>optionnally</em> provide an interface as in <a  href="https://llvm.org/docs/LibFuzzer.html">libFuzzer</a>. 
In that case, you should provide your own <code>main</code> to link with the fuzzing interface or dynamically load the code to be fuzzed as a library. 
A useful optimization to the launch time of the fuzzed program is the <a href="https://lcamtuf.blogspot.com/2014/10/fuzzing-binaries-without-execve.html">fork server</a>.
</p>
</div>

The input to the fuzzed program should be provided in possible 2 ways:

- through the standard input (stdin)
- through command line arguments and input files. The minimum is to support one positional argument, the path of the file, and to create and randomly fill the file. The file can be text or binary.

Optionally, you can support any command line arguments that will be specified to the fuzzer using json as follows:

```json
{
  "optional_args": [
    {
      "name": "-a",
      "type": ["bytes", "boolean", "string", "integer", "float", "file"],
      "fuzz:": [true, false], // if true, the value will be used for fuzzing
      "value": "what is needed here", // to be provided if fuzz = false, or if fuzz = true and type = file. In that case, that should be the file name the content of which will be fuzzed.
      "max_size": "number", //If not provided, will be 0
      "min_size": "number" //bytes or characters depending on the type (of the argument or of the file). If not provided, choose some sane default depending of the type
    }
  ],
  "positional_args": [
    // the order gives the position
    {
      "type": "see above",
      "fuzz:": [true, false], // see above
      "value": "see above",
      "min_size": 0,
      "max_size": "sane_default"
    }
  ]
}
```

## Oracle

The oracles to detect and de-deuplicate failures should be at least:

- return code
- `AddressSanitizer` (`ASAN`), with line to the error. You should support at least heap buffer and stack buffer overflow. Use regexes to find the line of the error. It also means that the fuzzed program must be compiled with the right flags to activate `ASAN`
- timeout for one run on one input.

Optionally:

- more from the `AdressSanitizer`
- `UndefinedBehaviourSanitizer`
- any other oracle you want to add
- automatically determine the timeout threshold by looking at the execution times of the first runs and using the median execution time scaled by a reasonable factor (e.g., AFL chooses 5x).

## Minimizer

You should use delta-debugging to minimize the failure-inducing input. The minimized input should lead to the same failure as the initial one.
If you find other failures along the way, keep them and minimize them also later.

## Results

Reports for failure-inducing inputs will be stored in subdirectories of a command-line provided result directory:

- crashes should be stored in the `crashes` subdirectory.
- hangs will be stored in the `hangs` directory.

A crash report is a json file with the following specification:

```json
{
  "input": "string",
  "oracle": "return_code|asan|timeout",
  "bug_info": "object|number",
  "execution_time": "number",
  "minimization": {
    "unminimized_size": "number",
    "nb_steps": "number",
    "execution_time": "number"
  }
}
```

- `input`: the failure inducing input, **minimized**. If you added a random byte generator, encode the input as base64 if the input contains `\0` and add a field `"base64" : "true"` in the json file.
- `oracle`: what kind of oracle detected it. Use the most specific one here.
- `bug_info`: depending on the type
  - `return_code`: the return code
  - `asan`: `{"file":"filename", "line":"number", "kind":"heap|stack"}` The first two fields are used to locate the error in the program. The 3rd one indicates it if was a stack or a heap overflow. You might want to add an additional field to deduplicate more precisely by comparing the stack trace.
  - `timeout`: after how long you decided it was a timeout.
- `execution_time` for this input (not including minimization) in **ms**
- `minimization`:
  - `unminimized_size`: size before minimization, in bytes
  - `nb_steps`: number of minimization steps before reaching a 1-minimal configuration
  - `execution_time`: in **ms**

You can add more fields and or more values for the fields if you need. For instance, you can add `free` for `bug_info.asan.kind` to indicate _use after free_.

## Statistics

Statistics are saved in a json file in the result directory. You can update them regularly (for instance, after running on a new input) if you want to follow the progress of the fuzzing campaign but you do not have to. However, they should be up-to-date when the fuzzer is stopped.

Here is the json file format:

```json
{
  "fuzzer_name": "string",
  "fuzzed_program": "string",
  "nb_runs": "number",
  "nb_failed_runs": "number",
  "nb_hanged_runs": "number",
  "execution_time": {
    "average": "number",
    "median": "number",
    "min": "number",
    "max": "number"
  },
  "nb_unique_failures": "number",
  "minimization": {
    "before": "number",
    "avg_steps": "number",
    "execution_time": {
      "average": "number",
      "median": "number",
      "min": "number",
      "max": "number"
    }
  }
}
```

Your fuzzer name should be a unique string (not the same as the other students). It can be your name, or anything else (for instance if you want to be anonymous on the leaderboard). It **must not** be empty.

`execution_time` holds information about the duration of one run in **ms**, from the generation of the input to its classification as failure or not by the oracle.

`nb_unique_failures` is the number of unique kinds of failures (different return codes, different error with the address sanitizer), and after minimization. Those are the ones that are saved in the `crashes` subdirectory.

`before` in `minimization` indicates how many failures you had after deduplication with the oracle, but before performing minimization on the input (not taking into accounts the ones that you might have discovered while minimizing);
`avg_steps` is the average number of steps before reaching a 1-minimal configuration; `execution_time` is the overall execution time added by all the minimization steps for a given input.

You will find a visualizer (in Rust) for the stat json in the [project-apt repo](https://gitlab.fit.cvut.cz/NI-APT/project-apt/-/tree/master/visualizer?ref_type=heads). It also has struct definitions for the crash results.

## Makefile

Your fuzzer should be run using a `Makefile` with at least the 3 following targets:

- `build`: build your fuzzer (and/or possibly install dependencies)
- `test`: run your test suite and generate the coverage report. A file `coverage.txt` containing the coverage percentage as a number should be also generated and will be added as an artifact for the the test job in the CI.
- `run`: run the fuzzer on binary specified by environment variable `FUZZED_PROG` and output the results in the directory specified by environment variable `RESULT_FUZZ`. The binaries provided to your fuzzer will be already compiled with the address sanitizer. Other environment variables that can be set by me (the grading script!) before calling target `run`:
  - `MIMIMIZE=[0|1]` to activate or deactivate the minimization. If minimization is deactivated, you do not have to have the fields related to the minimization in the results.
  - `INPUT=[stdin|{filen}]`; if `INPUT` is `stdin`, then your program should fuzzer should send inputs through stdin to the fuzzed program. If `INPUT=file`, it should send them by creating a file and passing it as the first argument of the fuzzed program.
  - `TIMEOUT` is another variable that gives you the timeout in seconds before your fuzzer will be asked to stop. You do not necessarily have to take it into account as your fuzzer will be killed by the grader after the timeout anyway.

## Testing

Your submission should be tested, using specification testing and structural testing.

You should have automated tests and run a code coverage tool. The line code coverage of your code should be a least 70%. If you have strictly less, justify with comments why you decided not to test some parts of the code.

I also give you a few [toy projects in C](https://gitlab.fit.cvut.cz/NI-APT/project-apt/-/tree/master/random-fuzzing?ref_type=heads) to test your fuzzer on.

## Optional features

1. Internal runner with libFuzzer-like interface.
2. Use the json specification to generate and fuzz command line arguments for the fuzzed program.
3. Mine the command line arguments of a program by replacing the `getopt` and `getopt_long` functions in the fuzzed programs (possibly at runtime, using `LD_PRELOAD`) and use it to generate the json specification.
4. Be able to restart a fuzzing campaign from a non-empty result directory, continuing deduplicating.

## Submission

Here are the instructions on how to submit the task:

- tag the last commit for task 1 with tag `task1`
- add a `fuzzer` directory at the root of the project
- Your code should run at least on Linux (on macOS and Windows, if you want)
- The `fuzzer` directory should contain a `Makefile`. See [Makefile](#makefile). The Makefile is used by the CI.  
  The gitlab CI file [`.gitlab-ci.yml`](../resources/code/.gitlab-ci.yml) has been updated. It will run your fuzzer on several binaries for a given time budget, check the bugs you find and send the results to the leaderboard page.[^3]

- There should be a README (or README.md) file with the following structure
- If you generate a html file with the code coverage of your own code, also add it
- Your code should run at least on Linux (additionnally on macOS and Windows, if you want)

```markdown
# Random fuzzer with test minimization - NI-APT task 2

Any information you want to write here. Especially what you have implemented and what you have not, and any bonus features.

## Testing

Line coverage: ..% (for instance, 89%)
You can add more coverage information (branch coverage, function coverage) if you want to.
```

## Points

| #   | Feature             |    Points |
| --- | ------------------- | --------: |
| 1   | Input generation    |         1 |
| 2   | Runner              |         3 |
| 3   | Oracle              |         3 |
| 4   | Handling of results |         2 |
| 5   | Minimizer           |         3 |
| 6   | Statistics          |         3 |
|     | _TOTAL_             |        15 |
|     | Testing             | See below |

**Testing**: the coverage (as a percentage) of your code determines a multiplying coefficient to the total of points.

Your mark is: $\text{total}\_{\text{new}} = \left\lceil \min(1, \frac{\lceil \text{coverage} \rceil + 30}{100}) \* \text{total} \right\rceil$

[^3]: not yet available...