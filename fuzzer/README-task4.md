# Generation of seeds from constants -- NI-APT task 4

In this improvement, the fuzzer stays (almost) the same. We instead focus on generating good seeds that the fuzzer can work with.
Using the tree-sitter project (the same one as for task 1), we managed to easily extract various constants from the source code.
This project is located in the seed-generator folder.
Before the fuzzer is run and source code is instrumented, we run our preprocessor on the source code. It indentifies and extracts these groups of constants:
- String and char literals. Our project unescapes them and creates a seed for each one. Using strings found in the program can help trigger various branches, or maybe puts a strange string somewhere it itsn't expected. It can also exploit vulnerabilites for printf, if the string is used directly, allowing for buffer overflows because of control sequencies possible in the strings.
- Integer literals. Our project converts them to strings and outpus a seed for each (same reasons as above), but also creates arbitrary strings with the length of these integers. This is done in order to create border values, for example for exploting buffers.
-  Other literals. Floats, preprocessor constants, etc. Our project tries to identify if these fit in some of the groups above (for example flooring the floating point value to create an integer), and also creates a file for each literal.

The only improvement that needed to be done on the fuzzer is to incentivize it to concatenate input seeds more often. Because our seeds our now high quality, we don't really need mutating them that much. Instead, we mainly care about joining them together. For this reason, this functionality was introduced in the fuzzer. To be compatible with previous task, the chance is settable via an argument when the fuzzer is started. We found that a value around 50% works the best - half of the time mutate existing seeds, the other half join several together, instead. As a bonus, sometimes they are joined also with a delimiter in-betweeen, to increase coverage for various scanf calls.

Our approach does not make any compromise on the speed of the fuzzer once it is running, so the throughput is not limited in any way. At the same time, the approach creates valuable seeds which the fuzzer a lot (see performance section), so it is also a lot smarter.

Given our testing, this makes for an improvement in each of programs. However, there could be programs where this behaviour would be detremenental, and therefore bear worse results than with random seeds. After all, we are limiting the chance to find random inputs in the benefit of finding tailored ones.

## Performance

Our improvement shows success in all programs - it manages to find 4 extra bugs, making all of them (14/14)! At one point it was, and maybe still is, the best result on the leaderboard.

It is very easy for the program to find exploits of passphrases, various sizes of strings and specific numbers that are identified in the source code - this is what it was made to do.
It is no suprise than that problems such as `buffer-overflow`, `minimization`, `infinite-loop` or even finding the secret passphrase in `string-length` pose no challange to our approach, and are usually found in under a second (after which they are minimized, which takes much more time).

Where it struggles is with the inputs, where the "secret input" is not in somewhere in one place in the source code. Problems such as finding the buffer overflow in `string-length` (requires 1 + newline + long string + newline + 3) are less likely to be found, but given enough time, our program finds them as well. This "time" is luckily under the set limit of 10 minutes, so it finds everything in the grader (I think). It even manages to find the `evil(1)` function call from `arith-expr` (I was truly suprised)!

Our idea also improves the speed under which the program is covered, especially at the start, where it finds faster what branches to take.

Naturally, it works best with a good fuzzer - the covered lines need to be clustered together, and the coverage tool should ideally cover also included headers for the power functions to work better. The latter is unfortunately not the case, but that is beyond the scope of this project. The former was added to the fuzzer from task 3 because of this exact reason.

## Statistics

Given the nature of the problem, it is hard to find additional statistics that could be printed during runtime. At the start of the program, when our preprocessor is run, we print how many constants of which type it found.

## Setup

After cloning, run:
`git submodule update --init --recursive`
This ensures that required libraries are available for the compilation process.

Also install packages: `make cmake g++ git libboost-all-dev libgtest-dev curl llvm-17 clang-17 mull-17`

Look in CI file to see how to, if problematic. Mull requires a setup, but you don't need it if you won't be mutating.

Build code-coverage and seed-generator before running new version of the fuzzer.

## Testing

### Coverage

Line coverage:
- 78% for the fuzzer project (sharing with task 2 and 3)
- x% for the seed-generator


### Property-based testing

Indicate in which file(s) you have the property-based tests.

### Mutation testing

Mutation score:
- 60% for the fuzzer project (sharing with task 3)
- x% for the seed-generator

Mutation testing heavily depends on randomness.

```
/builds/kocoumat/apt-2024/fuzzer/fuzzer.h:1382:22: warning: Survived: Replaced > with >= [cxx_gt_to_ge]
            return e > other.e;
```
The example mutant shows comparison between energies in the simple method. It is used when sorting the queue. `>` can be replaced by `>=` because in that case, their order does not matter.

# Project annotation

The goal of this task is to extend the greybox fuzzer of [task-3] with functionnalities that you can choose. I give here some suggestions.
If you want to do your own, you can, but ask me beforehands if I am ok with it. Part of the grade coresponds to the difficulty of the improvements you choose to do. In any case, you will have to write a detailed description of what you want and what you have implemented.

You should not have any regressions compared to [task-3].

# Testing

You code must be thoroughly tested, using specification-based testing, structural testing and property-based testing.

You should have automated tests and run a code coverage tool. The line code coverage of your code should be a least 70%. If you have strictly less, justify with comments why you decided not to test some parts of the code. For instance, the `main` function is usually excluded from the coverage. Many coverage tools support excluding a function or a file.

You should also have at least _3 property-based tests_.

Mutation testing should also be automated and reported. You should achieve a mutation score of at least 50%.
If your mutation score is strictly less than 100%, add at least one example of a mutant in the README.

# Performance

Your improvements should also improve on the performance of your fuzzer compared the one of task 3 and to a baseline.

The performance score of your fuzzer depends on how many bugs it finds in a given time budget, and how quickly it finds each of the bugs.
It also includes the speed at which it increases coverage.

You can see your performance on the [leaderboard](https://ni-apt.prl.fit.cvut.cz/). To have your fuzzer added to the leaderboard, send me a message to get a unique token. The token must be exported in the grading task through environment
variable `GRADER_TOKEN`. You can set the value in the gitlab CI file or [use the UI](https://docs.gitlab.com/ee/ci/variables/).

If your fuzzer is ranked among the first 3 fuzzers on the leaderboard, you will also get a bonus on your project grade.

# Documentation

All implemented functionalities must be correctly documented in the code.
You should also have a detailed list of what you have implemented in the README. Add references to articles if you used some of them.

# Statistics

Add any relevant statistics to the improvements you have made to your fuzzer.

For instance, if you do grammar-based fuzzing with Nautilus, add:

- the average of the degrees of validity of the input seeds so far
- how many inputs are totally degenerate (i.e. root as a custom node)

# Submission

Here are the instructions on how to submit the task:

- Tag the commit before starting task 4 with `task3` if not already tagged
- Tag the commit that you want me to look at for task 4 with `task4`
- Update the Gitlab CI file to `.gitlab-ci.yml`
- As always, your code should run at least on Linux (additionnally on macOS and Windows, if you want)
- Update the README (or README.md) file adding information as below

```markdown
# Name of your improvement -- NI-APT task 4

Description of what you decided to implement, along with references. Be precise. Explain what kind of tradeoff you are making
between throughput (number of runs/second) and smartness (crafting inputs that are more likely to crash the fuzzed program, but which may be slow to generate).

Describe any difficulties with the implementation of the features.

## Setup

Dependencies and how to install them.

## Testing

### Coverage

Line coverage: ..% (for instance, 89%)
You can add more coverage information (branch coverage, function coverage) if you want to.

### Property-based testing

Indicate in which file(s) you have the property-based tests.

### Mutation testing

Mutation score: ..%

If not 100%, put here a code block with at least 1 mutant not killed by your test suite.
```

## Makefile

Your fuzzer should be run using a `Makefile` with at least the 3 following targets, which are similar to those of task 2:

- `build`: build your fuzzer (and/or possibly install dependencies)
- `test`: run your test suite and generate the coverage report. Also runs mutation testing. A file `coverage.txt` containing the coverage percentage as a number and a file `mutation.txt` containing the mutation score should be also generated and will be added as an artifact for the the test job in the CI.
- `run`: run the fuzzer on binary specified by environment variable `FUZZED_PROG` and output the results in the directory specified by environment variable `RESULT_FUZZ`. Other environment variables that can be set by me (the grading script!) before calling target `run`:
  - `MIMIMIZE=[0|1]` to activate or deactivate the minimization. If minimization is deactivated, you do not have to have the fields related to the minimization in the results.
  - `INPUT=[stdin|{filen}]`; if `INPUT` is `stdin`, then your program should fuzzer should send inputs through stdin to the fuzzed program. If `INPUT=file`, it should send them by creating a file and passing it as the first argument of the fuzzed program.
  - `TIMEOUT` is another variable that gives you the timeout in seconds before your fuzzer will be asked to stop. You do not necessarily have to take it into account as your fuzzer will be killed by the grader after the timeout anyway.
  - `POWER_SCHEDULE=[simple|boosted]` to choose of the 2 power schedules. `simple` by default.
  - `INPUT_SEEDS`: directory where the initial seeds are located.
  - `FUZZER=[blackbox|greybox|best]` to specify which version of your fuzzer to use; defaults to `best` if not provided. `best` is your version with the improvements.
  - `FUZZ_GRAMMAR=path` indicates the path where to find the grammar describing the inputs. Not all programs have structured inputs, and some programs with structured inputs do not have grammars so this environment variable might be either absent or contain an empty string.

# Points

| #   | Parts                                           |    Points |
| --- | ----------------------------------------------- | --------: |
| 1   | Difficulty                                      |         5 |
| 2   | Specification, property-based, mutation testing |         5 |
| 3   | Performance                                     |         5 |
| 4   | Documentation                                   |         3 |
| 5   | Statistics                                      |         2 |
|     | _TOTAL_                                         |        20 |
|     | Structural testing                              | See below |

**Testing**: the coverage (as a percentage) of your code determines a multiplying coefficient to the total of points.

Your mark is: $\text{total}\_{\text{new}} = \left\lceil \min(1, \frac{\lceil \text{coverage} \rceil + 30}{100}) \* \text{total} \right\rceil$
