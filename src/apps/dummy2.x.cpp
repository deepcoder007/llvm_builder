#include "llvm_builder/value.h"
#include "llvm_builder/analyze.h"
#include "util/debug.h"
#include "util/cstring.h"
#include "llvm_builder/defines.h"
#include <iostream>
#include <string>

#include "util/string_util.h"
#include "llvm_builder/module.h"
#include "llvm_builder/type.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/function.h"
#include "llvm_builder/control_flow.h"
#include "llvm/context_impl.h"


LLVM_BUILDER_NS_BEGIN

class StdOutCallback : public object::Callback {
    using object_t = typename object::Callback::object_t;
private:
    void M_on_new(object_t type, uint64_t id, const std::string& name) override  {
        if (type == object_t::CURSOR) {
            std::cout << "         NEW cursor:" << id << " : " << name << std::endl;
        } else if (type == object_t::MODULE) {
            std::cout << "         NEW module:" << id << " : " << name << std::endl;
        } else if (type == object_t::PACKAGED_MODULE) {
            std::cout << "         NEW packaged module:" << id << " : " << name << std::endl;
        } else if (type == object_t::LINK_SYMBOL) {
            // std::cout << " NEW link_symbol:" << id << " : " << name << std::endl;
        } else if (type == object_t::FUNCTION) {
            std::cout << "         NEW function:" << id << " : " << name << std::endl;
        } else if (type == object_t::CODE_SECTION) {
            std::cout << "         NEW code section:" << id << " : " << name << std::endl;
        } else if (type == object_t::TYPE) {
            std::cout << "         NEW type:" << id << " : " << name << std::endl;
        } else if (type == object_t::VALUE) {
            // std::cout << " NEW value:" << id << " : " << name << std::endl;
        } else {
            std::cout << "         NEW unidentified:" << id << " : " << name << std::endl;
        }
    }
    void M_on_delete(object_t type, uint64_t id, const std::string& name) override {
        if (type == object_t::CURSOR) {
            std::cout << "         DELETE cursor:" << id << " : " << name << std::endl;
        } else if (type == object_t::MODULE) {
            std::cout << "         DELETE module:" << id << " : " << name << std::endl;
        } else if (type == object_t::PACKAGED_MODULE) {
            std::cout << "         DELETE packaged module:" << id << " : " << name << std::endl;
        } else if (type == object_t::LINK_SYMBOL) {
            // std::cout << " DELETE link_symbol:" << id << " : " << name << std::endl;
        } else if (type == object_t::FUNCTION) {
            std::cout << "         DELETE function:" << id << " : " << name << std::endl;
        } else if (type == object_t::CODE_SECTION) {
            std::cout << "         DELETE code section:" << id << " : " << name << std::endl;
        } else if (type == object_t::TYPE) {
            std::cout << "         DELETE type:" << id << " : " << name << std::endl;
        } else if (type == object_t::VALUE) {
            // std::cout << " DELETE value:" << id << " : " << name << std::endl;
        } else {
            std::cout << "         DELETE unidentified:" << id << " : " << name << std::endl;
        }
    }
};

LLVM_BUILDER_NS_END

using namespace llvm_builder;
using namespace llvm_builder;

int32_t main(int32_t argc, char **argv) {
    using namespace llvm_builder;
    using namespace llvm_builder;
    (void)argc;
    (void)argv;

    std::cout << " ======= APP BEGIN ======= " << std::endl;
    // object::Counter::singleton().add_callback(std::make_unique<StdOutCallback>());

    std::cout << " ======= APP END ======= " << std::endl;
    return 0;
}

