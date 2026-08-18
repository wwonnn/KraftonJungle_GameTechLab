#ifdef IS_OBJ_VIEWER
#include "Launch/ViewerEngineLoop.h"
#else
#include "Launch/EditorEngineLoop.h"
#endif

namespace 
{
    int GuardedMain(HINSTANCE HInstance, int NCmdShow)
    {
#ifdef IS_OBJ_VIEWER
        FViewerEngineLoop EngineLoop;
#else
        FEditorEngineLoop EngineLoop;
#endif  
        
        if (!EngineLoop.PreInit(HInstance, NCmdShow))
        {
            //  Error Code
            return -1;
        }
        
        const int32 ExitCode = EngineLoop.Run();
        
        EngineLoop.ShutDown();
        
        return ExitCode;
    }
}

int Launch(HINSTANCE HInstance, int NCmdShow)
{
    return GuardedMain(HInstance, NCmdShow);
}