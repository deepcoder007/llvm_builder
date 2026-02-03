//
// Created by vibhanshu on 2024-08-10
//

#include "ds/fixed_string.h"
#include "util/string_util.h"
#include "util/cstring.h"

LLVM_BUILDER_NS_BEGIN

//
// fixed_string
//
fixed_string::fixed_string()
    : m_str_ref{StringManager::singleton().intern(std::string{})} {
}

fixed_string::fixed_string(const std::string& s)
    : m_str_ref{StringManager::singleton().intern(s)} {
}

fixed_string::fixed_string(std::string&& s)
    : m_str_ref{StringManager::singleton().intern(std::move(s))} {
}

//
// StringManager
//
StringManager::StringManager() = default;

StringManager::~StringManager() = default;

auto StringManager::intern(const std::string& name) -> const std::string& {
    if (not is_frozen() and not name.empty()) {
        auto it = m_symbol_set.emplace(name);
        // TODO{vibhanshu}: add callback when a new string/symbol is added
        return *it.first;
    } else {
        return null();
    }
}

auto StringManager::intern(std::string&& name) -> const std::string& {
    if (not is_frozen() and not name.empty()) {
        auto it = m_symbol_set.emplace(std::move(name));
        // TODO{vibhanshu}: add callback when a new string/symbol is added
        return *it.first;
    } else {
        return null();
    }
}

void StringManager::print_all_symbols(std::ostream& os) const {
    os << "Strings[";
    separator_t sep{",\n\t"_cs};
    for (const std::string& s : m_symbol_set) {
        os << sep << s;
    }
    os << "]";
}

StringManager& StringManager::singleton() {
    static StringManager s_singleton;
    return s_singleton;
}

const std::string& StringManager::null() {
    static const std::string s_null{};
    return s_null;
}

LLVM_BUILDER_NS_END
