/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Public signature/PE-normalization observations on original data, not PA19 decoding. */
#include "../batch3-measure/measure-support.h"
#include <patchapi.h>
#include <stdlib.h>
typedef BOOL (WINAPI *signature_fn)(PBYTE,ULONG,ULONG,PVOID,ULONG,PPATCH_IGNORE_RANGE,ULONG,PPATCH_RETAIN_RANGE,ULONG,LPSTR);
typedef INT (WINAPI *normalize_fn)(PVOID,ULONG,ULONG,PATCH_OPTION_DATA *,ULONG,ULONG,ULONG,PPATCH_IGNORE_RANGE,ULONG,PPATCH_RETAIN_RANGE);
static signature_fn signature;
static normalize_fn normalize;
static void make_pe32(BYTE *data)
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
static ULONG changed(const BYTE *a,const BYTE *b,ULONG size)
{
    ULONG i,count=0;
    for(i=0;i<size;i++) if(a[i]!=b[i]) count++;
    return count;
}
static void observe(const char *label,const BYTE *original,ULONG size,ULONG flags)
{
    BYTE *copy = malloc(size);
    char text[96];
    BOOL ret;
    DWORD error;
    ULONG i;
    size_t length;
    if(!copy) { b3_error("AllocateCopy",ERROR_OUTOFMEMORY); return; }
    memcpy(copy,original,size);
    memset(text,0xcc,sizeof(text));
    SetLastError(0xdeadbeef);
    ret = signature(copy,size,flags,NULL,0,NULL,0,NULL,sizeof(text),text);
    error = GetLastError();
    for(length=0;length<sizeof(text) && text[length];length++);
    printf("SIGNATURE case=%s size=%lu flags=%08lx result=%u error=%08lx terminated=%u length=%u changed=%lu\n",
           label,size,flags,ret,error,length<sizeof(text),(unsigned int)length,changed(original,copy,size));
    if(ret && length<sizeof(text)) {
        BOOL hex=TRUE;
        for(i=0;i<length;i++) if(!((text[i]>='0'&&text[i]<='9')||(text[i]>='a'&&text[i]<='f')||(text[i]>='A'&&text[i]<='F'))) hex=FALSE;
        if(hex) printf("SIGNATURE_HEX case=%s flags=%08lx value=%s\n",label,flags,text);
        else printf("SIGNATURE_HEX case=%s flags=%08lx valid_hex=0\n",label,flags);
    }
    memcpy(copy,original,size);
    SetLastError(0xdeadbeef);
    {
        INT result=normalize(copy,size,flags,NULL,0x400000,0,0,NULL,0,NULL);
        error=GetLastError();
        printf("NORMALIZE case=%s flags=%08lx result=%d error=%08lx changed=%lu\n",label,flags,result,error,changed(original,copy,size));
    }
    free(copy);
}
int main(void)
{
    HMODULE module;
    BYTE *data;
    ULONG i,size=9*1024*1024+17;
    union { FARPROC raw; signature_fn call; } sig;
    union { FARPROC raw; normalize_fn call; } norm;
    if(!b3_start("mspatcha_signature_normalize",30000)) return 1;
    module=LoadLibraryW(L"mspatcha.dll");
    if(!module) { b3_error("LoadLibraryW",GetLastError()); return b3_finish(); }
    sig.raw=GetProcAddress(module,"GetFilePatchSignatureByBuffer"); signature=sig.call;
    norm.raw=GetProcAddress(module,"NormalizeFileForPatchSignature"); normalize=norm.call;
    printf("EXPORT signature=%u normalize=%u\n",signature!=NULL,normalize!=NULL);
    if(!signature||!normalize) { b3_error("GetOwnProbeExports",ERROR_PROC_NOT_FOUND); goto done; }
    data=malloc(size);
    if(!data) { b3_error("AllocateFixture",ERROR_OUTOFMEMORY); goto done; }
    for(i=0;i<size;i++) data[i]=(BYTE)((i*37u+11u)&255);
    observe("raw-small",data,1024,0);
    observe("raw-small",data,1024,PATCH_OPTION_SIGNATURE_MD5);
    observe("raw-large",data,size,0);
    make_pe32(data);
    observe("pe32",data,1024,0);
    observe("pe32",data,1024,PATCH_OPTION_NO_BINDFIX|PATCH_OPTION_NO_REBASE|PATCH_OPTION_NO_CHECKSUM|PATCH_OPTION_NO_RESTIMEFIX|PATCH_OPTION_NO_TIMESTAMP);
    printf("CASES completed=5 pa19_decode_covered=0\n");
    free(data);
done:
    FreeLibrary(module);
    return b3_finish();
}
