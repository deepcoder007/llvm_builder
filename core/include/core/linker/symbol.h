//
// Created by vibhanshu on 2024-02-08
//

#ifndef CORE_LINKER_SYMBOL_H_
#define CORE_LINKER_SYMBOL_H_

#include "defines.h"
#include "core/util/cstring.h"
#include <vector>

CORE_LINKER_NS_BEGIN

class ExportSymbol {
  public:
    enum : uint32_t {
        c_max_num_fields = 4u
    };

  private:
    std::vector<std::string> m_fields;

  public:
    explicit ExportSymbol() = default;
    explicit ExportSymbol(const std::vector<std::string>& fields);
    explicit ExportSymbol(std::initializer_list<CString> fields);
    ExportSymbol(ExportSymbol&&) = default;
    ExportSymbol(const ExportSymbol&) = default;
    ExportSymbol& operator = (const ExportSymbol&) = default;
    ExportSymbol& operator = (ExportSymbol&&) = default;

  public:
    bool is_valid() const {
        return m_fields.size() > 0;
    }
    size_t size() const {
        return m_fields.size();
    }
    const std::string& field(uint32_t i) const;
    bool match(const ExportSymbol& symbol) const;
    bool operator==(const ExportSymbol& symbol) const {
        return this->match(symbol);
    }
    bool is_callback() const;
    size_t hash() const;
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(ExportSymbol)
    std::string as_str() const;
};

CORE_LINKER_NS_END

template <>
struct std::hash<core::linker::ExportSymbol> {
    size_t operator () (const core::linker::ExportSymbol& s) const noexcept {
        return s.hash();
    }
};

#endif // CORE_LINKER_SYMBOL_H_
