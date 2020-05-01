#include "GraphicsDevice.h"


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


WaitableGpuFence::Label WaitableGpuFence::PutLabel(ID3D12CommandQueue *commandQueue) {
	UINT64 labelValue = mNextValue++;
	D3D_CHECK(commandQueue->Signal(mFence.Get(), labelValue));

	Label label;
	label.mLabelValue = labelValue;

	#if defined(_DEBUG)
		label.mOwner = this;
	#endif

	return label;
}


void WaitableGpuFence::WaitForLabel(const Label &label) {
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
