#pragma once

namespace zz
{
    template <
        typename handle_type,
        typename close_type,
        handle_type null_value = nullptr
        >
    class unique_handle
    {
        typedef unique_handle<
            handle_type,
            close_type,
            null_value
            > this_type;
        unique_handle() = delete;
        unique_handle(const this_type &) = delete;
        this_type & operator = (const this_type &) = delete;
    public:
        unique_handle(this_type &&) = default;
        this_type & operator = (this_type &&) = default;
        explicit unique_handle(close_type close) noexcept
            : _handle(null_value)
            , _close(close)
        {
        }
        this_type & operator = (handle_type handle) noexcept
        {
            if (_handle != handle) {
                if (_handle != null_value)
                    _close(_handle);
                _handle = handle;
            }
            return *this;
        }
        explicit operator bool () const noexcept
        {
            return _handle != null_value;
        }
        operator handle_type () const noexcept
        {
            return _handle;
        }
        handle_type * operator & () noexcept
        {
            return &_handle;
        }
        ~unique_handle() noexcept
        {
            if (_handle != null_value)
                _close(_handle);
        }
    private:
        handle_type _handle;
        close_type  _close;
    };
}
