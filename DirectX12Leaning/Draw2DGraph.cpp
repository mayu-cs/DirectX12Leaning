//API
#include <d3d12.h>
#include <DirectXMath.h>

//shader(HLSL)
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

//STL
#include <iostream>
#include <vector>

//Utility
#include "tempUtility.h"

//this
#include "Draw2DGraph.h"

Draw2DGraph::Draw2DGraph() {}
Draw2DGraph::Draw2DGraph(const wchar_t *fileName, const int fillMode, ID3D12Device *dev, ID3D12GraphicsCommandList *cmdList, const int window_width, const int window_height) :
	dev(dev),
	cmdList(cmdList),
	window_width(window_width),
	window_height(window_height)
{
	matrix = DirectX::XMMatrixIdentity();
	matProjection = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, (float)window_width, (float)window_height, 0.0f, 0.0f, 1.0f);

	SetVertices();
	SetHeapProperty();
	SetResourceDescription();
	CreateVertexBuffer();
	GetVertexMapVirtualMemory();
	SetVertexBufferView();
	SetIndices();
	SetIndexBuffer();
	GetIndexMapVirtualMemory();
	SetIndexBufferView();
	SetShader();
	SetConstantBufferHeapProperty();
	SetConstantBufferResourceDescription();
	SetDescripterHeap();
	CreateConstantBuffer();
	CreateTextureData(fileName);

	//Top layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	SetGraphicsPipeLine(fillMode);
	SetRenderTargetBlendDescription();

	//top layout setting
	gpipeline.InputLayout.pInputElementDescs = inputLayout;
	gpipeline.InputLayout.NumElements = std::extent<decltype(inputLayout), 0>::value;
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpipeline.NumRenderTargets = 1;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpipeline.SampleDesc.Count = 1;

	SetRootParameter();
	SetRootSignature();
	SetSignature();
}

void Draw2DGraph::Update(float x, float y, float rotate)
{
	matrix = DirectX::XMMatrixIdentity();
	matrix *= DirectX::XMMatrixRotationZ(rotate);
	//matrix *= DirectX::XMMatrixTranslation(x, y, 0.f);
	matrix *= matProjection;
}

void Draw2DGraph::execute(const DirectX::XMFLOAT4 color, const float adjustXPos, const float adjustYPos)
{
	//Get VirtualMemory
	Graph2DVertex *vertMap = nullptr;
	result = verBuff->Map(0, nullptr, (void **)&vertMap);

	vertices = {
		{{ -1.f, -1.f, 0.0f},{0.0f, 1.0f}},
		{{ -1.f, +1.f, 0.0f},{0.0f, 0.0f}},
		{{ +1.f, -1.f, 0.0f},{1.0f, 1.0f}},
		{{ +1.f, +1.f, 0.0f},{1.0f, 0.0f}}
	};

	for (auto &x : vertices) {
		x.pos.x += adjustXPos;
	}

	assert(result == S_OK);

	//Update point
	std::copy(vertices.begin(), vertices.end(), vertMap);
	verBuff->Unmap(0, nullptr);

	//Initialize const buffer
	ConstBufferData3D *constMap = nullptr;
	result = constBuff->Map(0, nullptr, (void **)&constMap);
	constMap->color = color;
	constMap->mat = matrix;
	constBuff->Unmap(0, nullptr);
	assert(result == S_OK);

	//pipeline
	cmdList->SetPipelineState(pipelinestate);
	cmdList->SetGraphicsRootSignature(rootsignature);

	//Set Descriptor heap
	ID3D12DescriptorHeap *ppHeaps[] = { basicDescHeap };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	//Set constant buffer view
	//Set constant buffer view
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle = basicDescHeap->GetGPUDescriptorHandleForHeapStart();
	cmdList->SetGraphicsRootDescriptorTable(0, gpuDescHandle);
	gpuDescHandle.ptr += this->dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cmdList->SetGraphicsRootDescriptorTable(1, gpuDescHandle);

	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);
	cmdList->DrawIndexedInstanced((int)indices.size(), 1, 0, 0, 0);
}

void Draw2DGraph::SetVertices()
{
	vertices = std::vector<Graph2DVertex>(4);
	vertices = {
		{{ -1.f, -1.f, 0.0f},{0.0f, 1.0f}},
		{{ -1.f, +1.f, 0.0f},{0.0f, 0.0f}},
		{{ +1.f, -1.f, 0.0f},{1.0f, 1.0f}},
		{{ +1.f, +1.f, 0.0f},{1.0f, 0.0f}}
	};
	sizeVB = static_cast<UINT>(sizeof(Graph2DVertex) * vertices.size());
}

void Draw2DGraph::SetHeapProperty()
{
	//Set vertex buffer
	heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;	//Transfer to GPU
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
}

void Draw2DGraph::SetResourceDescription()
{
	//Resource setting
	resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeVB;
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
}

void Draw2DGraph::CreateVertexBuffer()
{
	//Create top buffer
	verBuff = nullptr;
	result = this->dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&verBuff)
	);
	assert(result == S_OK);
}

void Draw2DGraph::SetVertexBufferView()
{
	//Create vertex buffer view
	vbView = {};
	vbView.BufferLocation = verBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(Graph2DVertex) * vertices.size();
	vbView.StrideInBytes = sizeof(Graph2DVertex);
}

void Draw2DGraph::SetIndices()
{
	indices = std::vector<unsigned short>(6);
	indices = {
		0, 1, 2,
		1, 2, 3
	};
}

void Draw2DGraph::SetIndexBuffer()
{
	//Add index buffer
	indexBuff = nullptr;
	resdesc.Width = sizeof(unsigned short) * indices.size();
	result = this->dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff)
	);
	assert(result == S_OK);
}

void Draw2DGraph::GetIndexMapVirtualMemory()
{
	//Get virtual memory
	indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void **)&indexMap);
	assert(result == S_OK);
	for (int i = 0; i < indices.size(); ++i) {
		indexMap[i] = indices[i];
	}
	indexBuff->Unmap(0, nullptr);
}

void Draw2DGraph::GetVertexMapVirtualMemory()
{
	//Get VirtualMemory
	vertMap = nullptr;
	result = verBuff->Map(0, nullptr, (void **)&vertMap);
	assert(result == S_OK);

	std::copy(vertices.begin(), vertices.end(), vertMap);
	verBuff->Unmap(0, nullptr);
}

void Draw2DGraph::SetIndexBufferView()
{
	//Create index buffer view
	ibView = {};
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(unsigned short) * indices.size();
}

void Draw2DGraph::SetShader()
{
	//shader
	vsBlob = nullptr;
	psBlob = nullptr;
	errorBlob = nullptr;

	result = D3DCompileFromFile(
		L"Shaders/Graph2DVS.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&vsBlob, &errorBlob
	);
	assert(result == S_OK);

	result = D3DCompileFromFile(
		L"Shaders/Graph2DPS.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&psBlob, &errorBlob
	);
	assert(result == S_OK);
}

void Draw2DGraph::SetConstantBufferHeapProperty()
{
	//Heap
	cbheapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
}

void Draw2DGraph::SetConstantBufferResourceDescription()
{
	//Resources
	cbresdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbresdesc.Width = (sizeof(ConstBufferData3D) + 0xff) & ~0xff;
	cbresdesc.Height = 1;
	cbresdesc.DepthOrArraySize = 1;
	cbresdesc.MipLevels = 1;
	cbresdesc.SampleDesc.Count = 1;
	cbresdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//Create
	constBuff = nullptr;
	result = this->dev->CreateCommittedResource(
		&cbheapprop,
		D3D12_HEAP_FLAG_NONE,
		&cbresdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);
	assert(result == S_OK);
}

void Draw2DGraph::SetDescripterHeap()
{
	//Descripter heap
	basicDescHeap = nullptr;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = 1;

	result = this->dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));
	assert(result == S_OK);

	basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
}

void Draw2DGraph::CreateConstantBuffer()
{
	//Create const buffer
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (UINT)constBuff->GetDesc().Width;
	this->dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
}

void Draw2DGraph::CreateTextureData(const wchar_t *fileName)
{
	result = LoadFromWICFile(
		fileName,
		DirectX::WIC_FLAGS_NONE,
		&metadata, scratchImg
	);
	assert(result == S_OK);

	const DirectX::Image *img = scratchImg.GetImage(0, 0, 0);

	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	texresDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	texresDesc.Format = metadata.format;
	texresDesc.Width = metadata.width;
	texresDesc.Height = (UINT)metadata.height;
	texresDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
	texresDesc.MipLevels = (UINT16)metadata.mipLevels;
	texresDesc.SampleDesc.Count = 1;

	texbuff = nullptr;
	result = this->dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&texresDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	result = texbuff->WriteToSubresource(
		0,
		nullptr,
		img->pixels,
		(UINT)img->rowPitch,
		(UINT)img->slicePitch
	);

	basicHeapHandle.ptr += this->dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	this->dev->CreateShaderResourceView(texbuff, &srvDesc, basicHeapHandle);
}

void Draw2DGraph::SetGraphicsPipeLine(const int fillMode)
{
	//Graphics pipeline
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = psBlob->GetBufferSize();

	//SampleMask
	//RasterizerState
	//Fill mode
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpipeline.RasterizerState.MultisampleEnable = false;
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpipeline.RasterizerState.FillMode = (D3D12_FILL_MODE)fillMode;
	gpipeline.RasterizerState.DepthClipEnable = true;
}

void Draw2DGraph::SetRenderTargetBlendDescription()
{
	//Blend
	D3D12_RENDER_TARGET_BLEND_DESC &blenddesc = gpipeline.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blenddesc.BlendEnable = true;
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	blenddesc.DestBlendAlpha = D3D12_BLEND_ONE;

	//Alpha blend
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
}

void Draw2DGraph::SetRootParameter()
{
	////root parameter
	//descRangeCBV.NumDescriptors = 1;
	//descRangeCBV.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	//descRangeCBV.BaseShaderRegister = 0;
	//descRangeCBV.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	////setting root parameter
	//rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//rootparam.DescriptorTable.pDescriptorRanges = &descRangeCBV;
	//rootparam.DescriptorTable.NumDescriptorRanges = 1;
	//rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//root parameter
	D3D12_DESCRIPTOR_RANGE descRangeCBV{};
	descRangeCBV.NumDescriptors = 1;
	descRangeCBV.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descRangeCBV.BaseShaderRegister = 0;
	descRangeCBV.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descRangeSRV{};
	descRangeSRV.NumDescriptors = 1;
	descRangeSRV.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRangeSRV.BaseShaderRegister = 0;
	descRangeSRV.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//setting root parameter
	D3D12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descRangeCBV;
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descRangeSRV;
	rootparam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC sampleDesc{};
	sampleDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampleDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampleDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampleDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampleDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampleDesc.MaxLOD = D3D12_FLOAT32_MAX;
	sampleDesc.MinLOD = 0.0f;
	sampleDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampleDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	//root signeture
	rootsignature = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootparam;
	rootSignatureDesc.NumParameters = _countof(rootparam);
	rootSignatureDesc.pStaticSamplers = &sampleDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	assert(result == S_OK);

	result = this->dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	assert(result == S_OK);

	rootSigBlob->Release();
}

void Draw2DGraph::SetRootSignature()
{
	////root signeture
	//rootsignature = nullptr;
	//rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	//rootSignatureDesc.pParameters = &rootparam;
	//rootSignatureDesc.NumParameters = 1;

	//rootSigBlob = nullptr;
	//result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	//assert(result == S_OK);

	//result = this->dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	//assert(result == S_OK);

	//rootSigBlob->Release();
}

void Draw2DGraph::SetSignature()
{
	//set signature
	gpipeline.pRootSignature = rootsignature;

	pipelinestate = nullptr;
	result = this->dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelinestate));
	assert(result == S_OK);
}