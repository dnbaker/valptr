#pragma once
#include <cassert>
#include <stdexcept>
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <cmath>

namespace vpr {
using std::uint64_t;
using std::uint32_t;
using std::uint16_t;
using std::uint8_t;
using std::int64_t;
using std::int32_t;
using std::int16_t;
using std::int8_t;

inline unsigned clz(unsigned val) {
    return __builtin_clz(val);
}
inline unsigned clz(unsigned long val) {
    return __builtin_clzl(val);
}
inline unsigned clz(unsigned long long val) {
    return __builtin_clzll(val);
}

template<typename T>
struct ConstFnHelper {
    void operator()(T *val) const {
        delete val;
    }
    void operator()(const T *val) const {
        this->operator()(const_cast<T *>(val));
    }
};

template<typename T>
struct DeleteFunctor: ConstFnHelper<T> {
    // Calls delete 
};
template<typename T>
struct ArrDeleteFunctor: ConstFnHelper<T> {
    void operator()(T *val) const {delete [] val;}
    // Calls delete 
};

template<typename T>
struct DoNothingFunctor: ConstFnHelper<T> {
    // Does nothing
    void operator()(T *val) const {}
};

template<typename T>
struct FreeFunctor: ConstFnHelper<T> {
    // Calls free
    void operator()(T *val) const {std::free(val);}
};

template<typename T, typename V=std::uint32_t, typename DelFunctor=DeleteFunctor<T>>
class valptr {
    // Like a combination of unique_ptr and an integer.
    // Modify what's called by deletion 
    static constexpr unsigned ALN_LEFTOVERS = std::log2(alignof(T));
    static constexpr uint64_t MASK          = (static_cast<uint64_t>(1) << 48) - 1;
    static constexpr uint64_t ALN_MASK      = (static_cast<uint64_t>(1) << ALN_LEFTOVERS) - 1;
    static constexpr uint64_t ZERO_VAL_MASK = ~(MASK & ~ALN_MASK);
    uint64_t    val_;

public:
    valptr(V val, T *ptr) {
        val_ = reinterpret_cast<uint64_t>(ptr);
        assert((val_ & ALN_MASK) == 0);
        setval(val);
#if !NDEBUG
        std::fprintf(stderr, "ptr after insertiOn: %p. MASK: %llx. LEFTOVERS: %u. ZERO_VAL_MASK: %llx\n",
                     get(), MASK, ALN_LEFTOVERS, ZERO_VAL_MASK);
#endif
    }
    T *setp(T *ptr) {
        V v = val();
        val_ = reinterpret_cast<uint64_t>(ptr);
        setval(v);
        return ptr;
    }
    V setval(V val) {
#if !NDEBUG
        std::fprintf(stderr, "val before: %llx. zvmask: %llx\n", val_, ZERO_VAL_MASK);
#endif
        val_ &= ~ZERO_VAL_MASK; // Set val bits to 0.
        uint64_t val64 = (uint64_t)(val);
        if(val64)
            val_ |= ((val64 & ~ALN_MASK) << (48 - ALN_LEFTOVERS)) | (val64 & ALN_MASK);
        return val;
    }
    void setvalcheck(V val) {
        if(__builtin_expect(static_cast<std::make_unsigned_t<V>>(val) >= (static_cast<uint64_t>(1) << (ALN_LEFTOVERS + 16)), 0))
            throw std::runtime_error("val is too large to encode.");
        setval(val);
    }
    const T *get() const {
        return reinterpret_cast<const T *>((val_ & MASK) & ~ALN_MASK);
    }
    T       *get()       {
        return reinterpret_cast<T *>((val_ & MASK) & ~ALN_MASK);
    }
    T &operator*() {
        return *this;
    }
    const T &operator*() const {
        return *this;
    }
    T *operator->() {
        return get();
    }
    const T *operator->() const {
        return get();
    }
    explicit operator T*() {
        return this->get();
    }
    explicit operator const T*() const {
        return this->get();
    }
    V        val() const {
        return V((val_ & ALN_MASK) | (val_ >> (48 - ALN_LEFTOVERS)));
    }
    ~valptr() {
        DelFunctor()(get());
    }
    valptr(const valptr &other) = delete;
    valptr(valptr &&other) {
        if(std::addressof(other) == this) return;
        val = other.val; other.val = 0;
    }
};
static_assert(sizeof(valptr<std::string, uint32_t>) == sizeof(uint64_t), "Functor must take no space.");

template<typename T, typename V, typename DelFunctor=DeleteFunctor<T>, typename... Args>
auto make_valptr(V val, Args &&... args) {
    return valptr<T, V, DelFunctor>(val, new T(std::forward<Args>(args)...));
}

} // namespace vpr
