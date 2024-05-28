#include "pch.h"

namespace utils
{
    // from https://stackoverflow.com/a/17387176
    std::string get_last_error_as_string()
    {
        //Get the error message, if any.
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0)
            return {}; //No error message has been recorded

        PTCHAR messageBuffer = nullptr;
        size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (PTCHAR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);

        //Free the buffer.
        LocalFree(messageBuffer);

        return message;
    }

    uintptr_t get_base_address()
    {
        static uintptr_t baseAddress = 0;

        if (!baseAddress)
        {
            MODULEINFO modInfo{};
            GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &modInfo, sizeof(modInfo));
            return reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
        }

        return baseAddress;
    }
}