C++ Multithreaded Sudoku Validator using POSIX
==============================================
### How to Compile 
```bash
$ g++ -std=c++11 -pthread Sudoku-Validator.cpp -o sudoku-validator
```

### How to Execute
```bash
$ ./sudoku-validator <sudoku-solution-filename> <results-filename>
```

### NOTE:
*The default number of threads is 3, but changing the NUM_THREADS variable in the source code will adjust the number of threads created.* 
*A sample valid sudoku solution is provided for tests.*

### Expected Output for Valid Sudoku Solution using 3 Threads:
```bash
[Thread 1] Row 1: Valid
[Thread 2] Column 1: Valid
[Thread 3] Row 2: Valid
[Thread 1] Column 2: Valid
[Thread 2] Row 3: Valid
[Thread 3] Column 3: Valid
[Thread 1] Row 4: Valid
[Thread 2] Column 4: Valid
[Thread 3] Row 5: Valid
[Thread 1] Column 5: Valid
[Thread 2] Row 6: Valid
[Thread 3] Column 6: Valid
[Thread 1] Row 7: Valid
[Thread 2] Column 7: Valid
[Thread 3] Row 8: Valid
[Thread 1] Column 8: Valid
[Thread 2] Row 9: Valid
[Thread 3] Column 9: Valid
[Thread 1] Subgrid R13C13: Valid
[Thread 2] Subgrid R13C46: Valid
[Thread 3] Subgrid R13C79: Valid
[Thread 1] Subgrid R46C13: Valid
[Thread 2] Subgrid R46C46: Valid
[Thread 3] Subgrid R46C79: Valid
[Thread 1] Subgrid R79C13: Valid
[Thread 2] Subgrid R79C46: Valid
[Thread 3] Subgrid R79C79: Valid
Valid rows: 9
Valid columns: 9
Valid subgrids: 9
This Sudoku solution is: Valid
```
