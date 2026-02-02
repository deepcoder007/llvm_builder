//
// Created by vibhanshu on 2024-08-10
//

#ifndef CORE_DS_FIXED_STRING_H_
#define CORE_DS_FIXED_STRING_H_

#include "core/util/defines.h"
#include "core/meta/noncopyable.h"

#include <string>
#include <unordered_set>
#include <unordered_map>

CORE_NS_BEGIN

class fixed_string {
    const std::string& m_str_ref;
public:
    explicit fixed_string();
    explicit fixed_string(const std::string& s);
    explicit fixed_string(std::string&& s);
    ~fixed_string() = default;
    fixed_string(fixed_string&&) = default;
    fixed_string(const fixed_string&) = default;
    fixed_string& operator = (const fixed_string&) = default;
    fixed_string& operator = (fixed_string&&) = default;
public:
    bool operator == (const fixed_string& o) const {
        return m_str_ref.data() == o.m_str_ref.data() and m_str_ref.size() == o.m_str_ref.size();
    }
    bool operator != (const fixed_string& o) const {
        return not operator == (o);
    }
    size_t hash() const {
        return std::hash<std::string>{}(m_str_ref);
    }
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(fixed_string)
};
static_assert(sizeof(fixed_string) == sizeof(nullptr_t));

template <typename T>
class fixed_string_map {
    class fixed_string_hash {
    public:
        size_t operator() (const fixed_string k) const {
            return k.hash();
        }
    };
    using raw_map_t = std::unordered_map<fixed_string, T, fixed_string_hash>;
public:
    using iterator = typename raw_map_t::iterator;
    using const_iterator = typename raw_map_t::const_iterator;
    using key_type = typename raw_map_t::key_type;
    using value_type = typename raw_map_t::value_type;
private:
    raw_map_t m_raw_map;
public:
    explicit fixed_string_map() = default;
    ~fixed_string_map() = default;
    fixed_string_map(fixed_string_map&&) = default;
    fixed_string_map(const fixed_string_map&) = default;
public:
    size_t size() const {
        return m_raw_map.size();
    }
    bool empty() const {
        return m_raw_map.empty();
    }
    const_iterator begin() const {
        return m_raw_map.begin();
    }
    const_iterator end() const {
        return m_raw_map.end();
    }
    iterator begin() {
        return m_raw_map.begin();
    }
    iterator end() {
        return m_raw_map.end();
    }
    const_iterator find(fixed_string k) const {
        return m_raw_map.find(k);
    }
    iterator find(fixed_string k) {
        return m_raw_map.find(k);
    }
    bool contains(fixed_string k) const {
        return m_raw_map.contains(k);
    }
    iterator erase(iterator pos) {
        return m_raw_map.erase(pos);
    }
    iterator erase(const_iterator pos) {
        return m_raw_map.erase(pos);
    }
    template <typename... Args>
    std::pair<iterator, bool> try_emplace(fixed_string k, Args&&... args) {
        return m_raw_map.try_emplace(k, std::forward<Args>(args)...);
    }
    T& operator[](fixed_string k) {
        return m_raw_map[k];
    }
    const T& at(fixed_string k) const {
        return m_raw_map.at(k);
    }
};

class StringManager : meta::noncopyable {
    std::unordered_set<std::string> m_symbol_set;
    bool m_frozen = false;
private:
    explicit StringManager();
public:
    ~StringManager();
public:
    bool is_frozen() const {
        return m_frozen;
    }
    void mark_frozen() {
        m_frozen = true;
    }
    void mark_unfrozen() {
        m_frozen = false;
    }
    const std::string& intern(const std::string& s);
    const std::string& intern(std::string&& s);
    void print_all_symbols(std::ostream& os) const;
public:
    static StringManager& singleton();
    static const std::string& null();
};

CORE_NS_END

#endif // CORE_DS_FIXED_STRING_H_
