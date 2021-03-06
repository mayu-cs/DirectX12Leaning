#pragma once
#include <wrl.h>

enum class SelectVSYNC {
	DisableVSYNC,
	EnableVSYNC
};

class DirectX12
{
private:
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public: //Public function
	//Initialize
	DirectX12(HWND hwnd, const int window_width, const int window_height, SelectVSYNC vsync = SelectVSYNC::EnableVSYNC);
	~DirectX12();

	void Initialize_components();

	//color getter
	DirectX::XMFLOAT4 GetColor(const float R, const float G, const float B, const float A);

	ID3D12Device *GetDevice();
	ID3D12GraphicsCommandList *GetCommandList();

	//Draw function
	void ClearDrawScreen(const DirectX::XMFLOAT4 color);
	void ScreenFlip();

private:
	ComPtr<ID3D12Device> dev;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	//ID3D12DescriptorHeap *dsvHeap;

private:
	//GPU
	void D3D12ListUpGPU();
	void D3D12SelectGPU();
	HRESULT D3D12FeatureLv();

	//Commands
	HRESULT D3D12CreateCommandAllocator();
	HRESULT D3D12CreateCommandList();
	HRESULT D3D12CreateCommandQueueDescription();

	//Swapchain
	void D3D12SetSwapchainDescription();

	//Heap
	void D3D12SetDescripterHeap();

	//Target veiw
	void D3D12SetTargetView();

	//Fence
	void D3D12CreateFence();

	//Draw
	void RestoreResourceBarrierSetting();
	void SetScissorrect();
	void SetViewport();

private:
	HRESULT result;
	HWND hwnd;
	int window_width;
	int window_height;
	int VSYNCMode;

	//GPU
	std::vector<ComPtr<IDXGIAdapter1>> adapters;
	IDXGIAdapter1 *tmpAdapter;
	ComPtr<IDXGIFactory6> dxgiFactory;
	D3D_FEATURE_LEVEL featurelevel;
	D3D_FEATURE_LEVEL levels[4] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	//Commands
	ID3D12CommandAllocator *cmdAllocator;
	ID3D12CommandQueue *cmdQueue;
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc;

	//Swapchain
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
	IDXGISwapChain4 *swapchain;

	//Heap
	ID3D12DescriptorHeap *rtvHeaps = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;

	//Target view
	std::vector<ComPtr<ID3D12Resource>> backBuffers = std::vector<ComPtr<ID3D12Resource>>(2);

	//Fence
	ID3D12Fence *fence;
	UINT64 fenceVal;

	//Draw
	D3D12_RESOURCE_BARRIER barrierDesc;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorrect;
};

