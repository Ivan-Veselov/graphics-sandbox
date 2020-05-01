#pragma once


#include "WindowsCommon.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>


using Microsoft::WRL::ComPtr;


class D3dException : public WindowsException {
public:
    D3dException(HRESULT hr, const std::string& fileName, int lineNumber)
    : WindowsException(hr, fileName, lineNumber) {
    }
};


#define D3D_CHECK(expression) \
{\
    HRESULT	__hr__ = (expression);\
    if (FAILED(__hr__)) {\
        throw D3dException(__hr__, __FILE__, __LINE__);\
    }\
}
