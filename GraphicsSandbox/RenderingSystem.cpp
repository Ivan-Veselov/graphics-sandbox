#include "RenderingSystem.h"
#include "d3dx12.h"


struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};


RenderingSystem::RenderingSystem(HWND hWnd, UINT width, UINT height)
: mFence(mDevice), mConstantBuffer(mDevice, NUMBER_OF_OBJECTS) {
    UINT dxgiFactoryFlags = 0;

	#if	defined(_DEBUG)
	{
        // Enable additional debug layers.
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	#endif

    ComPtr<IDXGIFactory7> factory;
    D3D_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        D3D_CHECK(mDevice.GetD3dDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
    }

    D3D_CHECK(mDevice.GetD3dDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mDirectCmdListAlloc)));

    D3D_CHECK(mDevice.GetD3dDevice()->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(&mCommandList)
    ));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    D3D_CHECK(mCommandList->Close());

    {
        mViewport.TopLeftX = 0.0f;
        mViewport.TopLeftY = 0.0f;
        mViewport.Width = static_cast<float>(width);
        mViewport.Height = static_cast<float>(height);
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;
        
        mScissorRect.left = 0;
        mScissorRect.top = 0;
        mScissorRect.right = static_cast<LONG>(width);
        mScissorRect.bottom = static_cast<LONG>(height);
    }

    DXGI_SAMPLE_DESC sampleDesc;
    sampleDesc.Count = 1;
    sampleDesc.Quality = 0;

    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = BACK_BUFFER_FORMAT;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc = sampleDesc;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = SWAP_CHAIN_BUFFERS_COUNT;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = 0;

        ComPtr<IDXGISwapChain1> swapChain;
        D3D_CHECK(factory->CreateSwapChainForHwnd(
            mCommandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain
        ));

        // This sample does not support fullscreen transitions.
        D3D_CHECK(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

        D3D_CHECK(swapChain.As(&mSwapChain));

        mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
    }

    {
        D3D12_RESOURCE_DESC	depthStencilDesc;
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        depthStencilDesc.Width = width;
        depthStencilDesc.Height = height;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = DEPTH_STENCIL_FORMAT;
        depthStencilDesc.SampleDesc = sampleDesc;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE optClear;
        optClear.Format = DEPTH_STENCIL_FORMAT;
        optClear.DepthStencil.Depth = 1.0f;
        optClear.DepthStencil.Stencil = 0;

        D3D_CHECK(mDevice.GetD3dDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optClear,
            IID_PPV_ARGS(&mDepthStencilBuffer)
        ));
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFERS_COUNT;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.NodeMask = 0;
        D3D_CHECK(mDevice.GetD3dDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = DEPTH_STENCIL_BUFFERS_COUNT;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;
        D3D_CHECK(mDevice.GetD3dDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));

        D3D12_DESCRIPTOR_HEAP_DESC cvbSrvUavHeapDesc;
        cvbSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cvbSrvUavHeapDesc.NumDescriptors = NUMBER_OF_OBJECTS;
        cvbSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        cvbSrvUavHeapDesc.NodeMask = 0;
        D3D_CHECK(mDevice.GetD3dDevice()->CreateDescriptorHeap(&cvbSrvUavHeapDesc, IID_PPV_ARGS(&mCbvSrvUavHeap)));

        mRtvDescriptorSize = mDevice.GetD3dDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        mDsvDescriptorSize = mDevice.GetD3dDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        mCbvSrvUavDescriptorSize = mDevice.GetD3dDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvUavHeapHandle(mCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart());

        for (UINT i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; i++) {
            D3D_CHECK(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
            mDevice.GetD3dDevice()->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
            rtvHeapHandle.Offset(1,	mRtvDescriptorSize);
        }

        mDevice.GetD3dDevice()->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, dsvHeapHandle);

        for (UINT i = 0; i < NUMBER_OF_OBJECTS; i++) {
            D3D12_CONSTANT_BUFFER_VIEW_DESC	cbvDesc;
            cbvDesc.BufferLocation = mConstantBuffer.GetGpuVirtualAddress(i);
            cbvDesc.SizeInBytes = decltype(mConstantBuffer)::ELEMENT_SIZE_IN_BYTES;

            mDevice.GetD3dDevice()->CreateConstantBufferView(&cbvDesc, cbvSrvUavHeapHandle);
            cbvSrvUavHeapHandle.Offset(1, mCbvSrvUavDescriptorSize);
        }
    }

    UploadResources();
}


RenderingSystem::~RenderingSystem() {
    // Redundant call
    // FlushCommandQueue();
}


void RenderingSystem::RenderFrame() {
    ObjectConstants constants;
    std::memset(&constants, 0, sizeof(constants));
    constants.WorldViewProj(0, 0) = 1.0f;
    constants.WorldViewProj(1, 1) = 1.0f;
    constants.WorldViewProj(2, 2) = 1.0f;
    constants.WorldViewProj(3, 3) = 1.0f;
    mConstantBuffer.UploadValue(0, constants);

    D3D_CHECK(mDirectCmdListAlloc->Reset());
    D3D_CHECK(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        mSwapChainBuffer[mCurrentBackBufferIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrentBackBufferIndex,
        mRtvDescriptorSize
    );

    const float clearColor[] = { 0.0f, 0.4f, 0.2f, 1.0f };
    mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        mSwapChainBuffer[mCurrentBackBufferIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    ));

    D3D_CHECK(mCommandList->Close());

    ID3D12CommandList *ppCommandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    D3D_CHECK(mSwapChain->Present(1, 0));

    FlushCommandQueue();
    mCurrentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
}


void RenderingSystem::UploadResources() {
    /*{ { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
    { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
    { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }*/

    std::vector<Vertex> vertices = {
        {XMFLOAT3(0.0f, 0.25f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(0.25f, -0.25f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
        {XMFLOAT3(-0.25f, -0.25f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)}
    };

    ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;

    D3D_CHECK(mDirectCmdListAlloc->Reset());
    D3D_CHECK(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mDevice.RecordCommandsToCreateDefaultBuffer(
        mCommandList.Get(),
        vertices.data(),
        UINT64(vertices.size()) * sizeof(decltype(vertices)::value_type),
        mVertexBuffer,
        VertexBufferUploader
    );

    D3D_CHECK(mCommandList->Close());

    ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    FlushCommandQueue();
}


void RenderingSystem::FlushCommandQueue() {
    auto label = mFence.PutLabel(mCommandQueue.Get());
    mFence.WaitForLabel(label);
}
