//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_META_BASIC_H
#define LLVM_BUILDER_META_BASIC_H

#include "llvm_builder/defines.h"

#include <type_traits>

#define LLVM_BUILDER_META_NS()     LLVM_BUILDER_NS()::meta
#define LLVM_BUILDER_META_NS_BEGIN namespace LLVM_BUILDER_META_NS() {
#define LLVM_BUILDER_META_NS_END   }

#endif
