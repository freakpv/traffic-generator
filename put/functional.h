#pragma once

namespace put // project utilities
{

template <typename T>
class fun_ref;

template <typename Ret, typename... Args>
class fun_ref<Ret(Args...)> final
{
    using callback_type = Ret (*)(void*, Args...);

    void* callable_         = nullptr;
    callback_type callback_ = nullptr;

public:
    fun_ref() noexcept = default;

    template <typename Callable>
        requires(std::is_invocable_r_v<Ret, Callable, Args...> &&
                 !std::is_same_v<std::decay_t<Callable>, fun_ref>)
    fun_ref(Callable&& rhs) noexcept : callable_((void*)std::addressof(rhs))
    {
        callback_ = [](void* callable, Args... args) -> Ret {
            return (*reinterpret_cast<std::add_pointer_t<Callable>>(callable))(
                std::forward<Args>(args)...);
        };
    }

    Ret operator()(Args... args) const
        noexcept(noexcept(callback_(callable_, std::forward<Args>(args)...)))
    {
        return callback_(callable_, std::forward<Args>(args)...);
    }
};

////////////////////////////////////////////////////////////////////////////////
// Very primitive callback helper type
// Don't want to add fully fledged in_place function or something like that
template <typename T>
class callback;

template <typename Ret, typename... Args>
class callback<Ret(Args...)> final
{
public:
    using ctx_ptr_type = void*;
    using fun_ptr_type = Ret (*)(ctx_ptr_type, Args...);

private:
    ctx_ptr_type ctx_ = nullptr;
    fun_ptr_type fun_ = nullptr;

public:
    callback() noexcept = default;

    callback(ctx_ptr_type ctx, fun_ptr_type fun) noexcept : ctx_(ctx), fun_(fun)
    {
    }

    Ret operator()(Args... args) const
    {
        return fun_(ctx_, std::forward<Args>(args)...);
    }
};

} // namespace put
