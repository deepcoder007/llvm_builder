//
// Created by vibhanshu on 2024-06-07
//

#include "util/debug.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/module.h"
#include "llvm/context_impl.h"
#include "util/string_util.h"
#include "ext_include.h"

#include <memory>
#include <unordered_map>

LLVM_BUILDER_NS_BEGIN

//
// JustInTimeRunner::Impl
//
class JustInTimeRunner::Impl {
    JustInTimeRunner& m_parent;
    std::unique_ptr<llvm::orc::LLJIT> m_handle;
    std::vector<std::string> m_public_decl_symbols;
    std::vector<std::string> m_public_def_symbols;
    std::vector<std::string> m_namespace_seq;
    std::unordered_map<std::string, runtime::Namespace> m_namespace_map;
    std::unique_ptr<llvm::FunctionPassManager> m_fpm;
    std::unique_ptr<llvm::FunctionAnalysisManager> m_fam;
    std::unique_ptr<llvm::ModuleAnalysisManager> m_mam;
    std::unique_ptr<llvm::LoopAnalysisManager> m_lam;
    std::unique_ptr<llvm::CGSCCAnalysisManager> m_cgam;
    std::unique_ptr<llvm::PassInstrumentationCallbacks> m_pic;
    std::unique_ptr<llvm::StandardInstrumentations> m_si;
    bool m_is_bind = false;
    static inline bool s_llvm_init = false;
public:
    explicit Impl(JustInTimeRunner& parent) : m_parent{parent} {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not parent.has_error());
        LLVM_BUILDER_ALWAYS_ASSERT(Cursor::Context::has_value());
        m_fpm = std::make_unique<llvm::FunctionPassManager>();
        m_fpm->addPass(llvm::InstCombinePass());
        m_fpm->addPass(llvm::ReassociatePass());
        m_fpm->addPass(llvm::GVNPass());
        m_fam = std::make_unique<llvm::FunctionAnalysisManager>();
        m_mam = std::make_unique<llvm::ModuleAnalysisManager>();
        m_lam = std::make_unique<llvm::LoopAnalysisManager>();
        m_cgam = std::make_unique<llvm::CGSCCAnalysisManager>();
        m_pic = std::make_unique<llvm::PassInstrumentationCallbacks>();
        // TODO{vibhanshu}: how to remove this dependency on cursor
        llvm::LLVMContext& l_context = CursorContextImpl::ctx();
        m_si = std::make_unique<llvm::StandardInstrumentations>(l_context, true);
        m_si->registerCallbacks(*m_pic, m_mam.get());

        llvm::PassBuilder PB;
        PB.registerModuleAnalyses(*m_mam);
        PB.registerFunctionAnalyses(*m_fam);
        PB.crossRegisterProxies(*m_lam, *m_fam, *m_cgam, *m_mam);

        if (not s_llvm_init) {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
            llvm::InitializeNativeTargetAsmParser();
            s_llvm_init = true;
        }
        // Use a JITTargetMachineBuilder with O0 optimization to avoid
        // problematic codegen passes in LLVM trunk (21.0.0git)
        auto JTMB = llvm::orc::JITTargetMachineBuilder::detectHost();
        if (!JTMB) {
            CODEGEN_PUSH_ERROR(JIT, "Failed to detect host target machine: " << llvm::toString(JTMB.takeError()));
            return;
        }
        JTMB->setCodeGenOptLevel(llvm::CodeGenOptLevel::None);
        auto jit_result = llvm::orc::LLJITBuilder()
            .setJITTargetMachineBuilder(std::move(*JTMB))
            .create();
        if (!jit_result) {
            CODEGEN_PUSH_ERROR(JIT, "Failed to create LLJIT: " << llvm::toString(jit_result.takeError()));
            return;
        }
        m_handle = std::move(*jit_result);
        if (llvm::Error err = m_handle->initialize(m_handle->getMainJITDylib())) {
            CODEGEN_PUSH_ERROR(JIT, "Failed to initialize JIT: " << llvm::toString(std::move(err)));
            m_handle.reset();
        }
    }
    ~Impl() {
        if (is_init()) {
            if (llvm::Error err = m_handle->deinitialize(m_handle->getMainJITDylib())) {
                CODEGEN_PUSH_ERROR(JIT, "Failed to deinitialize JIT: " << llvm::toString(std::move(err)));
            }
            // Explicitly reset JIT handle before pass managers
            m_handle.reset();
        }
        // Reset pass managers in reverse dependency order
        m_fpm.reset();
        m_si.reset();
        m_pic.reset();
        m_fam.reset();
        m_cgam.reset();
        m_lam.reset();
        m_mam.reset();
    }
    void bind() {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_init());
        LLVM_BUILDER_ASSERT(not is_bind());
        m_is_bind = true;
        // TODO{vibhanshu}: binding all at once is prohibitive, instead
        //                   allow binding namespace one at a time
        for (auto it = m_namespace_seq.rbegin(); it != m_namespace_seq.rend(); ++it) {
            get_namespace(*it).bind();
        }
    }
    bool is_init() const {
        return static_cast<bool>(m_handle);
    }
    bool is_bind() const {
        return m_is_bind;
    }
    bool contains_symbol_definition(const std::string& name) const {
        LLVM_BUILDER_ASSERT(not name.empty())
        for (const std::string& pub_sym : m_public_def_symbols) {
            if (pub_sym == name) {
                return true;
            }
        }
        return false;
    }
    bool contains_symbol_declaration(const std::string &name) const {
        LLVM_BUILDER_ASSERT(not name.empty())
        for (const std::string& pub_sym : m_public_decl_symbols) {
            if (pub_sym == name) {
                return true;
            }
        }
        return false;
    }
    bool contains_symbol_definition(const LinkSymbol& name) const {
        LLVM_BUILDER_ASSERT(not name.has_error())
        for (const std::string& pub_sym : m_public_def_symbols) {
            if (pub_sym == name.full_name()) {
                return true;
            }
        }
        return false;
    }
    bool contains_symbol_declaration(const LinkSymbol& name) const {
        LLVM_BUILDER_ASSERT(not name.has_error())
        for (const std::string& pub_sym : m_public_decl_symbols) {
            if (pub_sym == name.full_name()) {
                return true;
            }
        }
        return false;
    }
    void dump_symbol_info(std::ostream& os) const {
        os << "SYMBOL LIST:[";
        separator_t sep{",\n"};
        for (const std::string& pub_sym : m_public_def_symbols) {
            os << sep << pub_sym;
        }
        os << "]";
    }
    bool process_module_fn(Function& fn) {
        LLVM_BUILDER_ASSERT(not fn.has_error())
        if (m_fpm and m_fam and fn.native_handle()) {
            m_fpm->run(*fn.native_handle(), *m_fam);
            return true;
        } else {
            return false;
        }
    }
    void add_module(JustInTimeRunner& parent, Cursor& cursor) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not cursor.has_error());
        LLVM_BUILDER_ASSERT(cursor.is_bind_called());
        if (not is_init()) {
            CODEGEN_PUSH_ERROR(JIT, "JIT not yet init");
            parent.M_mark_error();
            return;
        }
        if (is_bind()) {
            CODEGEN_PUSH_ERROR(JIT, "JIT already bound");
            parent.M_mark_error();
            return;
        }
        cursor.for_each_module([this, &parent] (Module module) {
            CODEGEN_FN
            LLVM_BUILDER_ASSERT(module.is_init());
            M_add_module(parent, module);
        });
        cursor.cleanup();
    }
    void M_add_module(JustInTimeRunner& parent, Module& module) {
        CODEGEN_FN
        if (module.has_error() or not module.is_init()) {
            CODEGEN_PUSH_ERROR(JIT, "Invalid module can't be added");
            parent.M_mark_error();
            return;
        }
        auto tsm = module.take_thread_safe_module();
        if (not tsm) {
            CODEGEN_PUSH_ERROR(JIT, "Failed to take thread safe module:" << module.name());
            parent.M_mark_error();
            return;
        }
        if (llvm::Error err = m_handle->addIRModule(std::move(*tsm))) {
            CODEGEN_PUSH_ERROR(JIT, "Failed to add IR module: " << llvm::toString(std::move(err)));
            parent.M_mark_error();
            return;
        }
        if (llvm::Error err = m_handle->initialize(m_handle->getMainJITDylib())) {
            CODEGEN_PUSH_ERROR(JIT, "Failed to initialize JIT dylib: " << llvm::toString(std::move(err)));
            parent.M_mark_error();
            return;
        }
        for (const LinkSymbol& l_symbol : module.public_symbols()) {
            if (not l_symbol.is_valid()) {
                CODEGEN_PUSH_ERROR(JIT, "Symbol invalid in module:" << module.name());
                parent.M_mark_error();
                return;
            }
            const LinkSymbolName& l_sym_name = l_symbol.symbol_name();
            const std::string l_namespace_name = [&]() -> std::string {
                if (l_sym_name.is_global()) {
                    return "";
                } else {
                    return l_sym_name.namespace_name();
                }
            } ();
            auto it = m_namespace_map.try_emplace(l_namespace_name, m_parent, l_namespace_name, runtime::Namespace::construct_t{});
            runtime::Namespace& l_namespace = it.first->second;
            if (l_namespace.is_bind()) {
                CODEGEN_PUSH_ERROR(JIT, "Namespace already frozen, can't add symbol: " << l_namespace.name() << ": " << l_sym_name.full_name());
                parent.M_mark_error();
                return;
            }
            if (it.second) {
                m_namespace_seq.emplace_back(l_namespace_name);
            }
            // TODO{vibhanshu}: support external declare only symbols also, currenlty all symbosl are defined
            if (0) {
                m_public_decl_symbols.emplace_back(l_sym_name.full_name());
            } else {
                m_public_def_symbols.emplace_back(l_sym_name.full_name());
            }
            if (l_symbol.is_custom_struct()) {
                LLVM_BUILDER_ASSERT(l_namespace.is_global());
                const TypeInfo& l_struct_type = module.struct_type(l_sym_name.short_name());
                l_namespace.add_struct(l_struct_type);
            } else if (l_symbol.is_function()) {
                l_namespace.add_event(l_sym_name.short_name());
            } else {
                // TODO{vibhanshu}: decide what to do with such symbols
            }
        }
    }
    uint64_t M_get_symbol_address(const JustInTimeRunner& object, const std::string& symbol) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not object.has_error());
        if (symbol.empty()) {
            CODEGEN_PUSH_ERROR(JIT, "JIT can't search empty symbol");
            object.M_mark_error();
            return 0;
        }
        if (not is_init()) {
            CODEGEN_PUSH_ERROR(JIT, "JIT not yet init for symbol:" << symbol);
            object.M_mark_error();
            return 0;
        }
        if (not is_bind()) {
            CODEGEN_PUSH_ERROR(JIT, "JIT not yet bind for symbol:" << symbol);
            object.M_mark_error();
            return 0;
        }
        if (not contains_symbol_definition(symbol) and not contains_symbol_declaration(symbol)) {
            std::stringstream core_error;
            core_error << "Symbol not found:" << symbol << "\n";
            dump_symbol_info(core_error);
            CODEGEN_PUSH_ERROR(JIT, core_error.str());
            object.M_mark_error();
        }
        // TODO{vibhanshu}: collect lookup metric
        llvm::Expected<llvm::orc::ExecutorAddr> lookup_result = m_handle->lookup(symbol);

        if (not lookup_result) {
            llvm::Error err = lookup_result.takeError();
            if (err.isA<llvm::orc::SymbolsNotFound>()) {
               CODEGEN_PUSH_ERROR(JIT, "Symbol not found:" << symbol);
               object.M_mark_error();
            } else {
               CODEGEN_PUSH_ERROR(JIT, "unhandled error form llvm lib for Symbol not found:" << symbol);
               object.M_mark_error();
            }
            llvm::consumeError(std::move(err));
            return 0;
        }
        uint64_t r = lookup_result->getValue();
        if (r == 0) {
            CODEGEN_PUSH_ERROR(JIT, "invalid address for symbol:" << symbol);
            object.M_mark_error();
        }
        return r;
    }
    runtime::Namespace get_namespace(const std::string &name) const {
        CODEGEN_FN
        if (is_bind()) {
            if (m_namespace_map.contains(name)) {
                return meta::remove_const(m_namespace_map.at(name));
            } else {
                std::stringstream l_log;
                l_log << "Namespace not found:" << name << ", candidates:[";
                for (const auto &kv : m_namespace_map) {
                    l_log << ", " << kv.first;
                }
                l_log << "]";
                CODEGEN_PUSH_ERROR(JIT, l_log.str());
                return runtime::Namespace::null();
            }
        } else {
            CODEGEN_PUSH_ERROR(JIT, "JIT not yet ready for namespace lookup, please bind it first:" << name);
            return runtime::Namespace::null();
        }
    }
};

//
// JustInTimeRunner
//
JustInTimeRunner::JustInTimeRunner() : BaseT{State::VALID} {
    if (CursorContextImpl::has_value()) {
        m_impl = std::make_shared<Impl>(*this);
    } else {
        M_mark_error();
    }
}

JustInTimeRunner::JustInTimeRunner(null_tag_t) : BaseT{State::ERROR} {
}

JustInTimeRunner::~JustInTimeRunner() = default;

void JustInTimeRunner::bind() {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    m_impl->bind();
}

bool JustInTimeRunner::is_bind() const {
    if (has_error()) {
        return false;
    }
    return m_impl->is_bind();
}

bool JustInTimeRunner::contains_symbol_definition(const std::string& name) const {
    if (has_error()) {
        return false;
    }
    if (name.empty()) {
        return false;
    }
    return m_impl->contains_symbol_definition(name);
}

bool JustInTimeRunner::process_module_fn(Function& fn) {
    CODEGEN_FN
    if (has_error() or fn.has_error()) {
        return false;
    }
    return m_impl->process_module_fn(fn);
}

void JustInTimeRunner::add_module(Cursor& cursor) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (cursor.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't add modules, cursor object is invalid");
    } else {
        m_impl->add_module(*this, cursor);
    }
}

auto JustInTimeRunner::get_fn(const std::string& symbol) const -> fn_t* {
    if (has_error()) {
        return nullptr;
    }
    if (symbol.empty()) {
        CODEGEN_PUSH_ERROR(JIT, "Function name can't be empty");
        M_mark_error();
        return nullptr;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    uint64_t l_value = m_impl->M_get_symbol_address(*this, symbol);
    if (l_value != 0) {
        return reinterpret_cast<fn_t*>(l_value);
    } else {
        return nullptr;
    }
}

runtime::Namespace JustInTimeRunner::get_namespace(const std::string &name) const {
    CODEGEN_FN
    if (has_error()) {
        return runtime::Namespace::null();
    }
    return m_impl->get_namespace(name);
}

runtime::Namespace JustInTimeRunner::get_global_namespace() const {
    CODEGEN_FN
    return get_namespace("");
}

bool JustInTimeRunner::operator==(const JustInTimeRunner &o) const {
    if (has_error() and o.has_error()) {
        return true;
    }
    return m_impl.get() == o.m_impl.get();
}

const JustInTimeRunner& JustInTimeRunner::null() {
    static JustInTimeRunner s_runner{null_tag_t{}};
    return s_runner;
}

CONTEXT_DEF(JustInTimeRunner)

LLVM_BUILDER_NS_END
