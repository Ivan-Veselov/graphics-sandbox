#pragma once


#include "D3dCommon.h"
#include "GraphicsDevice.h"


class RenderingSystem {
public:
	RenderingSystem(HWND hWnd, UINT width, UINT height);
	~RenderingSystem();

	void RenderFrame();

private:
    void FlushCommandQueue();

private:
    static constexpr DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
    static constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    static constexpr UINT SWAP_CHAIN_BUFFERS_COUNT = 2;
    static constexpr UINT DEPTH_STENCIL_BUFFERS_COUNT = 1;

    GraphicsDevice mDevice;

    WaitableGpuFence mFence;

    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;

    ComPtr<IDXGISwapChain4> mSwapChain;
    UINT mCurrentBackBufferIndex;

    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    UINT mRtvDescriptorSize;
    UINT mDsvDescriptorSize;
    UINT mCbvSrvUavDescriptorSize;

    ComPtr<ID3D12Resource> mSwapChainBuffer[SWAP_CHAIN_BUFFERS_COUNT];
    ComPtr<ID3D12Resource> mDepthStencilBuffer;
};
