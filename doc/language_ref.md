# Complete tutorial of the language

To ensure language is thin, we ensure that a single tutorial which can be read and understood in an hour should be enough to understand everything about the framework if user is proficient in any host programming language. The target to make this language even more lean compared to embedded languages like LUA. 
 
 * language has following primitives:
     * events
     * types
         * integral and floating point types (scalar types)
         * array and vector of scalar types
         * C-style struct types with only scalar or pointer type fields
         * array of pointer types 
         * pointer type (only allowed of struct and array)
     * operations 
         * arithematic 
         * logical
         * vector 
         * load/store


* language semantic
    * host language generates events
    * event logic is defined by DSL as a circuit which load all the states at the start of event and states are updated by this circuit at the end of event.
    * no data races as read of any state during the DSL code execution is what at the start of event is event if it is changed during execution. all the udpates in state are reflected only after end of event. and only one state update is done (i.e. last state update on that variable in DSL).
        * if we define this in same event when pre-event value of x = 1:
            > x.store(10)
            > x.store(3)
            > x.store(x.load() + 1)
           x will have value 2 after the event as x.load() will always return 1 during event event if we have x.store(3) defined before and only the last store instruction will be generated and first 2 store instructions will be ignored by compiler
