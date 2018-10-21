// guids.h: definitions of GUIDs/IIDs/CLSIDs used in this VsPackage

/*
Do not use #pragma once, as this file needs to be included twice.  Once to declare the externs
for the GUIDs, and again right after including initguid.h to actually define the GUIDs.
*/



// package guid
// { 524579f9-6ca1-4b75-ae8b-b850135c1199 }
#define guidUnity_Native_Plugin_TemplatePkg { 0x524579F9, 0x6CA1, 0x4B75, { 0xAE, 0x8B, 0xB8, 0x50, 0x13, 0x5C, 0x11, 0x99 } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_Unity_Native_Plugin_Template,
0x524579F9, 0x6CA1, 0x4B75, 0xAE, 0x8B, 0xB8, 0x50, 0x13, 0x5C, 0x11, 0x99 );
#endif

// Command set guid for our commands (used with IOleCommandTarget)
// { d5e4a81d-9c33-4ef7-9fa1-e3555521b51e }
#define guidUnity_Native_Plugin_TemplateCmdSet { 0xD5E4A81D, 0x9C33, 0x4EF7, { 0x9F, 0xA1, 0xE3, 0x55, 0x55, 0x21, 0xB5, 0x1E } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_Unity_Native_Plugin_TemplateCmdSet, 
0xD5E4A81D, 0x9C33, 0x4EF7, 0x9F, 0xA1, 0xE3, 0x55, 0x55, 0x21, 0xB5, 0x1E );
#endif

//Guid for the image list referenced in the VSCT file
// { f39a8394-98b3-499e-b12a-cd508e90aa52 }
#define guidImages { 0xF39A8394, 0x98B3, 0x499E, { 0xB1, 0x2A, 0xCD, 0x50, 0x8E, 0x90, 0xAA, 0x52 } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_Images, 
0xF39A8394, 0x98B3, 0x499E, 0xB1, 0x2A, 0xCD, 0x50, 0x8E, 0x90, 0xAA, 0x52 );
#endif


