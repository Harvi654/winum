/* BEGIN winum actctx model test: original, AI-assisted (Codex), LGPL-2.1-or-later. */
static void test_com_threading_model_case(void)
{
    static const GUID clsid = {0x6e9bfe81, 0x6da1, 0x47a3,
                              {0x95, 0xcf, 0x32, 0x75, 0x18, 0xab, 0x0d, 0xe2}};
    /* Windows 10 x86_64/i686 MEASURE: zero denotes manifest rejection.
     * An omitted model is 3 for comClass and 4 for clrClass. */
    static const struct { const char *label, *value; DWORD model; } tests[] = {
        {"omitted", NULL, 3}, {"empty", "", 0}, {"unknown", "WinumInvalid", 0},
        {"apartment_title", "Apartment", 1}, {"apartment_lower", "apartment", 1},
        {"apartment_upper", "APARTMENT", 1}, {"apartment_mixed", "aPaRtMeNt", 1},
        {"free_title", "Free", 2}, {"free_lower", "free", 2},
        {"free_upper", "FREE", 2}, {"free_mixed", "fReE", 2},
        {"both_title", "Both", 4}, {"both_lower", "both", 4},
        {"both_upper", "BOTH", 4}, {"both_mixed", "bOtH", 4},
        {"neutral_title", "Neutral", 5}, {"neutral_lower", "neutral", 5},
        {"neutral_upper", "NEUTRAL", 5}, {"neutral_mixed", "nEuTrAl", 5},
        {"free_leading_space", " Free", 0}, {"free_trailing_space", "Free ", 0}
    };
    WCHAR directory[MAX_PATH], path[MAX_PATH];
    char attribute[128], element[512], xml[1536];
    unsigned int i, managed;
    DWORD length;

    length = GetTempPathW(ARRAY_SIZE(directory), directory);
    ok(length && length < ARRAY_SIZE(directory), "Temporary directory failed.\n");
    if (!length || length >= ARRAY_SIZE(directory)) return;
    for (managed = 0; managed < 2; managed++)
    {
        for (i = 0; i < ARRAY_SIZE(tests); i++)
        {
            ACTCTX_SECTION_KEYED_DATA data = {0};
            ACTCTXW request = {0};
            HANDLE file, context = INVALID_HANDLE_VALUE;
            ULONG_PTR cookie = 0;
            SIZE_T got = 0;
            DWORD written, error, prefix[3], model;
            BOOL ret, active = FALSE, complete = TRUE;
            int count;

            if (tests[i].value)
                snprintf(attribute, sizeof(attribute), " threadingModel=\"%s\"", tests[i].value);
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
                             "%s</assembly>", managed, i, element);
            ok(count > 0 && (unsigned int)count < sizeof(xml), "Manifest formatting failed.\n");
            if (count <= 0 || (unsigned int)count >= sizeof(xml)) return;
            ret = GetTempFileNameW(directory, L"wac", 0, path) != 0;
            ok(ret, "Temporary manifest filename failed.\n");
            if (!ret) return;
            file = CreateFileW(path, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            ok(file != INVALID_HANDLE_VALUE, "Manifest file open failed.\n");
            if (file == INVALID_HANDLE_VALUE) goto done;
            ret = WriteFile(file, xml, count, &written, NULL);
            ok(ret && written == (DWORD)count, "Manifest write failed.\n");
            if (!ret || written != (DWORD)count) complete = FALSE;
            ret = CloseHandle(file);
            ok(ret, "Manifest file close failed.\n");
            if (!ret || !complete) goto done;
            request.cbSize = sizeof(request);
            request.lpSource = path;
            SetLastError(0);
            context = CreateActCtxW(&request);
            error = GetLastError();
            if (!tests[i].model)
            {
                /* Rejection of invalid values is outside the case-folding fix. */
                todo_wine ok(context == INVALID_HANDLE_VALUE, "Kind %u %s was accepted.\n", managed, tests[i].label);
                if (context == INVALID_HANDLE_VALUE)
                    ok(error == ERROR_SXS_CANT_GEN_ACTCTX, "Kind %u %s error %lu.\n", managed, tests[i].label, error);
                goto done;
            }
            ok(context != INVALID_HANDLE_VALUE, "Kind %u %s create error %lu.\n", managed, tests[i].label, error);
            if (context == INVALID_HANDLE_VALUE) goto done;
            ret = ActivateActCtx(context, &cookie);
            ok(ret, "Kind %u %s activation failed.\n", managed, tests[i].label);
            if (!ret) goto done;
            active = TRUE;
            data.cbSize = sizeof(data);
            ret = FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, &clsid, &data);
            ok(ret, "Kind %u %s query failed.\n", managed, tests[i].label);
            if (!ret) goto done;
            ok(data.ulDataFormatVersion == 1, "Kind %u %s format %lu.\n", managed, tests[i].label, data.ulDataFormatVersion);
            ok(data.lpData && data.ulLength >= sizeof(prefix), "Kind %u %s record is too short.\n", managed, tests[i].label);
            if (!data.lpData || data.ulLength < sizeof(prefix)) goto done;
            /* Public query result: size/flags/model prefix as in open Wine's
             * actctx tests. Copy no more than the returned record length. */
            ret = ReadProcessMemory(GetCurrentProcess(), data.lpData, prefix, sizeof(prefix), &got);
            ok(ret && got == sizeof(prefix), "Kind %u %s record unreadable.\n", managed, tests[i].label);
            if (!ret || got != sizeof(prefix)) goto done;
            ok(prefix[0] == 120, "Kind %u %s record size %lu.\n", managed, tests[i].label, prefix[0]);
            ok(!prefix[1], "Kind %u %s record flags %#lx.\n", managed, tests[i].label, prefix[1]);
            model = !tests[i].value && managed ? 4 : tests[i].model;
            /* Missing-attribute defaults are separate Wine discrepancies. */
            todo_wine_if(!tests[i].value)
                ok(prefix[2] == model, "Kind %u %s model %lu, expected %lu.\n", managed, tests[i].label, prefix[2], model);
        done:
            if (active)
            {
                ret = DeactivateActCtx(0, cookie);
                ok(ret, "Activation context deactivation failed.\n");
                if (!ret)
                {
                    /* Do not continue with an unexpected active context. */
                    ReleaseActCtx(context);
                    ret = DeleteFileW(path);
                    ok(ret, "Manifest cleanup failed.\n");
                    return;
                }
            }
            if (context != INVALID_HANDLE_VALUE) ReleaseActCtx(context);
            ret = DeleteFileW(path);
            ok(ret, "Manifest cleanup failed.\n");
        }
    }
}
/* END winum actctx model test */
