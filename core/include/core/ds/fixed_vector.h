//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_FIXED_VECTOR_H
#define CORE_FIXED_VECTOR_H

#include "core/util/defines.h"

#include <limits>
#include <cstring>
#include <cassert>

CORE_NS_BEGIN

template <typename T>
class base_fixed_vector {
    enum : uint32_t {
        c_max_capacity = std::numeric_limits<uint32_t>::max()
    };
  public:
    using key_type = uint32_t;
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;
  protected:
    T* m_arr = nullptr;
    uint32_t m_size = 0;
    uint32_t m_capacity = 0;
  protected:
    explicit base_fixed_vector() = default;
    base_fixed_vector(T* arr, uint32_t size, uint32_t capacity) : m_arr{arr}, m_size{size}, m_capacity{capacity} {
    }
  public:
    uint32_t size() const {
        assert(m_size <= m_capacity);
        return m_size;
    }
    uint32_t capacity() const {
        assert(m_size <= m_capacity);
        return m_capacity;
    }
    uint32_t remaining_size() const {
        assert(m_size <= m_capacity);
        return m_capacity - m_size;
    }
    bool empty() const {
        assert(m_size <= m_capacity);
        return m_size == 0;
    }
    bool full() const {
        assert(m_size <= m_capacity);
        return m_size >= m_capacity;
    }
  public:
    T* data() {
        return m_arr;
    }
    T* begin() {
        assert(is_init());
        return m_arr;
    }
    T* end() {
        assert(is_init());
        return m_arr + size();
    }
    const T* data() const {
        return m_arr;
    }
    const T* begin() const {
        return m_arr;
    }
    const T* end() const {
        return m_arr + size();
    }
    uint32_t index_of(const T* obj) const {
        assert(obj >= begin() and obj < end());
        return shrink(obj - begin());
    }
    T& front() {
        assert(not empty());
        return m_arr[0];
    }
    T& back() {
        assert(not empty());
        return m_arr[m_size - 1u];
    }
    const T& front() const {
        assert(not empty());
        return m_arr[0];
    }
    const T& back() const {
        assert(not empty());
        return m_arr[m_size - 1u];
    }
    const T& at(key_type i) const {
        assert(i < size());
        return m_arr[i];
    }
    T& operator[](key_type i) {
        assert(i < size());
        return m_arr[i];
    }
    const T& operator[](key_type i) const {
        assert(i < size());
        return m_arr[i];
    }
    bool operator==(const base_fixed_vector& rhs) const {
        if (m_size != rhs.m_size) {
            return false;
        }
        if (m_arr != rhs.m_arr) {
            for (uint32_t i = 0; i != m_size; ++i) {
                if (not(m_arr[i] == rhs.m_arr[i])) {
                    return false;
                }
            }
        }
        return true;
    }
    bool operator!=(const base_fixed_vector& rhs) const {
        return not operator==(rhs);
    }
    bool is_init() const {
        return M_is_init();
    }
    template <typename... Args>
    T& emplace_back(Args&&... args) {
        assert(not full());
        T* p = m_arr + m_size;
        if constexpr (sizeof...(args) > 0u or not std::is_trivially_constructible_v<T>) {
            new (p) T{std::forward<Args>(args)...};
        } else {
            ::memset(p, 0, sizeof(T));
        }
        m_size++;
        return *p;
    }
    void push_back() {
        emplace_back();
    }
    void push_back(const T& x) {
        assert(not full());
        new (m_arr + m_size) T{x};
        m_size++;
    }
    void push_back(T&& x) {
        assert(not full());
        new (m_arr + m_size) T{std::move(x)};
        m_size++;
    }
    void pop_back() {
        if constexpr (not std::is_trivially_destructible_v<T>) {
            T& x = back();
            x.~T();
        }
        m_size--;
    }
    void pop_back_n(uint32_t n) {
        assert(n > 0u);
        assert(n <= m_size);
        if constexpr (not std::is_trivially_destructible_v<T>) {
            T* it = &back();
            T* end2 = it - n;
            for (; it != end2; --it) {
                it->~T();
            }
        }
        m_size -= n;
    }
    void clear() {
        if constexpr (std::is_trivially_destructible_v<T>) {
            m_size = 0u;
        } else if (m_size > 0u) {
            pop_back_n(m_size);
        }
    }

  protected:
    bool M_is_init() const {
        if (m_arr) {
            assert(m_capacity > 0 and m_size <= m_capacity);
            return true;
        } else {
            assert(m_capacity == 0 and m_size == 0);
            return false;
        }
    }
    void M_reset(const base_fixed_vector& rhs) {
        m_arr = rhs.m_arr;
        m_size = rhs.m_size;
        m_capacity = rhs.m_capacity;
    }
    void M_reset() {
        m_arr = nullptr;
        m_size = 0;
        m_capacity = 0;
    }
};

template <typename T>
class fixed_vector : public base_fixed_vector<T> {
    using BaseT = base_fixed_vector<T>;

  private:
    using allocator_t = std::allocator<T>;
    allocator_t m_allocator;

  public:
    explicit fixed_vector() = default;
    explicit fixed_vector(uint32_t capacity) : BaseT{m_allocator.allocate(capacity), 0, capacity} {
    }
    fixed_vector(const fixed_vector& copy) : fixed_vector(copy.capacity()) {
        for (const T& x : copy) {
            BaseT::push_back(x);
        }
    }
    fixed_vector(fixed_vector&& copy) : BaseT{copy} {
        copy.M_reset();
    }
    ~fixed_vector() {
        M_clear_and_deallocate();
    }

  public:
    void init(uint32_t capacity) {
        assert(not BaseT::M_is_init());
        BaseT::m_arr = m_allocator.allocate(capacity);
        BaseT::m_capacity = capacity;
    }
    void swap(fixed_vector&& rhs) {
        T* const l_arr = rhs.m_arr;
        const uint32_t l_size = rhs.m_size;
        const uint32_t l_capacity = rhs.m_capacity;
        rhs.m_arr = BaseT::m_arr;
        rhs.m_size = BaseT::m_size;
        rhs.m_capacity = BaseT::m_capacity;
        BaseT::m_arr = l_arr;
        BaseT::m_size = l_size;
        BaseT::m_capacity = l_capacity;
    }
  private:
    void M_clear_and_deallocate() {
        if (BaseT::m_arr) {
            BaseT::clear();
            M_deallocate();
        }
    }
    void M_deallocate() {
        if (BaseT::m_arr) {
            BaseT::clear();
            m_allocator.deallocate(BaseT::m_arr, BaseT::m_capacity);
            BaseT::M_reset();
        }
    }
};

template <typename T, uint32_t c_capacity>
class tiny_fixed_vector {
  private:
    static_assert(sizeof(T) % alignof(T) == 0);
    using array_t = char[sizeof(T) * c_capacity];
    static_assert(sizeof(array_t) == sizeof(T) * c_capacity);

  public:
    using key_type = uint32_t;
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

  public:
    using tiny_size_t = uint32_t;
    enum : tiny_size_t {
        c_small_capacity = c_capacity
    };

  private:
    alignas(alignof(T)) array_t m_arr{};
    tiny_size_t m_size = 0;

  public:
    explicit tiny_fixed_vector() = default;
    tiny_fixed_vector(const tiny_fixed_vector&) = default;
    tiny_fixed_vector(tiny_fixed_vector&&) = default;
    ~tiny_fixed_vector() {
        clear();
    }

  public:
    tiny_size_t size() const {
        assert(m_size <= c_small_capacity);
        return m_size;
    }
    static constexpr tiny_size_t capacity() {
        return c_small_capacity;
    }
    bool empty() const {
        return size() == 0;
    }
    bool full() const {
        return size() >= c_small_capacity;
    }
    uint32_t index_of(const T* obj) const {
        assert(obj >= begin() and obj < end());
        return shrink(obj - begin());
    }
    template <typename F>
    void foreach_value(F&& fn) const {
        for (const T& v : *this) {
            fn(v);
        }
    }
    template <typename F>
    void foreach_value(F&& fn) {
        for (T& v : *this) {
            fn(v);
        }
    }
    static constexpr size_t M_byte_offset(uint32_t i) {
        static_assert(sizeof(tiny_size_t) > 0u);
        static_assert(sizeof(T) > 0u);
        static_assert(offsetof(tiny_fixed_vector, m_arr) >= sizeof(tiny_size_t));
        return offsetof(tiny_fixed_vector, m_arr) + sizeof(T) * i;
    }

  public:
    T* data() {
        return reinterpret_cast<T*>(m_arr);
    }
    T* begin() {
        return data();
    }
    T* end() {
        return data() + size();
    }
    const T* data() const {
        return reinterpret_cast<const T*>(m_arr);
    }
    const T* begin() const {
        return data();
    }
    const T* end() const {
        return data() + size();
    }
    T& front() {
        assert(not empty());
        return data()[0];
    }
    T& back() {
        assert(not empty());
        return data()[size() - 1u];
    }
    const T& front() const {
        assert(not empty());
        return data()[0];
    }
    const T& back() const {
        assert(not empty());
        return data()[size() - 1u];
    }
    T& operator[](key_type i) {
        const uint32_t i2 = static_cast<uint32_t>(i);
        assert(i2 < size());
        return data()[i2];
    }
    const T& operator[](key_type i) const {
        const uint32_t i2 = static_cast<uint32_t>(i);
        assert(i2 < size());
        return data()[i2];
    }

  public:
    template <typename... Args>
    T& emplace_back(Args&&... args) {
        assert(not full());
        T* p = end();
        new (p) T{std::forward<Args>(args)...};
        ++m_size;
        return *p;
    }
    void push_back(const T& x) {
        assert(not full());
        new (end()) T{x};
        ++m_size;
    }
    void push_back(T&& x) {
        assert(not full());
        new (end()) T{std::move(x)};
        ++m_size;
    }
    template <typename... Args>
    void emplace_front(Args&&... args) {
        M_shift_right();
        new (begin()) T{std::forward<Args>(args)...};
    }
    void push_front(const T& x) {
        M_shift_right();
        new (begin()) T{x};
    }
    void push_front(T&& x) {
        M_shift_right();
        new (begin()) T{std::move(x)};
    }
    void pop_back() {
        T& x = back();
        x.~T();
        --m_size;
    }
    void pop_front() {
        M_shift_left();
    }

    void fast_pop_front(tiny_size_t idx) {
        M_pop_first_n(idx);
    }

    void clear() {
        while (m_size > 0) {
            pop_back();
        }
    }
    bool check_not_full() const {
        return size() < c_small_capacity;
    }
    template <typename... Args>
    void emplace(const tiny_size_t idx, Args&&... args) {
        assert(idx <= m_size);
        M_shift_right_from(idx);
        new (data() + idx) T{std::forward<Args>(args)...};
    }
    template <typename... Args>
    void fast_emplace(const tiny_size_t idx, Args&&... args) {
        assert(idx <= m_size);
        M_fast_shift_right_from(idx);
        new (data() + idx) T{std::forward<Args>(args)...};
    }
    void insert(const tiny_size_t idx, const T& x) {
        assert(idx <= m_size);
        M_shift_right_from(idx);
        new (data() + idx) T{x};
    }
    void fast_insert(const tiny_size_t idx, const T& x) {
        assert(idx <= m_size);
        M_fast_shift_right_from(idx);
        new (data() + idx) T{x};
    }
    void insert(const tiny_size_t idx, T&& x) {
        assert(idx <= m_size);
        M_shift_right_from(idx);
        new (data() + idx) T{std::move(x)};
    }
    void erase(const tiny_size_t idx) {
        assert(idx < m_size);
        M_shift_left_from(idx);
    }
    void fast_erase(const tiny_size_t idx) {
        assert(idx < m_size);
        M_fast_shift_left_from(idx);
    }

  private:
    void M_shift_right_from(const tiny_size_t from_idx) {
        assert(not full());
        const uint32_t n = m_size;
        if (n >= 1u) {
            T&& l_back = std::move(back());
            new (end()) T{std::move(l_back)};
            for (uint32_t i = n - 1u; i > from_idx; --i) {
                operator[](i) = std::move(operator[](i - 1u));
            }
        }
        ++m_size;
    }
    void M_fast_shift_right_from(const tiny_size_t from_idx) {
        const uint32_t n = m_size;
        if ((m_size == 0) || (from_idx == m_size - 1)) {
            return;
        }
        if (n >= 1u) {
            memmove((void*)(data() + from_idx + 1), (void*)(data() + from_idx), sizeof(T) * (n - from_idx - 1));
            // for (uint32_t i = n - 1u; i > from_idx; --i) {
            //     operator[](i) = std::move(operator[](i - 1u));
            // }
        }
    }
    void M_shift_right() {
        M_shift_right_from(0u);
    }
    void M_shift_left_from(const tiny_size_t from_idx) {
        assert(not empty());
        uint32_t n = m_size;
        if (n >= 1u) {
            --n;
            for (uint32_t i = from_idx; i < n; ++i) {
                operator[](i) = std::move(operator[](i + 1u));
            }
        }
        T& l_back = back();
        l_back.~T();
        --m_size;
    }
    void M_fast_shift_left_from(const tiny_size_t from_idx) {
        assert(not empty());
        uint32_t n = m_size;
        if (n >= 1u) {
            memmove((void*)(data() + from_idx), (void*)(data() + from_idx + 1), sizeof(T) * (n - from_idx - 1));
        }
        T& l_back = back();
        l_back.~T();
        --m_size;
        emplace_back();
    }
    void M_shift_left() {
        M_shift_left_from(0u);
    }
    void M_pop_first_n(tiny_size_t idx) {
        if (idx == 0) {
            return;
        }
        assert(not empty());
        uint32_t n = m_size;
        memmove((void*)(data()), (void*)(data() + idx), sizeof(T) * (n - idx));
        m_size -= idx;
        for (uint32_t i = n - idx; i < n; i++) {
            emplace_back();
        }
    }
};

CORE_NS_END

#endif
