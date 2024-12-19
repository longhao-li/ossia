#pragma once

#include "promise.hpp"

namespace ossia {
namespace detail {

/// \class future_awaitable
/// \tparam T
///   Return type of corresponding future.
/// \brief
///   \c future_awaitable is an awaitable type that allows to wait for a future to be ready.
template <class T>
class future_awaitable {
public:
    using value_type       = T;
    using promise_type     = promise<value_type>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    /// \brief
    ///   For internal usage. Create a \c future_awaitable from a coroutine handle to be awaited.
    /// \param coroutine
    ///   Handle to the coroutine that will be awaited. This value should not be null.
    explicit future_awaitable(coroutine_handle coroutine) : m_coroutine{coroutine} {}

    /// \brief
    ///   For internal usage. C++20 coroutine standard API. Checks if the corresponding coroutine is
    ///   done. Coroutines should not be resumed again once it is completed.
    /// \retval true
    ///   The corresponding coroutine is completed and should not be resumed.
    /// \retval false
    ///   The corresponding coroutine is not completed yet.
    [[nodiscard]]
    auto await_ready() const noexcept -> bool {
        return m_coroutine.done();
    }

    /// \brief
    ///   For internal usage. C++20 coroutine standard API. Suspend current coroutine and start the
    ///   callee coroutine.
    /// \tparam U
    ///   Promise type of the caller coroutine.
    /// \param caller
    ///   Handle to the caller coroutine to be suspended.
    /// \return
    ///   Handle to the callee coroutine to be resumed.
    template <class U>
    auto await_suspend(std::coroutine_handle<U> caller) noexcept -> std::coroutine_handle<> {
        auto &promise = static_cast<promise_base &>(m_coroutine.promise());
        auto &parent  = static_cast<promise_base &>(caller.promise());

        promise.m_parent       = &parent;
        promise.m_stack_bottom = parent.m_stack_bottom;

        return m_coroutine;
    }

    /// \brief
    ///   For internal usage. C++20 coroutine standard API. Get the result of the callee coroutine.
    /// \return
    ///   The result of the callee coroutine.
    auto await_resume() const -> value_type {
        return std::move(m_coroutine.promise()).result();
    }

private:
    coroutine_handle m_coroutine;
};

} // namespace detail

/// \class future
/// \tparam T
///   Return type of the future. Default is \c void.
/// \brief
///   \c future represents the result of an asynchronous operation.
template <class T>
class future {
public:
    using value_type       = T;
    using promise_type     = detail::promise<value_type>;
    using coroutine_handle = std::coroutine_handle<promise_type>;
    using awaitable_type   = detail::future_awaitable<value_type>;

    /// \brief
    ///   Create an empty future. This future will not be ready until it is assigned to a valid
    ///   future.
    future() noexcept : m_coroutine() {}

    /// \brief
    ///   For internal usage. Wrap a coroutine handle into a future.
    /// \param coroutine
    ///   Handle to the coroutine that will be wrapped. This value should not be null.
    explicit future(coroutine_handle coroutine) noexcept : m_coroutine(coroutine) {}

    /// \brief
    ///   Copy constructor of \c future. This will increase the reference count of the coroutine.
    /// \note
    ///   Atomic reference count is not used. It is the user's responsibility to ensure that data
    ///   hazard does not occur when copying futures.
    /// \param other
    ///   The future to be copied.
    future(const future &other) noexcept : m_coroutine(other.m_coroutine) {
        if (m_coroutine)
            m_coroutine.promise().acquire();
    }

    /// \brief
    ///   Move constructor of \c future. This will transfer the ownership of the coroutine.
    /// \param[in, out] other
    ///   The future to be moved. The moved future will be empty after this operation.
    future(future &&other) noexcept : m_coroutine(other.m_coroutine) {
        other.m_coroutine = nullptr;
    }

    /// \brief
    ///   Destructor of \c future. This will decrease the reference count of the coroutine.
    ~future() {
        if (m_coroutine)
            m_coroutine.promise().release();
    }

    /// \brief
    ///   Copy assignment operator of \c future. This will increase the reference count of the
    ///   copied coroutine if it is not null.
    /// \note
    ///   Atomic reference count is not used. It is the user's responsibility to ensure that data
    ///   hazard does not occur when copying futures.
    /// \param other
    ///   The future to be copied.
    /// \return
    ///   Reference to this future.
    auto operator=(const future &other) noexcept -> future & {
        if (this == &other) [[unlikely]]
            return *this;

        if (m_coroutine == other.m_coroutine) [[unlikely]]
            return *this;

        if (m_coroutine != nullptr)
            m_coroutine.promise().release();

        m_coroutine = other.m_coroutine;
        if (m_coroutine != nullptr)
            m_coroutine.promise().acquire();

        return *this;
    }

    /// \brief
    ///   Move assignment operator of \c future. This will transfer the ownership of the coroutine.
    /// \param[in, out] other
    ///   The future to be moved. The moved future will be empty after this operation.
    /// \return
    ///   Reference to this future.
    auto operator=(future &&other) noexcept -> future & {
        if (this == &other) [[unlikely]]
            return *this;

        if (m_coroutine != nullptr)
            m_coroutine.promise().release();

        m_coroutine       = other.m_coroutine;
        other.m_coroutine = nullptr;

        return *this;
    }

    /// \brief
    ///   Checks if this future is null.
    /// \retval true
    ///   This future is null.
    /// \retval false
    ///   This future is not null.
    [[nodiscard]]
    auto is_null() const noexcept -> bool {
        return m_coroutine == nullptr;
    }

    /// \brief
    ///   Checks if this future is ready.
    /// \retval true
    ///   This future is ready.
    /// \retval false
    ///   This future is not ready.
    [[nodiscard]]
    auto is_ready() const noexcept -> bool {
        return m_coroutine != nullptr && m_coroutine.done();
    }

    /// \brief
    ///   Detach coroutine from this future. This future will be empty after this operation and you
    ///   should manage lifetime of the returned coroutine manually. This method does not affect
    ///   reference count of the coroutine.
    /// \return
    ///   Handle to the coroutine that is detached.
    [[nodiscard]]
    auto detach() noexcept -> coroutine_handle {
        return std::exchange(m_coroutine, nullptr);
    }

    /// \brief
    ///   Get handle to the underlying coroutine of this future.
    /// \return
    ///   Handle to the underlying coroutine of this future.
    [[nodiscard]]
    auto coroutine() const noexcept -> coroutine_handle {
        return m_coroutine;
    }

    /// \brief
    ///   C++20 coroutine standard API. Suspend the caller coroutine and start executing coroutine
    ///   for this future.
    auto operator co_await() const noexcept -> awaitable_type {
        return awaitable_type(m_coroutine);
    }

private:
    coroutine_handle m_coroutine;
};

} // namespace ossia

namespace ossia::detail {

/// \brief
///   For internal usage. C++20 coroutine API. Create a \c future object from this promise.
/// \return
///   \c promise object created from this promise.
template <class T>
auto promise<T>::get_return_object() noexcept -> future<T> {
    auto coroutine = std::coroutine_handle<promise>::from_promise(*this);
    m_coroutine    = coroutine;
    m_stack_bottom = this;
    return future<T>(coroutine);
}

/// \brief
///   For internal usage. C++20 coroutine API. Create a \c future object from this promise.
/// \return
///   \c promise object created from this promise.
template <class T>
auto promise<T &>::get_return_object() noexcept -> future<T &> {
    auto coroutine = std::coroutine_handle<promise>::from_promise(*this);
    m_coroutine    = coroutine;
    m_stack_bottom = this;
    return future<T &>(coroutine);
}

/// \brief
///   For internal usage. C++20 coroutine API. Create a \c future object from this promise.
/// \return
///   \c promise object created from this promise.
inline auto promise<void>::get_return_object() noexcept -> future<void> {
    auto coroutine = std::coroutine_handle<promise>::from_promise(*this);
    m_coroutine    = coroutine;
    m_stack_bottom = this;
    return future<void>(coroutine);
}

} // namespace ossia::detail
