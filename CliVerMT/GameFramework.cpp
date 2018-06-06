//-----------------------------------------------------------------------------
// File: CGameFramework.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "GameFramework.h"

extern SOCKET g_mysocket;
extern WSABUF	send_wsabuf;
extern char 	send_buffer[BUF_SIZE];
extern WSABUF	recv_wsabuf;
extern char	recv_buffer[BUF_SIZE];
extern char	packet_buffer[BUF_SIZE];
extern int		g_myid;


CGameFramework::CGameFramework()
{
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	for (int i = 0; i < m_nSwapChainBuffers; i++) m_ppd3dSwapChainBackBuffers[i] = NULL;
	m_nSwapChainBufferIndex = 0;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dCommandList = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;

	m_nRtvDescriptorIncrementSize = 0;
	m_nDsvDescriptorIncrementSize = 0;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	m_pScene = NULL;
	m_pPlayer = NULL;

	in_packet_size = 0;
	saved_packet_size = 0;

	_tcscpy_s(m_pszFrameRate, _T("Monster Tower ("));
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeaps();
	CreateSwapChain();

	BuildObjects();

	return(true);
}

//#define _WITH_SWAPCHAIN

void CGameFramework::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

#ifdef _WITH_CREATE_SWAPCHAIN_FOR_HWND
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	dxgiSwapChainDesc.Flags = 0;
#else
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1 **)&m_pdxgiSwapChain);
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.Windowed = TRUE;
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	dxgiSwapChainDesc.Flags = 0;
#else
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif

	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain **)&m_pdxgiSwapChain);
#endif

	if (!m_pdxgiSwapChain)
	{
		MessageBox(NULL, L"Swap Chain Cannot be Created.", L"Error", MB_OK);
		::PostQuitMessage(0);
		return;
	}

	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

#if defined(_DEBUG)
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void **)&m_pd3dDebugController);
	m_pd3dDebugController->EnableDebugLayer();
#endif

	hResult = ::CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void **)&m_pdxgiFactory);

	IDXGIAdapter1 *pd3dAdapter = NULL;

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice))) break;
	}

	if (!m_pd3dDevice)
	{
		hResult = m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIAdapter1), (void **)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice);
	}

	if (!m_pd3dDevice)
	{
		MessageBox(NULL, L"Direct3D 12 Device Cannot be Created.", L"Error", MB_OK);
		::PostQuitMessage(0);
		return;
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void **)&m_pd3dFence);
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 1;
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	::gnCbvSrvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void **)&m_pd3dCommandQueue);

	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)&m_pd3dCommandAllocator);

	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void **)&m_pd3dCommandList);
	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dRtvDescriptorHeap);
	m_nRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dDsvDescriptorHeap);
	m_nDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void **)&m_ppd3dSwapChainBackBuffers[i]);
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dSwapChainBackBuffers[i], NULL, d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += m_nRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void **)&m_pd3dDepthStencilBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, NULL, d3dDsvCPUDescriptorHandle);
//	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, &d3dDepthStencilViewDesc, d3dDsvCPUDescriptorHandle);
}

void CGameFramework::OnResizeBackBuffers()
{
	WaitForGpuComplete();

	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();
	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	m_nSwapChainBufferIndex = 0;
#else
	//m_pdxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth, m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);
	m_nSwapChainBufferIndex = 0;
#endif
	CreateRenderTargetViews();
	CreateDepthStencilView();

	m_pd3dCommandList->Close();

	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
	switch (nMessageID)
	{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			::SetCapture(hWnd);
			::GetCursorPos(&m_ptOldCursorPos);
			break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			::ReleaseCapture();
			break;
		case WM_MOUSEMOVE:
			break;
		default:
			break;
	}
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);

	cs_packet_animation *my_packet = reinterpret_cast<cs_packet_animation*>(send_buffer);
	my_packet->size = sizeof(my_packet);
	send_wsabuf.len = sizeof(my_packet);
	my_packet->type = CS_ANIM;

	DWORD iobyte;

	int anim_num = player_idle;

	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'W':
			if (!bCharaterRunFront)
			{
				anim_num = front_run;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);
				bCharaterRunFront = true;
				bCharaterRunBack = false;
				bCharaterRunLeft = false;
				bCharaterRunRight = false;
				bCharaterAttackFireStorm = false;
				
				my_packet->anim_num = anim_num;

				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
		case 'S':
			if (!bCharaterRunBack)
			{
				anim_num = back_run;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);
				bCharaterRunFront = false;
				bCharaterRunBack = true;
				bCharaterRunLeft = false;
				bCharaterRunRight = false;
				bCharaterAttackFireStorm = false;

				my_packet->anim_num = anim_num;


				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
		case 'A':
			if (!bCharaterRunLeft)
			{
				anim_num = left_run;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);
				bCharaterRunFront = false;
				bCharaterRunBack = false;
				bCharaterRunLeft = true;
				bCharaterRunRight = false;
				bCharaterAttackFireStorm = false;
				
				my_packet->anim_num = anim_num;


				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
		case 'D':
			if (!bCharaterRunRight)
			{
				anim_num = right_run;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);
				bCharaterRunFront = false;
				bCharaterRunBack = false;
				bCharaterRunLeft = false;
				bCharaterRunRight = true;
				bCharaterAttackFireStorm = false;

				my_packet->anim_num = anim_num;


				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
		case VK_SPACE:
			if (!bCharaterAttackFireStorm)
			{
				anim_num = 5;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);

				bCharaterRunFront = false;
				bCharaterRunBack = false;
				bCharaterRunLeft = false;
				bCharaterRunRight = false;
				bCharaterAttackFireStorm = true;

				my_packet->anim_num = anim_num;


				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
		default:
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case 'W':
			if (bCharaterRunFront)
			{
				anim_num = player_idle;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);

				bCharaterRunFront = false;
				bCharaterRunBack = false;
				bCharaterRunLeft = false;
				bCharaterRunRight = false;

				my_packet->anim_num = anim_num;


				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
		case 'S':
			if (bCharaterRunBack)
			{
				anim_num = player_idle;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);
				bCharaterRunFront = false;
				bCharaterRunBack = false;
				bCharaterRunLeft = false;
				bCharaterRunRight = false;

				my_packet->anim_num = anim_num;


				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
		case 'A':
			if (bCharaterRunLeft)
			{
				anim_num = player_idle;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);
				bCharaterRunFront = false;
				bCharaterRunBack = false;
				bCharaterRunLeft = false;
				bCharaterRunRight = false;
				my_packet->anim_num = anim_num;



				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
		case 'D':
			if (bCharaterRunRight)
			{
				anim_num = player_idle;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);
				bCharaterRunFront = false;
				bCharaterRunBack = false;
				bCharaterRunLeft = false;
				bCharaterRunRight = false;

				my_packet->anim_num = anim_num;


				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
		case VK_SPACE:
			if (bCharaterAttackFireStorm)
			{
				anim_num = player_idle;
				m_pPlayer->GetMesh()->SetAnimation(anim_num);
				bCharaterRunFront = false;
				bCharaterRunBack = false;
				bCharaterRunLeft = false;
				bCharaterRunRight = false;

				bCharaterAttackFireStorm = false;
				my_packet->anim_num = anim_num;


				int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
				if (ret) {
					int error_code = WSAGetLastError();
					printf("Error while sending packet [%d]", error_code);
				}
			}
			break;
				case VK_ESCAPE:
					::PostQuitMessage(0);
					break;
				case VK_RETURN:
					break;
				case VK_F1:
				case VK_F2:
				case VK_F3:
					m_pCamera = m_pPlayer->ChangeCamera((DWORD)(wParam - VK_F1 + 1), m_GameTimer.GetTimeElapsed());
					break;
				case VK_F9:
				{
					BOOL bFullScreenState = FALSE;
					m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
					m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

					DXGI_MODE_DESC dxgiTargetParameters;
					dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					dxgiTargetParameters.Width = m_nWndClientWidth;
					dxgiTargetParameters.Height = m_nWndClientHeight;
					dxgiTargetParameters.RefreshRate.Numerator = 60;
					dxgiTargetParameters.RefreshRate.Denominator = 1;
					dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
					dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
					m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

					OnResizeBackBuffers();

					break;
				}
				case VK_F10:
					break;
				default:
					break;
	}
			break;
		default:
			m_pScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
			break;
	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE)
				m_GameTimer.Stop();
			else
				m_GameTimer.Start();
			break;
		}
		case WM_SIZE:
		{
			m_nWndClientWidth = LOWORD(lParam);
			m_nWndClientHeight = HIWORD(lParam);

			OnResizeBackBuffers();
			break;
		}
		case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
			OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
			OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
			break;
		case WM_SOCKET:
			Server(wParam, lParam);
	}
	return(0);
}

void CGameFramework::OnDestroy()
{
    ReleaseObjects();

	::CloseHandle(m_hFenceEvent);

#if defined(_DEBUG)
	if (m_pd3dDebugController) m_pd3dDebugController->Release();
#endif

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();

	if (m_pd3dCommandAllocator) m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();

	if (m_pd3dFence) m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
    if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();
}

void CGameFramework::BuildObjects()
{
	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);


	m_pScene = new CScene();
	

	//m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pPlayer);
	
	m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList);
	m_pPlayer = new CAirplanePlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), m_pScene->GetTerrain(), 1);
	m_fireball = new CFireBall(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), m_pScene->GetTerrain(), 1);

	m_pCamera = m_pPlayer->GetCamera();

	//--------------------------------------------
	// Create OtherPlayer for rendering
	//-------------------------------------------

	m_ppOtherPlayers = new CPlayer*[MAX_USER];
	m_ppMonsters = new CGameObject*[MAX_OBJECT_INDEX - MAX_USER];


	for (int i = 0; i < MAX_USER; ++i) {
		m_ppOtherPlayers[i] = new COtherPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), m_pScene->GetTerrain(), 1);
		m_ppOtherPlayers[i]->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
		m_ppOtherPlayers[i]->SetVisible(false);
	}
	for (int i = 0; i < MAX_OBJECT_INDEX - MAX_USER; ++i) {
		m_ppMonsters[i] = new CDinosour(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
		m_ppMonsters[i]->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
		m_ppMonsters[i]->SetVisible(false);
	}


	m_pd3dCommandList->Close();
	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();
	
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (m_ppOtherPlayers)
			m_ppOtherPlayers[i]->ReleaseUploadBuffers();
	}
	for (int i = 0; i < MAX_OBJECT_INDEX - MAX_USER; ++i)
	{
		if (m_ppMonsters)
			m_ppMonsters[i]->ReleaseUploadBuffers();
	}

	if (m_pScene) m_pScene->ReleaseUploadBuffers();

	m_GameTimer.Reset();
}

void CGameFramework::ReleaseObjects()
{
	if (m_pPlayer) delete m_pPlayer;
	for (int i = 0; i < MAX_USER; ++i) {
		if (m_ppOtherPlayers)
			delete m_ppOtherPlayers[i];
	}
	for (int i = 0; i < MAX_OBJECT_INDEX - MAX_USER; ++i)
	{
		if (m_ppMonsters)
			delete m_ppMonsters[i];
	}

	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) delete m_pScene;
}

void CGameFramework::ProcessInput()
{
	static UCHAR pKeysBuffer[256];
	bool bProcessedByScene = false;
	if (GetKeyboardState(pKeysBuffer) && m_pScene) bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);
	if (!bProcessedByScene)
	{
		DWORD dwDirection = 0;
		int x = 0, y = 0;
		//if (pKeysBuffer[VK_DOWN] & 0xF0) dwDirection |= DIR_FORWARD;
		//if (pKeysBuffer[VK_UP] & 0xF0) dwDirection |= DIR_BACKWARD;
		//if (pKeysBuffer[VK_RIGHT] & 0xF0) dwDirection |= DIR_LEFT;
		//if (pKeysBuffer[VK_LEFT] & 0xF0) dwDirection |= DIR_RIGHT;
		//if (pKeysBuffer[VK_PRIOR] & 0xF0) dwDirection |= DIR_UP;
		//if (pKeysBuffer[VK_NEXT] & 0xF0) dwDirection |= DIR_DOWN;

		if (pKeysBuffer['D'] & 0xF0) {
			x -= 1;
		}
		if (pKeysBuffer['A'] & 0xF0) {
			x += 1;
		}
		if (pKeysBuffer['W'] & 0xF0) {
			y += 1;
		}
		if (pKeysBuffer['S'] & 0xF0) {
			y -= 1;
		}
		//if (pKeysBuffer[VK_SPACE] & 0xF0)
		//	teleport = true;


		//// 총알추가 
		if (pKeysBuffer[VK_SPACE] & 0xF0)
		{
			if (m_bisFire == false)
			{
				m_fireball->SetPosition(m_pPlayer->GetPosition());
			}
			m_bisFire = true;
		}
		else
			m_bisFire = false;

		cs_packet_move *my_packet = reinterpret_cast<cs_packet_move*>(send_buffer);
		my_packet->size = sizeof(my_packet);
		send_wsabuf.len = sizeof(my_packet);

		DWORD iobyte;
		float tempx, tempz;
		tempx = (roundf(m_pPlayer->GetLookVector().x * 100) / 100) * 100;
		tempz = (roundf(m_pPlayer->GetLookVector().z * 100) / 100) * 100;

		my_packet->dir_x = tempx;
		my_packet->dir_z = tempz;

		if (0 != x) {
			if (1 == x) my_packet->type = CS_RIGHT;
			else my_packet->type = CS_LEFT;

			int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			if (ret) {
				int error_code = WSAGetLastError();
				printf("Error while sending packet [%d]", error_code);
			}
		}
		if (0 != y) {
			if (1 == y) my_packet->type = CS_DOWN;
			else my_packet->type = CS_UP;

			int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			if (ret) {
				int error_code = WSAGetLastError();
				printf("Error while sending packet [%d]", error_code);
			}
		}

		//if (teleport) {
		//	my_packet->type = CS_TELEPORT;

		//	int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
		//	if (ret) {
		//		int error_code = WSAGetLastError();
		//		printf("Error while sending packet [%d]", error_code);
		//	}
		//}

		if (pKeysBuffer['F'] & 0xF0)
		{
//			my_packet->type = CS_ATTACK;

			int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			if (ret) {
				int error_code = WSAGetLastError();
				printf("Error while sending packet [%d]", error_code);
			}
		}



		//if (pKeysBuffer['1'] & 0xF0) {
		//	InitServer();
		//	m_pCurrScene = m_pTownScene;
		//	m_pPlayer = m_TownPlayer;
		//m_pScene->m_pPlayer = m_pPlayer;
		//m_pPlayer->SetPosition(XMFLOAT3(512.0f, m_pScene->GetTerrain()->GetHeight(512.f, 512.f), 512.0f));
		//for (int i = 0; i < MAX_USER; ++i)
		//	m_ppOtherPlayers[i]->SetPlayerUpdatedContext(m_pScene->GetTerrain());
		//m_pCamera = m_pPlayer->GetCamera();
		//}

		float cxDelta = 0.0f, cyDelta = 0.0f;
		POINT ptCursorPos;
		if (GetCapture() == m_hWnd)
		{
			SetCursor(NULL);
			GetCursorPos(&ptCursorPos);
			cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
			cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;
			SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
		}

		if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
		{
			if (cxDelta || cyDelta)
			{
				if (pKeysBuffer[VK_RBUTTON] & 0xF0)
					m_pPlayer->Rotate(cyDelta, 0.0f, -cxDelta);
				else
					m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
			}
			if (dwDirection) m_pPlayer->Move(dwDirection, 50.0f * m_GameTimer.GetTimeElapsed(), true);

			cs_packet_sight  *my_packet = reinterpret_cast<cs_packet_sight*>(send_buffer);
			my_packet->size = sizeof(my_packet);
			send_wsabuf.len = sizeof(my_packet);
			DWORD iobyte;
			float tempx, tempz;
			tempx = (roundf(m_pPlayer->GetLookVector().x * 100) / 100) * 100;
			tempz = (roundf(m_pPlayer->GetLookVector().z * 100) / 100) * 100;

			my_packet->type = CS_SIGHT;
			my_packet->dir_x = tempx;
			my_packet->dir_z = tempz;

			int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			if (ret) {
				int error_code = WSAGetLastError();
				printf("Error while sending packet [%d]", error_code);
			}
		}
	}
	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());

	for (int i = 0; i < MAX_USER; ++i)
	{
		if (m_ppOtherPlayers[i]->Getvisible() && (g_myid != i))
			m_ppOtherPlayers[i]->Update(m_GameTimer.GetTimeElapsed());
	}

	if (m_fireball)
	{
		XMFLOAT3 xmf3Shift = XMFLOAT3(0, 0, 0);
		xmf3Shift = Vector3::Add(xmf3Shift, m_pPlayer->GetLook(), m_GameTimer.GetTimeElapsed()*(-50));
		m_fireball->Move(xmf3Shift, TRUE);
		m_fireball->Update(m_GameTimer.GetTimeElapsed());
	}

	//for (int i = 0; i < MAX_USER; ++i)
	//{
	//	if (m_ppMonsters[i]->Getvisible() && (g_myid != i))
	//		m_ppMonsters[i]->Update(m_GameTimer.GetTimeElapsed());
	//}
}

void CGameFramework::AnimateObjects()
{
	float fTimeElapsed = m_GameTimer.GetTimeElapsed();
	if (m_pPlayer)
	{
		m_pPlayer->Animate(fTimeElapsed);
		m_pPlayer->GetMesh()->FBXFrameAdvance(fTimeElapsed);
	}

	for (int i = 0; i < MAX_USER; ++i)
	{
		if (m_ppOtherPlayers[i]->Getvisible() && (g_myid != i))
		{
			m_ppOtherPlayers[i]->Animate(fTimeElapsed);
			m_ppOtherPlayers[i]->GetMesh()->FBXFrameAdvance(fTimeElapsed);
		}
	}
	for (int i = 0; i < MAX_OBJECT_INDEX - MAX_USER; ++i)
	{
		if (m_ppMonsters[i]->Getvisible()) {
			m_ppMonsters[i]->Animate(fTimeElapsed);
			m_ppMonsters[i]->GetMesh()->FBXFrameAdvance(fTimeElapsed);
		}
	}
					
	//for (int i = 0; i < m_pPlayer->m_vBullets.size(); ++i)
	//	m_pPlayer->m_vBullets[i]->Animate(fTimeElapsed);
	if (m_pScene)
	{
 		m_pScene->AnimateObjects(fTimeElapsed, m_pCamera);
		m_pScene->AnimateObjects(fTimeElapsed);

	}
	
}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	//m_nSwapChainBufferIndex = (m_nSwapChainBufferIndex + 1) % m_nSwapChainBuffers;

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

//#define _WITH_PLAYER_TOP

void CGameFramework::FrameAdvance()
{    
	m_GameTimer.Tick(0.0f);
	
	ProcessInput();

    AnimateObjects();

	HRESULT hResult = m_pd3dCommandAllocator->Reset();
	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);

	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor/*Colors::Azure*/, 0, NULL);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);

	m_pScene->Render(m_pd3dCommandList, m_pCamera);

#ifdef _WITH_PLAYER_TOP
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
#endif
	if (m_pPlayer != NULL)
		m_pPlayer->UpdateTransform(NULL);
	if (m_pPlayer != NULL)
		m_pPlayer->Render(m_pd3dCommandList,m_pCamera);

	for (int i = 0; i < MAX_USER; ++i)
	{
		if (m_ppOtherPlayers[i]->Getvisible() && (g_myid != i))
		{
			m_ppOtherPlayers[i]->UpdateTransform(NULL);
			m_ppOtherPlayers[i]->Render(m_pd3dCommandList, m_pCamera);
		}
	}

	for (int i = 0; i < MAX_OBJECT_INDEX - MAX_USER; ++i)
	{
		if (m_ppMonsters[i]->Getvisible())
		{
			m_ppMonsters[i]->UpdateTransform(NULL);
			m_ppMonsters[i]->Render(m_pd3dCommandList, m_pCamera);
		}
	}

	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	hResult = m_pd3dCommandList->Close();
	
	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

#ifdef _WITH_PRESENT_PARAMETERS
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
#else
#ifdef _WITH_SYNCH_SWAPCHAIN
	m_pdxgiSwapChain->Present(1, 0);
#else
	m_pdxgiSwapChain->Present(0, 0);
#endif
#endif

   // m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	MoveToNextFrame();

	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}


////////// Game Server


void CGameFramework::InitServer()
{
	WSAStartup(MAKEWORD(2, 2), &wsa);

	g_mysocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

	char ip[128] = { 0 };

	cout << " 서버 IP 입력 : ";
	cin >> ip;

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(MY_SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(ip);

	int Result = WSAConnect(g_mysocket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

	WSAAsyncSelect(g_mysocket, m_hWnd, WM_SOCKET, FD_CLOSE | FD_READ);
	send_wsabuf.buf = send_buffer;
	send_wsabuf.len = BUF_SIZE;
	recv_wsabuf.buf = recv_buffer;
	recv_wsabuf.len = BUF_SIZE;

}

void CGameFramework::ShutDownServer()
{
	WSACleanup();
}

void CGameFramework::ReadPacket(SOCKET sock)
{
	DWORD iobyte, ioflag = 0;

	int ret = WSARecv(sock, &recv_wsabuf, 1, &iobyte, &ioflag, NULL, NULL);
	if (ret) {
		int err_code = WSAGetLastError();
		printf("Recv Error [%d]\n", err_code);
	}

	BYTE *ptr = reinterpret_cast<BYTE *>(recv_buffer);

	while (0 != iobyte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (iobyte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			iobyte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, iobyte);
			saved_packet_size += iobyte;
			iobyte = 0;
		}
	}
}

void CGameFramework::ProcessPacket(char * ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_PUT_PLAYER:
	{
		sc_packet_put_player *my_packet = reinterpret_cast<sc_packet_put_player *>(ptr);
		int id = my_packet->id;
		if (first_time) {
			first_time = false;
			g_myid = id;
		}
		if (id == g_myid) {
			XMFLOAT3 temp = { (float)(my_packet->x)*0.1f, 0.0, (float)my_packet->y*0.1f };
			temp.y = m_pScene->GetTerrain()->GetHeight(temp.x, temp.z);
			m_pPlayer->SetPosition(temp);
			m_pPlayer->SetVisible(true);
			m_pPlayer->SetScene(TOWN);
			cout << temp.x << ", " << temp.z << endl;
		}
		else if (id < MAX_USER)
		{
			XMFLOAT3 temp = { (float)my_packet->x*0.1f, 0.0, (float)my_packet->y*0.1f };
			XMFLOAT3 newlookvector = { (float)my_packet->sight_x*0.01f, 0.0, (float)my_packet->sight_z*0.01f };
			XMFLOAT3 currlookvector = m_ppOtherPlayers[id]->GetLookVector();
			temp.y = m_pScene->GetTerrain()->GetHeight(temp.x, temp.z);

			float degree = GetAngleBetweenTwoVectors2(newlookvector.x, newlookvector.z, currlookvector.x, currlookvector.z);

			m_ppOtherPlayers[id]->Rotate(0.0, degree, 0.0);
			m_ppOtherPlayers[id]->SetVisible(true);
			m_ppOtherPlayers[id]->SetPosition(temp);
		}
		else
		{
			XMFLOAT3 temp = { (float)my_packet->x*0.1f, 0.0, (float)my_packet->y*0.1f };
			XMFLOAT3 newlookvector = { (float)my_packet->sight_x*0.01f, 0.0, (float)my_packet->sight_z*0.01f };
			XMFLOAT3 currlookvector = m_ppMonsters[id - MAX_USER]->GetLook();
			temp.y = m_pScene->GetTerrain()->GetHeight(temp.x, temp.z);
			
			float degree = GetAngleBetweenTwoVectors2(newlookvector.x, newlookvector.z, currlookvector.x, currlookvector.z);
			
			m_ppMonsters[id - MAX_USER]->Rotate(0.0, degree + 180.0, 0.0);
			m_ppMonsters[id - MAX_USER]->SetVisible(true);
			m_ppMonsters[id - MAX_USER]->SetPosition(temp);
		}
		break;
	}
	case SC_POS:
	{
		sc_packet_pos *my_packet = reinterpret_cast<sc_packet_pos *>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			XMFLOAT3 temp = { (float)my_packet->x*0.1f, 0.0,(float)my_packet->y*0.1f };
			temp.y = m_pScene->GetTerrain()->GetHeight(temp.x, temp.z);
			m_pPlayer->SetPosition(temp);
			cout << "x : " << temp.x << ", y : " << temp.y << endl;

		}
		else if (other_id < MAX_USER)
		{
			XMFLOAT3 temp = { (float)my_packet->x*0.1f, 0.0, (float)my_packet->y*0.1f };
			XMFLOAT3 newlookvector = { (float)my_packet->sight_x*0.01f, 0.0, (float)my_packet->sight_z*0.01f };
			XMFLOAT3 currlookvector = m_ppOtherPlayers[other_id]->GetLookVector();
			temp.y = m_pScene->GetTerrain()->GetHeight(temp.x, temp.z);

			float degree = GetAngleBetweenTwoVectors2(newlookvector.x, newlookvector.z, currlookvector.x, currlookvector.z);

			m_ppOtherPlayers[other_id]->Rotate(0.0, degree, 0.0);
			m_ppOtherPlayers[other_id]->SetPosition(temp);
		}
		else
		{
			XMFLOAT3 temp = { (float)my_packet->x*0.1f, 0.0, (float)my_packet->y*0.1f };
			XMFLOAT3 newlookvector = { (float)my_packet->sight_x*0.01f, 0.0, (float)my_packet->sight_z*0.01f };
			XMFLOAT3 currlookvector = m_ppMonsters[other_id - MAX_USER]->GetLook();
			float degree = GetAngleBetweenTwoVectors2(newlookvector.x, newlookvector.z, currlookvector.x, currlookvector.z);
			temp.y = m_pScene->GetTerrain()->GetHeight(temp.x, temp.z) + 95;
			
			m_ppMonsters[other_id - MAX_USER]->Rotate(0.0, degree, 0.0);
			m_ppMonsters[other_id - MAX_USER]->SetPosition(temp);
			m_ppMonsters[other_id - MAX_USER]->SetVisible(true);
		}
		break;
	}
	case SC_REMOVE_PLAYER:
	{
		sc_packet_remove_player *my_packet = reinterpret_cast<sc_packet_remove_player *>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid)
		{
			m_pPlayer->SetVisible(false);
		}
		else if (other_id < MAX_USER)
			m_ppOtherPlayers[other_id]->SetVisible(false);
		else
			m_ppMonsters[other_id - MAX_USER]->SetVisible(false);
		break;
	}
	case SC_CHAT:
	{
		sc_packet_chat *my_packet = reinterpret_cast<sc_packet_chat *>(ptr);
		break;
	}
	case SC_SCENE:
	{
		sc_packet_scene_player * my_packet = reinterpret_cast<sc_packet_scene_player *>(ptr);
		
		if (my_packet->scene == FIELD)
		{
			//m_pPlayer->SetScene(FIELD);
			//
			//m_pCurrScene = m_pFieldScene;
			//m_pPlayer = m_FieldPlayer;
			//m_pPlayer->SetPosition(XMFLOAT3(512.0f, m_pScene->GetTerrain()->GetHeight(512.f, 950.f), 900.0f));
		
			for (int i = 0; i < MAX_USER; ++i)
				m_ppOtherPlayers[i]->SetPlayerUpdatedContext(m_pScene->GetTerrain());
		
			m_pCamera = m_pPlayer->GetCamera();
		}
		
		if (my_packet->scene == TOWN)
		{
			m_pPlayer->SetScene(TOWN);
			//m_pCurrScene = m_pTownScene;
			//m_pPlayer = m_TownPlayer;
			//m_pPlayer->SetPosition(XMFLOAT3(512.0f, m_pScene->GetTerrain()->GetHeight(512.f, 100.f), 100.0f));
			
			for (int i = 0; i < MAX_USER; ++i)
				m_ppOtherPlayers[i]->SetPlayerUpdatedContext(m_pScene->GetTerrain());
			
			m_pCamera = m_pPlayer->GetCamera();
		}
		break;
	}
	case SC_ANIM:
	{
		cs_packet_animation *my_packet = reinterpret_cast<cs_packet_animation *>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			m_pPlayer->GetMesh()->SetAnimation(my_packet->anim_num);
		}
		else if (other_id < MAX_USER)
		{
			m_ppOtherPlayers[other_id]->GetMesh()->SetAnimation(my_packet->anim_num);
		}
		else
		{
			//XMFLOAT3 temp = { (float)my_packet->x*0.1f, 0.0, (float)my_packet->y*0.1f };
			//XMFLOAT3 newlookvector = { (float)my_packet->sight_x*0.01f, 0.0, (float)my_packet->sight_z*0.01f };
			//XMFLOAT3 currlookvector = m_ppMonsters[other_id - MAX_USER]->GetLook();
			//float degree = GetAngleBetweenTwoVectors2(newlookvector.x, newlookvector.z, currlookvector.x, currlookvector.z);
			//temp.y = m_pCurrScene->GetTerrain()->GetHeight(temp.x, temp.z);
			//
			//m_ppMonsters[other_id - MAX_USER]->Rotate(0.0, degree + 180.0, 0.0);
			//m_ppMonsters[other_id - MAX_USER]->SetPosition(temp);
			//m_ppMonsters[other_id - MAX_USER]->SetVisible(true);
		}
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

float CGameFramework::GetAngleBetweenTwoVectors2(float dVec1X, float dVec1Y, float dVec2X, float dVec2Y)
{
	float dAngle1 = RAD2DEG(atan2(dVec1X, dVec1Y));

	float dAngle2 = RAD2DEG(atan2(dVec2X, dVec2Y));

	float dDiffAngles = dAngle1 - dAngle2;


	if (dDiffAngles < 0)

		dDiffAngles = 360 + dDiffAngles;

	return dDiffAngles;
}

void CGameFramework::Server(WPARAM wParam, LPARAM lParam)
{
	if (WSAGETSELECTERROR(lParam)) {
		closesocket((SOCKET)wParam);
		exit(1);
	}
	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_READ:
		ReadPacket((SOCKET)wParam);
		break;
	case FD_CLOSE:
		closesocket((SOCKET)wParam);
		exit(1);
		break;
	}
}
