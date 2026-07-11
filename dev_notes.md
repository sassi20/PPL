## Development Notes

This project was developed as the final assignment for the **Probabilistic Programming** course. The goal was to reimplement a miniature probabilistic programming language (mini-PPL), originally written in Python, in a different language. The original implementation consists of a complete interpreter—including a parser, execution machine, closures, and three inference algorithms (Likelihood Weighting, Sequential Monte Carlo, and Metropolis-Hastings)—that communicate through a message-passing interface based on `sample`, `observe`, and `done`.

I chose to implement the system in **C++**, aiming to stay as faithful as possible to the original design while taking advantage of static typing, a more declarative architecture, and the language's performance characteristics.

The first step was defining the core structures: `Closure` and `Machine`. The main challenge quickly became apparent:
 **How can Python's dynamic runtime values be represented in a statically typed language like C++?**
To solve this, I introduced a polymorphic `Value` hierarchy capable of representing every value manipulated by the interpreter. This design allows heterogeneous values to coexist in the value stack and execution environment while preserving type safety.

During this stage I also relied on several C++ standard library facilities as: `std::shared_ptr` and `std::dynamic_pointer_cast` for shared ownership and safe polymorphism; `std::mt19937` from `<random>` for deterministic sampling; `<limits>` for numerical operations required by probability distributions and a buch of other packages that you can se on the beginning of `tiny_ppl_core.h` all with a comment on the side with what was used from them.

Once this first representation was in place, I implemented the supported probability distributions (Normal,Bernoulli) together with `log_prob`, which computes the likelihood of an observed value under a given distribution.

The next milestone was implementing the interpreter itself.

`initMachine` parses the input program, builds the global environment, creates closures for every function definition, and initializes the execution machine for the program's entry point.

`pushBody` translates sequences of expressions into a collection of low-level instructions that populate the control stack.

`send` resumes execution after a probabilistic operation pause by delivering the sampled or observed value back to the suspended machine.

`resume` executes the interpreter loop, repeatedly evaluating instructions until reaching a `sample`, `observe`, or `done` message.


While implementing `resume`, I discovered that representing the control stack as a `std::vector<std::string>` was insufficient. This was because the original interpreter stores heterogeneous tuples such as:

* `('ev', expr, env, addr)`
* `('letk', binds, i, body, env, addr)`
* `('ifk', then, else, env, addr)`

These instructions combine different kinds of data—expressions, environments, addresses, indices, and values—which cannot be represented as plain strings.

To address this, I introduced dedicated `instruction` type together with an `Expr` hierarchy representing the program's abstract syntax tree (AST). Separating expressions from runtime values greatly simplified the evaluation process, brought the interpreter structure closer to the original implementation, and made the execution flow easier to reason about by explicitly handling each instruction and expression type.


During this stage I also implemented the set of arithmetic and logical **primitives**, as well as the `Message` structure, which encapsulates the interpreter state whenever execution pauses so that inference algorithms can decide how to proceed before resuming execution.

In the original interpreter, symbols such as `+`, `-`, `*`, `/`, comparison operators, and distribution constructors (`normal` and `bernoulli`) where resolved as built-in functions rather than user-defined closures. To model this behavior, I introduced a `PrimitiveValue` type that stores both the primitive's name and the C++ function implementing it's behavior. All primitives are registered in a global lookup table, allowing the evaluator to resolve them by name during execution. Each primitive receives a vector of runtime `Value` objects and returns another `Value`, making them "integrate" naturally with the rest of the interpreter. Since every argument is represented by the polymorphic `Value` hierarchy, helper functions performing type checks before extracting numeric values. This keeps the evaluator generic while ensuring that invalid primitive applications produce meaningful runtime errors. Besides implementing the arithmetic (`+`, `-`, `*`, `/`) and comparison (`<`, `>`, `<=`, `>=`, `=`) operators, the primitive table also provides constructors for the supported probability distributions (`normal` and `bernoulli`), allowing probabilistic expressions to be handled uniformly by the execution machine.

After this I went ahead implementing a major component the parser

Although implementing the parser was not strictly part of the project requirements and could have been considered a provided component, implementing it made it possible to test the entire pipeline, from source code to execution, without manually constructing ASTs. It's implementation is contained in `parser.cpp`.

To correctly support recursive and mutually recursive functions, I first populate the global environment with every function definition. Once all closures have been created, I perform a second pass to update each closure with the completed global environment. This logic lives in initMachine, not in the parser, the parser is only responsible for turning the program's text into an Expr tree; it's initMachine that walks that tree, builds the global environment, and does this

After having ready all of my "skeleton", I went ahead on the implementations to cover the core of the project, the inference algorithms including Likelihood Weighting (LW), Sequential Monte Carlo (SMC), and Single-Site Metropolis-Hastings (SSMH). 

`softmax` was used for converting a vector of logarithmic weights into a vector of normalized probabilities. Nothing really fancy, subtract the max before exponentiating so it doesn't blow up numerically. Had to write this from scratch since there's no numpy handing it to you in C++.

`run_lw` is the function serves as the controller for a single execution pass within the Likelihood Weighting (LW) inference. It executes the probabilistic program from it's initial state and upon encountering a Message::SAMPLE where it draws a value from the prior distribution, or a Message::OBSERVE where it calculates the log-probability of the observed value and accumulates this into the MachineState's log_w attribute and building the trace's log-weight. The function returns the final computed value and the accumulated log_w upon completion.

`likelihood_weighting` is the function that orchestrates the complete Likelihood Weighting inference algorithm. Given an program, a random number and a N, it performs N independent calls to run_lw, collecting N sampled values and their corresponding log-weights. It utilizes the softmax function to normalize these collected log-weights, transforming them into a valid probability distribution. The function outputs the vector of sampled values and their normalized weights, enabling weighted estimation of the posterior distribution.

`advance` is the specialized helper function primarily used by the Sequential Monte Carlo (SMC) controller. Its purpose is to advance the execution of a given MachineState (particle) through the probabilistic program. It iteratively calls resume, handling any Message::SAMPLE by drawing a new value and continuing execution. The function's execution pauses and returns the generated Message only when it encounters a Message::OBSERVE or Message::DONE. This mechanism ensures that all particles in an SMC ensemble can be synchronized at common 'breakpoints' (observation points) before proceeding.

`run_smc` implements the Sequential Monte Carlo inference algorithm. It manages a population of N particles, each representing a potential trace of the program. The algorithm covers this synchronized stages:
- **Initialization:** N `MachineState` particles are created, each with an independent random number generator.
- **Advancement:** All particles are advanced in parallel to their next synchronized breakpoint (`observe` or `done`) using the `advance` function.
- **Termination:** If all particles reach `Message::DONE`, their final values are collected and returned.
- **Observation/Resampling:** If all particles reach a `Message::OBSERVE` breakpoint, an incremental log-weight is calculated for each based on the observation. A resampling step is then performed, where new particles are created by probabilistically forking (duplicating) existing particles based on these weights, concentrating the computational effort on more probable traces.

`mh_log_alpha`is a function  which computes the logarithm of the Metropolis-Hastings acceptance ratio specifically for a single-site proposal. It takes both the current and proposed program traces (comprising values, sample log-probabilities, and observe log-probabilities) along with the `resampled_address`. It identifies which sites were newly generated (`forward_sites`) or discarded (`rev`) during the proposal. The function then aggregates log-probabilities from the relevant sample and observe sites, incorporating a dimension-correction term based on the trace lengths. The output `log_alpha` determines the probability of accepting the proposed trace.

`run` is thefunction that is tailored for generating proposed traces within the Single-Site Metropolis-Hastings (SSMH) algorithm. It executes the probabilistic program, managing sample and observe messages. Crucially, when a Message::SAMPLE for address a is encountered it checks the following:
- If a matches x0 (the target address for resampling) OR if a is not found in the cache (indicating a newly explored site) a new value is sampled from the distribution.
- Otherwise (if a is not x0 and is present in the cache), the existing value for a from the cache is explicitly reused. Ensuring that only the intended site is resampled, while other relevant parts of the trace are preserved. The function returns the final value and the generated traces (X, S, O). 

`single_site_mh` is a function implements the Single-Site Metropolis-Hastings inference algorithm. It begins by generating an initial trace (X, S, O). Subsequently, it iterates for a specified number of steps:
- **Proposal:** A random address (`random_address`) is selected from the current trace. A proposed trace (`proposed_trace_values`, `proposed_sample_log`, `proposed_observe_log`) is then generated by calling the specialized `run` function, which attempts to resample at `random_address` while reusing other values from the current trace (`current_trace_values`).
- **Acceptance:** The `mh_log_alpha` function is used to calculate the log-acceptance ratio. This ratio is then compared against a randomly drawn log-uniform value. If the proposal is accepted, the current trace is updated to the proposed trace (`proposed_value`, `proposed_trace_values`, `proposed_sample_log`, `proposed_observe_log`).
- **Chain Collection:** After the warmup period, the value of the current trace is appended to `posterior_chain`, forming the final posterior sample. This C++ implementation accurately reflects the full `single_site_mh` algorithm as designed in Python.


Reimplementing this mini-PPL from scratch was, above all, a walkthrough of everything we covered in the course — going from a working Python implementation to writing every piece myself in C++ forced me to understand the design, rather than just follow along. Writing the three inference algorithms side by side over the same runtime — Likelihood Weighting, SMC, and single-site Metropolis-Hastings — made the "one runtime, many algorithms" idea concrete: watching the same `sample`/`observe` messages get handled completely differently depending on which controller was consuming them. It showed me why keeping the evaluator free of any inference logic actually matters, instead of just being a nice architectural principle.

I did lean on Claude for a couple of specific pieces mainly the parser and the primitive functions, which in the Python version came for free from existing packages and simply didn't exist in C++. But havig to actually write those from scratch, instead of importing them, ended up teaching me things the Python version kept quietly hidden — like why an environment shared by reference in Python turns into a recursion bug the moment it's copied by value in C++, or why a random number generator's state needs to be manually synced across calls when the language doesn't do it automatically.

Side note: project structure

As a last step on this project, I decided to restructure and re-organize the hole project so each part would be easier to look at on its own, instead of being crammed together all in one file which, full disclosure, was exactly how I worked through the entire build before this final tidy-up. I'm leaving a small diagram showing how it ended up:


```text
└── src/
    ├── main.cpp           ← main program (tests)
    ├── tiny_ppl_core.h    ← core language declarations
    ├── tiny_ppl_core.cpp  ← core language implementation 
    ├── parser.cpp         ← parser implementation 
    ├── primitives.cpp     ← built-in primitive operations 
    ├── machine.cpp        ← state machine implementation 
    └── inference.cpp      ← inference algorithms
``` 