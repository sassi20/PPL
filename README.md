A C++ reimplementation of a miniature probabilistic programming language (mini-PPL), featuring a parser, execution engine, and three inference algorithms: Likelihood Weighting, Sequential Monte Carlo, and Metropolis-Hastings.


## Requirements

* C++ compiler with **C++17** support or later (`g++`, `clang++`, etc.)
* macOS / Linux (tested on macOS with Apple Clang)

## How to Compile and Run

From the project's root directory:

```bash
g++ -std=c++17 -Wall -Wextra \ src/tiny_ppl_core.cpp \ src/parser.cpp \ src/primitives.cpp \ src/inference.cpp \ src/machine.cpp \ src/main.cpp \ -o main
./main
```

**Important:** You must compile **all** source files. Compiling only `main.cpp` will cause the linker to fail with undefined symbol errors.

### Alternative - Using the Makefile

A Makefile is provided for convinience. If you have `make` installed:

```bash
make
./main
```

To remove the executable:

```bash
make clean
```

## Project Structure

```text
mariappl/
├── README.md          ← this file
├── dev_notes.md       ← development notes for the project
├── Makefile           ← Make build file
└── src/
    ├── main.cpp       ← main program (tests)
    ├── tiny_ppl_core.h ← core language declarations
    ├── tiny_ppl_core.cpp ← core language implementation 
    ├── parser.cpp ← parser implementation 
    ├── primitives.cpp ← built-in primitive operations 
    ├── machine.cpp ← state machine implementation 
    └── inference.cpp ← inference algorithms

## Expected Output

When you run `./main`, you should see tests for:

* Normal and Bernoulli distributions (`log_prob`)
* Sampling with a fixed random seed
* `MachineState` initialization


Feel free to check out the Developing Notes (dev_notes.md) where I go into a bit more detail about how I approached this project — what each part does, how it ended up organized, and some of the more interesting bumps I ran into along the way and how I aproached them.
