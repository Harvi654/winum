/* Original MEASURE program, AI-assisted (Codex), LGPL-2.1-or-later. */
#define COBJMACROS
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <stdio.h>

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define TEST_ROOT L"Software\\winum-test"

static void print_variant(const VARIANT *v)
{
    VARTYPE type = V_VT(v);

    printf("%u:", type);
    switch (type)
    {
    case VT_EMPTY: printf("empty"); break;
    case VT_NULL: printf("null"); break;
    case VT_I2: printf("%d", V_I2(v)); break;
    case VT_I4: printf("%ld", V_I4(v)); break;
    case VT_UI4: printf("%lu", V_UI4(v)); break;
    case VT_INT: printf("%d", V_INT(v)); break;
    case VT_UINT: printf("%u", V_UINT(v)); break;
    case VT_BOOL: printf("%d", V_BOOL(v)); break;
    case VT_ERROR: printf("0x%08lx", (unsigned long)V_ERROR(v)); break;
    case VT_BSTR: printf("%ls", V_BSTR(v) ? V_BSTR(v) : L"<null>"); break;
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_VARIANT:
    {
        LONG i, lo, hi;
        SAFEARRAY *array = V_ARRAY(v);

        if (!array || SafeArrayGetDim(array) != 1 ||
            FAILED(SafeArrayGetLBound(array, 1, &lo)) ||
            FAILED(SafeArrayGetUBound(array, 1, &hi)))
        {
            printf("<unavailable>");
            break;
        }
        printf("[%ld..%ld]", lo, hi);
        for (i = lo; i <= hi; ++i)
        {
            VARIANT entry;
            HRESULT hr;

            VariantInit(&entry);
            if (type == (VT_ARRAY | VT_BSTR))
            {
                V_VT(&entry) = VT_BSTR;
                V_BSTR(&entry) = NULL;
                hr = SafeArrayGetElement(array, &i, &V_BSTR(&entry));
            }
            else hr = SafeArrayGetElement(array, &i, &entry);
            printf(" ");
            if (SUCCEEDED(hr)) print_variant(&entry);
            else printf("element_hr=0x%08lx", (unsigned long)hr);
            VariantClear(&entry);
        }
        break;
    }
    default: printf("<unprinted>"); break;
    }
}

enum argument { HIVE, KEY, NAME, STR, DWORD_VALUE, VARIANT_OUT, TYPED_OUT };

static void invoke(IDispatch *object, WORD flags, const char *scenario, WCHAR *method,
                   const WCHAR *key, UINT count, const enum argument *arguments)
{
    VARIANTARG args[5];
    VARIANT result, out, typed;
    DISPPARAMS params = {args, NULL, count, 0};
    EXCEPINFO ex = {0};
    UINT i, argerr = ~0u;
    DISPID id = DISPID_UNKNOWN;
    HRESULT name_hr, hr;
    int has_out = 0, has_typed = 0;
    const WCHAR *string;

    VariantInit(&result);
    VariantInit(&out);
    VariantInit(&typed);
    V_VT(&out) = VT_BSTR;
    V_BSTR(&out) = SysAllocString(L"out-sentinel");
    V_VT(&typed) = VT_BSTR;
    V_BSTR(&typed) = SysAllocString(L"out-sentinel");
    for (i = 0; i < count; ++i)
    {
        VARIANTARG *arg = &args[count - i - 1];

        VariantInit(arg);
        switch (arguments[i])
        {
        case HIVE:
            V_VT(arg) = VT_I4;
            V_I4(arg) = (LONG)0x80000001u;
            break;
        case DWORD_VALUE:
            V_VT(arg) = VT_I4;
            V_I4(arg) = 42;
            break;
        case VARIANT_OUT:
            has_out = 1;
            V_VT(arg) = VT_VARIANT | VT_BYREF;
            V_VARIANTREF(arg) = &out;
            break;
        case TYPED_OUT:
            has_typed = 1;
            V_VT(arg) = VT_BSTR | VT_BYREF;
            V_BSTRREF(arg) = &V_BSTR(&typed);
            break;
        default:
            string = arguments[i] == KEY ? key :
                     arguments[i] == NAME ? L"sample" : L"winum-value";
            V_VT(arg) = VT_BSTR;
            V_BSTR(arg) = SysAllocString(string);
            break;
        }
    }

    name_hr = IDispatch_GetIDsOfNames(object, &IID_NULL, &method, 1, LOCALE_USER_DEFAULT, &id);
    hr = name_hr;
    if (SUCCEEDED(name_hr))
        hr = IDispatch_Invoke(object, id, &IID_NULL, LOCALE_USER_DEFAULT, flags,
                              &params, &result, &ex, &argerr);
    if (ex.pfnDeferredFillIn) ex.pfnDeferredFillIn(&ex);
    printf("invoke flags=0x%x case=%s method=%ls argc=%u lookup=0x%08lx hr=0x%08lx"
           " argerr=%u excep=0x%08lx/%u result=",
           flags, scenario, method, count, (unsigned long)name_hr, (unsigned long)hr,
           argerr, (unsigned long)ex.scode, ex.wCode);
    print_variant(&result);
    if (has_out)
    {
        printf(" out_init=8:out-sentinel out=");
        print_variant(&out);
    }
    if (has_typed)
    {
        printf(" out_init=8:out-sentinel typed_out=");
        print_variant(&typed);
    }
    putchar('\n');
    SysFreeString(ex.bstrSource);
    SysFreeString(ex.bstrDescription);
    SysFreeString(ex.bstrHelpFile);
    for (i = 0; i < count; ++i) VariantClear(&args[i]);
    VariantClear(&typed);
    VariantClear(&out);
    VariantClear(&result);
}

static void measure(IDispatch *object, WORD flags, const WCHAR *root)
{
    static const enum argument key_args[] = {HIVE, KEY},
        set_string[] = {HIVE, KEY, NAME, STR}, get_string[] = {HIVE, KEY, NAME, VARIANT_OUT},
        get_typed[] = {HIVE, KEY, NAME, TYPED_OUT}, enum_key[] = {HIVE, KEY, VARIANT_OUT},
        set_dword[] = {HIVE, KEY, NAME, DWORD_VALUE}, extra[] = {HIVE, KEY, NAME, VARIANT_OUT, STR};
    WCHAR key[140], leaf[160], missing[160];

    swprintf(key, COUNT(key), L"%ls\\%ls", root, flags == DISPATCH_METHOD ? L"method" : L"both");
    swprintf(leaf, COUNT(leaf), L"%ls\\leaf", key);
    swprintf(missing, COUNT(missing), L"%ls\\missing", root);
    invoke(object, flags, "create", L"CreateKey", key, COUNT(key_args), key_args);
    invoke(object, flags, "create_leaf", L"CreateKey", leaf, COUNT(key_args), key_args);
    invoke(object, flags, "write_string", L"SetStringValue", key, COUNT(set_string), set_string);
    invoke(object, flags, "read_string", L"GetStringValue", key, COUNT(get_string), get_string);
    invoke(object, flags, "read_string_typed", L"GetStringValue", key, COUNT(get_typed), get_typed);
    invoke(object, flags, "enum", L"EnumKey", key, COUNT(enum_key), enum_key);
    invoke(object, flags, "read_no_out_string", L"GetStringValue", key, 3, get_string);
    invoke(object, flags, "read_extra_string", L"GetStringValue", key, COUNT(extra), extra);
    invoke(object, flags, "write_dword", L"SetDWORDValue", key, COUNT(set_dword), set_dword);
    invoke(object, flags, "read_missing", L"GetStringValue", missing, COUNT(get_string), get_string);
    invoke(object, flags, "create_no_args", L"CreateKey", key, 0, key_args);
    invoke(object, flags, "read_no_out", L"GetStringValue", key, 3, get_string);
    invoke(object, flags, "read_extra", L"GetStringValue", key, COUNT(extra), extra);
    invoke(object, flags, "delete_leaf", L"DeleteKey", leaf, COUNT(key_args), key_args);
    invoke(object, flags, "delete", L"DeleteKey", key, COUNT(key_args), key_args);
}

static void run_vbs(const WCHAR *name, const WCHAR *key)
{
    WCHAR system[MAX_PATH], cmd[512];
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;
    DWORD wait, code;
    UINT len = GetSystemDirectoryW(system, COUNT(system));

    if (!len || len >= COUNT(system) ||
        swprintf(cmd, COUNT(cmd), L"\"%ls\\cscript.exe\" //nologo \"%ls.vbs\" \"%ls\"",
                 system, name, key) < 0)
    {
        printf("cscript kind=%ls command_error\n", name);
        return;
    }
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    fflush(stdout);
    if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        printf("cscript kind=%ls create_error=%lu\n", name, GetLastError());
        return;
    }
    wait = WaitForSingleObject(pi.hProcess, 60000);
    if (wait != WAIT_OBJECT_0)
    {
        printf("cscript kind=%ls wait=0x%08lx\n", name, wait);
        if (TerminateProcess(pi.hProcess, 124)) WaitForSingleObject(pi.hProcess, 5000);
    }
    else if (GetExitCodeProcess(pi.hProcess, &code)) printf("cscript kind=%ls exit=%lu\n", name, code);
    else printf("cscript kind=%ls status_error=%lu\n", name, GetLastError());
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

int main(void)
{
    WCHAR display[] = L"winmgmts:{impersonationLevel=impersonate}!\\\\.\\root\\default:StdRegProv";
    WCHAR guid[40], key[100];
    HKEY parent, child;
    DWORD parent_disp, child_disp, subkeys, values;
    GUID id;
    LONG status;
    HRESULT hr;
    IBindCtx *ctx = NULL;
    IMoniker *moniker = NULL;
    IDispatch *object = NULL;
    ULONG eaten = 0;
    int bound;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("measure=v2 bits=%u\n", (unsigned int)(sizeof(void *) * 8));
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    printf("ole_init=0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 2;
    if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(&id, guid, COUNT(guid)))
    {
        CoUninitialize();
        return 2;
    }
    swprintf(key, COUNT(key), L"%ls\\%ls", TEST_ROOT, guid);
    status = RegCreateKeyExW(HKEY_CURRENT_USER, TEST_ROOT, 0, NULL, 0, KEY_READ | KEY_WRITE | DELETE,
                            NULL, &parent, &parent_disp);
    printf("scratch_parent=%ld\n", status);
    if (status != ERROR_SUCCESS)
    {
        CoUninitialize();
        return 2;
    }
    status = RegCreateKeyExW(parent, guid, 0, NULL, 0, KEY_WRITE, NULL, &child, &child_disp);
    printf("scratch_case=%ld\n", status);
    if (status != ERROR_SUCCESS || child_disp != REG_CREATED_NEW_KEY)
    {
        if (status == ERROR_SUCCESS) RegCloseKey(child);
        RegCloseKey(parent);
        CoUninitialize();
        return 2;
    }
    RegCloseKey(child);

    hr = CreateBindCtx(0, &ctx);
    printf("bind_context=0x%08lx\n", (unsigned long)hr);
    if (SUCCEEDED(hr))
    {
        hr = MkParseDisplayName(ctx, display, &eaten, &moniker);
        printf("parse_moniker=0x%08lx eaten=%lu\n", (unsigned long)hr, eaten);
        if (SUCCEEDED(hr))
        {
            hr = IMoniker_BindToObject(moniker, ctx, NULL, &IID_IDispatch, (void **)&object);
            printf("bind_dispatch=0x%08lx\n", (unsigned long)hr);
        }
    }
    if (!object)
    {
        hr = CoGetObject(display, NULL, &IID_IDispatch, (void **)&object);
        printf("cogetobject=0x%08lx\n", (unsigned long)hr);
    }
    bound = object != NULL;
    if (bound)
    {
        measure(object, DISPATCH_METHOD, key);
        measure(object, DISPATCH_METHOD | DISPATCH_PROPERTYGET, key);
        IDispatch_Release(object);
    }
    if (moniker) IMoniker_Release(moniker);
    if (ctx) IBindCtx_Release(ctx);
    run_vbs(L"direct", key);
    run_vbs(L"runtime-error", key);

    status = RegDeleteTreeW(parent, guid);
    printf("scratch_cleanup=%ld\n", status);
    if (parent_disp == REG_CREATED_NEW_KEY)
    {
        LONG info = RegQueryInfoKeyW(parent, NULL, NULL, NULL, &subkeys, NULL,
                                     NULL, &values, NULL, NULL, NULL, NULL);
        if (info == ERROR_SUCCESS && !subkeys && !values)
            printf("scratch_parent_cleanup=%ld\n", RegDeleteKeyW(HKEY_CURRENT_USER, TEST_ROOT));
        else printf("scratch_parent_retained query=%ld\n", info);
    }
    RegCloseKey(parent);
    CoUninitialize();
    printf("measure_complete dispatch=%d\n", bound);
    return status != ERROR_SUCCESS;
}
