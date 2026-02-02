//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_LLVM_MODULE_H_
#define LLVM_BUILDER_LLVM_MODULE_H_

#include "llvm_builder/type.h"
#include "llvm_builder/meta/noncopyable.h"
#include "llvm_builder/defines.h"
#include "llvm_builder/util/cstring.h"
#include "llvm_builder/util/object.h"
#include "llvm_builder/function.h"
#include "value.h"

#include <unordered_map>
#include <memory>
#include <functional>

namespace llvm {
    class Module;
    namespace orc {
        class ThreadSafeModule;
    }
};

LLVM_BUILDER_NS_BEGIN

class CursorPtr;
class Function;
class Module;
class PackagedModule;
class JustInTimeRunner;

// TODO{vibhanshu}: do we need one context or multiple?
class Cursor : public _BaseObject<Cursor> {
    using BaseT = _BaseObject<Cursor>;
    friend class CursorPtr;
public:
    class Impl;
public:
    CONTEXT_DECL(Cursor)
    using on_main_module_fn_t = std::function<void(Module&)>;
    using on_module_fn_t = std::function<void (Module)>;
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit Cursor(const std::string& name);
    explicit Cursor();
    ~Cursor();
public:
    const std::string& name();
    Module main_module();
    Module gen_module();
    void main_module_hook_fn(on_main_module_fn_t&& fn);
    bool is_bind_called();
    void bind();
    void cleanup();
    void for_each_module(on_module_fn_t&& fn);
    bool operator == (const Cursor& o) const;
    static Cursor null();
};

class ModuleImpl;

class Module : public _BaseObject<Module> {
    using BaseT = _BaseObject<Module>;
    friend class PackagedModule;
    friend class Cursor::Impl;
    // TODO{vibhanshu}: do we need to support inner modules ? probably inner module
    //                  can help seperate declaration and definition modules
    CONTEXT_DECL(Module)
    class Impl;
private:
    std::weak_ptr<Impl> m_impl;
private:
    explicit Module(const ModuleImpl& impl);
public:
    explicit Module();
    ~Module();
public:
    bool is_init() const;
    const std::string& name() const;
    const std::vector<LinkSymbol>& public_symbols() const;
    TypeInfo struct_type(const std::string &name) const;
    llvm::Module *native_handle() const;
    std::vector<std::string> exported_symbol_names() const;
    std::vector<std::string> transformed_public_symbols(std::function<std::string(const LinkSymbol&)>&& fn) const;
    std::string public_symbol_name(const std::string& symbol) const;
    bool contains(const std::string& symbol) const;
    void register_symbol(const LinkSymbol& link_symbol);
    void init_standard();
    PackagedModule package();
    Function get_function(const std::string& name);
    void add_struct_definition(const TypeInfo& struct_type);
    void write_to_file() const;
    void write_llvm_output(const std::string& suffix = "") const;
    void write_to_ostream() const;
    bool operator == (const Module& o) const;
    static Module null();
};

class PackagedModule : public _BaseObject<PackagedModule> {
    using BaseT = _BaseObject<PackagedModule>;
    friend class JustInTimeRunner;
public:
    class Impl;
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit PackagedModule(Module module);
    explicit PackagedModule();
    ~PackagedModule();
public:
    bool is_valid() const;
    const std::string &name() const;
    const std::vector<LinkSymbol> &public_symbols() const;
    llvm::orc::ThreadSafeModule* native_handle() const;
    const TypeInfo& struct_type(const std::string &name) const;
    bool operator == (const PackagedModule& o) const;
    static PackagedModule null();
public:
    static PackagedModule from_module(Module m);
};

LLVM_BUILDER_NS_END

#endif // LLVM_BUILDER_LLVM_MODULE_H_
