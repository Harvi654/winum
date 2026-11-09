/* Original test, AI-assisted (Codex), LGPL-2.1-or-later.
 * Expected values: own Windows 10 MEASURE, 2026-11-09, x86_64 and i686.
 * This tests signatures/normalization only, never PA19 apply or LZXD. */
typedef BOOL (WINAPI *signature_buffer_fn)(PBYTE,ULONG,ULONG,PVOID,ULONG,PPATCH_IGNORE_RANGE,ULONG,PPATCH_RETAIN_RANGE,ULONG,LPSTR);
typedef INT (WINAPI *signature_normalize_fn)(PVOID,ULONG,ULONG,PATCH_OPTION_DATA *,ULONG,ULONG,ULONG,PPATCH_IGNORE_RANGE,ULONG,PPATCH_RETAIN_RANGE);

static void signature_make_pe32(BYTE *data)
{
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)data;
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(data + 0x80);
    IMAGE_SECTION_HEADER *section;
    memset(data,0,1024);
    dos->e_magic = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = 0x80;
    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
    nt->FileHeader.NumberOfSections = 1;
    nt->FileHeader.TimeDateStamp = 0x12345678;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(nt->OptionalHeader);
    nt->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_RELOCS_STRIPPED;
    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    nt->OptionalHeader.AddressOfEntryPoint = 0x1000;
    nt->OptionalHeader.BaseOfCode = 0x1000;
    nt->OptionalHeader.ImageBase = 0x400000;
    nt->OptionalHeader.SectionAlignment = 0x1000;
    nt->OptionalHeader.FileAlignment = 0x200;
    nt->OptionalHeader.MajorOperatingSystemVersion = 6;
    nt->OptionalHeader.MajorSubsystemVersion = 6;
    nt->OptionalHeader.SizeOfImage = 0x2000;
    nt->OptionalHeader.SizeOfHeaders = 0x200;
    nt->OptionalHeader.SizeOfCode = 0x200;
    nt->OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    nt->OptionalHeader.SizeOfStackReserve = 0x100000;
    nt->OptionalHeader.SizeOfStackCommit = 0x1000;
    nt->OptionalHeader.SizeOfHeapReserve = 0x100000;
    nt->OptionalHeader.SizeOfHeapCommit = 0x1000;
    nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    section = IMAGE_FIRST_SECTION(nt);
    memcpy(section->Name,".text",5);
    section->Misc.VirtualSize = 1;
    section->VirtualAddress = 0x1000;
    section->SizeOfRawData = 0x200;
    section->PointerToRawData = 0x200;
    section->Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
    data[0x200] = 0xc3; /* Own inert RET byte; this file is never loaded or executed. */
}
static ULONG signature_changed(const BYTE *a,const BYTE *b,ULONG size)
{
    ULONG i,count=0;
    for(i=0;i<size;i++) if(a[i]!=b[i]) count++;
    return count;
}

static void signature_check_case(signature_buffer_fn signature, signature_normalize_fn normalize,
                                 const char *label, const BYTE *original, ULONG size,
                                 ULONG flags, const char *expected, ULONG expected_changes)
{
    BYTE *copy = malloc(size);
    char text[96];
    ULONG changes;
    size_t length;
    DWORD error;
    BOOL success;
    INT result;

    ok(copy != NULL, "%s: allocation failed\n", label);
    if(!copy) return;
    memcpy(copy, original, size);
    memset(text, 0xcc, sizeof(text));
    SetLastError(0xdeadbeef);
    success = signature(copy, size, flags, NULL, 0, NULL, 0, NULL, sizeof(text), text);
    error = GetLastError();
    ok(success, "%s: signature failed, error %#lx\n", label, error);
    ok(error == 0xdeadbeef, "%s: signature error %#lx\n", label, error);
    for(length = 0; length < sizeof(text) && text[length]; ++length);
    ok(length < sizeof(text), "%s: signature is not terminated\n", label);
    ok(length == strlen(expected) && !memcmp(text, expected, strlen(expected) + 1),
       "%s: signature differs from the native result\n", label);
    changes = signature_changed(original, copy, size);
    ok(changes == expected_changes, "%s: signature changed %lu bytes, expected %lu\n",
       label, changes, expected_changes);

    /* Separate fresh input: Normalize's supplied base/time differ from the
     * signature API's internal normalization request. Do not equate them. */
    memcpy(copy, original, size);
    SetLastError(0xdeadbeef);
    result = normalize(copy, size, flags, NULL, 0x400000, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(result == 1, "%s: Normalize result %d, expected 1\n", label, result);
    ok(error == 0xdeadbeef, "%s: Normalize error %#lx\n", label, error);
    ok(!memcmp(copy, original, size), "%s: Normalize changed the fresh input\n", label);
    free(copy);
}

static void test_buffer_signatures(void)
{
    HMODULE module;
    BYTE *data;
    ULONG i, size = 9 * 1024 * 1024 + 17;
    union { FARPROC raw; signature_buffer_fn call; } signature;
    union { FARPROC raw; signature_normalize_fn call; } normalize;

    module = LoadLibraryW(L"mspatcha.dll");
    ok(module != NULL, "Failed to load mspatcha, error %#lx\n", GetLastError());
    if(!module) return;
    signature.raw = GetProcAddress(module, "GetFilePatchSignatureByBuffer");
    normalize.raw = GetProcAddress(module, "NormalizeFileForPatchSignature");
    ok(signature.raw != NULL, "Missing signature export\n");
    ok(normalize.raw != NULL, "Missing Normalize export\n");
    if(signature.raw && normalize.raw)
    {
        data = malloc(size);
        ok(data != NULL, "Fixture allocation failed\n");
        if(data)
        {
            for(i = 0; i < size; ++i) data[i] = (BYTE)((i * 37u + 11u) & 255);
            signature_check_case(signature.call, normalize.call, "raw-small", data, 1024,
                                 0, "6fea9368", 0);
            signature_check_case(signature.call, normalize.call, "raw-small-md5", data, 1024,
                                 PATCH_OPTION_SIGNATURE_MD5, "d4883a545d61557ce12ff4e95350e997", 0);
            signature_check_case(signature.call, normalize.call, "raw-large", data, size,
                                 0, "af841acf", 0);
            signature_make_pe32(data);
            signature_check_case(signature.call, normalize.call, "pe32", data, 1024,
                                 0, "864575c5", 6);
            signature_check_case(signature.call, normalize.call, "pe32-no-fixups", data, 1024,
                                 PATCH_OPTION_NO_BINDFIX | PATCH_OPTION_NO_REBASE | PATCH_OPTION_NO_CHECKSUM
                                 | PATCH_OPTION_NO_RESTIMEFIX | PATCH_OPTION_NO_TIMESTAMP, "e2fe48a8", 0);
            free(data);
        }
    }
    ok(FreeLibrary(module), "Failed to release mspatcha\n");
}
