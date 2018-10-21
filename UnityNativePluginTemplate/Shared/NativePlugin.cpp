// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "Unity/IUnityGraphics.h"
#include "PlatformBase.h"
#include "UnityDeviceResource.h"

#include "Plugin.Mesh.h"

namespace impl
{
    using namespace winrt::$ext_safeprojectname$::Plugin::implementation;
}

namespace winrt
{
    using namespace winrt::$ext_safeprojectname$::Plugin;
}

static INSTANCE_HANDLE s_lastPluginHandleIndex = INSTANCE_HANDLE_INVALID;
static std::unordered_map<INSTANCE_HANDLE, winrt::IModule> s_instances;
HRESULT GetModule(INSTANCE_HANDLE id, _Out_ winrt::IModule& module)
{
    if (id < INSTANCE_HANDLE_START)
    {
        IFR(E_INVALIDARG);
    }

    auto it = s_instances.find(id);
    if (it == s_instances.end())
    {
        IFR(HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    }

    module = it->second;

    NULL_CHK_HR(module, E_POINTER);

    return S_OK;
}

static std::shared_ptr<IUnityDeviceResource> s_deviceResource;
static UnityGfxRenderer s_deviceType = kUnityGfxRendererNull;
static IUnityInterfaces* s_unityInterfaces = nullptr;
static IUnityGraphics* s_unityGraphics = nullptr;

HRESULT TrackModule(winrt::IModule &module, INSTANCE_HANDLE * handleId)
{
    // get the last index value;
    auto handle = s_lastPluginHandleIndex;

    // add to map
    auto success = s_instances.emplace(handle, module);
    if (success.second)
    {
        *handleId = handle++;
        s_lastPluginHandleIndex = handle;
    }

    return (success.second ? S_OK : E_UNEXPECTED);
}

// --------------------------------------------------------------------------
// UnitySetInterfaces
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    s_lastPluginHandleIndex = INSTANCE_HANDLE_START;
    s_instances.clear();

    s_unityInterfaces = unityInterfaces;
    s_unityGraphics = s_unityInterfaces->Get<IUnityGraphics>();
    s_unityGraphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    for (auto&& kv : s_instances)
    {
        kv.second.Shutdown();
        kv.second = nullptr;
    }
    s_instances.clear();

    s_lastPluginHandleIndex = INSTANCE_HANDLE_INVALID;

    s_unityGraphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

// --------------------------------------------------------------------------
// GraphicsDeviceEvent
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    // Create graphics API implementation upon initialization
    if (eventType == kUnityGfxDeviceEventInitialize)
    {
        assert(s_deviceResource == nullptr);

        s_deviceType = s_unityGraphics->GetRenderer();

        s_deviceResource = CreateDeviceResource(s_deviceType);
    }

    // Let the implementation process the device related event
    if (s_deviceResource != nullptr)
    {
        s_deviceResource->ProcessDeviceEvent(eventType, s_unityInterfaces);
    }

    // Cleanup graphics API implementation upon shutdown
    if (eventType == kUnityGfxDeviceEventShutdown)
    {
        s_deviceResource.reset();
        s_deviceResource = nullptr;
        s_deviceType = kUnityGfxRendererNull;
    }
}


// --------------------------------------------------------------------------
// OnRenderEvent
// This will be called for GL.IssuePluginEvent script calls; eventID will
// be the integer passed to IssuePluginEvent. In this example, we just ignore
// that value.
static void UNITY_INTERFACE_API OnRenderEvent(int32_t eventID)
{
    DWORD dwId = static_cast<DWORD>(eventID);

    INSTANCE_HANDLE id = static_cast<INSTANCE_HANDLE>(LOWORD(dwId));

    winrt::IModule module = nullptr;
    if (SUCCEEDED(GetModule(id, module)))
    {
        uint16_t frameId = static_cast<uint16_t>(HIWORD(dwId));

        module.OnRenderEvent(frameId);
    }
}

// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.
extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
    return OnRenderEvent;
}


// --------------------------------------------------------------------------
// Other function
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ReleaseInstance(
    _In_ INSTANCE_HANDLE id)
{
    winrt::IModule module = nullptr;
    if (SUCCEEDED(GetModule(id, module)))
    {
        s_instances.erase(id);
        module.Shutdown();
        module = nullptr;
    }
}


// Mesh
extern "C" int32_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API CreateMesh(
    _In_ StateChangedCallback fnCallback,
    _Out_ INSTANCE_HANDLE* handleId)
{
    winrt::IModule module = impl::Mesh::Create(s_deviceResource, fnCallback);

    return TrackModule(module, handleId);
}

extern "C" int32_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetMeshBuffers(
    _In_ INSTANCE_HANDLE id,
    _In_ void* vertexBufferHandle,
    _In_ void* indexBufferHandle)
{
    winrt::IModule module = nullptr;
    HRESULT hr = GetModule(id, module);
    if (SUCCEEDED(hr))
    {
        auto mesh = module.as<IMeshPriv>();

        NULL_CHK_HR(mesh, HRESULT_FROM_WIN32(ERROR_INVALID_INDEX));

        hr = mesh->SetUnityBuffers(static_cast<ID3D11Buffer*>(vertexBufferHandle), static_cast<ID3D11Buffer*>(indexBufferHandle));
    }

    return hr;
}

extern "C" int32_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ResetMeshBuffers(
    _In_ INSTANCE_HANDLE id)
{
    winrt::IModule module = nullptr;
    HRESULT hr = GetModule(id, module);
    if (SUCCEEDED(hr))
    {
        auto mesh = module.as<IMeshPriv>();

        NULL_CHK_HR(mesh, HRESULT_FROM_WIN32(ERROR_INVALID_INDEX));

        hr = mesh->ResetUnityBuffers();
    }

    return hr;
}

