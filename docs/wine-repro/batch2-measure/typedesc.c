/* Original, AI-assisted (Codex), LGPL-2.1-or-later. Public API MEASURE.
 * No Windows expectations: creation failures are observations, not passes. */
#define COBJMACROS
#include <windows.h>
#include <oleauto.h>
#include <stdio.h>
#include <string.h>

static unsigned errors;
static int result(const char *op, HRESULT hr)
{
    printf("API op=%s hr=%08lx\n", op, (unsigned long)hr);
    if (FAILED(hr)) errors++;
    return SUCCEEDED(hr);
}

static void describe(const TYPEDESC *td, unsigned depth)
{
    printf("DESC depth=%u vt=%04x\n", depth, td->vt);
    if (depth == 8) { puts("LIMIT depth=8"); return; }
    if (td->vt == VT_PTR || td->vt == VT_SAFEARRAY)
    {
        if (td->lptdesc) describe(td->lptdesc, depth + 1);
        else { puts("ERROR null_pointee"); errors++; }
    }
    else if (td->vt == VT_CARRAY)
    {
        const ARRAYDESC *a = td->lpadesc;
        if (!a) { puts("ERROR null_array"); errors++; return; }
        printf("ARRAY dims=%u\n", a->cDims);
        for (unsigned i = 0; i < a->cDims && i < 4; i++)
            printf("BOUND index=%u count=%lu low=%ld\n", i,
                   (unsigned long)a->rgbounds[i].cElements, (long)a->rgbounds[i].lLbound);
        describe(&a->tdescElem, depth + 1);
    }
}

static void library(const char *label, TYPEDESC *desc, unsigned count)
{
    WCHAR dir[MAX_PATH], path[MAX_PATH], name[40];
    ICreateTypeLib2 *create = NULL;
    ITypeLib *loaded = NULL;
    unsigned before = errors;
    if (!GetTempPathW(MAX_PATH, dir) || !GetTempFileNameW(dir, L"wbt", 0, path))
    { puts("ERROR tempfile"); errors++; return; }
    DeleteFileW(path);
    printf("CASE name=%s types=%u\n", label, count);
    if (!result("CreateTypeLib2", CreateTypeLib2(sizeof(void *) == 8 ? SYS_WIN64 : SYS_WIN32, path, &create))) goto done;
    if (!result("SetName", ICreateTypeLib2_SetName(create, L"WinumSyntheticTypes"))) goto done;
    for (unsigned i = 0; i < count; i++)
    {
        ICreateTypeInfo *type = NULL;
        HRESULT hr;
        swprintf(name, 40, L"Alias%u", i);
        hr = ICreateTypeLib2_CreateTypeInfo(create, name, TKIND_ALIAS, &type);
        if (FAILED(hr)) { result("CreateTypeInfo", hr); goto done; }
        if (count > 1)
        {
            /* Unique inner bounds prevent deduplication of the nested chain.
             * Both array-descriptor and type-descriptor tables grow. */
            ARRAYDESC inner = {0};
            ARRAYDESC outer = {0};
            TYPEDESC pointer;
            inner.tdescElem.vt = VT_I4;
            inner.cDims = outer.cDims = 1;
            inner.rgbounds[0].cElements = 2 + i;
            outer.rgbounds[0].cElements = 2;
            pointer.vt = VT_PTR;
            pointer.lptdesc = (TYPEDESC *)&(TYPEDESC){.lpadesc=&inner, .vt=VT_CARRAY};
            outer.tdescElem = pointer;
            TYPEDESC array = {.lpadesc=&outer, .vt=VT_CARRAY};
            TYPEDESC root = {.lptdesc=&array, .vt=VT_PTR};
            hr = ICreateTypeInfo_SetTypeDescAlias(type, &root);
        }
        else hr = ICreateTypeInfo_SetTypeDescAlias(type, desc);
        ICreateTypeInfo_Release(type);
        if (FAILED(hr)) { result("SetTypeDescAlias", hr); goto done; }
    }
    if (!result("SaveAllChanges", ICreateTypeLib2_SaveAllChanges(create))) goto done;
    ICreateTypeLib2_Release(create);
    create = NULL;
    if (!result("LoadTypeLibEx", LoadTypeLibEx(path, REGKIND_NONE, &loaded))) goto done;
    printf("LIB types=%u\n", ITypeLib_GetTypeInfoCount(loaded));
    for (unsigned i = 0; i < count; i++)
    {
        ITypeInfo *type = NULL;
        TYPEATTR *attr = NULL;
        if (count > 1 && i != 0 && i != 1500 && i != 3000 && i != count - 1) continue;
        printf("TYPE index=%u\n", i);
        if (!result("GetTypeInfo", ITypeLib_GetTypeInfo(loaded, i, &type))) continue;
        if (result("GetTypeAttr", ITypeInfo_GetTypeAttr(type, &attr)))
        {
            printf("ATTR kind=%u\n", (unsigned)attr->typekind);
            describe(&attr->tdescAlias, 0);
            ITypeInfo_ReleaseTypeAttr(type, attr);
        }
        ITypeInfo_Release(type);
    }
done:
    if (loaded) ITypeLib_Release(loaded);
    if (create) ICreateTypeLib2_Release(create);
    if (!DeleteFileW(path) && GetLastError() != ERROR_FILE_NOT_FOUND)
    { puts("ERROR cleanup"); errors++; }
    printf("CASE_END name=%s api_errors=%u\n", label, errors - before);
}

int main(void)
{
    TYPEDESC simple = {.vt=VT_I4}, encoded = {.vt=VT_ARRAY | VT_I4};
    TYPEDESC safe = {.lptdesc=&simple, .vt=VT_SAFEARRAY};
    TYPEDESC pointer = {.lptdesc=&simple, .vt=VT_PTR};
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("MEASURE schema=1 topic=typedesc bits=%u\n", (unsigned)(8 * sizeof(void *)));
    if (!result("CoInitializeEx", hr)) return 2;
    library("pointer_i4", &pointer, 1);
    pointer.lptdesc = &encoded;
    library("pointer_encoded_array_i4", &pointer, 1);
    pointer.lptdesc = &safe;
    library("pointer_safearray_i4", &pointer, 1);
    library("large_nested_arrays", NULL, 5000);
    CoUninitialize();
    printf("SUMMARY api_errors=%u expectations=none\n", errors);
    return errors ? 2 : 0;
}
