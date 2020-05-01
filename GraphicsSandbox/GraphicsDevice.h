#pragma once


#include "D3dCommon.h"


class GraphicsDevice {
public:
	GraphicsDevice();
	GraphicsDevice(const GraphicsDevice&) = delete;

	GraphicsDevice& operator = (const GraphicsDevice&) = delete;

	ComPtr<ID3D12Device> GetD3dDevice() {
		return mD3dDevice;
	}

private:
	ComPtr<ID3D12Device> mD3dDevice;
};


// This class is not thread-safe
class WaitableGpuFence {
	friend GraphicsDevice;

public:
	class Label {
		friend WaitableGpuFence;

	private:
		UINT64 mLabelValue;

	#if defined(_DEBUG)
		WaitableGpuFence *mOwner;
	#endif
	};

public:
	WaitableGpuFence(GraphicsDevice& device);
	WaitableGpuFence(const WaitableGpuFence&) = delete;
	~WaitableGpuFence();

	WaitableGpuFence& operator = (const WaitableGpuFence&) = delete;

	Label PutLabel(ID3D12CommandQueue *commandQueue);
	void WaitForLabel(const Label &label);

private:
	ComPtr<ID3D12Fence> mFence;
	UINT64 mNextValue = 0;
	HANDLE mEvent;
};
