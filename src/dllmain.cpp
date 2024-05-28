#include "pch.h"
#include "dinput8.h"
#include "utils.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        try
        {
            load_id5Tweaker();
        }
        catch (std::runtime_error e)
        {
            MessageBox(NULL, std::format("{}", e.what()).c_str(), "ERROR", MB_OK);
            std::exit(-1);
        }
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
