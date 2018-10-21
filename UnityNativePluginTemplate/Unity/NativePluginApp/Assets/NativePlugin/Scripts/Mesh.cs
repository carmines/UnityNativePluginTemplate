// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using UnityEngine;

namespace $ext_safeprojectname$
{
    [RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
    internal class Mesh : BasePlugin<Mesh>
    {
        [SerializeField]
        private MeshFilter meshFilter = null;

        [SerializeField]
        private MeshRenderer meshRenderer = null;

        [SerializeField]
        private BoxCollider boxCollider = null;

        private IntPtr meshVB = IntPtr.Zero;
        private IntPtr meshIB = IntPtr.Zero;
        private Bounds bounds = new Bounds();

        protected override void Awake()
        {
            base.Awake();

            boxCollider = GetComponent<BoxCollider>();

            meshFilter = GetComponent<MeshFilter>();

            meshRenderer = GetComponent<MeshRenderer>();

            if (meshRenderer.material == null)
            {
                Debug.LogWarning("No Material set on the MeshRenderer");
            }
        }

        protected override void OnEnable()
        {
            base.OnEnable();

            CreateMesh();
        }

        protected override void OnDisable()
        {
            CheckHR(Native.ResetMeshBuffers(instanceId));

            base.OnDisable();
        }

        protected override void OnCallback(Wrapper.CallbackType type, Wrapper.CallbackState args)
        {
            switch (args.MeshSate.Type)
            {
                case Wrapper.MeshStateType.Created:
                    OnMeshUpdated(args.MeshSate);
                    break;
            }
        }

        private void OnMeshUpdated(Wrapper.MeshState state)
        {
            CreateMesh(state.VertexCount, state.IndexCount);

            bounds.SetMinMax(state.MinPoint, state.MaxPoint);
            boxCollider.center = bounds.center;
            boxCollider.size = bounds.size;
        }

        private void CreateMesh(int vertexCount, int indexCount)
        {
            if (vertexCount == 0 || indexCount == 0)
            {
                Debug.LogError("Cannot create a mesh with a vertexCount: " + vertexCount + ", and indexCount: " + indexCount);

                return;
            }

            // make empty mesh for filter
            if (meshFilter.mesh == null)
            {
                meshFilter.mesh = new UnityEngine.Mesh();
            }

            // https://unity3d.com/unity/whats-new/unity-2017.3.0
#if UNITY_2017_3_OR_NEWER
            meshFilter.mesh.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
#endif
            meshFilter.mesh.Clear(false);

            meshFilter.mesh.SetVertices(new Vector3[vertexCount].ToList<Vector3>());
            meshFilter.mesh.SetColors(new Color[vertexCount].ToList<Color>());
            meshFilter.mesh.SetIndices(new int[indexCount], MeshTopology.Triangles, 0);

            // get native gpu buffer pointers
            meshVB = meshFilter.mesh.GetNativeVertexBufferPtr(0);
            meshIB = meshFilter.mesh.GetNativeIndexBufferPtr();

            CheckHR(Native.SetMeshBuffers(instanceId, meshVB, meshIB));
        }

        private static class Native
        {
            [DllImport(Wrapper.ModuleName, CallingConvention = CallingConvention.StdCall, EntryPoint = "SetMeshBuffers")]
            internal static extern Int32 SetMeshBuffers(Int32 handle, IntPtr vertexBuffer, IntPtr indexBuffer);

            [DllImport(Wrapper.ModuleName, CallingConvention = CallingConvention.StdCall, EntryPoint = "ResetMeshBuffers")]
            internal static extern Int32 ResetMeshBuffers(Int32 handle);
        }
    }
}
