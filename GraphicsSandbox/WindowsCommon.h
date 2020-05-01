#pragma once


#include <windows.h>

#include <stdexcept>
#include <string>


class WindowsException : public std::runtime_error {
public:
    WindowsException(HRESULT hr, const std::string& fileName, int lineNumber)
    : std::runtime_error(ToString(hr, fileName, lineNumber)), mHr(hr) {
    }

    HRESULT Error() const {
        return mHr;
    }

private:
    std::string ToString(HRESULT hr, const std::string& fileName, int lineNumber) {
        char str[64] = {};
        sprintf_s(str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
        return std::string(str) + "\n" + fileName + " (" + std::to_string(lineNumber) + ")";
    }

private:
    const HRESULT mHr;
};


#define WINDOWS_CHECK(expression) \
{\
    if (!(expression)) {\
        throw WindowsException(HRESULT_FROM_WIN32(GetLastError()), __FILE__, __LINE__);\
    }\
}
