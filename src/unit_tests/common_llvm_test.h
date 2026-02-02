#ifndef COMMON_LLVM_TEST_H_
#define COMMON_LLVM_TEST_H_

#include <iostream>

#define GEN_MODULE_FILE 0
#define WRITE_MODULE_OSTREAM 0


#if WRITE_MODULE_OSTREAM
#if GEN_MODULE_FILE
#define INIT_MODULE( MODULE )                                       \
    std::cout << " ========= BEGIN WRITE MODULE ===============" << std::endl;   \
    MODULE. write_to_ostream();                                     \
    std::cout << " ========= END WRITE MODULE =================" << std::endl;   \
    MODULE. write_to_file();                                        \
    LLVM_BUILDER_ALWAYS_ASSERT(MODULE.is_init());                           \
/**/
#else
#define INIT_MODULE( MODULE )                                       \
    std::cout << " ========= BEGIN WRITE MODULE ===============" << std::endl;   \
    MODULE. write_to_ostream();                                     \
    std::cout << " ========= END WRITE MODULE =================" << std::endl;   \
    LLVM_BUILDER_ALWAYS_ASSERT(MODULE.is_init());                           \
/**/
#endif
#else
#if GEN_MODULE_FILE
#define INIT_MODULE( MODULE )                                        \
    MODULE. write_to_file();                                         \
    LLVM_BUILDER_ALWAYS_ASSERT(MODULE. is_init());                           \
/**/
#else
#define INIT_MODULE( MODULE )                                        \
    LLVM_BUILDER_ALWAYS_ASSERT(MODULE.is_init());                            \
/**/
#endif
#endif

#endif // COMMON_LLVM_TEST_H_
