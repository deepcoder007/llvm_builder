//
// Created by vibhanshu on 2024-02-08
//

#include "core/linker/symbol.h"
#include "util/debug.h"
#include "util/string_util.h"

CORE_LINKER_NS_BEGIN

//
// ExportSymbol
//
ExportSymbol::ExportSymbol(const std::vector<std::string>& fields) {
    CORE_ALWAYS_ASSERT(fields.size() > 0);
    CORE_ALWAYS_ASSERT(fields.size() < c_max_num_fields);
    for (const std::string& s : fields) {
        m_fields.emplace_back(s);
    }
}

ExportSymbol::ExportSymbol(std::initializer_list<CString> fields) {
    CORE_ALWAYS_ASSERT(fields.size() > 0);
    CORE_ALWAYS_ASSERT(fields.size() < c_max_num_fields);
    for (CString s : fields) {
        m_fields.emplace_back(s.str());
    }
}

auto ExportSymbol::field(uint32_t i) const -> const std::string& {
    CORE_ALWAYS_ASSERT(i < size());
    return m_fields[i];
}

bool ExportSymbol::match(const ExportSymbol& symbol) const {
    if (size() != symbol.size()) {
        return false;
    }
    size_t l_size = m_fields.size();
    for (uint32_t i = 0; i != l_size; ++i) {
        if (m_fields[i] != symbol.m_fields[i]) {
            return false;
        }
    }
    return true;
}

bool ExportSymbol::is_callback() const {
    CORE_ASSERT(is_valid());
    return CString{m_fields[0]}.startswith("on_");
}

size_t ExportSymbol::hash() const {
    // NOTE{vibhanshu}: intentionally making this a very simple hash
    CORE_ASSERT(is_valid());
    return std::hash<std::string>{}(m_fields[0]);
}

void ExportSymbol::print(std::ostream& os) const {
    os << "{ExportSymbol:";
    separator_t sep{"|"};
    for (const std::string& s: m_fields) {
        os << sep << s;
    }
    os << "}";
}

auto ExportSymbol::as_str() const -> std::string {
    return CORE_CONCAT << *this;
}

CORE_LINKER_NS_END
