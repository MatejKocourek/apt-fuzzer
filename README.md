# Fuzzer project (NI-APT 2024/25)

Welcome to my fuzzer project repository.

All 4 tasks are implemented. To provide no regression, all 4 tasks work from the main branch, no tags neccessary. The fuzzer is configurable from the command line, see makefile in the fuzzer directory.

To see descriptions of the problems and how they were tackled, see individual READMEs in the fuzzer directory for tasks [2](fuzzer/README-task2.md), [3](fuzzer/README-task3.md) and [4](fuzzer/README-task4.md), and in directory code-coverage for task [1](code-coverage/README.md).

# Project annotation

The semestral project consists of several tasks that you will implement gradually during the semester,
in order to make a fully-fledged fuzzer.
Each task has a different level of difficulty which is respected in the amount of points you can get for it.

Each task has some compulsory items you have to do to get the full amount of points and also optional extensions
that will give you bonus points.

| #   | Task                                            | Points |  Start |    End |
| --- | ----------------------------------------------- | -----: | -----: | -----: |
| 1   | [Code coverage][task-1]                         |     10 | 10.10. | 24.10. |
| 2   | [Random fuzzing with test minimization][task-2] |     15 |  24.10 |   9.11 |
| 3   | [Greybox fuzzer][task-3]                        |     15 |   9.11 |  30.11 |
| 4   | [Improving your fuzzer][task-4]                 |     20 |   2.12 |    4.1 |

<div class="admonition-block note">
<p>
The end date in the table above is a <i>soft</i> deadline.
This means that if you miss it, it does not mean that you failed the course.
However, handing something on the soft deadline gives you two things:
<ol>
  <li> tentative score with a feedback about your solution,</li>
  <li> ability to improve it.</li>
</ol>
The whole point of this is to provide a forcing function, so you keep a steady progress during the semester.
</p>
<p>
<b>The hard deadline is on 20.1.2024</b>
After that, no further submissions will be possible.
</p>
</div>

Please note that in order for your implementation to be testable with the automated testing framework, it has to adhere to a specific command line interface.
All details are provided in each of the task.

As it is a course about testing, we expect you to **thoroughly test** your programs, at least with unit tests. You should aim at a coverage of at least 70%.

## Handing in the tasks

All your code should be hosted on FIT gitlab.
Share it with `donatpie` with role `reporter`.
After the deadline, we will always use the latest commit right before the deadline as indicated above.
Feel free to use any branching name you want, but we will only check main.
