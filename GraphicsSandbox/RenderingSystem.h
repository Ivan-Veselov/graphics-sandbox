#pragma once


#include "D3dCommon.h"
#include "GraphicsDevice.h"
#include "MappedUploadBuffer.h"


using DirectX::XMFLOAT4;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4X4;


struct ObjectConstants {
    XMFLOAT4X4 WorldViewProj;
};


class RenderingSystem {
public:
	RenderingSystem(HWND hWnd, UINT width, UINT height);
	~RenderingSystem();

	void RenderFrame();

private:
    void UploadResources();
    void FlushCommandQueue();

private:
    static constexpr DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
    static constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    static constexpr UINT SWAP_CHAIN_BUFFERS_COUNT = 2;
    static constexpr UINT DEPTH_STENCIL_BUFFERS_COUNT = 1;
    static constexpr UINT NUMBER_OF_OBJECTS = 1;

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
    ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap;

    UINT mRtvDescriptorSize;
    UINT mDsvDescriptorSize;
    UINT mCbvSrvUavDescriptorSize;

    ComPtr<ID3D12Resource> mSwapChainBuffer[SWAP_CHAIN_BUFFERS_COUNT];
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    ComPtr<ID3D12Resource> mVertexBuffer;
    MappedUploadBuffer<BufferMode::CONSTANT_BUFFER, ObjectConstants> mConstantBuffer;
};
