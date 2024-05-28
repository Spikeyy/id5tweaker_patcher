#pragma once

namespace hook
{
    class pattern
    {
    public:
        pattern(const std::string& patternStr)
        {
            MODULEINFO modInfo{};
            GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &modInfo, sizeof(modInfo));

            initialize(patternStr, reinterpret_cast<uint8_t*>(modInfo.lpBaseOfDll), reinterpret_cast<uint8_t*>(modInfo.lpBaseOfDll) + modInfo.SizeOfImage);
        }

        pattern(const std::string& patternStr, uintptr_t startAddress, uintptr_t endAddress) : pattern(patternStr, reinterpret_cast<uint8_t*>(startAddress), reinterpret_cast<uint8_t*>(endAddress))
        {
        }

        pattern(const std::string& patternStr, uint8_t* startAddress, uint8_t* endAddress)
        {
            initialize(patternStr, startAddress, endAddress);
        }

        uintptr_t find() const
        {
            auto addr = std::search(this->startAddress, this->endAddress, patternBytes.cbegin(), patternBytes.cend(),
                [](const uint8_t current, const uint8_t patternByte)
                {
                    // -1 for ?/??
                    return patternByte == static_cast<uint8_t>(-1) || current == patternByte;
                });

            if (addr == endAddress)
            {
                return 0;
            }

            return reinterpret_cast<uintptr_t>(addr);
        }

        std::vector<uintptr_t> find_all()
        {
            uint8_t* start = this->startAddress;
            std::vector<uintptr_t> results;
            while (true)
            {
                auto addr = std::search(start, this->endAddress, patternBytes.cbegin(), patternBytes.cend(),
                    [](const uint8_t current, const uint8_t patternByte)
                    {
                        return patternByte == static_cast<uint8_t>(-1) || current == patternByte;
                    });

                if (addr == this->endAddress)
                {
                    break;
                }

                results.push_back(reinterpret_cast<uintptr_t>(addr));

                start = addr + 1;
            }

            return results;
        }

    private:
        std::vector<uint8_t> get_pattern_bytes(const std::string& pattern) const
        {
            std::vector<uint8_t> bytes;
            for (size_t i = 0; i < pattern.length();)
            {
                if (pattern[i] == '?')
                {
                    bytes.push_back(-1);
                    if (pattern[i + 1] == '?')
                    {
                        i++;
                    }
                    i += 2; // skip ? + space
                }
                else
                {
                    bytes.push_back(strtoul(&pattern[i], nullptr, 16));
                    i += 3;
                }
            }

            return bytes;
        }

        void initialize(const std::string& pattern, uint8_t* startAddress, uint8_t* endAddress)
        {
            this->startAddress = startAddress;
            this->endAddress = endAddress;
            this->patternBytes = get_pattern_bytes(pattern);
        }

        uint8_t* startAddress;
        uint8_t* endAddress;
        std::vector<uint8_t> patternBytes;
        //std::vector<uintptr_t> matches;
    };

    inline uintptr_t find_pattern(const std::string& patternStr, ptrdiff_t offset = 0)
    {
        return pattern(patternStr).find() + offset;
    }

    inline uintptr_t find_pattern(const std::string& module, const std::string& patternStr, ptrdiff_t offset = 0)
    {
        MODULEINFO modInfo = {};
        GetModuleInformation(GetCurrentProcess(), GetModuleHandle(module.c_str()), &modInfo, sizeof(modInfo));

        return pattern(patternStr, reinterpret_cast<uint8_t *>(modInfo.lpBaseOfDll), reinterpret_cast<uint8_t*>(modInfo.lpBaseOfDll) + modInfo.SizeOfImage).find() + offset;
    }

    inline std::vector<uintptr_t> find_patterns(const std::string& patternStr)
    {
        return pattern(patternStr).find_all();
    }

    inline std::vector<uintptr_t> find_patterns(const std::string &module, const std::string& patternStr)
    {
        MODULEINFO modInfo = {};
        GetModuleInformation(GetCurrentProcess(), GetModuleHandle(module.c_str()), &modInfo, sizeof(modInfo));

        return pattern(patternStr, reinterpret_cast<uint8_t*>(modInfo.lpBaseOfDll), reinterpret_cast<uint8_t*>(modInfo.lpBaseOfDll) + modInfo.SizeOfImage).find_all();
    }

    template<typename T = uintptr_t>
    inline T absolute_address_from_instruction(uintptr_t address)
    {
        auto relative = *reinterpret_cast<int32_t*>(address);

        return T(address + sizeof(int32_t) + relative);
    }

    inline void nop(void* place, size_t size)
    {
        DWORD oldProtect;
        VirtualProtect(place, size, PAGE_EXECUTE_READWRITE, &oldProtect);

        memset(place, 0x90, size);

        VirtualProtect(place, size, oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), place, size);
    }

    inline void nop(uintptr_t address, size_t size)
    {
        nop(reinterpret_cast<void*>(address), size);
    }

    uintptr_t get_vtable_func(uintptr_t vtable, size_t tableIndex)
    {
        return vtable + sizeof(intptr_t) * tableIndex;
    }

    uintptr_t vtable_func_from_instance(uintptr_t instance, size_t tableIndex)
    {
        auto vtable = *(uintptr_t*)instance;

        return *reinterpret_cast<uintptr_t *>(get_vtable_func(vtable, tableIndex));
    }
}
