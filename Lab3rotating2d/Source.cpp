#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3d10.h>
#include <d3dx10.h>
#include <dwrite.h>
#include <dinput.h>
#include <xnamath.h>

#pragma comment(lib, "d3d10.lib") 
#pragma comment(lib, "d3dx10.lib")
#pragma comment (lib, "dinput8.lib")

// ������ � ������ ����
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

// ���������
struct SimpleVertex
{
	D3DXVECTOR3 Pos;
	D3DXVECTOR4 Color;
};

// ���������� ����������
HWND g_hWnd = NULL;
D3D10_DRIVER_TYPE g_driverType = D3D10_DRIVER_TYPE_NULL;
ID3D10Device* g_pd3dDevice = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D10RenderTargetView* g_pRenderTargetView = NULL;
ID3D10Effect* g_pEffect = NULL;
ID3D10EffectTechnique* g_pTechnique = NULL;
ID3D10InputLayout* g_pVertexLayout = NULL;
ID3D10Buffer* g_pVertexBuffer = NULL;
D3DXMATRIX g_World;
D3DXMATRIX g_View;
D3DXMATRIX g_Projection;
ID3D10EffectMatrixVariable* g_pWorldVariable = NULL;
ID3D10EffectMatrixVariable* g_pViewVariable = NULL;
ID3D10EffectMatrixVariable* g_pProjectionVariable = NULL;


XMMATRIX camView;
XMMATRIX camProjection;
XMVECTOR camPosition;
XMVECTOR camTarget;
XMVECTOR camUp;
XMVECTOR DefaultForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
XMVECTOR DefaultRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
XMVECTOR camForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
XMVECTOR camRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
XMMATRIX camRotationMatrix;
XMMATRIX groundWorld;

float moveLeftRight = 0.0f;
float moveBackForward = 0.0f;

float camYaw = 0.0f;
float camPitch = 0.0f;

XMMATRIX Rotation;
XMMATRIX Scale;
XMMATRIX Translation;
float rot = 0.01f;

IDirectInputDevice8* DIKeyboard;
IDirectInputDevice8* DIMouse;

// ��������� �������
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDirect3D10();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Cleanup();
void RenderScene();
void UpdateCamera();

void UpdateCamera() {
	camRotationMatrix = XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);
	camTarget = XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	camTarget = XMVector3Normalize(camTarget);

	///////////////**************new**************////////////////////
		/*
		// First-Person Camera
		XMMATRIX RotateYTempMatrix;
		RotateYTempMatrix = XMMatrixRotationY(camYaw);

		camRight = XMVector3TransformCoord(DefaultRight, RotateYTempMatrix);
		camUp = XMVector3TransformCoord(camUp, RotateYTempMatrix);
		camForward = XMVector3TransformCoord(DefaultForward, RotateYTempMatrix);*/

		// Free-Look Camera
	camRight = XMVector3TransformCoord(DefaultRight, camRotationMatrix);
	camForward = XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	camUp = XMVector3Cross(camForward, camRight);
	///////////////**************new**************////////////////////

	camPosition += moveLeftRight * camRight;
	camPosition += moveBackForward * camForward;

	moveLeftRight = 0.0f;
	moveBackForward = 0.0f;

	camTarget = camPosition + camTarget;

	camView = XMMatrixLookAtLH(camPosition, camTarget, camUp);
}

// � ���� ������� ���������� ���������� ���������
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// ������� ���� ����������
	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;
	//
	if (FAILED(InitDirect3D10()))
	{
		Cleanup();
		return 0;
	}
	// ���� ��������� ���������
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			RenderScene();
		}
	}
	Cleanup();
	return (int)msg.wParam;
}

// ����������� ������ � �������� ����
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// ������������ ����� ����
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"SimpleWindowClass";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (!RegisterClassEx(&wc))
		return E_FAIL;
	// ������� ����
	g_hWnd = CreateWindow(
		L"SimpleWindowClass",
		L"DirectX 10: ������ �����������",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);
	// ���� �� ������� ������� ���� - ������� �� �������
	if (!g_hWnd)
		return E_FAIL;
	// ���������� ���� �� ������
	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);
	return S_OK;
}

// ������������� Direct3D
HRESULT InitDirect3D10()
{
	HRESULT hr = S_OK;
	// ������� ���������� ������� ����
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;
	// ������ ��������� ����� ����������
	D3D10_DRIVER_TYPE driverTypes[] =
	{
	D3D10_DRIVER_TYPE_HARDWARE,
	D3D10_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = sizeof(driverTypes) / sizeof(driverTypes[0]);
	// ��������� ���������
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	// �������� ������� ����������, ������� �� ������
	// ��� ������ ���������� - ������� �� �����
	for (UINT driverTypeIndex = 0;
		driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D10CreateDeviceAndSwapChain(NULL,
			g_driverType, NULL, 0, D3D10_SDK_VERSION, &sd,
			&g_pSwapChain, &g_pd3dDevice);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;
	// ������������� ������ ���
	// ������ ������������
	ID3D10Texture2D* pBackBuffer;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D),
		(LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;
	// �������� ������������� ������
	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL,
		&g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;
	// ������ ����� ������������ � ����������� ����������
	g_pd3dDevice->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
	// �������� ������� �����������
	D3D10_VIEWPORT vp;
	vp.Width = width;
	vp.Height = height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports(1, &vp);
	// ���������� ���� �������
	// � ����� �������� shaders.fx
	hr = D3DX10CreateEffectFromFile(L"shaders.fx", NULL, NULL, "fx_4_0",
		D3D10_SHADER_ENABLE_STRICTNESS, 0, g_pd3dDevice, NULL, NULL,
		&g_pEffect, NULL, NULL);
	if (FAILED(hr))
	{
		MessageBox(NULL, L"�� ������� ���������� ���� ��������(FX). ���� �������� ������ ���������� � ��� �� �����, ��� � ����������� ����", L"������", MB_OK );
			return hr;
	}
	// ��������� ������� �����������
	g_pTechnique = g_pEffect->GetTechniqueByName("RenderColor");
	// ����� � ����������� �������
	g_pWorldVariable = g_pEffect->
		GetVariableByName("World")->AsMatrix();
	g_pViewVariable = g_pEffect->
		GetVariableByName("View")->AsMatrix();
	g_pProjectionVariable = g_pEffect->
		GetVariableByName("Projection")->AsMatrix();
	// ��������� ������ ������� ������
	D3D10_INPUT_ELEMENT_DESC layout[] =
	{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
	D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
	D3D10_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = sizeof(layout) / sizeof(layout[0]);
	// ������� ������ ������� ������
	D3D10_PASS_DESC PassDesc;
	g_pTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements,
		PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize,
		&g_pVertexLayout);
	if (FAILED(hr))
		return hr;
	// ��������� ������ ������� ������ � ����������� ����������
	g_pd3dDevice->IASetInputLayout(g_pVertexLayout);
	// ������� ����� ������
	SimpleVertex vertices[] =
	{
	 D3DXVECTOR3(0.0f, 0.5f, 0.0f),
	 D3DXVECTOR4(1.0f, 0.0f, 0.0f, 1.0f),
	 D3DXVECTOR3(0.5f, -0.5f, 0.0f),
	 D3DXVECTOR4(0.0f, 1.0f, 0.0f, 1.0f),
	 D3DXVECTOR3(-0.5f, -0.5f, 0.0f),
	 D3DXVECTOR4(0.0f, 0.0f, 1.0f, 1.0f)
	};
	D3D10_BUFFER_DESC bd;
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 3;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;
	// ��������� ����� ������ � ����������� ����������
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pd3dDevice->IASetVertexBuffers(0, 1, &g_pVertexBuffer,
		&stride, &offset);
	// ������ ��� � ������ ���������� ����������
	g_pd3dDevice->IASetPrimitiveTopology(
		D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// �������������� ������� �������
	D3DXMatrixIdentity(&g_World);

	// �������������� ������� ����
	D3DXVECTOR3 Eye(0.0f, 0.0f, -1.7f);
	D3DXVECTOR3 At(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 Up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_View, &Eye, &At, &Up);
	// �������������� ������� ��������
	D3DXMatrixPerspectiveFovLH(&g_Projection, (float)D3DX_PI / 4,
		width / (float)height, 0.1f, 100.0f);
	return S_OK;
}

// ���������� ���������� �����
void RenderScene()
{
	// ���� ��������
	static float t = 0.0f;
	if (g_driverType == D3D10_DRIVER_TYPE_REFERENCE)
	{
		t += (float)D3DX_PI * 0.0125f;
	}
	else
	{
		static DWORD dwTimeStart = 0; DWORD dwTimeCur = GetTickCount();
		if (dwTimeStart == 0)
			dwTimeStart = dwTimeCur;
		t = (dwTimeCur - dwTimeStart) / 1000.0f;
	}
	// ������� �������� ������������
	// �� ������ ���� � ����������� �� �������
	D3DXMatrixRotationY(&g_World, t);
	// �������� ���������� � �������� ��������������
	g_pWorldVariable->SetMatrix((float*)&g_World);
	g_pViewVariable->SetMatrix((float*)&g_View);
	g_pProjectionVariable->SetMatrix((float*)&g_Projection);
	// ������� ��������� �����
	//(���������� ��������, ��������, ������, ������������)
	float ClearColor[4] = { 0.1f, 0.5f, 0.1f, 1.0f };
	g_pd3dDevice->ClearRenderTargetView(g_pRenderTargetView,
		ClearColor);
	// ������ ��� �����������
	D3D10_TECHNIQUE_DESC techDesc;
	g_pTechnique->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		g_pTechnique->GetPassByIndex(p)->Apply(0);
		g_pd3dDevice->Draw(3, 0);
	}
	g_pSwapChain->Present(0, 0);
}

// ��������� ���������
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
	LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// ������� ������
void Cleanup()
{
	if (g_pd3dDevice) g_pd3dDevice->ClearState();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pEffect) g_pEffect->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}