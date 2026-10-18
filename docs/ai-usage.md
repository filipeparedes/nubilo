# AI usage

This document declares how generative AI was used in the Nubilo project.

## Declaration

The tool Claude, developed by Anthropic, was used in this project as support in the following phases: writing automated tests, writing documentation, diagnosing problems and fixing bugs,
and as a critical interlocutor for code review and technical discussion. All contributions were analysed, selected and validated by me, and I kept intellectual and technical control over 
the design decisions and over the final version of the project. I take full responsibility for the content submitted.

## Tool

| Tool | Provider | Used for |
|---|---|---|
| Claude | Anthropic | Tests, documentation, diagnostics and bug fixing, code review and technical discussion |

## How AI was used

| Area | How it was used | What stays mine |
|---|---|---|
| Tests | Writing unit and integration tests (GoogleTest) for the modules I implemented, including edge cases and failure scenarios. | I decide what must be tested, run every test and review the results. When a test fails, I decide whether the code or the test is wrong. |
| Documentation | Drafting module READMEs, Doxygen comments, architecture notes, architecture decision records (ADRs) and the bibliography structure. | I review and edit every text so it matches the code, and I choose the references. |
| Diagnostics and bug fixing | Analysing compiler errors, failing tests and sanitizer reports, and explaining the cause of a problem. | I confirm the diagnosis in the code, apply the fix and check it with the tests. |
| Code review and technical discussion | Reviewing code I wrote, pointing out problems and comparing alternatives, for example how to structure error handling or concurrency. | I make the decisions. Suggestions I disagree with are not applied. |
| Project planning | Helping to organise sprint planning and retrospectives, and to draft issue and pull request descriptions. | I define the scope, priorities and roadmap, and I edit the final text. |

## What was not delegated

The architecture and design decisions, the scope of the project and the choice of what to build in each sprint are mine. I also write the implementation of the system. AI is used to check, test and document it, not to replace the reasoning behind it.

In a few cases, under time pressure, I asked Claude to write a specific function or fix, which I then read, tested and adjusted before accepting. These cases are limited to small pieces of code inside modules whose design I had already defined.

## Validation

Every contribution from the AI goes through the same checks before it becomes part of the project:

* I read and understand it, and I change or reject what does not fit the design.
* The code is compiled and the whole test suite is run, including a build with AddressSanitizer and UndefinedBehaviorSanitizer enabled.
* Documentation is compared against the code it describes.
* Changes reach the main branch through pull requests under my name, following the Git workflow in [ADR 0003](./decisions/0003-git-workflow.md).

AI output is treated as a suggestion that can be wrong. Generated code and text can look correct and still contain errors, so nothing is accepted without being checked.

## Data protection

The tool was used with the source code and documentation of the project. No personal data of third parties was provided to it, and examples in tests and documentation use generic placeholders such as `user@example.com`.

## Attribution in the repository

Commits and pull requests do not carry AI co-author lines. The use of AI is declared in this document instead, so that it is described in one place with detail.

## Maintenance

This document is updated whenever the way AI is used in the project changes, and a final version accompanies the delivery of the project.