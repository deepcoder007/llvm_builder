# Motivation

I have been working as software developer in Quant Trading domain for 9+ year at the time of writing. My work experience span a mix of C++/Python based market-data processing, low-latency strategy development, order management and execution platform. 
Low-latency trading strategy and execution development I found quite challenging and intense because of the technical complexity and depth required to build a production quality product.  
Over my career I developed few ideas to solve hard problems in strategy implementation and this project is an attempt to materialize those ideas.  
Strategy implementation I found challenging because of the following recurrent patterns:

### Performace 
Tick-data/market-data volume in financial markets is monotonically increasing, hence data processing logic needs to be "thin", i.e. developers work harder to make code efficient. Performance issues are linked to modern programming languages using higher level of abstractions and programming safety features. 
 * In an ideal world developers implement software in assembly instruction-by-instruction. and will be conscious about wasted cpu cycles. assembly coding is not practical to build applications of the scale popular now. 
 * while scaling codebase, it's hard to maintain performance objectives of system. developers with complementary skillsets might not be that focused on performance because of more pressing concerns like time-to-market, un-expected business feature requirements.
 * higher level languages add layers of abstraction and checks at compile-time and runtime. 
   * abstractions are not always zero-cost. e.g. RAII constructs in C++ often show up in perf benchmark as bottleneck
   * scale of codebase leads to "accidental" performance bottlenecks e.g. syscalls and multi-threading synchornizations behind abstractions which are not always filtered in code-review.
   * harder to enforce coding and design standards via code-review for maintainers because of scale and rush to go-live.
   * lot of tools for doing performance benchmarking but hard to use them on live and industrial applications.
 * Compile-time languages like C++/C/Rust are optimal for performance but:
   * development phase is slow for large codebases as frequent **whole project** re-compile required for even minor changes for testing.
   * high entry barrried for developers to start development on even basic applications.
   * strong type system and other langauge safety features come at the cost of high learning curve and post production maintenance cost because of "thick" language.
   * debugging is harder for optimized code.
 * Interpreted languages like Python/javascript/R are used in research but not in production because:
   * latency of code translation and dynamic resolution of symbols
   * repetitive executions of code does repetitive tasks like symbol resolution, type-checks, null pointer checks etc.
   * even if JIT is available, it's so far not been possible to get predictable performance like compiled languages.
   * need to convert code to a compiled language for productionization

### Backward and forward compatibility of software
   * portability and backward compatibility issues lead to high development and maintenance costs.
   * newer versions of third-party dependencies require const update and testing of the code-base.
   * New feature implementation might require a full-rewrite or significant upgrade of codebase.

### Maintenace and Extensibility of product
  * redundant features in language and third-party dependencies accumulated over years are challenging for developers to maintain.
  * modern feature heavy programming languages have steap learning curve for production usage.
  * evolving nature of business requirements mean, maintenance costs keep on rising monotonically over time.


### Interoperability with existing softwares
  * hard to integrate new platform with an existing stable tech stack as technologies are not always developed with interoperability as priority.
  * moving existing business and retiring the existing stack takes time, sometimes years.
  * integrating new language is a very difficult from both developement and knowledge transfer viewpoint.
  * Often there is insufficient supporting toolchain.
  * temporary scaffolding to support integration is often poorly thought off.
  * enforcing migration of a big organization to a different developer toolchain is a big undertaking, often increasing cost.
