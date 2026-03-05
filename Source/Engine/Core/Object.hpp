#pragma once

#include <memory>
#include <optional> // Added to declare std::optional
#include <utility>  // Added to use std::forward

class Object
{
public:
    Object() = default;
    virtual ~Object() = default;

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    Object(Object&&) noexcept = default;
    Object& operator=(Object&&) noexcept = default;
};

// Alias
template <typename T>
using UniquePtr = std::unique_ptr<T>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T>
using OptionalPtr = std::optional<T>;

template<typename T, typename ...Args>
UniquePtr<T> MakeUnique(Args && ...args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename ...Args>
SharedPtr<T> MakeShared(Args && ...args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

