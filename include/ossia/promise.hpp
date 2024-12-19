#pragma once

#include <coroutine>
#include <cstdint>
#include <exception>
#include <optional>

namespace ossia {

/// \class future
/// \tparam T
///   Return type of the future. Default is \c void.
/// \brief
///   \c future represents the result of an asynchronous operation.
template <class T = void>
class future;

} // namespace ossia

namespace ossia::detail {

/// \struct final_awaitable
/// \brief
///   For internal usage. Manage the coroutine call stack frames once the coroutine is done.
struct final_awaitable {
    /// \brief
    ///   For internal usage. C++20 coroutine API. Always enter \c await_suspend to manage the
    ///   coroutine call stack frames once the coroutine is done.
    /// \return
    ///   Always returns \c false.
    [[nodiscard]]
    static constexpr auto await_ready() noexcept -> bool {
        return false;
    }

    /// \brief
    ///   For internal usage. C++20 coroutine API. Maintain the coroutine call stack status and
    ///   resume the caller.
    /// \tparam T
    ///   Promise type of the coroutine to be finalized.
    /// \param current
    ///   Handle to the coroutine to be finalized.
    /// \return
    ///   Handle to the caller of the coroutine to be resumed. \c std::noop_coroutine() will be
    ///   returned if the caller is the root frame.
    template <class T>
    static auto await_suspend(std::coroutine_handle<T> current) noexcept -> std::coroutine_handle<>;

    /// \brief
    ///   For internal usage. C++20 coroutine API. Do nothing.
    static constexpr auto await_resume() noexcept -> void {}
};

/// \class promise_base
/// \brief
///   Base class for promise types.
class promise_base {
public:
    /// \brief
    ///   Constructs a promise base.
    promise_base() noexcept
        : m_reference_count(1),
          m_coroutine(),
          m_parent(nullptr),
          m_stack_bottom(nullptr),
          m_exception() {}

    /// \brief
    ///   \c promise_base is not copyable.
    promise_base(const promise_base &other) = delete;

    /// \brief
    ///   \c promise_base is not movable.
    promise_base(promise_base &&other) = delete;

    /// \brief
    ///   Destroy this promise base.
    ~promise_base() = default;

    /// \brief
    ///   \c promise_base is not copyable.
    auto operator=(const promise_base &other) = delete;

    /// \brief
    ///   \c promise_base is not movable.
    auto operator=(promise_base &&other) = delete;

    /// \brief
    ///   For internal usage. C++20 coroutine API. Futures should always be suspended once they are
    ///   created.
    /// \return
    ///   Always returns an empty \c std::suspend_always object.
    [[nodiscard]]
    static constexpr auto initial_suspend() noexcept -> std::suspend_always {
        return {};
    }

    /// \brief
    ///   For internal usage. C++20 coroutine API. Manage the coroutine call stack frames once the
    ///   coroutine is done.
    /// \return
    ///   Always returns an empty \c final_awaitable object.
    [[nodiscard]]
    static constexpr auto final_suspend() noexcept -> final_awaitable {
        return {};
    }

    /// \brief
    ///   For internal usage. C++20 coroutine API. Capture and store the exception thrown by this
    ///   coroutine.
    auto unhandled_exception() noexcept -> void {
        m_exception = std::current_exception();
    }

    /// \brief
    ///   Increase reference count for this coroutine stack frame.
    auto acquire() noexcept -> void {
        m_reference_count += 1;
    }

    /// \brief
    ///   Decrease reference count for this coroutine stack frame. If the reference count is zero,
    ///   destroy this coroutine stack frame.
    /// \note
    ///   Atomic reference count is not used. This function should be called in the same thread.
    auto release() noexcept -> void {
        m_reference_count -= 1;
        if (m_reference_count == 0)
            m_coroutine.destroy();
    }

    /// \brief
    ///   Get handle to this coroutine stack frame.
    /// \return
    ///   Handle to this coroutine stack frame.
    [[nodiscard]]
    auto coroutine() const noexcept -> std::coroutine_handle<> {
        return m_coroutine;
    }

    /// \brief
    ///   Get promise of the bottom coroutine stack frame.
    /// \return
    ///   Reference to the promise of the bottom coroutine stack frame.
    [[nodiscard]]
    auto stack_bottom() const noexcept -> promise_base & {
        return *m_stack_bottom;
    }

    friend struct final_awaitable;

    template <class>
    friend class future_awaitable;

private:
    /// \brief
    ///   Reference count of this coroutine stack frame.
    std::uint32_t m_reference_count;

protected:
    /// \brief
    ///   C++20 coroutine handle to this coroutine stack frame.
    std::coroutine_handle<> m_coroutine;

    /// \brief
    ///   Pointer to caller of this coroutine frame. This is used to resume the caller when this
    ///   coroutine frame is done. This value is \c nullptr if this coroutine frame is the root
    ///   frame.
    promise_base *m_parent;

    /// \brief
    ///   Pointer to the bottom of the coroutine stack.
    promise_base *m_stack_bottom;

    /// \brief
    ///   Exception thrown by this coroutine.
    std::exception_ptr m_exception;
};

/// \brief
///   For internal usage. C++20 coroutine API. Maintain the coroutine call stack status and resume
///   the caller.
/// \tparam T
///   Promise type of the coroutine to be finalized.
/// \param current
///   Handle to the coroutine to be finalized.
/// \return
///   Handle to the caller of the coroutine to be resumed. \c std::noop_coroutine() will be returned
///   if the caller is the root frame.
template <class T>
auto final_awaitable::await_suspend(std::coroutine_handle<T> current) noexcept
    -> std::coroutine_handle<> {
    auto &promise = static_cast<promise_base &>(current.promise());
    auto *parent  = promise.m_parent;
    return parent ? parent->m_coroutine : std::noop_coroutine();
}

/// \class promise
/// \tparam T
///   Return type of the coroutine.
/// \brief
///   Promise type for coroutine.
template <class T>
class promise final : public promise_base {
public:
    /// \brief
    ///   Create a promise.
    promise() noexcept : promise_base(), m_value() {}

    /// \brief
    ///   For internal usage. C++20 coroutine API. Create a \c future object from this promise.
    /// \return
    ///   \c promise object created from this promise.
    auto get_return_object() noexcept -> future<T>;

    /// \brief
    ///   For internal usage. C++20 coroutine API. Set the return value of this coroutine.
    /// \tparam Arg
    ///   Type of the return value. The type should be convertible to \c T.
    /// \param arg
    ///   Reference to the actual return value of this coroutine.
    template <class Arg = T>
        requires(std::is_constructible_v<T, Arg &&>)
    auto return_value(Arg &&arg) noexcept(std::is_nothrow_constructible_v<T, Arg &&>) -> void {
        m_value.emplace(std::forward<Arg>(arg));
    }

    /// \brief
    ///   Get result of this coroutine. Exceptions may be thrown if the coroutine is done with an
    ///   exception.
    /// \return
    ///   Reference to the result of this coroutine.
    [[nodiscard]]
    auto result() & -> T & {
        if (m_exception != nullptr) [[unlikely]]
            std::rethrow_exception(m_exception);
        return *m_value;
    }

    /// \brief
    ///   Get result of this coroutine. Exceptions may be thrown if the coroutine is done with an
    ///   exception.
    /// \return
    ///   Reference to the result of this coroutine.
    [[nodiscard]]
    auto result() const & -> T & {
        if (m_exception != nullptr) [[unlikely]]
            std::rethrow_exception(m_exception);
        return *m_value;
    }

    /// \brief
    ///   Get result of this coroutine. Exceptions may be thrown if the coroutine is done with an
    ///   exception.
    /// \return
    ///   Reference to the result of this coroutine.
    [[nodiscard]]
    auto result() && -> T && {
        if (m_exception != nullptr) [[unlikely]]
            std::rethrow_exception(m_exception);
        return *std::move(m_value);
    }

private:
    std::optional<T> m_value;
};

/// \class promise
/// \tparam T
///   Returned reference type of the coroutine.
/// \brief
///   Promise type for coroutine that returns a reference.
template <class T>
class promise<T &> final : public promise_base {
public:
    /// \brief
    ///   Create a promise.
    promise() noexcept : promise_base(), m_value() {}

    /// \brief
    ///   For internal usage. C++20 coroutine API. Create a \c future object from this promise.
    /// \return
    ///   \c promise object created from this promise.
    auto get_return_object() noexcept -> future<T &>;

    /// \brief
    ///   For internal usage. C++20 coroutine API. Set the return value of this coroutine.
    /// \param[in] value
    ///   The value returned by coroutine.
    auto return_value(T &value) noexcept -> void {
        m_value = std::addressof(value);
    }

    /// \brief
    ///   Get result of this coroutine. Exceptions may be thrown if the coroutine is done with an
    ///   exception.
    /// \return
    ///   Reference to the result of this coroutine.
    [[nodiscard]]
    auto result() const noexcept -> T & {
        if (m_exception != nullptr) [[unlikely]]
            std::rethrow_exception(m_exception);
        return *m_value;
    }

private:
    T *m_value;
};

/// \class promise
/// \brief
///   Promise type for coroutine that returns nothing.
template <>
class promise<void> final : public promise_base {
public:
    /// \brief
    ///   Create a promise.
    promise() noexcept : promise_base() {}

    /// \brief
    ///   For internal usage. C++20 coroutine API. Create a \c future object from this promise.
    /// \return
    ///   \c promise object created from this promise.
    auto get_return_object() noexcept -> future<void>;

    /// @brief
    ///   For internal usage. C++20 coroutine API. Marks that this coroutine has no return value.
    auto return_void() noexcept -> void {}

    /// \brief
    ///   Get result of this coroutine. Exceptions may be thrown if the coroutine is done with an
    ///   exception.
    auto result() const -> void {
        if (m_exception != nullptr) [[unlikely]]
            std::rethrow_exception(m_exception);
    }
};

} // namespace ossia::detail
