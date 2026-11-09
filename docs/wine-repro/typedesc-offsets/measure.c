/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * MEASURE only: flat CARRAYs referencing generated records through PTRs.
 * Uses normal native serialization, no file modification or nested CARRAY. */
#define COBJMACROS
#include <windows.h>
#include <oleauto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RECORDS 5000
static unsigned errors;
static unsigned setup_calls, setup_non_s_ok;
static int api(const char *name, HRESULT hr)
{
    printf("API name=%s hr=%08lx\n", name, (unsigned long)hr);
    if (FAILED(hr)) errors++;
    return SUCCEEDED(hr);
}
static int step(const char *name, unsigned index, HRESULT hr)
{
    setup_calls++;
    if (hr == S_OK) return 1;
    setup_non_s_ok++;
    printf("API_SETUP name=%s index=%u hr=%08lx\n", name, index, (unsigned long)hr);
    if (SUCCEEDED(hr)) return 1;
    errors++;
    return 0;
}
static DWORD read32(const BYTE *p)
{
    return (DWORD)p[0] | (DWORD)p[1] << 8 | (DWORD)p[2] << 16 | (DWORD)p[3] << 24;
}
static int span(DWORD start, DWORD length, DWORD size)
{
    return start <= size && length <= size - start;
}
static int audit(const WCHAR *path)
{
    HANDLE file = INVALID_HANDLE_VALUE;
    BYTE *data = NULL;
    DWORD size, got, count, dir, desc, desc_size, arrays, array_size;
    DWORD signed_refs[3] = {0}, high_refs[3] = {0};
    int success = 0;
    file = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) goto done;
    size = GetFileSize(file, NULL);
    if (size < 0x54 || size > 32 * 1024 * 1024) goto done;
    data = malloc(size);
    if (!data || !ReadFile(file, data, size, &got, NULL) || got != size || memcmp(data,"MSFT",4)) goto done;
    count = read32(data + 0x20);
    if (count > (size - 0x54) / 4) goto done;
    dir = 0x54 + count * 4 + ((read32(data + 0x14) & 0x100) ? 4 : 0);
    if (!span(dir, 15 * 16, size)) goto done;
    desc = read32(data + dir + 9 * 16); desc_size = read32(data + dir + 9 * 16 + 4);
    arrays = read32(data + dir + 10 * 16); array_size = read32(data + dir + 10 * 16 + 4);
    if (!span(desc, desc_size, size) || desc_size % 8 || !span(arrays, array_size, size)) goto done;
    for (DWORD i = 0; i < desc_size; i += 8)
    {
        DWORD vt = read32(data + desc + i) & 0xffff, ref = read32(data + desc + i + 4);
        if ((vt == VT_PTR || vt == VT_SAFEARRAY) && !(ref & 0x80000000))
        {
            if (ref % 8 || !span(ref, 8, desc_size)) goto done;
            signed_refs[0] += !!(ref & 0x8000); high_refs[0] += ref >= 0x10000;
        }
        if (vt == VT_CARRAY)
        {
            DWORD element;
            if (!span(ref, 8, array_size)) goto done;
            signed_refs[1] += !!(ref & 0x8000); high_refs[1] += ref >= 0x10000;
            element = read32(data + arrays + ref);
            if (!(element & 0x80000000))
            {
                if (element % 8 || !span(element, 8, desc_size)) goto done;
                signed_refs[2] += !!(element & 0x8000); high_refs[2] += element >= 0x10000;
            }
        }
    }
    printf("SERIAL types=%lu desc_bytes=%lu array_bytes=%lu\n", (unsigned long)count, (unsigned long)desc_size, (unsigned long)array_size);
    for (unsigned i = 0; i < 3; i++)
        printf("COVERAGE path=%u low_bit15=%lu high_word=%lu\n", i, (unsigned long)signed_refs[i], (unsigned long)high_refs[i]);
    printf("COVERAGE complete=%u\n", !!(signed_refs[0] && high_refs[0] &&
           signed_refs[1] && high_refs[1] && signed_refs[2] && high_refs[2]));
    success = 1;
done:
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    free(data);
    if (!success) { puts("ERROR own_file_audit"); errors++; }
    return success;
}
static void describe(ITypeInfo *owner, const TYPEDESC *td, unsigned depth)
{
    printf("DESC depth=%u vt=%04x\n", depth, td->vt);
    if (depth >= 8) { puts("LIMIT depth=8"); return; }
    if (td->vt == VT_PTR || td->vt == VT_SAFEARRAY)
    {
        if (td->lptdesc) describe(owner, td->lptdesc, depth + 1);
        else { puts("ERROR null_pointee"); errors++; }
    }
    else if (td->vt == VT_CARRAY)
    {
        const ARRAYDESC *a = td->lpadesc;
        if (!a) { puts("ERROR null_array"); errors++; return; }
        printf("ARRAY depth=%u dims=%u\n", depth, a->cDims);
        for (unsigned i=0; i<a->cDims && i<4; i++)
            printf("BOUND index=%u count=%lu low=%ld\n", i, (unsigned long)a->rgbounds[i].cElements, (long)a->rgbounds[i].lLbound);
        describe(owner, &a->tdescElem, depth + 1);
    }
    else if (td->vt == VT_USERDEFINED)
    {
        ITypeInfo *record = NULL;
        TYPEATTR *attr = NULL;
        VARDESC *var = NULL;
        BSTR name = NULL;
        char text[80] = {0};
        printf("REF href=%08lx\n", (unsigned long)td->hreftype);
        if (!api("GetRefTypeInfo", ITypeInfo_GetRefTypeInfo(owner, td->hreftype, &record))) return;
        if (api("GetRefTypeAttr", ITypeInfo_GetTypeAttr(record, &attr)))
        {
            printf("RECORD kind=%u vars=%u size=%lu\n", (unsigned)attr->typekind, attr->cVars, (unsigned long)attr->cbSizeInstance);
            puts("CLEANUP before=record_ReleaseTypeAttr"); ITypeInfo_ReleaseTypeAttr(record, attr); puts("CLEANUP after=record_ReleaseTypeAttr");
        }
        if (api("GetVarDesc", ITypeInfo_GetVarDesc(record, 0, &var)))
        {
            printf("FIELD kind=%u vt=%04x\n", (unsigned)var->varkind, var->elemdescVar.tdesc.vt);
            puts("CLEANUP before=ReleaseVarDesc"); ITypeInfo_ReleaseVarDesc(record, var); puts("CLEANUP after=ReleaseVarDesc");
        }
        if (api("GetDocumentation", ITypeInfo_GetDocumentation(record, MEMBERID_NIL, &name, NULL, NULL, NULL)))
        {
            if (name && WideCharToMultiByte(CP_UTF8,0,name,-1,text,sizeof(text),NULL,NULL)) printf("RECORD name=%s\n",text);
            SysFreeString(name);
        }
        puts("CLEANUP before=record_Release"); ITypeInfo_Release(record); puts("CLEANUP after=record_Release");
    }
}
int main(void)
{
    WCHAR directory[MAX_PATH], path[MAX_PATH] = {0}, fresh[MAX_PATH] = {0}, name[40];
    ICreateTypeLib2 *create = NULL;
    ICreateTypeInfo *record = NULL, *alias = NULL;
    ITypeInfo *record_info = NULL, *info = NULL;
    ITypeLib *lib = NULL;
    TYPEATTR *attr = NULL;
    BOOL path_owned = FALSE, fresh_owned = FALSE;
    DWORD length;
    const unsigned samples[] = {0,1500,3000,4999};
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    setvbuf(stdout,NULL,_IONBF,0);
    printf("MEASURE topic=typedesc_record_offsets schema=1 bits=%u\n",(unsigned)(8*sizeof(void*)));
    if (!api("CoInitializeEx",CoInitializeEx(NULL,COINIT_APARTMENTTHREADED))) return 2;
    length=GetTempPathW(MAX_PATH,directory);
    if (!length || length>=MAX_PATH || !GetTempFileNameW(directory,L"wtr",0,path)) goto setup_error;
    path_owned=TRUE;
    if (!GetTempFileNameW(directory,L"wtl",0,fresh)) goto setup_error;
    fresh_owned=TRUE;
    if (!DeleteFileW(path)) goto setup_error;
    if (!api("CreateTypeLib2",CreateTypeLib2(sizeof(void*)==8?SYS_WIN64:SYS_WIN32,path,&create))) goto cleanup;
    if (!api("SetName",ICreateTypeLib2_SetName(create,L"WinumRecordOffsets"))) goto cleanup;
    for (unsigned i=0;i<RECORDS;i++)
    {
        VARDESC var = {0};
        HREFTYPE href;
        TYPEDESC user, pointer, array, root;
        ARRAYDESC bound = {0};
        swprintf(name,40,L"Record%u",i);
        if (!step("CreateRecord",i,ICreateTypeLib2_CreateTypeInfo(create,name,TKIND_RECORD,&record))) goto cleanup;
        var.memid=MEMBERID_NIL; var.varkind=VAR_PERINSTANCE; var.elemdescVar.tdesc.vt=VT_I4;
        if (!step("AddVarDesc",i,ICreateTypeInfo_AddVarDesc(record,0,&var))) goto cleanup;
        if (!step("SetVarName",i,ICreateTypeInfo_SetVarName(record,0,L"value"))) goto cleanup;
        if (!step("LayOut",i,ICreateTypeInfo_LayOut(record))) goto cleanup;
        if (!step("GetRecordInfo",i,ICreateTypeInfo_QueryInterface(record,&IID_ITypeInfo,(void**)&record_info))) goto cleanup;
        swprintf(name,40,L"Alias%u",i);
        if (!step("CreateAlias",i,ICreateTypeLib2_CreateTypeInfo(create,name,TKIND_ALIAS,&alias))) goto cleanup;
        if (!step("AddRefTypeInfo",i,ICreateTypeInfo_AddRefTypeInfo(alias,record_info,&href))) goto cleanup;
        user.vt=VT_USERDEFINED; user.hreftype=href;
        pointer.vt=VT_PTR; pointer.lptdesc=&user;
        bound.tdescElem=pointer; bound.cDims=1; bound.rgbounds[0].cElements=i+2;
        array.vt=VT_CARRAY; array.lpadesc=&bound;
        root.vt=VT_PTR; root.lptdesc=&array;
        if (!step("SetTypeDescAlias",i,ICreateTypeInfo_SetTypeDescAlias(alias,&root))) goto cleanup;
        ICreateTypeInfo_Release(alias); alias=NULL;
        ITypeInfo_Release(record_info); record_info=NULL;
        ICreateTypeInfo_Release(record); record=NULL;
    }
    printf("BUILD records=%u aliases=%u setup_calls=%u setup_non_s_ok=%u\n",
           RECORDS,RECORDS,setup_calls,setup_non_s_ok);
    if (!api("SaveAllChanges",ICreateTypeLib2_SaveAllChanges(create))) goto cleanup;
    puts("CLEANUP before=writer_Release"); ICreateTypeLib2_Release(create); create=NULL; puts("CLEANUP after=writer_Release");
    if (!CopyFileW(path,fresh,FALSE)) goto setup_error;
    if (!audit(fresh)) goto cleanup;
    if (!api("LoadTypeLibEx",LoadTypeLibEx(fresh,REGKIND_NONE,&lib))) goto cleanup;
    printf("LIB types=%u\n",ITypeLib_GetTypeInfoCount(lib));
    for (unsigned i=0;i<sizeof(samples)/sizeof(samples[0]);i++)
    {
        unsigned index=2*samples[i]+1;
        printf("TYPE sample=%u index=%u\n",samples[i],index);
        if (!api("GetTypeInfo",ITypeLib_GetTypeInfo(lib,index,&info))) continue;
        if (api("GetTypeAttr",ITypeInfo_GetTypeAttr(info,&attr)))
        {
            printf("ATTR kind=%u\n",(unsigned)attr->typekind);
            if (attr->typekind==TKIND_ALIAS) describe(info,&attr->tdescAlias,0);
            else { puts("ERROR unexpected_fixture_type"); errors++; }
            puts("CLEANUP before=alias_ReleaseTypeAttr"); ITypeInfo_ReleaseTypeAttr(info,attr); attr=NULL; puts("CLEANUP after=alias_ReleaseTypeAttr");
        }
        puts("CLEANUP before=alias_Release"); ITypeInfo_Release(info); info=NULL; puts("CLEANUP after=alias_Release");
    }
    goto cleanup;
setup_error:
    printf("ERROR fixture_setup code=%lu\n",(unsigned long)GetLastError()); errors++;
cleanup:
    if (attr) ITypeInfo_ReleaseTypeAttr(info,attr);
    if (info) ITypeInfo_Release(info);
    if (lib) { puts("CLEANUP before=reader_Release"); ITypeLib_Release(lib); puts("CLEANUP after=reader_Release"); }
    if (alias) ICreateTypeInfo_Release(alias);
    if (record_info) ITypeInfo_Release(record_info);
    if (record) ICreateTypeInfo_Release(record);
    if (create) ICreateTypeLib2_Release(create);
    if (path_owned && !DeleteFileW(path) && GetLastError()!=ERROR_FILE_NOT_FOUND) errors++;
    if (fresh_owned && !DeleteFileW(fresh) && GetLastError()!=ERROR_FILE_NOT_FOUND) errors++;
    puts("CLEANUP before=CoUninitialize"); CoUninitialize(); puts("CLEANUP after=CoUninitialize");
    printf("SUMMARY api_errors=%u expectations=none\n",errors);
    return errors?2:0;
}
