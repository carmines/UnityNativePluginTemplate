// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "Plugin.Mesh.h"
#include <winrt/windows.storage.h>
#include <winrt/windows.storage.streams.h>
#include <chrono>

using namespace winrt;
using namespace $ext_safeprojectname$::Plugin::implementation;
using namespace Windows::Foundation; 

using winrtMesh = $ext_safeprojectname$::Plugin::Mesh;

_Use_decl_annotations_
$ext_safeprojectname$::Plugin::IModule Mesh::Create(
    std::weak_ptr<IUnityDeviceResource> const& unityDevice,
    StateChangedCallback fnCallback)
{
    auto mesh = make<Mesh>();

    if (SUCCEEDED(mesh.as<IModulePriv>()->Initialize(unityDevice, fnCallback)))
    {
        return mesh;
    }

    return nullptr;
}

void Mesh::Shutdown()
{
    if (m_prepareNextFrameAsyncOp != nullptr)
    {
        m_prepareNextFrameAsyncOp.Cancel();
        m_prepareNextFrameAsyncOp = nullptr;
    }

    if (m_createMeshAsyncOp != nullptr)
    {
        m_createMeshAsyncOp.Cancel();
        m_createMeshAsyncOp = nullptr;
    }

    ResetUnityBuffers();

    Module::Shutdown();
}

void Mesh::OnRenderEvent(uint16_t frameNumber)
{
    Module::OnRenderEvent(frameNumber);

    if (m_prepareNextFrameAsyncOp != nullptr && m_prepareNextFrameAsyncOp.Status() != AsyncStatus::Completed)
    {
        // preparing the next frame, wait to refresh mesh
        return;
    }

    // update mesh on calling thread, don't need to update index buffer
    if (SUCCEEDED(UpdateBuffers(false)))
    {
        // fire and forget, can bubble up a callback
        // but the thread that calls this function is not a Unity ui thread
        m_prepareNextFrameAsyncOp = PrepareNextFrameAsync(frameNumber);

        m_prepareNextFrameAsyncOp.Completed([=](auto const& asyncOp, auto const& status)
        {
            UNREFERENCED_PARAMETER(asyncOp);

            if (status == AsyncStatus::Canceled)
            {
                return;
            }

            CALLBACK_STATE state{};
            ZeroMemory(&state, sizeof(CALLBACK_STATE));

            if (status == AsyncStatus::Error)
            {
                state.type = CallbackType::Failed;

                ZeroMemory(&state.value.failedState, sizeof(FAILED_STATE));

                state.value.failedState.hresult = to_hresult();
            }
            else
            {
                // notify plugin of the mesh vertex and index size
                state.type = CallbackType::Mesh;

                ZeroMemory(&state.value.meshState, sizeof(MESH_STATE));
                state.value.meshState.stateType = MeshStateType::Updated;
                state.value.meshState.frame = m_frameNumber;
                state.value.meshState.vertexCount = static_cast<int32_t>(m_vertices.size());
                state.value.meshState.indexCount = static_cast<int32_t>(m_indicies.size());
                state.value.meshState.min = m_min;
                state.value.meshState.max = m_max;
            }

            Callback(state);
        });
    }
}

event_token Mesh::Closed(EventHandler<winrtMesh> const& handler)
{
    return m_closedEvent.add(handler);
}

void Mesh::Closed(event_token const& token)
{
    m_closedEvent.remove(token);
}

_Use_decl_annotations_
HRESULT Mesh::Initialize(
    std::weak_ptr<IUnityDeviceResource> const& unityDevice,
    StateChangedCallback stateCallback)
{
    IFR(Module::Initialize(unityDevice, stateCallback));

    m_frameNumber = 0;

    m_createMeshAsyncOp = CreateMeshAsync();
    if (m_createMeshAsyncOp.Status() != AsyncStatus::Completed)
    {
        m_createMeshAsyncOp.Completed([=](auto const& asyncOp, auto const& status)
        {
            UNREFERENCED_PARAMETER(asyncOp);

            if (status == AsyncStatus::Canceled)
            {
                return;
            }

            CALLBACK_STATE state{};
            ZeroMemory(&state, sizeof(CALLBACK_STATE));

            if (status == AsyncStatus::Error)
            {
                state.type = CallbackType::Failed;

                ZeroMemory(&state.value.failedState, sizeof(FAILED_STATE));

                state.value.failedState.hresult = to_hresult();
            }
            else
            {
                // notify plugin of the mesh vertex and index size
                state.type = CallbackType::Mesh;

                ZeroMemory(&state.value.meshState, sizeof(MESH_STATE));
                state.value.meshState.stateType = MeshStateType::Created;
                state.value.meshState.frame = m_frameNumber;
                state.value.meshState.vertexCount = static_cast<int32_t>(m_vertices.size());
                state.value.meshState.indexCount = static_cast<int32_t>(m_indicies.size());
                state.value.meshState.min = m_min;
                state.value.meshState.max = m_max;
            }

            Callback(state);
        });
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT Mesh::SetUnityBuffers(_In_ ID3D11Buffer* pVertexBuffer, _In_ ID3D11Buffer* pIndexBuffer)
{
    NULL_CHK_HR(pVertexBuffer, E_INVALIDARG);

    NULL_CHK_HR(pIndexBuffer, E_INVALIDARG);

    // check for CPU writes
    D3D11_BUFFER_DESC vertexDesc;
    pVertexBuffer->GetDesc(&vertexDesc);
    if (vertexDesc.Usage == D3D11_USAGE_DYNAMIC)
    {
        IFR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    D3D11_BUFFER_DESC indexDesc;
    pIndexBuffer->GetDesc(&indexDesc);
    if (indexDesc.Usage == D3D11_USAGE_DYNAMIC)
    {
        IFR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    // store
    pVertexBuffer->AddRef();
    m_vertexBuffer.attach(pVertexBuffer);
    m_vertexBufferDesc = vertexDesc;

    pIndexBuffer->AddRef();
    m_indexBuffer.attach(pIndexBuffer);
    m_indexBufferDesc = indexDesc;

    return UpdateBuffers(true);
}

_Use_decl_annotations_
HRESULT Mesh::ResetUnityBuffers()
{
    ZeroMemory(&m_vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
    m_vertexBuffer = nullptr;

    ZeroMemory(&m_indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
    m_indexBuffer = nullptr;

    return S_OK;
}

IAsyncAction Mesh::CreateMeshAsync()
{
    co_await resume_background();

    // TODO: DO SOMETHING TO LOAD AND CREATE THE VERTEX AND INDEX array's
    m_vertices = {
        { { -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { { -1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
    };

    m_indicies = { 0, 1, 2, 2, 3, 0 };

    m_min = { -1.0f, -1.0f, 0.0f, 1.0f };
    m_max = { 1.0f, 1.0f, 0.0f, 1.0f };

    co_return;
}

HRESULT Mesh::UpdateBuffers(bool updateIndexBuffer)
{
    if (m_vertexBuffer == nullptr || m_indexBuffer == nullptr)
    {
        IFR(E_NOT_VALID_STATE);
    }

    HRESULT hr = S_OK;

    auto resources = m_deviceResources.lock();
    if (resources != nullptr)
    {
        com_ptr<ID3D11DeviceResource> spD3D11Resources = nullptr;
        hr = resources->QueryInterface(__uuidof(ID3D11DeviceResource), spD3D11Resources.put_void());
        if (SUCCEEDED(hr))
        {
            com_ptr<ID3D11DeviceContext> spContext;
            spD3D11Resources->GetDevice()->GetImmediateContext(spContext.put());

            uint32_t vertexBufferPitch = m_vertexBufferDesc.ByteWidth / static_cast<uint32_t>(m_vertices.size());
            spContext->UpdateSubresource(m_vertexBuffer.get(), 0, nullptr, m_vertices.data(), vertexBufferPitch, 0);

            if (updateIndexBuffer)
            {
                uint32_t indexBufferPitch = m_indexBufferDesc.ByteWidth / static_cast<uint32_t>(m_indicies.size());
                spContext->UpdateSubresource(m_indexBuffer.get(), 0, nullptr, m_indicies.data(), indexBufferPitch, 0);
            }
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

IAsyncAction Mesh::PrepareNextFrameAsync(uint16_t frameNumber)
{
    using namespace std::chrono;

    co_await resume_background();

    // render the requested frame number

    // simulate work
    co_await 500ms;

    // set the currentframe number to request frame
    m_frameNumber = frameNumber;
}
