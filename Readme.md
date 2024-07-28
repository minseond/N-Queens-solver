# N-Queens Solver
This project is a multithreaded solution for the N-Queens problem, where N queens are placed on an N×N chessboard such that no two queens threaten each other.

### Main Components
- **Bounded Buffer**: Thread-safe queue for producer-consumer communication.
- **Stack**: Manages queen placements.
- **Multithreading**: 
  - `producer`: Explores and enqueues valid queen placements.
  - `consumer`: Dequeues and prints solutions.
- **Signal Handling**: Gracefully handles `SIGINT` to display the total solutions found.

### Main Functions
- `find_n_queens(int N)`: Solves the N-Queens problem.
- `find_n_queens_with_prepositions(int N, struct stack_t *prep)`: Solves with pre-positioned queens.


## Requirements Fulfillment
1. **Command-line Argument for Threads**:
   - The program uses `getopt()` to parse the number of concurrent threads from the command-line argument (`-t num_threads`).

2. **Bounded Buffer for Parallelization**:
   - Implemented a `bounded_buffer` structure to allow producer threads to enqueue valid queen placements and a consumer thread to dequeue and print these placements.

3. **Safe Printing of Solutions**:
   - Ensured that feasible N-Queen arrangements are printed to the standard output without being intermixed due to race conditions by using a single consumer thread for printing.

4. **Graceful Termination on Ctrl+C**:
   - Implemented a signal handler (`handle_sigint`) to catch the `SIGINT` signal (Ctrl+C). When triggered, it prints the total number of solutions found up to that point and terminates the program gracefully.

5. **Minor Code Changes and New Functions**:
   - Modified the existing code to incorporate the above functionalities and added necessary functions for bounded buffer operations, signal handling, and multithreading.


## Run Example
1. make
2. ./nqueens -t 4
