#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "GameApp.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
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
