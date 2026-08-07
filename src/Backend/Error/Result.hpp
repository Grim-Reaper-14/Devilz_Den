#pragma once

#include "Error.hpp"

#include <optional>
#include <stdexcept>
#include <utility>

namespace Devilz::Backend
{
template <typename T>
class Result
{
public:
    static Result Success(T value) { return Result(std::move(value)); }
    static Result Failure(Error error) { return Result(std::move(error)); }

    [[nodiscard]] bool HasValue() const noexcept { return m_value.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

    T& Value()
    {
        if (!m_value) throw std::logic_error("Result has no value");
        return *m_value;
    }

    const T& Value() const
    {
        if (!m_value) throw std::logic_error("Result has no value");
        return *m_value;
    }

    const Error& Failure() const
    {
        if (!m_error) throw std::logic_error("Result has no error");
        return *m_error;
    }

private:
    explicit Result(T value) : m_value(std::move(value)) {}
    explicit Result(Error error) : m_error(std::move(error)) {}

    std::optional<T> m_value;
    std::optional<Error> m_error;
};

template <>
class Result<void>
{
public:
    static Result Success() { return Result(); }
    static Result Failure(Error error) { return Result(std::move(error)); }

    [[nodiscard]] bool HasValue() const noexcept { return !m_error.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

    const Error& Failure() const
    {
        if (!m_error) throw std::logic_error("Result has no error");
        return *m_error;
    }

private:
    Result() = default;
    explicit Result(Error error) : m_error(std::move(error)) {}
    std::optional<Error> m_error;
};
}
