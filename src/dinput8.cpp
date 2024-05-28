#include "pch.h"
#include "utils.h"
#include "hook.h"

using DirectInput8Create_t = HRESULT(*)(HINSTANCE hInst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);
static DirectInput8Create_t oDirectInput8Create;

template<>
struct std::formatter<std::vector<unsigned char>> : std::formatter<std::string_view>
{
    auto format(const std::vector<unsigned char> &vec, std::format_context& ctx) const
    {
        std::string temp = "";
        for (const auto& elem : vec)
        {
            std::format_to(std::back_inserter(temp), "{:X} ", (int)elem);
        }

        return std::formatter<std::string_view>::format(temp, ctx);
    }
};

std::string get_process_name()
{
    char buf[MAX_PATH];
    GetModuleFileName(NULL, buf, sizeof(buf));
    return std::filesystem::path(buf).stem().string();
}

enum class WolfGame
{
    THE_OLD_BLOOD,
    THE_NEW_ORDER,
    INVALID
};

void patch_id5Tweaker(const WolfGame game)
{
    if (game == WolfGame::INVALID)
    {
        return;
    }

    const auto cmdSystemLocal = *hook::absolute_address_from_instruction<uintptr_t*>(hook::find_pattern("48 8B 0D ?? ?? ?? ?? 48 8B 01 48 8D 15 ?? ?? ?? ?? FF 50 ?? 8B D3", 3));
    const auto cmdSystemLocal_AddCommand = hook::vtable_func_from_instance(cmdSystemLocal, 3);
    const auto cmdSystemLocal_ExecuteCommandText = hook::vtable_func_from_instance(cmdSystemLocal, 8);

    const auto cvarSystemLocal = *hook::absolute_address_from_instruction<uintptr_t*>(hook::find_pattern("48 8B 0D ?? ?? ?? ?? 48 8B 01 45 33 C9 45 33 C0 48 8D 15 ?? ?? ?? ?? FF 50 ?? 48 8D 15", 3));
    
    const auto cvarSystemLocalVftable = hook::absolute_address_from_instruction(hook::find_pattern("48 8D 05 ?? ?? ?? ?? 48 89 01 33 F6 48 89 71 08 E8 ?? ?? ?? ?? C7 43 18 00 00 31 00", 3));
    const auto cvarSystem_Find = *reinterpret_cast<uintptr_t*>(hook::get_vtable_func(cvarSystemLocalVftable, 4));

    const auto setGameHz = hook::absolute_address_from_instruction(hook::find_pattern("E8 ?? ?? ?? ?? 80 BE ?? 03 00 00 00 74 08", 1));
    const auto consoleLocal_keyCatching = hook::absolute_address_from_instruction(hook::find_pattern("C6 05 ?? ?? ?? ?? ?? 74 07", 2)) + 1;
    const auto idCvar_Set = hook::find_pattern("48 89 5C 24 08 57 48 83 EC 20 48 8B FA 48 8B D9 45 84 C0 75");

    const auto gameLocalVftable = hook::absolute_address_from_instruction(hook::find_pattern("48 8D 05 ? ? ? ? 48 89 01 48 8B D1 48 8D 4C 24 ?", 3));
    const auto loadMapOffset = game == WolfGame::THE_NEW_ORDER ? 0xC1 : 0xC5;
    const auto gameLocal_LoadMap = *reinterpret_cast<uintptr_t*>(hook::get_vtable_func(gameLocalVftable, loadMapOffset));

    // Key = pattern to search for in id5tweaker
    // value = memory address to replace with
    std::unordered_map<std::string, uintptr_t> patternMap;
    if(game == WolfGame::THE_NEW_ORDER)
    {
        patternMap.emplace("20 A1 5B 41 01", cmdSystemLocal);
        patternMap.emplace("30 DF 9E 40 01", cmdSystemLocal_AddCommand);
        patternMap.emplace("E0 CC 15", setGameHz);
        patternMap.emplace("40 54 86 40 01", gameLocal_LoadMap);
        //patternMap.emplace("40 54 86 40 01", gameLocal_LoadMap);
        patternMap.emplace("90 E1 9E 40 01", cmdSystemLocal_ExecuteCommandText);
        patternMap.emplace("D0 D2 9D 40 01", idCvar_Set);
        patternMap.emplace("40 6F EF 41 01", cvarSystemLocal);
        patternMap.emplace("80 D5 9D 40 01", cvarSystem_Find);
        patternMap.emplace("60 17 6B 41 01", consoleLocal_keyCatching);
    }
    else if(game == WolfGame::THE_OLD_BLOOD)
    {
        patternMap.emplace("90 00 7A 41 01", cmdSystemLocal);
        patternMap.emplace("60 36 9C 40 01", cmdSystemLocal_AddCommand);
        patternMap.emplace("70 84 15", setGameHz);
        patternMap.emplace("80 65 83 40 01", gameLocal_LoadMap);
        //patternMap.emplace("80 65 83 40 01", gameLocal_LoadMap);
        patternMap.emplace("A0 4B 9C 40 01", cmdSystemLocal_ExecuteCommandText);
        patternMap.emplace("20 6A 9B 40 01", idCvar_Set);
        patternMap.emplace("60 9D 0F 42 01", cvarSystemLocal);
        patternMap.emplace("C0 5B 9B 40 01", cvarSystem_Find);
        patternMap.emplace("40 6B 8B 41 01", consoleLocal_keyCatching);
    }


    for (const auto [pattern, address] : patternMap)
    {
        auto patterns = hook::find_patterns("id5tweaker", pattern);
        for (const auto location : patterns)
        {
            if (!location)
            {
                throw std::runtime_error("Couldn't find pattern " + pattern);
            }

            uint32_t offset = address - utils::get_base_address();

            OutputDebugString(std::format("Location {:X} offset {:X}\n", *reinterpret_cast<uintptr_t*>(location), offset).c_str());

            if (*reinterpret_cast<uintptr_t*>(location) == offset || *reinterpret_cast<uintptr_t*>(location) == (offset + 0x140000000))
            {
                continue;
            }

            //#ifdef _DEBUG
            //{
            //    auto get_bytes_at_addr = [](uintptr_t addr, size_t numBytes) {
            //        std::vector<unsigned char> bytes;
            //        bytes.resize(numBytes);
            //        memcpy(&bytes[0], (uint8_t*)addr, numBytes);

            //        return bytes;
            //    };

            //    OutputDebugString(std::format("Changing bytes at address {:X} from {} to {:X}\n", location, get_bytes_at_addr(location, 3), _byteswap_uint64(offset)).c_str());
            //}
            //#endif

            DWORD oldProtect;
            constexpr auto NUM_BYTES = 3;
            VirtualProtect(reinterpret_cast<void*>(location), NUM_BYTES, PAGE_READWRITE, &oldProtect);
            memcpy(reinterpret_cast<void*>(location), reinterpret_cast<uint8_t*>(&offset), NUM_BYTES);
            VirtualProtect(reinterpret_cast<void*>(location), NUM_BYTES, oldProtect, &oldProtect);
        }
    }
}

void load_id5Tweaker()
{
    auto hMod = LoadLibrary("id5tweaker.dll");
    if (hMod == nullptr)
    {
        throw std::runtime_error(std::format("Error loading id5tweaker.dll\n\n{}", utils::get_last_error_as_string()).c_str());
    }

    oDirectInput8Create = reinterpret_cast<DirectInput8Create_t>(GetProcAddress(hMod, "DirectInput8Create"));

    if (oDirectInput8Create == NULL)
    {
        throw std::runtime_error(std::format("Error loading DirectInput8Create: {}", utils::get_last_error_as_string()));
    }

    const std::string processName = get_process_name();
    OutputDebugString(std::format("Process name - {}\n", processName).c_str());
    
    WolfGame game = WolfGame::INVALID;
    if (processName == "WolfNewOrder_x64")
    {
        game = WolfGame::THE_NEW_ORDER;
    }
    else if (processName == "WolfOldBlood_x64")
    {
        game = WolfGame::THE_OLD_BLOOD;
    }

    patch_id5Tweaker(game);
}

extern "C"
{
    __declspec(dllexport) HRESULT DirectInput8Create(HINSTANCE hInst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
    {
        return oDirectInput8Create(hInst, dwVersion, riidltf, ppvOut, punkOuter);
    }
}

