//
// Created by vibhanshu on 2024-06-07
//

#include "util/cstring.h"
#include "util/debug.h"
#include "meta/simple.h"

#include <bits/basic_string.h>
#include <algorithm>
#include <cstring>

LLVM_BUILDER_NS_BEGIN

//
// find_result_t
//
find_result_t::find_result_t(ptrdiff_t i) : m_i{static_cast<uint32_t>(i)} {
}

find_result_t& find_result_t::operator=(ptrdiff_t i) {
    m_i = static_cast<uint32_t>(i);
    return *this;
}

uint32_t find_result_t::operator+(uint32_t dist) const {
    LLVM_BUILDER_ASSERT(*this);
    return m_i + dist;
}

//
// CString
//
CString::CString(const char* s, uint32_t sz) : m_s{s}, m_size{sz} {
    LLVM_BUILDER_ASSERT(meta::bool_if(s == nullptr, sz == 0));
}

CString::CString(const std::string_view& s) : m_s{s.data()}, m_size{static_cast<uint32_t>(s.size())} {
}

CString::CString(const std::string& s) : m_s{s.data()}, m_size{static_cast<uint32_t>(s.size())} {
}


char CString::operator[](uint32_t i) const {
    LLVM_BUILDER_ASSERT(i < m_size);
    return m_s[i];
}

char CString::at(uint32_t i) const {
    LLVM_BUILDER_ASSERT(i < m_size);
    return m_s[i];
}

CString CString::substr_to(uint32_t pos) const {
    LLVM_BUILDER_ASSERT(pos <= m_size);
    return CString{m_s, pos};
}

CString CString::substr_from(uint32_t pos) const {
    LLVM_BUILDER_ASSERT(pos <= m_size);
    return CString{m_s + pos, m_size - pos};
}

find_result_t CString::find(char ch) const {
    for (uint32_t i = 0; i < m_size; ++i) {
        if (m_s[i] == ch) {
            return find_result_t{ i };
        }
    }
    return nullptr;
}

find_result_t CString::rfind(char ch) const {
    for (uint32_t i = m_size; i > 0u; --i) {
        if (m_s[i - 1u] == ch) {
            return find_result_t{ i - 1u };
        }
    }
    return nullptr;
}

find_result_t CString::find(CString s) const {
    if (s.size() > size()) {
        return nullptr;
    }
    if (s.empty()) {
        return find_result_t{ 0u };
    }
    const uint32_t n = size() - s.size();
    for (uint32_t i = 0; i <= n; ++i) {
        LLVM_BUILDER_ASSERT(i + s.size() <= size());
        if (::memcmp(m_s + i, s.data(), s.size()) == 0) {
            return find_result_t{ i };
        }
    }
    return nullptr;
}

std::vector<CString> CString::split(char ch) const {
    std::vector<CString> arr;
    uint32_t begin = 0;
    for (uint32_t i = 0; i < m_size; ++i) {
        if (m_s[i] == ch) {
            arr.push_back(CString::from_range(m_s + begin, m_s + i));
            begin = i + 1;
        }
    }
    arr.push_back(CString::from_range(m_s + begin, end()));
    return arr;
}

std::vector<std::string> CString::dir_split() const {
    std::vector<std::string> arr;
    uint32_t begin = 0;
    for (uint32_t i = 0; i < m_size; ++i) {
        if (m_s[i] == '/') {
            const CString part = CString::from_range(m_s + begin, m_s + i);
            if (not part.empty()) {
                arr.emplace_back(CString::from_range(m_s + begin, m_s + i).str());
            }
            begin = i + 1;
        }
    }
    const CString part = CString::from_range(m_s + begin, end());
    if (not part.empty()) {
        arr.emplace_back(part.str());
    }
    return arr;
}

const char& CString::front() const {
    LLVM_BUILDER_ASSERT(not empty());
    return m_s[0];
}

const char& CString::back() const {
    LLVM_BUILDER_ASSERT(not empty());
    return m_s[m_size - 1];
}

const char* CString::end() const {
    LLVM_BUILDER_ASSERT(meta::bool_if(m_s == nullptr, m_size == 0));
    return m_s + m_size;
}

void CString::pop_front(uint32_t num_chars) {
    LLVM_BUILDER_ASSERT(num_chars <= m_size);
    m_s += num_chars;
    m_size -= num_chars;
}

void CString::pop_back(uint32_t num_chars) {
    LLVM_BUILDER_ASSERT(num_chars <= m_size);
    m_size -= num_chars;
}

bool CString::startswith(CString prefix) const {
    if (prefix.size() > size()) {
        return false;
    }
    if (prefix.empty()) {
        return true;
    }
    if (begin() == prefix.begin()) {
        return true;
    }
    return ::memcmp(begin(), prefix.begin(), prefix.size()) == 0;
}

bool CString::endswith(CString prefix) const {
    if (prefix.size() > size()) {
        return false;
    }
    if (prefix.empty()) {
        return true;
    }
    const uint32_t offset = size() - prefix.size();
    if (begin() + offset == prefix.begin()) {
        return true;
    }
    return ::memcmp(begin() + offset, prefix.begin(), prefix.size()) == 0;
}

void CString::set_end(const char* ptr) {
    LLVM_BUILDER_ASSERT(ptr != nullptr);
    LLVM_BUILDER_ASSERT(ptr >= m_s);
    m_size = static_cast<uint32_t>(std::distance(m_s, ptr));
}

bool CString::empty() const {
    LLVM_BUILDER_ASSERT(meta::bool_if(m_size > 0, m_s != nullptr));
    return m_size == 0;
}

bool CString::is_c_str() const {
    LLVM_BUILDER_ASSERT(not empty());
    return *(data() + size()) == '\0';
}

void CString::trim() {
    while (m_size > 0 and m_s[0] == ' ') {
        ++m_s;
        --m_size;
    }
    while (m_size > 0 and m_s[m_size - 1] == ' ') {
        --m_size;
    }
}

void CString::trim_sp() {
    while (m_size > 0 and std::isspace(m_s[0])) {
        ++m_s;
        --m_size;
    }
    while (m_size > 0 and std::isspace(m_s[m_size - 1])) {
        --m_size;
    }
}

auto CString::toupper() const -> std::string {
    std::string res = str();
    std::transform(m_s, m_s + m_size, res.data(), ::toupper);
    return res;
}

auto CString::tolower() const -> std::string {
    std::string res = str();
    std::transform(m_s, m_s + m_size, res.data(), ::tolower);
    return res;
}

CString CString::from_range(const char* begin, const char* end) {
    LLVM_BUILDER_ASSERT(begin <= end);
    return CString{begin, static_cast<uint32_t>(end - begin)};
}

CString CString::from_pointer(const char* p, uint32_t max_size) {
    if (p == nullptr) {
        return CString{};
    }
    uint32_t len = static_cast<uint32_t>(::strnlen(p, max_size));
    return CString{ p, len };
}

size_t CString::hash() const {
    return std::_Hash_impl::hash(m_s, m_size);
}

CString operator""_cs(const char* s, size_t len) {
    return CString::from_pointer(s, static_cast<uint32_t>(len));
}

LLVM_BUILDER_NS_END
