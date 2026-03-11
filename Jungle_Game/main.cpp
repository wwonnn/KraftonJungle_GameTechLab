#define _CRTDBG_MAP_ALLOC

#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "GameApp.h"
#include <crtdbg.h>

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    GameApp* gameApp = new GameApp();
    if (!gameApp->Initialize(hInstance))
    {
        return 0;
    }

    gameApp->Run();
    gameApp->Finalize();
    delete gameApp;
    return 0;
}
