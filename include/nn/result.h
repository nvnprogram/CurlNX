#ifndef NN_RESULT_MIN_H
#define NN_RESULT_MIN_H

#include <cstdint>

#ifndef NN_NOEXCEPT
#define NN_NOEXCEPT noexcept
#endif

namespace nn {

class Result {
public:
    typedef uint32_t InnerType;

    Result() NN_NOEXCEPT : m_Value(0) {}

    constexpr bool IsSuccess() const NN_NOEXCEPT { return m_Value == 0; }
    constexpr bool IsFailure() const NN_NOEXCEPT { return m_Value != 0; }

    constexpr InnerType GetValue() const NN_NOEXCEPT { return m_Value; }
    constexpr InnerType GetInnerValueForDebug() const NN_NOEXCEPT { return m_Value; }

    constexpr int GetModule() const NN_NOEXCEPT {
        return static_cast<int>(m_Value & 0x1FF);
    }
    constexpr int GetDescription() const NN_NOEXCEPT {
        return static_cast<int>((m_Value >> 9) & 0x1FFF);
    }

private:
    InnerType m_Value;
};

static_assert(sizeof(Result) == 4, "nn::Result must be 4 bytes");

}

#endif /* NN_RESULT_MIN_H */
