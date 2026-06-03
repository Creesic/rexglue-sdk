#pragma once

#ifndef _WIN32

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#define COM_NO_WINDOWS_H

#ifndef __declspec
#define __declspec(x)
#endif

#ifndef __stdcall
#define __stdcall
#endif

#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE __stdcall
#endif

#ifndef STDMETHODIMP
#define STDMETHODIMP HRESULT STDMETHODCALLTYPE
#endif

#ifndef STDMETHODIMP_
#define STDMETHODIMP_(type) type STDMETHODCALLTYPE
#endif

#ifndef EXTERN_C
#define EXTERN_C extern "C"
#endif

#ifndef CoTaskMemAlloc
#define CoTaskMemAlloc std::malloc
#endif

#ifndef CoTaskMemFree
#define CoTaskMemFree std::free
#endif

#ifndef DECLARE_CROSS_PLATFORM_UUIDOF
#define DECLARE_CROSS_PLATFORM_UUIDOF(T)
#endif

#ifndef DEFINE_CROSS_PLATFORM_UUIDOF
#define DEFINE_CROSS_PLATFORM_UUIDOF(T)
#endif

#ifndef _In_
#define _In_
#endif

#ifndef _Out_
#define _Out_
#endif

#ifndef _In_opt_
#define _In_opt_
#endif

#ifndef _In_z_
#define _In_z_
#endif

#ifndef _In_opt_z_
#define _In_opt_z_
#endif

#ifndef _In_count_
#define _In_count_(x)
#endif

#ifndef _In_opt_count_
#define _In_opt_count_(x)
#endif

#ifndef _In_reads_bytes_
#define _In_reads_bytes_(x)
#endif

#ifndef _In_bytecount_
#define _In_bytecount_(x)
#endif

#ifndef _Maybenull_
#define _Maybenull_
#endif

#ifndef _COM_Outptr_
#define _COM_Outptr_
#endif

#ifndef _COM_Outptr_opt_
#define _COM_Outptr_opt_
#endif

#ifndef _COM_Outptr_result_maybenull_
#define _COM_Outptr_result_maybenull_
#endif

#ifndef _COM_Outptr_opt_result_maybenull_
#define _COM_Outptr_opt_result_maybenull_
#endif

#ifndef _Outptr_opt_result_z_
#define _Outptr_opt_result_z_
#endif

#ifndef _Outptr_result_nullonfailure_
#define _Outptr_result_nullonfailure_
#endif

#ifndef _Outptr_result_bytebuffer_maybenull_
#define _Outptr_result_bytebuffer_maybenull_(x)
#endif

#ifndef _Outptr_result_maybenull_z_
#define _Outptr_result_maybenull_z_
#endif

typedef unsigned char BYTE;
typedef unsigned char UINT8;
typedef bool BOOL;
typedef int INT;
typedef long LONG;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef std::size_t SIZE_T;
typedef wchar_t WCHAR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef WCHAR OLECHAR;
typedef OLECHAR* BSTR;
typedef long HRESULT;

struct GUID {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t Data4[8];
};

typedef GUID IID;
typedef GUID CLSID;
typedef const IID& REFIID;
typedef const CLSID& REFCLSID;

#ifndef S_OK
#define S_OK ((HRESULT)0L)
#endif

#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif

struct IUnknown {
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv_object) = 0;
  virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
  virtual ULONG STDMETHODCALLTYPE Release() = 0;

 protected:
  ~IUnknown() = default;
};

struct IStream;

#endif  // !_WIN32
