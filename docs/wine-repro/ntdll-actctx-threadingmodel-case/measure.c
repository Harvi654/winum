/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Observe our own activation contexts through public APIs, without loading
 * a COM server or CLR. No expected native threading-model values. */
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))
static const GUID class_id = {0x6e9bfe81, 0x6da1, 0x47a3,
                             {0x95, 0xcf, 0x32, 0x75, 0x18, 0xab, 0x0d, 0xe2}};
static const struct model_case { const char *label, *value; } cases[] = {
    {"omitted", NULL}, {"empty", ""}, {"unknown", "WinumInvalid"},
    {"apartment_title", "Apartment"}, {"apartment_lower", "apartment"},
    {"apartment_upper", "APARTMENT"}, {"apartment_mixed", "aPaRtMeNt"},
    {"free_title", "Free"}, {"free_lower", "free"},
    {"free_upper", "FREE"}, {"free_mixed", "fReE"},
    {"both_title", "Both"}, {"both_lower", "both"},
    {"both_upper", "BOTH"}, {"both_mixed", "bOtH"},
    {"neutral_title", "Neutral"}, {"neutral_lower", "neutral"},
    {"neutral_upper", "NEUTRAL"}, {"neutral_mixed", "nEuTrAl"},
    {"free_leading_space", " Free"}, {"free_trailing_space", "Free "}
};
static unsigned int attempted, created, rejected, queried, query_failed, setup_errors;

static void setup_error(const char *operation, DWORD error)
{
    printf("ERROR operation=%s error=%lu\n", operation, error);
    setup_errors++;
}

static BOOL measure_case(unsigned int index, BOOL managed)
{
    const struct model_case *test = &cases[index];
    const char *kind = managed ? "clrClass" : "comClass";
    WCHAR directory[MAX_PATH], path[MAX_PATH];
    char attribute[128], element[512], xml[1536];
    HANDLE file, context = INVALID_HANDLE_VALUE;
    ACTCTXW request = {0};
    ACTCTX_SECTION_KEYED_DATA data = {0};
    ULONG_PTR cookie = 0;
    DWORD length, written, error;
    BOOL active = FALSE, complete = TRUE, ret;
    int count;

    attempted++;
    printf("CASE kind=%s id=%s value=%s\n", kind, test->label,
           test->value ? test->value : "<omitted>");
    if (test->value) snprintf(attribute, sizeof(attribute), " threadingModel=\"%s\"", test->value);
    else attribute[0] = 0;
    if (managed)
        snprintf(element, sizeof(element), "<clrClass clsid=\"{6e9bfe81-6da1-47a3-95cf-327518ab0de2}\""
                 " name=\"Winum.Measure.Class\" runtimeVersion=\"v4.0.30319\"%s/>", attribute);
    else
        snprintf(element, sizeof(element), "<file name=\"winum-measure.dll\">"
                 "<comClass clsid=\"{6e9bfe81-6da1-47a3-95cf-327518ab0de2}\"%s/></file>", attribute);
    count = snprintf(xml, sizeof(xml), "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                     "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
                     "<assemblyIdentity name=\"Winum.Model.%u.%u\" type=\"win32\" version=\"1.0.0.0\"/>"
                     "%s</assembly>", (unsigned int)managed, index, element);
    if (count < 0 || (unsigned int)count >= sizeof(xml))
    {
        setup_error("format_manifest", ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    length = GetTempPathW(COUNT(directory), directory);
    if (!length || length >= COUNT(directory))
    {
        setup_error("GetTempPathW", length ? ERROR_INSUFFICIENT_BUFFER : GetLastError());
        return FALSE;
    }
    if (!GetTempFileNameW(directory, L"wac", 0, path))
    {
        setup_error("GetTempFileNameW", GetLastError());
        return FALSE;
    }
    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        setup_error("CreateFileW", GetLastError());
        complete = FALSE;
        goto done;
    }
    ret = WriteFile(file, xml, (DWORD)count, &written, NULL);
    error = ret ? ERROR_WRITE_FAULT : GetLastError();
    if (!ret || written != (DWORD)count)
    {
        setup_error("WriteFile", error);
        complete = FALSE;
    }
    if (!CloseHandle(file)) { setup_error("CloseHandle", GetLastError()); complete = FALSE; }
    if (!complete) goto done;
    request.cbSize = sizeof(request);
    request.lpSource = path;
    SetLastError(0);
    context = CreateActCtxW(&request);
    error = context == INVALID_HANDLE_VALUE ? GetLastError() : 0;
    printf("CREATE kind=%s id=%s success=%u error=%lu\n", kind, test->label,
           context != INVALID_HANDLE_VALUE, error);
    if (context == INVALID_HANDLE_VALUE) { rejected++; goto done; }
    created++;
    ret = ActivateActCtx(context, &cookie);
    if (!ret) { setup_error("ActivateActCtx", GetLastError()); complete = FALSE; goto done; }
    active = TRUE;
    data.cbSize = sizeof(data);
    SetLastError(0);
    ret = FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, &class_id, &data);
    error = ret ? 0 : GetLastError();
    printf("QUERY kind=%s id=%s success=%u error=%lu\n", kind, test->label, ret, error);
    if (!ret) query_failed++;
    else
    {
        /* Only the three-DWORD prefix of the public API's returned record.
         * Field layout is from open Wine 11.20 and its actctx tests, not
         * native code. Print it without presuming the model's enum value. */
        DWORD prefix[3];
        SIZE_T got = 0;
        queried++;
        if (!data.lpData || data.ulLength < sizeof(prefix) ||
            !ReadProcessMemory(GetCurrentProcess(), data.lpData, prefix, sizeof(prefix), &got) || got != sizeof(prefix))
        {
            setup_error("read_returned_record", ERROR_INVALID_DATA);
            complete = FALSE;
        }
        else printf("RECORD kind=%s id=%s format=%lu data_length=%lu size=%lu flags=%08lx model=%lu\n",
                    kind, test->label, data.ulDataFormatVersion, data.ulLength, prefix[0], prefix[1], prefix[2]);
    }
done:
    if (active && !DeactivateActCtx(0, cookie))
    {
        setup_error("DeactivateActCtx", GetLastError());
        complete = FALSE;
    }
    if (context != INVALID_HANDLE_VALUE) ReleaseActCtx(context);
    if (!DeleteFileW(path)) { setup_error("DeleteFileW", GetLastError()); complete = FALSE; }
    printf("CASE_END kind=%s id=%s complete=%u\n", kind, test->label, complete);
    return complete;
}

int main(void)
{
    unsigned int i, managed;
    BOOL complete = TRUE;
    setvbuf(stdout, NULL, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    printf("MEASURE topic=actctx_threadingmodel_case schema=1 bits=%u cases=%u\n",
           (unsigned int)(8 * sizeof(void *)), (unsigned int)(2 * COUNT(cases)));
    for (managed = 0; managed < 2 && complete; managed++)
        for (i = 0; i < COUNT(cases) && complete; i++) complete = measure_case(i, managed);
    printf("SUMMARY attempted=%u created=%u rejected=%u queried=%u query_failed=%u setup_errors=%u complete=%u expectations=none\n",
           attempted, created, rejected, queried, query_failed, setup_errors, complete);
    return setup_errors || !complete ? 1 : 0;
}
