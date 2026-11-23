/* Original COM measurement server, AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#include <windows.h>
#include <objidl.h>

#ifndef MARKER
#error Specify the server marker
#endif

static HRESULT WINAPI object_query(IPersist *iface, REFIID iid, void **out)
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IPersist)) return E_NOINTERFACE;
    *out = iface;
    IPersist_AddRef(iface);
    return S_OK;
}

/* The single stateless object lives for the lifetime of the loaded DLL. */
static ULONG WINAPI object_addref(IPersist *iface) { (void)iface; return 2; }
static ULONG WINAPI object_release(IPersist *iface) { (void)iface; return 1; }

static HRESULT WINAPI object_class(IPersist *iface, CLSID *clsid)
{
    static const CLSID id = {MARKER, 0x9402, 0x41dd, {0xa8,0x65,0xe2,0xdb,0x26,0x97,0xb8,0x66}};
    (void)iface;
    if (!clsid) return E_POINTER;
    *clsid = id;
    return S_OK;
}

static IPersistVtbl object_vtbl = {object_query, object_addref, object_release, object_class};
static IPersist object = {&object_vtbl};

static HRESULT WINAPI factory_query(IClassFactory *iface, REFIID iid, void **out)
{
    if (!out) return E_POINTER;
    *out = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IClassFactory)) return E_NOINTERFACE;
    *out = iface;
    IClassFactory_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI factory_addref(IClassFactory *iface) { (void)iface; return 2; }
static ULONG WINAPI factory_release(IClassFactory *iface) { (void)iface; return 1; }

static HRESULT WINAPI factory_create(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    (void)iface;
    if (!out) return E_POINTER;
    *out = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;
    return IPersist_QueryInterface(&object, iid, out);
}

static HRESULT WINAPI factory_lock(IClassFactory *iface, BOOL lock)
{
    (void)iface;
    (void)lock;
    return S_OK;
}

static IClassFactoryVtbl factory_vtbl =
{
    factory_query, factory_addref, factory_release, factory_create, factory_lock
};
static IClassFactory factory = {&factory_vtbl};

__declspec(dllexport) HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    (void)clsid;
    return IClassFactory_QueryInterface(&factory, iid, out);
}

__declspec(dllexport) HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
