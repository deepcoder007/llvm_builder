# Solution Proposed by framework

Our framework is focused on solving mentioned problems in the following way:
* performance critical and actively used part of code is typically just 10-20% of entire codebase. rest of the codebase is either:
    * system startup and recovery management
    * configuration management and sanity checks over external human input
    * less frequently executed code like taking backup, loading latest version of slow changing external input.
    * UI/UX with consumers like reading input from GUI, TUI or from other intermediate input generation via third-party applications and displaying back some output.
* sandboxing this performance sensitive code in a thin **embedded DSL** which is:
    * Simple enough to not require learning anything complex. It has only the most basic operations.
    * Easy to integrate with any wide-spread programming language via C-style foreign-function interface (FFI)
    * Easy to benchmark code and analyze it at instruction level and more transparency in machine level code-generation.
    * In this sandbox only basic data-manipulation is allowed
    * Other operations like disk I/O, configuration management, less frequently executed code, multi-threading, multi-core programming, error handling etc. to be done in host language. 
    * Easy to transfer data back and forth between host language and this DSL sandbox via shared-memory, hence zero-memcopy semantic.
    * Host language invoke events which does data-manipulation and then results can be fetched in real-time
    * No new toolchains, this framework is just a library/module imported in the host language. hence existing host language toolchain will be sufficient
* This embedding is inspired by the way CUDA does computation, i.e. push kernels to GPU
    * DSL defined kernels are scheduled on CPU, no data-transfer is required (unlike CUDA) which require main-memory, GPU-memory data transfers.
    * DSL is very thin and defines only elementary CPU operations, kernel can be pushed to a remote process or process running in another machine.
        * like Lua scripting in NeoVIM, but this DSL is even leaner and performance optimized.
    * Predictable performance as low level binary code generation via LLVM api **at runtime** and before first use.
    * memory allocation (and other I/O syscalls) in host language outside the designated kernel code.
    * software constructs like strings, text processing will be done in host language as per user requirement, framework only for mathematical and logical computation.
    * use macro-programming like LISP to generate our DSL code in host language. 
        * Host language acts as macro-programming language used to construct instruction sequence in DSL
    * type-checking and other sanity checks will be done at system startup configuration stage before instruction lowering in DSL.
        * DSL generated code is pure arithematic and logical operations which act only on data in main-memory, caches.
        * by-pass unpredictable performance hits because of repetitive symbol-resolution, dynamic dispatch, vtable lookups, context-switches, thread-based synchronizations, error handling, as these operations are done in host language and not in performance critical code path.
* DSL is a symbolic-programming framework which makes it easier to:
    * write and debug recursive programming language
    * DSL is defining a circuit via which data flows and updates states after event is triggered
        * logically, data is loaded before event to circuit and all states are updated at the end of the event. 
    * optimized for performance and writing complex transformations as only the logical and mathematical aspects are done in this DSL.
    * using LLMs/AI to generate this code is easier because having less number of elements in DSL, LLMs learn grammar and higher order representation much faster then for more complex languages like C++,Python,Java, Rust etc.
