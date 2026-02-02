#ifndef COMMON_LLVM_TEST_H_
#define COMMON_LLVM_TEST_H_

#define GEN_MODULE_FILE 0
#define WRITE_MODULE_OSTREAM 0


#if WRITE_MODULE_OSTREAM
#if GEN_MODULE_FILE
#define INIT_MODULE( MODULE )                                       \
    CORE_INFO << " ========= BEGIN WRITE MODULE ===============";   \
    MODULE. write_to_ostream();                                     \
    CORE_INFO << " ========= END WRITE MODULE =================";   \
    MODULE. write_to_file();                                        \
    CORE_ALWAYS_ASSERT(MODULE.is_init());                           \
/**/
#else
#define INIT_MODULE( MODULE )                                       \
    CORE_INFO << " ========= BEGIN WRITE MODULE ===============";   \
    MODULE. write_to_ostream();                                     \
    CORE_INFO << " ========= END WRITE MODULE =================";   \
    CORE_ALWAYS_ASSERT(MODULE.is_init());                           \
/**/
#endif
#else
#if GEN_MODULE_FILE
#define INIT_MODULE( MODULE )                                        \
    MODULE. write_to_file();                                         \
    CORE_ALWAYS_ASSERT(MODULE. is_init());                           \
/**/
#else
#define INIT_MODULE( MODULE )                                        \
    CORE_ALWAYS_ASSERT(MODULE.is_init());                            \
/**/
#endif
#endif

#endif // COMMON_LLVM_TEST_H_
