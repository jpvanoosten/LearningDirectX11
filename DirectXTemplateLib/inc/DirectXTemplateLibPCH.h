// System includes
#include <windows.h>
// Windows Runtime Template Library
#include <wrl.h>

// DirectX includes
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

// Declare the XM_CALLCONV macro if we are using an old version of the DirectX Math library.
// For more information about DirecX Math Library internals, see http://msdn.microsoft.com/en-us/library/windows/desktop/ee418728(v=vs.85).aspx
#if (DIRECTXMATH_VERSION < 305) && !defined(XM_CALLCONV)
#define XM_CALLCONV __fastcall
typedef const DirectX::XMVECTOR& HXMVECTOR;
typedef const DirectX::XMMATRIX& FXMMATRIX;
#endif

// STL includes
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

// Link library dependencies
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
