/* 
Basic Direct3D12 Program in C.
Authour : James Henry-Moore
This Source Code Form is subject to the terms of the MIT License.
If a copy of the MIT License was not distributed with this file,
*/
#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>

#include<d3dcompiler.h>
#ifdef _DEBUG
#include<stdio.h>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxguid.lib")


/* Create a structure for a basic 3D Vector */
typedef struct 
{
	float x, y, z;
} XMFLOAT3;

void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	//printf(format, valist);
	va_end(valist);
#endif
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

/* Declare the device contexts */
IDXGIFactory6* _dxgiFactory = NULL;
ID3D12Device* _dev = NULL;
ID3D12CommandAllocator* _cmdAllocator = NULL;
ID3D12GraphicsCommandList* _cmdList = NULL;
ID3D12CommandQueue* _cmdQueue = NULL;
IDXGISwapChain4* _swapchain = NULL;

void EnableDebugLayer() {
	ID3D12Debug* debugLayer = NULL;
	if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug, &debugLayer))) {
		ID3D12Debug_EnableDebugLayer(debugLayer);
		ID3D12Debug_Release(debugLayer);
	}
}

/* Boilerplate Windows Main Loop*/
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow) {
	DebugOutputFormatString("Show window test.");
	HINSTANCE hInst = GetModuleHandle(NULL);
	WNDCLASSEX w;
	ZeroMemory(&w, sizeof(WNDCLASSEX));
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;
	w.lpszClassName = _T("DirectXTest");
	w.hInstance = GetModuleHandle(0);
	w.style = CS_HREDRAW | CS_VREDRAW;
	w.hCursor = LoadCursor(NULL, IDC_ARROW);
	w.hbrBackground = (HBRUSH)COLOR_WINDOW;
	w.lpszClassName = L"WindowClass";

	RegisterClassEx(&w);

	RECT wrc = { 0,0, window_width, window_height };
	AdjustWindowRectEx(&wrc, WS_BORDER, FALSE, WS_EX_APPWINDOW);
	
	HWND hwnd;
	hwnd = CreateWindowEx(NULL,
		L"WindowClass",
		L"Our First Direct3D Program",
		WS_OVERLAPPEDWINDOW,
		300,
		300,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		NULL,
		NULL,
		hInstance,
		NULL);


#ifdef _DEBUG
	EnableDebugLayer();
#endif
	//DirectX12 Feature Level enumeration
	D3D_FEATURE_LEVEL levels[4] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	HRESULT result = CreateDXGIFactory1(&IID_IDXGIFactory6, &_dxgiFactory);
	IDXGIAdapter** adapters = NULL;
	IDXGIAdapter* tmpAdapter = NULL;
	int adapterCount = 0;
	/* Count the number of available adapters. I know this is a waste of some cycles as we are not storing any adapters */
	for (int i = 0; IDXGIFactory6_EnumAdapters(_dxgiFactory,i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapterCount = i;
	}
	
	/* Store adapters in a list */
	adapters = malloc(sizeof(IDXGIAdapter*) * adapterCount);
	for(int i =0; i < adapterCount; ++i) {
		IDXGIFactory6_EnumAdapters(_dxgiFactory, i, &tmpAdapter);
		adapters[i] = tmpAdapter;
	}
	for (int i = 0; i < adapterCount; i++) {
		DXGI_ADAPTER_DESC adesc;
		IDXGIAdapter_GetDesc(adapters[i], &adesc);
		WCHAR strDesc[128] = { adesc.Description };
		if (wcsstr(strDesc, L"NVIDIA") != -1) {
			tmpAdapter = adapters[i];
			break;
		}
	}
	free(adapters);

	D3D_FEATURE_LEVEL featureLevel;
	for (int i = 0; i < 4; i++) {
		if (D3D12CreateDevice(tmpAdapter, levels[i], &IID_ID3D12Device, &_dev) == S_OK) {
			featureLevel = levels[i];
			break;
		}
	}

	result = ID3D12Device_CreateCommandAllocator(_dev, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, &_cmdAllocator);
	result = ID3D12Device_CreateCommandList(_dev, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, NULL, &IID_ID3D12CommandList, &_cmdList);
	//ID3D12GraphicsCommandList_Close();
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = ID3D12Device_CreateCommandQueue(_dev,&cmdQueueDesc, &IID_ID3D12CommandQueue, &_cmdQueue);

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = FALSE;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	
	result = IDXGIFactory6_CreateSwapChainForHwnd(
		_dxgiFactory,
		_cmdQueue,
		hwnd,
		&swapchainDesc,
		NULL,
		NULL,
		(IDXGISwapChain1**)&_swapchain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ID3D12DescriptorHeap* rtvHeaps = NULL;
	result = ID3D12Device_CreateDescriptorHeap(_dev,&heapDesc, &IID_ID3D12DescriptorHeap, &rtvHeaps);
	DXGI_SWAP_CHAIN_DESC swcDesc;
	result = IDXGISwapChain4_GetDesc(_swapchain, &swcDesc);
	ID3D12Resource** _backBuffers = malloc(sizeof(ID3D12Resource*)*swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	handle = *(ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(rtvHeaps, &handle));
	
	for (UINT i = 0; i < swcDesc.BufferCount; ++i) {
		//ID3D12Resource* backBuffer = NULL;
		result = IDXGISwapChain4_GetBuffer(_swapchain, i, &IID_ID3D12Resource, &_backBuffers[i]);
		ID3D12Device_CreateRenderTargetView(_dev, _backBuffers[i], NULL, handle);
		handle.ptr += ID3D12Device_GetDescriptorHandleIncrementSize(_dev, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	ID3D12Fence* _fence = NULL;
	UINT64 _fenceVal = 0;
	result = ID3D12Device_CreateFence(_dev, _fenceVal, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, &_fence);

	ShowWindow(hwnd, SW_SHOW);

	/* Create a square using vertices */
	XMFLOAT3 vertices[] = {
		{-0.4f,-0.7f,0.0f} ,
		{-0.4f,0.7f,0.0f} ,
		{0.4f,-0.7f,0.0f} ,
		{0.4f,0.7f,0.0f}
	};

	D3D12_HEAP_PROPERTIES heapprop = {0};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	//heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//heapprop.CreationNodeMask = 0;
	//heapprop.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resdesc = {0};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


	ID3D12Resource* vertBuff = NULL;
	result = ID3D12Device_CreateCommittedResource(
		_dev,
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		&IID_ID3D12Resource,
		&vertBuff);

	XMFLOAT3* vertMap = NULL;
	result = ID3D12Resource_Map(vertBuff, 0, NULL, &vertMap);

	memcpy(vertMap, vertices, sizeof(*vertices) * 4);

	ID3D12Resource_Unmap(vertBuff, 0, NULL);

	D3D12_VERTEX_BUFFER_VIEW vbView = {0};
	vbView.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(vertBuff);
	vbView.SizeInBytes = sizeof(vertices);
	vbView.StrideInBytes = sizeof(vertices[0]);

	unsigned short indices[] = { 0,1,2, 2,1,3 };

	ID3D12Resource* idxBuff = NULL;


	resdesc.Width = sizeof(indices);
	result = ID3D12Device_CreateCommittedResource(
		_dev,
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		&IID_ID3D12Resource, 
		&idxBuff);

	/* Copy index buffer to mapped buffer */
	unsigned short* mappedIdx = NULL;
	ID3D12Resource_Map(idxBuff, 0, NULL, &mappedIdx);
	memcpy(mappedIdx, indices, sizeof(*indices) * 6);
	ID3D12Resource_Unmap(idxBuff, 0, NULL);


	D3D12_INDEX_BUFFER_VIEW ibView;
	ibView.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(idxBuff);
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);



	ID3DBlob* _vsBlob = { 0 };
	ID3DBlob* _psBlob = { 0 };

	ID3DBlob* errorBlob = { 0 };
	result = D3DCompileFromFile(L"BasicVertexShader.hlsl",
		NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_vsBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			OutputDebugStringA("VertexShader not found");
		}
		else {
			char* errstr = malloc(sizeof(char)*ID3D10Blob_GetBufferSize(errorBlob));
			memcpy(errstr, (char*)ID3D10Blob_GetBufferPointer(errorBlob), ID3D10Blob_GetBufferSize(errorBlob));
			errstr += '\n';
			OutputDebugStringA(errstr);
			free(errstr);
		}
		exit(1);
	}
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",
		NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			OutputDebugStringA("PixelShader not found");
		}
		else {
			char* errstr = malloc(sizeof(char)*ID3D10Blob_GetBufferSize(errorBlob));
			memcpy(errstr,(char*)ID3D10Blob_GetBufferPointer(errorBlob) , ID3D10Blob_GetBufferSize(errorBlob));
			errstr += '\n';
			OutputDebugStringA(errstr);
			free(errstr);
		}
		exit(1);
	}
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 2 * sizeof(float), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = { 0 };
	gpipeline.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(_vsBlob);
	gpipeline.VS.BytecodeLength = ID3D10Blob_GetBufferSize(_vsBlob);
	gpipeline.PS.pShaderBytecode =  ID3D10Blob_GetBufferPointer(_psBlob);
	gpipeline.PS.BytecodeLength = ID3D10Blob_GetBufferSize(_psBlob);

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//
	gpipeline.BlendState.AlphaToCoverageEnable = FALSE;
	gpipeline.BlendState.IndependentBlendEnable = FALSE;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = { 0 };

	renderTargetBlendDesc.BlendEnable = FALSE;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	renderTargetBlendDesc.LogicOpEnable = FALSE;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;


	gpipeline.RasterizerState.MultisampleEnable = FALSE;
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpipeline.RasterizerState.DepthClipEnable = TRUE;


	gpipeline.RasterizerState.FrontCounterClockwise = FALSE;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = FALSE;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


	gpipeline.DepthStencilState.DepthEnable = FALSE;
	gpipeline.DepthStencilState.StencilEnable = FALSE;

	gpipeline.InputLayout.pInputElementDescs = inputLayout;
	gpipeline.InputLayout.NumElements = _countof(inputLayout);

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeline.NumRenderTargets = 1;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	gpipeline.SampleDesc.Count = 1;
	gpipeline.SampleDesc.Quality = 0;

	ID3D12RootSignature* rootsignature = {0};

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = { 0 };
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* rootSigBlob = { 0 };
	
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, NULL);
	result = ID3D12Device_CreateRootSignature(_dev, 0, ID3D10Blob_GetBufferPointer(rootSigBlob), ID3D10Blob_GetBufferSize(rootSigBlob), &IID_ID3D12RootSignature, &rootsignature);

	gpipeline.pRootSignature = rootsignature;
	ID3D12PipelineState* _pipelinestate = { 0 };
	result = ID3D12Device_CreateGraphicsPipelineState(_dev, &gpipeline, &IID_ID3D12PipelineState, &_pipelinestate);
	ID3D10Blob_Release(rootSigBlob);


	D3D12_VIEWPORT viewport;
	viewport.Width = window_width;
	viewport.Height = window_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;


	D3D12_RECT scissorrect;
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + window_width;
	scissorrect.bottom = scissorrect.top + window_height;



	MSG msg ;
	unsigned int frame = 0;
	while (TRUE) {

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT) {
			break;
		}



		UINT backBufferIndex = IDXGISwapChain4_GetCurrentBackBufferIndex(_swapchain);

		D3D12_RESOURCE_BARRIER BarrierDesc;
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[backBufferIndex];
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		ID3D12GraphicsCommandList_ResourceBarrier(_cmdList, 1, &BarrierDesc);

		ID3D12GraphicsCommandList_SetPipelineState(_cmdList, _pipelinestate);



		D3D12_CPU_DESCRIPTOR_HANDLE rtvH;
		ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(rtvHeaps, &rtvH);
		rtvH.ptr += backBufferIndex * ID3D12Device_GetDescriptorHandleIncrementSize(_dev, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		ID3D12GraphicsCommandList_OMSetRenderTargets(_cmdList, 1, &rtvH, FALSE, NULL);

	

		float r, g, b;
		r = (float)(0xff & frame >> 16) / 255.0f;
		g = (float)(0xff & frame >> 8) / 255.0f;
		b = (float)(0xff & frame >> 0) / 255.0f;
		float clearColor[] = { r,g,b,1.0f };
		ID3D12GraphicsCommandList_ClearRenderTargetView(_cmdList, rtvH, clearColor, 0, NULL);
		++frame;
		ID3D12GraphicsCommandList_RSSetViewports(_cmdList, 1, &viewport);
		ID3D12GraphicsCommandList_RSSetScissorRects(_cmdList, 1, &scissorrect);
		ID3D12GraphicsCommandList_SetGraphicsRootSignature(_cmdList, rootsignature);

		ID3D12GraphicsCommandList_IASetPrimitiveTopology(_cmdList, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ID3D12GraphicsCommandList_IASetVertexBuffers(_cmdList, 0, 1, &vbView);
		ID3D12GraphicsCommandList_IASetIndexBuffer(_cmdList, &ibView);


		//ID3D12GraphicsCommandList_DrawInstanced(4, 1, 0, 0);
		ID3D12GraphicsCommandList_DrawIndexedInstanced(_cmdList, 6, 1, 0, 0, 0);

		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		ID3D12GraphicsCommandList_ResourceBarrier(_cmdList, 1, &BarrierDesc);

		ID3D12GraphicsCommandList_Close(_cmdList);



		
		ID3D12CommandList* cmdlists[] = { _cmdList };
		ID3D12CommandQueue_ExecuteCommandLists(_cmdQueue,1, cmdlists);

		ID3D12CommandQueue_Signal(_cmdQueue, _fence, ++_fenceVal);

		if (ID3D12Fence_GetCompletedValue(_fence) != _fenceVal) {
			HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);
			ID3D12Fence_SetEventOnCompletion(_fence, _fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		ID3D12CommandAllocator_Reset(_cmdAllocator);
		ID3D12GraphicsCommandList_Reset(_cmdList,_cmdAllocator, _pipelinestate);


		//Swap
		IDXGISwapChain4_Present(_swapchain, 1, 0);

	}
	if(_swapchain)
		IDXGISwapChain4_Release(_swapchain);
	if(_dxgiFactory)
		IDXGIFactory4_Release(_dxgiFactory);
	if(_cmdQueue)
		ID3D12CommandQueue_Release(_cmdQueue);
	if(_cmdAllocator)
		ID3D12CommandAllocator_Release(_cmdAllocator);
	if(_cmdList)
		ID3D12GraphicsCommandList_Release(_cmdList);
	if(_fence)
		ID3D12Fence_Release(_fence);
	if(_vsBlob)
		ID3D10Blob_Release(_vsBlob);
	if(_psBlob)
		ID3D10Blob_Release(_psBlob);
	if(rootsignature)
		ID3D12RootSignature_Release(rootsignature);
	if (idxBuff)
		ID3D12Resource_Release(idxBuff);
	if (errorBlob)
		ID3D10Blob_Release(errorBlob);
	if(_dev)
		ID3D12Device_Release(_dev);
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}
