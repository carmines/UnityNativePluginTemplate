// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Plugin/Mesh.g.h"
#include "Plugin.Module.h"

/* Unity Vertex layouts*/
//float3 position(12 bytes)
//float3 normal(12 bytes)
//byte4 color32(4 bytes) or float4 color(16 bytes)
//float2 | float3 | float4 uv(8, 12 or 16 bytes)
//float2 | float3 | float4 uv2(8, 12 or 16 bytes)
//float2 | float3 | float4 uv3(8, 12 or 16 bytes)
//float2 | float3 | float4 uv4(8, 12 or 16 bytes)
//float4 tangent(16 bytes)

struct MeshVertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

struct __declspec(uuid("01823938-e0e3-4831-aae5-204c73ef19e6")) IMeshPriv : ::IUnknown
{
    virtual HRESULT __stdcall SetUnityBuffers(_In_ ID3D11Buffer* pVertexBuffer, _In_ ID3D11Buffer* pIndexBuffer) = 0;
    virtual HRESULT __stdcall ResetUnityBuffers() = 0;
};

namespace winrt::$ext_safeprojectname$::Plugin::implementation
{

    struct Mesh : MeshT<Mesh, Module, IMeshPriv>
    {

        static IModule Create(
            _In_ std::weak_ptr<IUnityDeviceResource> const& unityDevice,
            _In_ StateChangedCallback fnCallback);

        Mesh() = default;

        virtual void Shutdown();
        virtual void OnRenderEvent(uint16_t frameNumber);

        event_token Closed(Windows::Foundation::EventHandler<Plugin::Mesh> const& handler);
        void Closed(event_token const& token);

        // IModule
        STDOVERRIDEMETHODIMP Initialize(_In_ std::weak_ptr<IUnityDeviceResource> const& unityDevice, _In_ StateChangedCallback stateCallback);

        // IMeshPriv
        STDOVERRIDEMETHODIMP SetUnityBuffers(_In_ ID3D11Buffer* pVertexBuffer, _In_ ID3D11Buffer* pIndexBuffer);
        STDOVERRIDEMETHODIMP ResetUnityBuffers();

    private:
        Windows::Foundation::IAsyncAction CreateMeshAsync();
        HRESULT UpdateBuffers(bool updateIndexBuffer);
        Windows::Foundation::IAsyncAction PrepareNextFrameAsync(uint16_t frameNumber);

    private:
        Windows::Foundation::IAsyncAction m_createMeshAsyncOp;
        Windows::Foundation::IAsyncAction m_prepareNextFrameAsyncOp;

        D3D11_BUFFER_DESC m_vertexBufferDesc;
        com_ptr<ID3D11Buffer> m_vertexBuffer;

        D3D11_BUFFER_DESC m_indexBufferDesc;
        com_ptr<ID3D11Buffer> m_indexBuffer;

        std::vector<MeshVertex> m_vertices;
        std::vector<int32_t> m_indicies;
        DirectX::XMFLOAT4 m_min;
        DirectX::XMFLOAT4 m_max;

        uint16_t m_frameNumber;

        event<Windows::Foundation::EventHandler<Plugin::Mesh>> m_closedEvent;
    };

}

namespace winrt::$ext_safeprojectname$::Plugin::factory_implementation
{
    struct Mesh : MeshT<Mesh, implementation::Mesh>
    {
    };
}
