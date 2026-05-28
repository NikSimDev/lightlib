# Contributing to lightlib

 First off, thank you for considering contributing to lightlib. It's people
 like you that make lightlib a great tool.

## Code of Conduct

 This project and everyone participating in it is governed by the lightlib
 Code of Conduct. By participating, you are expected to uphold this code.
 Please report unacceptable behavior to **hello@lightlib.org**.

## How Can I Contribute?

### Reporting Bugs

 Before creating bug reports, please check the issue tracker as you might
 find out that you don't need to create one. When you are creating a bug
 report, please include as many details as possible:

 - Use a clear and descriptive title
 - Describe the exact steps which reproduce the problem
 - Describe the behavior you observed after following the steps
 - Explain which behavior you expected to see instead and why
 - Include code snippets or links to repositories that demonstrate the issue

 **Please do not report security vulnerabilities in public issues!**
 Instead, report by email to **hello@lightlib.org** with `[SECURITY]` in the
 subject line.

### Suggesting Enhancements

 Enhancement suggestions are tracked as issues. When creating an enhancement
 suggestion, please include:

 - Use a clear and descriptive title
 - Provide a step-by-step description of the suggested enhancement
 - Provide specific examples to demonstrate the steps
 - Describe the current behavior and explain which behavior you expected to
   see instead
 - Explain why this enhancement would be useful

### Pull Requests

 - Fill in the required pull request template
 - Do not include issue numbers in the PR title
 - Follow the C++ style guide (see below)
 - Include thoughtfully-worded, well-structured tests
 - Document new code
 - End all files with a newline
 - **Sign off your commits**: `git commit -s -m "Your message"`
 - Ensure every new or modified file has the LGPL header (see example below)

## Getting Started

 1. Fork the repository on GitVerse:
    `https:gitverse.ru/SergeevSEVSU/lightlib`
 2. Clone your fork:
    ```bash
    git clone https:gitverse.ru/your-username/lightlib.git
    cd lightlib
    ```
 3. Build the project using CMake and vcpkg:
    ```bash
    mkdir build && cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build . --config Release
    ```

 Now you can create branches, start adding features & bugfixes, and create
 pull requests.

## Style Guides

### C++ Style Guide

 Based on the existing lightlib code style (see `Model.hpp` for reference):

 - Use 4 spaces for indentation (not tabs)
 - Use `snake_case` for functions and variables
 - Use `PascalCase` for class names
 - Use `UPPER_CASE` for macros and constants
 - Place opening braces on the same line for functions
 - Use `#pragma once` instead of include guards
 - Keep lines under 100 characters when possible
 - Include headers in the following order:
   1. Corresponding header for the .cpp file
   2. C++ standard library headers (`<string>`, `<vector>`, `<map>`, etc.)
   3. Third-party library headers (`<nlohmann/json.hpp>`)
   4. lightlib internal headers (`Database.hpp`, `Logger.hpp`, etc.)

### Required License Header

 Every source file (`.cpp`, `.h`, `.hpp`) must start with the LGPL header:

 ```cpp
 /*
  * Copyright (c) 2025 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
  * SPDX-License-Identifier: LGPL-3.0-or-later
  *
  * This file is part of lightlib.
  *
  * lightlib is free software; you can redistribute it and/or modify
  * it under the terms of the GNU Lesser General Public License as published by
  * the Free Software Foundation; either version 3 of the License, or
  * (at your option) any later version.
  *
  * lightlib is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public License
  * along with lightlib; if not, see <https:www.gnu.org/licenses/>.
  */
 ```

### Comments

 - Write all comments in English
 - For new public API, use [Doxygen](http:www.doxygen.nl/) format:

 ```cpp
 /**
  * @brief Brief description of the method
  * @param param_name Description of parameter
  * @return Description of return value
  */
 ```

### Code Example

 Following the pattern from `Model.hpp`:

 ```cpp
 namespace lightlib {

 template <typename Derived>
 class Model {
 public:
     static ModelQueryBuilder<Derived> query() {
         return ModelQueryBuilder<Derived>(Derived::table_name);
     }

     void setAttribute(const std::string& key, const std::string& value) {
         if (isField(key)) {
             attributes[key] = value;
         }
     }

     bool save() {
          implementation
         return true;
     }

 private:
     std::map<std::string, std::string> attributes;
 };

 }  namespace lightlib
 ```

## Sign Your Commits

 All commits must be signed off, certifying that you have the right to
 submit the code under LGPL-3.0-or-later:

 ```bash
 git commit -s -m "Your commit message"
 ```

 The sign-off line should look like:
 ```
 Signed-off-by: Kirill Sergeev <kirill@lightlib.org>
 ```

 By signing off, you agree to the
 [Developer Certificate of Origin](https:developercertificate.org/) (DCO).

## Building and Testing

 Before submitting a PR, make sure your changes compile:

 ```bash
 mkdir build && cd build
 cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
 cmake --build . --config Release
 ```

 If you add new functionality, please add corresponding tests. If you fix a
 bug, write a test that breaks in the old version but works in the new one.

## Branch Naming

 Use descriptive branch names to make your intent clear:

 - `bugfix/123-fix-memory-leak` — for bug fixes (123 = issue number)
 - `feature/456-add-websocket` — for new features
 - `docs/update-readme` — for documentation changes
 - Skip the number if no issue exists: `bugfix/fix-typo`

## Commit Messages

 Write clear, descriptive commit messages in English:

 - Good: "Add WebSocket support for async handlers"
 - Good: "Fix connection pool memory leak"
 - Bad: "fix bug" or "update code"

## Project Maintainers

 Collaborators with write access should follow these guidelines:

 1. Ensure all tests pass before merging
 2. Use **Squash and Merge** for PRs with multiple small commits
 3. Write a descriptive commit message for the squashed PR (or use the PR
    title)
 4. Keep the `main` branch history clean and readable

## License

 By contributing to lightlib, you agree that your contributions will be
 licensed under LGPL-3.0-or-later.

 ----------------------------------------------------------------------------
 Thank you for contributing to lightlib!
 ----------------------------------------------------------------------------*/*/