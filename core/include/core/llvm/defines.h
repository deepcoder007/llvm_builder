//
// Created by vibhanshu on 2024-06-07
//


#ifndef CORE_INTERFACE_DEFINES_H
#define CORE_INTERFACE_DEFINES_H

#include "core/util/defines.h"

#define LLVM_NS()     CORE_NS()::llvm_proxy
#define LLVM_NS_BEGIN namespace LLVM_NS() {
#define LLVM_NS_END   }

#define FOR_EACH_LLVM_INT_TYPE(macro)                \
    macro(int8)                                      \
    macro(int16)                                     \
    macro(int32)                                     \
    macro(int64)                                     \
/**/                                                 \

#define FOR_EACH_LLVM_UINT_TYPE(macro)               \
    macro(uint8)                                     \
    macro(uint16)                                    \
    macro(uint32)                                    \
    macro(uint64)                                    \
/**/                                                 \

#define FOR_EACH_LLVM_FLOAT_TYPE(macro)              \
    macro(float32)                                   \
    macro(float64)                                   \
/**/                                                 \
   
#define FOR_EACH_LLVM_TYPE(macro)                    \
    macro(bool)                                      \
    FOR_EACH_LLVM_INT_TYPE(macro)                    \
    FOR_EACH_LLVM_UINT_TYPE(macro)                   \
    FOR_EACH_LLVM_FLOAT_TYPE(macro)                  \
/**/                                                 \

#define FOR_EACH_ARITHEMATIC_OP(macro)               \
    macro(add)                                       \
    macro(sub)                                       \
    macro(mult)                                      \
    macro(div)                                       \
    macro(remainder)                                 \
/**/                                                 

#define FOR_EACH_LOGICAL_OP(macro)                   \
    macro(less_than)                                 \
    macro(less_than_equal)                           \
    macro(greater_than)                              \
    macro(greater_than_equal)                        \
    macro(equal)                                     \
    macro(not_equal)                                 \
/**/                                                 

#define FOR_EACH_BINARY_OP(macro)                    \
    FOR_EACH_ARITHEMATIC_OP(macro)                   \
    FOR_EACH_LOGICAL_OP(macro)                       \
/**/

#endif // CORE_INTERFACE_DEFINES_H
