# README

This guide explains how to compile and execute the helper program along with your solution.

## Steps to Compile and Execute

1. **Setup Directory Structure**
   Place the following files in the **same directory**:
   - The **helper program** (e.g., `helper-program.c`)
   - The **test case generator**
   - Your **solution code** (e.g., `solution.c`)

2. **Compile Your Solution**
   Compile your solution code into an executable named `solution`:
   ```bash
   gcc solution.c -lpthread -o solution
   ```

3. **Set Test Case Parameters**
   - Open the test case generator file.
   - Set the desired test case parameters, including the test case number, near the top of the file.
   - Run the test case generator to generate the test case.

4. **Compile and Run the Helper Program**
   Compile the helper program using:
   ```bash
   gcc helper-program.c -lpthread -o helper
   ```
   Run the helper program by passing the test case number as a command-line argument:
   ```bash
   ./helper <testcase-number-here>
   ```
   Replace `<testcase-number-here>` with the test case number you set in the test case generator.

## Example Usage
Assuming you have `solution.c`, `helper-program.c`, and a test case generator ready:

1. Place all files in the same directory.

2. Compile your solution:
   ```bash
   gcc solution.c -lpthread -o solution
   ```

3. Set the test case parameters in the test case generator and run it.

4. Compile the helper program:
   ```bash
   gcc helper-program.c -lpthread -o helper
   ```

5. Execute the helper program with test case `1`:
   ```bash
   ./helper 1
   ```

This will run the helper program using the specified test case and your solution executable.
