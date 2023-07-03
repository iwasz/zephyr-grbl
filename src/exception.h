/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <etl/string.h>
#include <exception>

namespace exc {
using ErrorBuffer = etl::string<18 * 4>;
using StatusCode = uint8_t;

/**
 * TODO differentiate to HardwareException, UserException and other types. The problem here are the varargs
 */
class Exception : public std::exception {
public:
        Exception (Exception const &) = default;
        Exception &operator= (Exception const &) = default;
        Exception (Exception &&) = default;
        Exception &operator= (Exception &&) = default;
        ~Exception () override = default;

        Exception (StatusCode ec, const char *fmt, ...) : errorCode{ec}
        {
                // C++20 <format> is not implemnted yet, and fmt makes code grow in size by 100%. Therefore I'm using
                // what;s already available and widely used.
                msg_.resize (msg_.max_size ());

                va_list args;
                va_start (args, fmt);
                auto len = vsnprintf (msg_.data (), msg_.max_size (), fmt, args);
                va_end (args);
                msg_.resize (len);
        }

        ErrorBuffer const &msg () const { return msg_; }
        const char *what () const noexcept override { return msg_.c_str (); }
        auto code () const { return errorCode; }

private:
        ErrorBuffer msg_;
        StatusCode errorCode;
};

} // namespace exc