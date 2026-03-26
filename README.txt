/* Group: 09
   Members: PARINKARN Sasina   (EID: sparinkar2, Student ID: 59016540)
            LE Vinh Thanh Linh (EID: vtlle2,     Student ID: 59257310)
            BONGONI Revan      (EID: rbongoni2,  Student ID: 59036838)
*/

CS3103 Project — Midnight Warehouse OS Simulation

Group 9

Members:
  - PARINKARN Sasina      (EID: sparinkar2, Student ID: 59016540)
  - LE Vinh Thanh Linh    (EID: vtlle2,     Student ID: 59257310)
  - BONGONI Revan         (EID: rbongoni2,  Student ID: 59036838)

Project Structure:
  09-project/
  ├── src/
  │   ├── problem1.c        Q1 - Order Compression Pipeline
  │   ├── problem2.c        Q2 - Warehouse Directory Concurrency Control
  │   ├── problem3.c        Q3 - Order Scheduling
  │   └── helpers.c         Provided helper module (simulated delays)
  ├── include/
  │   └── helpers.h         Header for helpers module
  ├── test_cases/
  │   ├── q1/               Sample inputs for Q1 (pipeline config)
  │   ├── q2/               Sample inputs for Q2 (directory operations)
  │   └── q3/               Sample inputs for Q3 (scheduling jobs)
  ├── Makefile              Build script
  ├── 09-report.pdf         Project report
  └── README.txt            This file

How to Compile:
  make                      Build all three programs
  make pipeline             Build Q1 only
  make warehouse            Build Q2 only
  make scheduler            Build Q3 only
  make clean                Remove all compiled executables

How to Run:
  Q1 (Pipeline):
    ./pipeline <P> <M> <N> <num_orders> <T> <cnt_0> ... <cnt_{T-1}> <tA_0> <tB_0> ... <tA_{P-1}> <tB_{P-1}>

    Example (P=3, M=4, N=4, 90 orders, T=3, 2 tokens each, pairs (0,1)(1,2)(0,2)):
      ./pipeline 3 4 4 90 3 2 2 2 0 1 1 2 0 2

  Q2 (Warehouse Directory):
    ./warehouse <input_file>

    Example:
      ./warehouse test_cases/q2/case1.txt

  Q3 (Scheduler):
    ./scheduler <input_file>

    Example:
      ./scheduler test_cases/q3/case1.txt

Environment:
  - Tested on: CS gateway (csgw.cs.cityu.edu.hk)
  - Compiler: gcc with -pthread flag
  - OS: Linux

Known Limitations:
  - None at this time.
