## C Conventions

* Each file's header (.c/.h) must include at least the authors of that specific file. It is highly recommended to briefly describe what the file contains/defines/implements.
* All headers should look consistent across all files.
* Almost every function should be accompanied by a documentation comment (docstring)
* We will use Doxygen, clang-tidy, and clang-format.


| X | Convention |
| :--- | :--- |
| **Functions** | `snake_case` |
| **Variables** | `camelCase` |
| **Struct / typedef** | `PascalCase` |
| **Macros / `#define` constants** | `UPPER_SNAKE_CASE` |
| **File names** | `snake_case`|


## Git Conventions
* Using Git branches are required (assignment)
    * Nothing should be pushed directly to the main branch
    * Branch naming must follow naming convencion
      * Format: slash/kebab-case
      * feat/ (new functionality)
      * fix/ (normal bug fix)
      * hotfix/ (urgent production fix)
      * chore/ (maintenance/dependencies)
      * test/
      * docs/
      * release/

* Before merging with main you should run 'make dev' and after that valgrind
  * Make dev format and checks for bugs
* Git commit conventions: https://www.conventionalcommits.org/en/v1.0.0/

## Other Conventions
* The entire project will be English only

## Examples
```c
/**
 * @file    main.c
 * @author  xlogin00
 * @brief   Brief info
 *
 * More description
 */
```


```c
/**
 * @brief Computes the sum of two integers
 *
 * @param a First operand
 * @param b Second operand
 * @return  Sum of a and b
 */
int addNumbers(int a, int b);
```
