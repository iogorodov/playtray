// radiotray.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
    UNREFERENCED_PARAMETER(prevInstance);
    UNREFERENCED_PARAMETER(cmdLine);
    UNREFERENCED_PARAMETER(cmdShow);

    App app(instance);
    return app.Run();
}
