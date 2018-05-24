#pragma once
#include <cstdint>
#include <cstdlib>

namespace vpr {
using std::uint64_t;
using std::uint32_t;
using std::uint16_t;
using std::uint8_t;
using std::int64_t;
using std::int32_t;
using std::int16_t;
using std::int8_t;

template<typename T>
struct ConstFnHelper {
    void operator()(T *val) const {
        delete val;
    }
    void operator()(const T *val) const {
        this->operator()(const_cast<T *>(val));
    }
}

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
}

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
    static constexpr uint64_t ZERO_VAL_MASK = ~(MASK << ALN_LEFTOVERS);

    uint64_t    val_;
    DelFunctor func_;
    valptr(V val, T *ptr) {
        val_ = reinterpret_cast<uint64_t>(ptr) << ALN_LEFTOVERS;
        setval(val);
    }
    void setval(V val) {
        val_ &= ZERO_VAL_MASK; // Set val bits to 0.
        val_ |= (val << (48 - ALN_LEFTOVERS)) | (val & ALN_MASK);
    }
    void setvalcheck(V val) {
        if(__builtin_expect(val >= (static_cast<uint64_t>(1) << (ALN_LEFTOVERS + 16)), 0))
            throw std::runtime_error("val is too large to encode.");
        setval(val);
    }
    const T *ptr() const {
        return ((val_ >> ALN_LEFTOVERS) & MASK) << ALN_LEFTOVERS;
    }
    V        val() const {
        return V(val_ & ALN_MASK) | V(val_ >> (48 - ALN_LEFTOVERS));
    }
    ~valptr() {
        func(ptr());
    }
};

} // namespace vpr
