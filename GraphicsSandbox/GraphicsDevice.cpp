#include "GraphicsDevice.h"
#include "d3dx12.h"


GraphicsDevice::GraphicsDevice() {
	#if	defined(_DEBUG)
	{
		// Enable the D3D12 debug layer.
		ComPtr<ID3D12Debug>	debugController;
		D3D_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
	#endif

	// Use the default adapter
	D3D_CHECK(
		D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mD3dDevice))
	);
}


// Note: uploadBuffer has to be kept alive after the above function
// calls because the command list has not been executed yet that
// performs the actual copy.
void GraphicsDevice::RecordCommandsToCreateDefaultBuffer(
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	LONG_PTR byteSize,
	ComPtr<ID3D12Resource>& defaultBuffer,
	ComPtr<ID3D12Resource>& uploadBuffer
) {
	D3D_CHECK(mD3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)
	));

	D3D_CHECK(mD3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)
	));

	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = byteSize;

	// Schedule to copy the data to the default buffer resource.
	// At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.
	// Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.

	UpdateSubresources<1>(
		cmdList,
		defaultBuffer.Get(),
		uploadBuffer.Get(),
		0, 0, 1, &subResourceData
	);

	cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ
		)
	);
}


WaitableGpuFence::WaitableGpuFence(GraphicsDevice& device) {
	D3D_CHECK(device.GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	
	mEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	WINDOWS_CHECK(mEvent != nullptr);
}


WaitableGpuFence::~WaitableGpuFence() {
	// TODO: Returned value is not handled
	// Can't throw exceptions from destructor
	// Issue #16
	CloseHandle(mEvent);
}


WaitableGpuFence::Label WaitableGpuFence::PutLabel(ID3D12CommandQueue* commandQueue) {
	UINT64 labelValue = mNextValue++;
	D3D_CHECK(commandQueue->Signal(mFence.Get(), labelValue));

	Label label;
	label.mLabelValue = labelValue;

	#if defined(_DEBUG)
		label.mOwner = this;
	#endif

	return label;
}


void WaitableGpuFence::WaitForLabel(const Label& label) {
	#if defined(_DEBUG)
		if (label.mOwner != this) {
			// TODO: need ASSERT statement
			// Issue #17
		}
	#endif

	if (mFence->GetCompletedValue() < label.mLabelValue) {
		D3D_CHECK(mFence->SetEventOnCompletion(label.mLabelValue, mEvent));
		WINDOWS_CHECK(WaitForSingleObject(mEvent, INFINITE) != WAIT_FAILED);
	}
}
