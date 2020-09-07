#include <TextureAndLightingPCH.h>
#include <Application.h>
#include <Window.h>

#include <TextureAndLightingDemo.h>

const char* g_WindowName = "Texture and Lighting Demo";
int g_WindowWidth = 800;
int g_WindowHeight = 600;
bool g_VSync = false;
bool g_Windowed = true;

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow )
{
    UNREFERENCED_PARAMETER( prevInstance );
    UNREFERENCED_PARAMETER( cmdLine );

    Application::Create(hInstance);
    Application& app = Application::Get();

    Window& window = app.CreateRenderWindow( g_WindowName, g_WindowWidth, g_WindowHeight, g_VSync, g_Windowed );

    TextureAndLightingDemo* pDemo = new TextureAndLightingDemo(window);

    if ( !pDemo->Initialize() )
    {
        return -1;
    }

    if ( !pDemo->LoadContent() )
    {
        return -1;
    }

    int exitCode = app.Run();

    pDemo->UnloadContent();
    pDemo->Cleanup();

    delete pDemo;

    return exitCode;
}