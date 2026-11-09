/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Windows MEASURE for a reader-only offset fixture. No expected HRESULTs. */
#define COBJMACROS
#include <windows.h>
#include <oleauto.h>
#include <stdio.h>
#include "fixture.h"
#ifndef OFFSETS_BOUNDARY
#define OFFSETS_BOUNDARY 0x8000
#endif
static unsigned errors;
static int api(const char *name, HRESULT hr)
{
    printf("API name=%s hr=%08lx\n", name, (unsigned long)hr);
    if (FAILED(hr)) errors++;
    return SUCCEEDED(hr);
}
static void describe(const TYPEDESC *td, unsigned depth)
{
    printf("DESC depth=%u vt=%04x\n", depth, td->vt);
    if (depth >= 8) { puts("LIMIT depth=8"); return; }
    if (td->vt == VT_PTR || td->vt == VT_SAFEARRAY)
    {
        if (td->lptdesc) describe(td->lptdesc, depth + 1);
        else { puts("ERROR null_pointee"); errors++; }
    }
    if (td->vt == VT_CARRAY)
    {
        const ARRAYDESC *a = td->lpadesc;
        if (!a) { puts("ERROR null_array"); errors++; return; }
        printf("ARRAY depth=%u dims=%u\n", depth, a->cDims);
        for (unsigned i = 0; i < a->cDims && i < 4; i++)
            printf("BOUND index=%u count=%lu low=%ld\n", i,
                   (unsigned long)a->rgbounds[i].cElements, (long)a->rgbounds[i].lLbound);
        describe(&a->tdescElem, depth + 1);
    }
}
int main(void)
{
    WCHAR directory[MAX_PATH], seed_path[MAX_PATH] = {0}, fixture_path[MAX_PATH] = {0};
    ICreateTypeLib2 *create = NULL;
    ICreateTypeInfo *type = NULL;
    ITypeLib *lib = NULL;
    ITypeInfo *info = NULL;
    TYPEATTR *attr = NULL;
    TYPEDESC simple = {.vt=VT_I4}, pointer = {.lptdesc=&simple, .vt=VT_PTR};
    HANDLE file = INVALID_HANDLE_VALUE;
    unsigned char *seed = NULL, *fixture = NULL;
    size_t fixture_size = 0;
    DWORD length, size, done;
    HRESULT hr;
    BOOL ret, seed_owned = FALSE, fixture_owned = FALSE;

    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("MEASURE topic=typedesc_reader_offsets schema=1 bits=%u boundary=%x\n",
           (unsigned)(8 * sizeof(void *)), OFFSETS_BOUNDARY);
    if (!api("CoInitializeEx", CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) return 2;
    length = GetTempPathW(MAX_PATH, directory);
    if (!length || length >= MAX_PATH || !GetTempFileNameW(directory, L"wzs", 0, seed_path)) goto error;
    seed_owned = TRUE;
    if (!GetTempFileNameW(directory, L"wzf", 0, fixture_path)) goto error;
    fixture_owned = TRUE;
    if (!DeleteFileW(seed_path)) goto error;
    if (!api("CreateTypeLib2", CreateTypeLib2(sizeof(void *) == 8 ? SYS_WIN64 : SYS_WIN32, seed_path, &create))) goto cleanup;
    if (!api("SetName", ICreateTypeLib2_SetName(create, L"WinumOffsetSeed"))) goto cleanup;
    if (!api("CreateTypeInfo", ICreateTypeLib2_CreateTypeInfo(create, L"Alias0", TKIND_ALIAS, &type))) goto cleanup;
    if (!api("SetTypeDescAlias", ICreateTypeInfo_SetTypeDescAlias(type, &pointer))) goto cleanup;
    ICreateTypeInfo_Release(type); type = NULL;
    if (!api("SaveAllChanges", ICreateTypeLib2_SaveAllChanges(create))) goto cleanup;
    ICreateTypeLib2_Release(create); create = NULL;
    file = CreateFileW(seed_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) goto error;
    size = GetFileSize(file, NULL);
    if (!size || size > 1024 * 1024) goto error;
    seed = malloc(size);
    if (!seed || !ReadFile(file, seed, size, &done, NULL) || done != size) goto error;
    CloseHandle(file); file = INVALID_HANDLE_VALUE;
    if (!offsets_fixture(seed, size, OFFSETS_BOUNDARY, &fixture, &fixture_size))
    { puts("ERROR unsupported_generated_seed"); errors++; goto cleanup; }
    file = CreateFileW(fixture_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (file == INVALID_HANDLE_VALUE || fixture_size > MAXDWORD ||
        !WriteFile(file, fixture, (DWORD)fixture_size, &done, NULL) || done != fixture_size ||
        !FlushFileBuffers(file)) goto error;
    CloseHandle(file); file = INVALID_HANDLE_VALUE;
    printf("FIXTURE root_ref=%x outer_array_ref=%x element_ref=%x inner_ref=%x inner_array_ref=%x\n",
           OFFSETS_BOUNDARY, OFFSETS_BOUNDARY, OFFSETS_BOUNDARY + 8,
           OFFSETS_BOUNDARY + 16, OFFSETS_BOUNDARY + 16);
    hr = LoadTypeLibEx(fixture_path, REGKIND_NONE, &lib);
    if (!api("LoadTypeLibEx", hr)) goto cleanup;
    printf("LIB types=%u\n", ITypeLib_GetTypeInfoCount(lib));
    if (!api("GetTypeInfo", ITypeLib_GetTypeInfo(lib, 0, &info))) goto cleanup;
    if (!api("GetTypeAttr", ITypeInfo_GetTypeAttr(info, &attr))) goto cleanup;
    printf("ATTR kind=%u\n", (unsigned)attr->typekind);
    if (attr->typekind == TKIND_ALIAS) describe(&attr->tdescAlias, 0);
    goto cleanup;
error:
    printf("ERROR fixture_setup code=%lu\n", (unsigned long)GetLastError()); errors++;
cleanup:
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (attr) ITypeInfo_ReleaseTypeAttr(info, attr);
    if (info) ITypeInfo_Release(info);
    if (lib) ITypeLib_Release(lib);
    if (type) ICreateTypeInfo_Release(type);
    if (create) ICreateTypeLib2_Release(create);
    free(seed); free(fixture);
    if (seed_owned) { ret = DeleteFileW(seed_path); if (!ret && GetLastError() != ERROR_FILE_NOT_FOUND) errors++; }
    if (fixture_owned) { ret = DeleteFileW(fixture_path); if (!ret && GetLastError() != ERROR_FILE_NOT_FOUND) errors++; }
    CoUninitialize();
    printf("SUMMARY api_errors=%u expectations=none\n", errors);
    return errors ? 2 : 0;
}
