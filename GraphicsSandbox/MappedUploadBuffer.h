#pragma once


#include "GraphicsDevice.h"
#include "d3dx12.h"


enum class BufferMode {
	BUFFER,
	CONSTANT_BUFFER
};


template <BufferMode Mode, typename ElementType>
class MappedUploadBuffer {
public:
	MappedUploadBuffer(GraphicsDevice& device, UINT64 elements) {
		D3D_CHECK(device.GetD3dDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(elements * ELEMENT_SIZE_IN_BYTES),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)
		));

		D3D_CHECK(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
	}

	~MappedUploadBuffer() {
		mUploadBuffer->Unmap(0, nullptr);
	}

	MappedUploadBuffer(const MappedUploadBuffer&) = delete;
	MappedUploadBuffer& operator = (const MappedUploadBuffer&) = delete;

	ID3D12Resource* GetResource() const {
		return mUploadBuffer.Get();
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(UINT index) const {
		// TODO: assert for index out of bounds
		// Issue #17

		return mUploadBuffer->GetGPUVirtualAddress() + UINT64(index) * ELEMENT_SIZE_IN_BYTES;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const {
		return GetGpuVirtualAddress(0);
	}

	void UploadValue(UINT index, const ElementType& value) {
		// TODO: assert for index out of bounds
		// Issue #17

		std::memcpy(&mMappedData[index * ELEMENT_SIZE_IN_BYTES], &value, sizeof(ElementType));
	}

private:
	static constexpr UINT CalcConstantBufferByteSize(UINT effectiveSizeBytes) {
		// Constant buffers must be a multiple of the minimum hardware
		// allocation size (usually 256 bytes).

		return (effectiveSizeBytes + 255) & ~255;
	}

public:
	static constexpr UINT ELEMENT_SIZE_IN_BYTES =
		(Mode == BufferMode::CONSTANT_BUFFER) ? CalcConstantBufferByteSize(sizeof(ElementType)) : sizeof(ElementType);

private:
	ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData;
};
