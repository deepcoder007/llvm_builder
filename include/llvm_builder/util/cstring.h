//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_CSTRING_H
#define LLVM_BUILDER_CSTRING_H

#include "llvm_builder/defines.h"

#include <string>
#include <string_view>
#include <limits>
#include <vector>

LLVM_BUILDER_NS_BEGIN

class find_result_t {
  public:
    enum : uint32_t {
        npos = std::numeric_limits<uint32_t>::max()
    };
    uint32_t m_i;

  public:
    explicit find_result_t(uint32_t i) : m_i{i} {
    }
    explicit find_result_t(ptrdiff_t i);
    find_result_t(std::nullptr_t) : m_i{npos} {
    }

  public:
    find_result_t& operator=(uint32_t i) {
        m_i = i;
        return *this;
    }
    find_result_t& operator=(ptrdiff_t i);
    find_result_t& operator=(bool i) = delete;
    find_result_t& operator=(char i) = delete;
    find_result_t& operator=(std::byte i) = delete;

  public:
    uint32_t operator+(uint32_t dist) const;

  public:
    operator char() const = delete;
    operator std::byte() const = delete;
    operator int32_t() const = delete;

  public:
    operator uint32_t() const {
        return m_i;
    }
    operator ptrdiff_t() const {
        return m_i;
    }
    operator bool() const {
        return m_i != npos;
    }
    bool operator!() const {
        return m_i == npos;
    }
};

class CString {
  public:
    enum : uint32_t {
        npos = find_result_t::npos
    };

  private:
    const char* m_s = nullptr;
    uint32_t m_size = 0u;

  public:
    explicit CString() = default;
    CString(const CString&) = default;
    CString(CString&&) = default;
    CString(const char* s, uint32_t sz);
    explicit CString(const std::string_view& s);
    explicit CString(const std::string& s);

  public:
    explicit CString(const char* s) = delete;
    explicit CString(std::string&&) = delete;

  public:
    char operator[](uint32_t i) const;
    char at(uint32_t i) const;
    CString substr_to(uint32_t pos) const;
    CString substr_from(uint32_t pos) const;
    CString substr_from(uint32_t pos, uint32_t len) const {
        return substr_from(pos).substr_to(len);
    }
    find_result_t find(CString s) const;
    find_result_t find(const std::string& s) const {
        return find(CString{s});
    }
    find_result_t find(const char* s) const = delete;
    find_result_t rfind(char ch) const;
    find_result_t find(char ch) const;
    std::vector<CString> split(char ch) const;
    std::vector<std::string> dir_split() const;
    const char* data() const {
        return m_s;
    }
    const char* begin() const {
        return m_s;
    }
    const char& front() const;
    const char& back() const;
    const char* end() const;
    void pop_front(uint32_t num_chars);
    void pop_back(uint32_t num_chars);
    std::string str() const {
        return std::string{m_s, m_size};
    }
    uint32_t size() const {
        return m_size;
    }
    bool startswith(CString s) const;
    bool startswith(const char* s) const {
        return startswith(CString::from_pointer(s, m_size + 1u));
    }
    bool startswith(char ch) const {
        return m_size > 0 and m_s[0] == ch;
    }
    bool startswith(const std::string& s) const {
        return startswith(CString{s});
    }
    bool endswith(CString s) const;
    bool endswith(const char* s) const {
        return endswith(CString::from_pointer(s, m_size + 1u));
    }
    bool endswith(char ch) const {
        return m_size > 0 and back() == ch;
    }
    bool endswith(const std::string& s) const {
        return endswith(CString{s});
    }
    bool has_addr() const {
        return m_s != nullptr;
    }
    void set_end(const char* ptr);
    bool empty() const;
    bool is_c_str() const;
    void trim();
    void trim_sp();
    std::string toupper() const;
    std::string tolower() const;

  public:
    static CString from_range(const char* begin, const char* end);
    static CString from_pointer(const char* p, uint32_t max_size = 64 * 1024);
    template <size_t N>
    static CString from_array(const char (&arr)[N]) {
        return CString{arr, N};
    }

  public:
    bool contains(char ch) const {
        return find(ch);
    }
    bool equals(CString s) const {
        if (m_size != s.m_size) {
            return false;
        }
        if (m_s == s.m_s) {
            return true;
        }
        return startswith(s);
    }
    bool equalsCI(CString s) const {
        if (m_size != s.m_size) {
            return false;
        }
        if (m_s == s.m_s) {
            return true;
        }
        uint32_t n = m_size;
        for (uint32_t i = 0; i != n; ++i) {
            const int c1 = ::tolower(m_s[i]);
            const int c2 = ::tolower(s.m_s[i]);
            if (c1 != c2) {
                return false;
            }
        }
        return true;
    }
    bool equals(const std::string& s) const {
        return equals(CString{s});
    }
    bool equalsCI(const std::string& s) const {
        return equalsCI(CString{s});
    }
    bool equals(const char* s) const {
        return equals(CString::from_pointer(s, m_size + 1u));
    }
    bool equalsCI(const char* s) const {
        return equalsCI(CString::from_pointer(s, m_size + 1u));
    }
    bool operator==(const std::string& s) const {
        return equals(s);
    }
    bool operator!=(const std::string& s) const {
        return (not operator==(s));
    }
    template <typename T>
    auto lexical_cast() const -> T;

  public:
    CString& operator=(const CString& rhs) = default;
    CString& operator=(CString&& rhs) = default;
    explicit operator bool() const {
        return not empty();
    }
    operator std::string_view() const {
        return std::string_view{m_s, m_size};
    }
    operator char() const = delete;
    operator const char*() const = delete;
    operator const std::string&() const = delete;
    size_t hash() const;
    friend std::ostream& operator<<(std::ostream& os, const CString& cs) {
        if (cs.m_s && cs.m_size > 0) {
            os.write(cs.m_s, cs.m_size);
        }
        return os;
    }
};

CString operator""_cs(const char* s, size_t len);

LLVM_BUILDER_NS_END

#endif
