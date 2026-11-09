/* BEGIN winum typedesc offsets test: original, AI-assisted (Codex), LGPL-2.1-or-later. */
static DWORD offsets_dword(const BYTE *p)
{
    return (DWORD)p[0] | (DWORD)p[1] << 8 | (DWORD)p[2] << 16 | (DWORD)p[3] << 24;
}

static BOOL offsets_region(DWORD start, DWORD length, DWORD size)
{
    return start <= size && length <= size - start;
}

/* Inspect only our own generated file, using the open Wine MSFT layout.
 * Require low-word sign-bit and high-word references in all three paths. */
static void check_offsets_coverage(const WCHAR *path)
{
    HANDLE file;
    BYTE *data = NULL;
    DWORD size, read, count, dir, desc, desc_size, arrays, array_size, mask = 0, i;
    BOOL ret;

    file = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Coverage file open failed.\n");
    if (file == INVALID_HANDLE_VALUE) return;
    size = GetFileSize(file, NULL);
    if (size < 0x54 || size > 32 * 1024 * 1024) goto invalid;
    data = malloc(size);
    if (!data) goto invalid;
    ret = ReadFile(file, data, size, &read, NULL);
    if (!ret || read != size || memcmp(data, "MSFT", 4)) goto invalid;
    count = offsets_dword(data + 0x20);
    if (count != 5000) goto invalid;
    dir = 0x54 + 4 * count + ((offsets_dword(data + 0x14) & 0x100) ? 4 : 0);
    if (!offsets_region(dir, 15 * 16, size)) goto invalid;
    desc = offsets_dword(data + dir + 9 * 16);
    desc_size = offsets_dword(data + dir + 9 * 16 + 4);
    arrays = offsets_dword(data + dir + 10 * 16);
    array_size = offsets_dword(data + dir + 10 * 16 + 4);
    if (!offsets_region(desc, desc_size, size) || desc_size % 8 ||
        !offsets_region(arrays, array_size, size)) goto invalid;
    for (i = 0; i < desc_size; i += 8)
    {
        DWORD vt = offsets_dword(data + desc + i) & 0xffff;
        DWORD ref = offsets_dword(data + desc + i + 4), element;
        if ((vt == VT_PTR || vt == VT_SAFEARRAY) && !(ref & 0x80000000))
        {
            if (ref % 8 || !offsets_region(ref, 8, desc_size)) goto invalid;
            if (ref & 0x8000) mask |= 1;
            if (ref >= 0x10000) mask |= 2;
        }
        if (vt != VT_CARRAY) continue;
        if (!offsets_region(ref, 8, array_size)) goto invalid;
        if (ref & 0x8000) mask |= 4;
        if (ref >= 0x10000) mask |= 8;
        element = offsets_dword(data + arrays + ref);
        if (element & 0x80000000) continue;
        if (element % 8 || !offsets_region(element, 8, desc_size)) goto invalid;
        if (element & 0x8000) mask |= 16;
        if (element >= 0x10000) mask |= 32;
    }
    ok(mask == 63, "Missing boundary coverage, mask %#lx.\n", mask);
    goto done;
invalid:
    ok(0, "Unexpected generated MSFT layout or failed coverage read.\n");
done:
    free(data);
    CloseHandle(file);
}

static void check_offsets_alias(ITypeLib *lib, UINT index)
{
    ITypeInfo *info = NULL;
    TYPEATTR *attr = NULL;
    const TYPEDESC *td;
    const ARRAYDESC *array;
    HRESULT hr;
    unsigned int depth;

    hr = ITypeLib_GetTypeInfo(lib, index, &info);
    ok(hr == S_OK, "Type %u GetTypeInfo returned %#lx.\n", index, hr);
    if (FAILED(hr)) return;
    hr = ITypeInfo_GetTypeAttr(info, &attr);
    ok(hr == S_OK, "Type %u GetTypeAttr returned %#lx.\n", index, hr);
    if (FAILED(hr)) goto done;
    ok(attr->typekind == TKIND_ALIAS, "Type %u kind %u.\n", index, attr->typekind);
    td = &attr->tdescAlias;
    for (depth = 0; depth < 4; depth++)
    {
        VARTYPE expected = depth % 2 ? VT_CARRAY : VT_PTR;
        ok(td->vt == expected, "Type %u depth %u vt %#x, expected %#x.\n", index, depth, td->vt, expected);
        if (td->vt != expected) goto done;
        if (expected == VT_PTR)
        {
            ok(td->lptdesc != NULL, "Type %u null pointee.\n", index);
            if (!(td = td->lptdesc)) goto done;
        }
        else
        {
            array = td->lpadesc;
            ok(array != NULL, "Type %u null array.\n", index);
            if (!array) goto done;
            ok(array->cDims == 1, "Type %u dimension count %u.\n", index, array->cDims);
            if (array->cDims != 1) goto done;
            ok(array->rgbounds[0].lLbound == 0, "Type %u lower bound %ld.\n", index, array->rgbounds[0].lLbound);
            ok(array->rgbounds[0].cElements == (depth == 1 ? 2 : index + 2),
               "Type %u depth %u bound %lu.\n", index, depth, array->rgbounds[0].cElements);
            td = &array->tdescElem;
        }
    }
    ok(td->vt == VT_I4, "Type %u final vt %#x.\n", index, td->vt);
done:
    if (attr) ITypeInfo_ReleaseTypeAttr(info, attr);
    ITypeInfo_Release(info);
}

static void test_large_typedesc_offsets(void)
{
    static const UINT samples[] = {0, 1500, 3000, 4999};
    WCHAR directory[MAX_PATH], path[MAX_PATH] = {0}, name[40];
    ICreateTypeLib2 *create = NULL;
    ICreateTypeInfo *type = NULL;
    ITypeLib *lib = NULL;
    HRESULT hr;
    DWORD length;
    UINT i;
    BOOL ret;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(SUCCEEDED(hr), "CoInitializeEx returned %#lx.\n", hr);
    if (FAILED(hr)) return;
    length = GetTempPathW(ARRAY_SIZE(directory), directory);
    ok(length && length < ARRAY_SIZE(directory), "Temporary directory failed.\n");
    if (!length || length >= ARRAY_SIZE(directory)) goto done;
    ret = GetTempFileNameW(directory, L"wzo", 0, path) != 0;
    ok(ret, "Temporary file failed.\n");
    if (!ret) goto done;
    ret = DeleteFileW(path);
    ok(ret, "Initial placeholder cleanup failed.\n");
    if (!ret) goto done;
    hr = CreateTypeLib2(sizeof(void *) == 8 ? SYS_WIN64 : SYS_WIN32, path, &create);
    ok(hr == S_OK, "CreateTypeLib2 returned %#lx.\n", hr);
    if (FAILED(hr)) goto done;
    hr = ICreateTypeLib2_SetName(create, L"WinumSyntheticTypes");
    ok(hr == S_OK, "SetName returned %#lx.\n", hr);
    if (FAILED(hr)) goto done;
    for (i = 0; i < 5000; i++)
    {
        ARRAYDESC inner = {0}, outer = {0};
        TYPEDESC inner_array, inner_pointer, outer_array, root;
        inner.tdescElem.vt = VT_I4;
        inner.cDims = outer.cDims = 1;
        inner.rgbounds[0].cElements = i + 2;
        outer.rgbounds[0].cElements = 2;
        inner_array.vt = VT_CARRAY;
        inner_array.lpadesc = &inner;
        inner_pointer.vt = VT_PTR;
        inner_pointer.lptdesc = &inner_array;
        outer.tdescElem = inner_pointer;
        outer_array.vt = VT_CARRAY;
        outer_array.lpadesc = &outer;
        root.vt = VT_PTR;
        root.lptdesc = &outer_array;
        swprintf(name, ARRAY_SIZE(name), L"Alias%u", i);
        hr = ICreateTypeLib2_CreateTypeInfo(create, name, TKIND_ALIAS, &type);
        ok(hr == S_OK, "CreateTypeInfo %u returned %#lx.\n", i, hr);
        if (FAILED(hr)) goto done;
        hr = ICreateTypeInfo_SetTypeDescAlias(type, &root);
        ok(hr == S_OK, "SetTypeDescAlias %u returned %#lx.\n", i, hr);
        ICreateTypeInfo_Release(type);
        type = NULL;
        if (FAILED(hr)) goto done;
    }
    hr = ICreateTypeLib2_SaveAllChanges(create);
    ok(hr == S_OK, "SaveAllChanges returned %#lx.\n", hr);
    if (FAILED(hr)) goto done;
    ICreateTypeLib2_Release(create);
    create = NULL;
    check_offsets_coverage(path);
    hr = LoadTypeLibEx(path, REGKIND_NONE, &lib);
    ok(hr == S_OK, "LoadTypeLibEx returned %#lx.\n", hr);
    if (FAILED(hr)) goto done;
    ok(ITypeLib_GetTypeInfoCount(lib) == 5000, "Unexpected loaded type count.\n");
    for (i = 0; i < ARRAY_SIZE(samples); i++) check_offsets_alias(lib, samples[i]);
done:
    if (type) ICreateTypeInfo_Release(type);
    if (create) ICreateTypeLib2_Release(create);
    if (lib) ITypeLib_Release(lib);
    if (path[0]) { ret = DeleteFileW(path); ok(ret, "Temporary library cleanup failed.\n"); }
    CoUninitialize();
}
/* END winum typedesc offsets test */
