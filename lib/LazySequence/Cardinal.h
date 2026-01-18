#ifndef CARDINAL_H
#define CARDINAL_H

#include <stdexcept>
#include <string>

enum class CardinalType {
    FINITE,
    ALEPH_NULL  //  - countable infinity
};

class Cardinal {
private:
    CardinalType type_;
    size_t finite_value_;

public:
    Cardinal(size_t value = 0);
    static Cardinal AlephNull();

    bool IsFinite() const;
    bool IsInfinite() const;
    size_t GetFiniteValue() const;
    std::string ToString() const;

    bool operator==(const Cardinal& other) const;
    bool operator<(const Cardinal& other) const;
};

//////////////////////////////////////////////////////////////////////

inline Cardinal::Cardinal(size_t value) : type_(CardinalType::FINITE), finite_value_(value) {}

inline Cardinal Cardinal::AlephNull() {
    Cardinal c;
    c.type_ = CardinalType::ALEPH_NULL;
    return c;
}

//////////////////////////////////////////////////////////////////////

inline bool Cardinal::IsFinite() const {
    return type_ == CardinalType::FINITE;
}

inline bool Cardinal::IsInfinite() const {
    return type_ == CardinalType::ALEPH_NULL;
}

inline size_t Cardinal::GetFiniteValue() const {
    if (!IsFinite()) {
        throw std::logic_error("Cardinal is infinite");
    }
    return finite_value_;
}

inline std::string Cardinal::ToString() const {
    if (IsFinite()) {
        return std::to_string(finite_value_);
    }
    else {
        return "aleph_0";  // Countable infinity symbol
    }
}

//////////////////////////////////////////////////////////////////////

inline bool Cardinal::operator==(const Cardinal& other) const {
    if (type_ != other.type_) return false;
    if (IsFinite()) return finite_value_ == other.finite_value_;
    return true;
}

inline bool Cardinal::operator<(const Cardinal& other) const {
    if (IsFinite() && other.IsFinite()) {
        return finite_value_ < other.finite_value_;
    }
    if (IsFinite() && other.IsInfinite()) return true;
    return false;
}

#endif // CARDINAL_H
