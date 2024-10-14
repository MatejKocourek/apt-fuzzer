The goal of this project is to create a line coverage tool for C programs.

| #   | Task          | Points |  Start |    End |
| --- | ------------- | -----: | -----: | -----: |
| 1   | Code coverage |     10 | 10.10. | 24.10. |

The end date is a soft dead line.

Given C files with one `main`, the code coverage tool should output instrumented C files, which, after running the `main`, will
generate a `coverage.lcov` containing the line coverage using the [LCOV tracefile format](https://ltp.sourceforge.net/coverage/lcov/geninfo.1.php).
LCOV trace files are commonly supported by coverage viewers. If you use VSCode, you can use the extension [Coverage Gutters](https://marketplace.visualstudio.com/items?itemName=ryanluker.vscode-coverage-gutters) to visualize the results of your tool.

## Line coverage

We only compute the coverage of lines _inside_ functions. A line is assumed to be covered if the program control reaches the beginning of the line.[^1]
To simplify, when a statement spans multiple lines, we only consider the first line of the statement.

### Lcov file

Here is the content of a `lcov.info` trace file for our purposes:

```
TN:test
SF:tests/test2.c
DA:5,6
DA:7,1
DA:11,5
DA:17,1
DA:23,1
DA:24,1
DA:26,1
DA:28,1
LH:8
LF:9
end_of_record
```

- `TN` is the name of the test
- `SF` starts a section for one given C file
- `DA` gives the covered line number and the number of times it was hit
- `LH` is the number of covered lines
- `LF` is the number of instrumented lines
- `end_of_record` ends a section giving coverage for a given C file

You can have several sections in one `lcov.info` file.
The line coverage for one file as a percentage is `LH / LF * 100`.

## Minimal supported subset of C

Here is the description of the subset of C you have to at least support (and not support). Of course, more is better! You can find a rather readable reference of the C language [here](https://learn.microsoft.com/en-us/cpp/c-language/c-language-reference?view=msvc-170) (yes!).

Besides, any tests I provide must be supported by your coverage tool.

- **H**: indicates that you have to support it
- **NH**: indicates that you do not have to support it

- **H**: multiple files, with only one `main` function
- **NH**: C preprocessor
- **NH**: pragmas
- **H**: blocks (compound statements, function declarations)
- **NH**: `for`, `if`, `while` with a one-liner body (and not with a compound statement)
- **NH**: `do ... while`
- **NH**: `switch`

## How to instrument the C files to get the coverage

<div class="admonition-block note">
<p>
This is a simple way of computing the coverage. You can use other methods (e.g., LLVM IR and basic blocks) as long as you get the right results and that you understand what you are doing.
</p>
</div>

- Parse the C code. I advise you to use an existing C parser that can give you the line and column numbers of any tokens; `tree_sitter` variants have been used with success. You can also roll out your own C parser; you only need to be able to find functions and compound statements, i.e. blocks surrounded by `{` and `}`.
- Inject a global variable `instrumentation_filename` (e.g. per file) as an array of size the number of lines in the file
- Before each line that can be executed (i.e. not blank lines, lines only with comments...), inject an incrementation to `instrumentation_filename[line_number]`
- In the file with the `main` function, inject helper functions to generate the lcov file
- In the same file, inject at the beginning of `main` a call to register the function that generates the lcov file to be called at exit (see [`atexit`](https://en.cppreference.com/w/c/program/atexit))
- Pay attention to one-liner functions, compound statements

Then you just have to compile the instrumented C files (preferably with `-O0`) and run the resulting executable `prog` to get the coverage in `prog.lcov`.

## Testing

Your submission should be tested, using specification testing and structural testing.

You should have automated tests and run a code coverage tool. The line code coverage of your code should be a least 70%. If you have strictly less, justify with comments why you decided not to test some parts of the code.
If you code in C, you can use your code to test your own coverage!

For system tests, use the examples [provided examples](https://gitlab.fit.cvut.cz/NI-APT/project-apt/-/tree/master/coverage?ref_type=heads) to compare your resulting lcov files with mines. You can also use `gcov` and then `lcov` or `gcovr` to obtain a LCOV coverage file.

## Performance

Make a small benchmark that runs the instrumented C files and the non-instrumented ones, with compilation optimization at `-O0` and compare their execution time.

## Submission

Here are instructions on how to submit the task:

### Repository

- You should create a repository on FIT gitlab. You will use the same repository for all the assignments.
- The repository name should contain `apt-2024`
- Share it with `donatpie` with role `reporter`.

### Structure

- The code coverage tool should be in its own directory `code-coverage` at the root of the repository, which will contain all related source files for this first assignment.
- If you generate a html file with the code coverage of your own code, also add it

### Feedback

- Your code should run at least on Linux (on macOS and Windows, if you want)
- the `code-coverage` directory should contain a `Makefile` with at least the following targets:
  - `build`: build your program (if you do the task in Python, you probably do not need this step)
  - `test`: run your test suite and generate the coverage report
  - `benchmark`: run the microbenchmark and display on stdout at least a line with `Speedup: [value]` where `[value]` is replaced by the actual speedup. If you have several microbenchmarks, put there the average speedup.
  - `run`: runs the code coverage tool on the C file(s) in the directory specified by environment variable `TARGET_COV`, outputs a `coverage.lcov` containing the coverage in `TARGET_COV`
- You should have the attached `.gitlab-ci.yml` file at the root of your repository. It will run automatic tests on your coverage tool and will check the coverage of it. You will need to tailor it to what you need for the language(s) you chose to implement the task.

### Documentation

- There should be a README (or README.md) file with the following structure

```markdown
# Code coverage - NI-APT task 1

Any information you want to write here

## Testing

Line coverage: ..% (for instance, 89%)
You can add more coverage information if you want to.

## Benchmark

Slow-down of instrumentation: ...x (for instance, 10x slower)
```

## Optional features

You can add more features:

1. You will learn more
2. You will get bonus points

Here are the features:

- more of the C language
- support for coverage information on any region, not only lines (and show all the lines of a multiple-line statement as covered)
- add branch coverage
- add condition + branch coverage
- optimization: only insert one instrumentation instruction per basic block
- whatever you think is a good idea (document it in the README)

## Points

| #   | Feature                                                  | Points |
| --- | -------------------------------------------------------- | -----: |
| 1   | Line coverage on statements without loops and conditions |      2 |
| 2   | Line coverage on statements with loops and conditions    |      3 |
| 3   | Line coverage on a file with a main function             |      1 |
| 4   | Line coverage on several files                           |      1 |
| 5   | The code is tested (70% line coverage at least...)       |      2 |
| 6   | Benchmark                                                |      1 |
|     | _TOTAL_                                                  |     10 |

[^1]: It means that if a failure happens during the execution of a line, we assume that the line is covered.
