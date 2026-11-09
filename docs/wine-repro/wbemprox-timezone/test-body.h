/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Oracles: own Windows 10 measurements, x86_64/i686, 2026-11-09.
 * Value comparisons are limited to the measured standard-time conditions. */
static BOOL timezone_hr(HRESULT hr, const char *operation)
{
    ok(hr == S_OK, "%s returned %#lx\n", operation, hr);
    return hr == S_OK;
}

static size_t timezone_length(const WCHAR *text, size_t capacity)
{
    size_t length;
    for(length = 0; length < capacity && text[length]; ++length);
    return length;
}

static BOOL timezone_display(const DYNAMIC_TIME_ZONE_INFORMATION *tz, WCHAR *display)
{
    WCHAR path[256];
    HKEY key;
    DWORD bytes = 512 * sizeof(WCHAR), type = 0;
    LONG result;
    int length;

    if(!tz->TimeZoneKeyName[0] || timezone_length(tz->TimeZoneKeyName, 128) == 128) return FALSE;
    length = swprintf(path, ARRAY_SIZE(path),
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\%ls", tz->TimeZoneKeyName);
    if(length < 0 || (size_t)length >= ARRAY_SIZE(path)) return FALSE;
    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key);
    if(result != ERROR_SUCCESS) return FALSE;
    memset(display, 0, 512 * sizeof(WCHAR));
    result = RegQueryValueExW(key, L"Display", NULL, &type, (BYTE *)display, &bytes);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && type == REG_SZ && bytes >= sizeof(WCHAR)
           && bytes <= 512 * sizeof(WCHAR) && !(bytes % sizeof(WCHAR))
           && timezone_length(display, bytes / sizeof(WCHAR)) < bytes / sizeof(WCHAR);
}

static void timezone_number(IWbemClassObject *object, const WCHAR *name, CIMTYPE expected_type,
                            VARTYPE expected_variant, LONGLONG expected, BOOL compare)
{
    VARIANT value;
    CIMTYPE type = 0;
    LONGLONG number;
    HRESULT hr;

    VariantInit(&value);
    hr = IWbemClassObject_Get(object, name, 0, &value, &type, NULL);
    ok(hr == S_OK, "%ls: Get returned %#lx\n", name, hr);
    if(hr == S_OK)
    {
        ok(type == expected_type, "%ls: CIM type %lu, expected %lu\n", name, (DWORD)type, (DWORD)expected_type);
        ok(V_VT(&value) == expected_variant, "%ls: variant type %u, expected %u\n", name, V_VT(&value), expected_variant);
        if(compare && V_VT(&value) == expected_variant)
        {
            number = expected_variant == VT_UI1 ? V_UI1(&value) : V_I4(&value);
            ok(number == expected, "%ls: numeric value differs from the measured Win32 relation\n", name);
        }
    }
    VariantClear(&value);
}

static void timezone_string(IWbemClassObject *object, const WCHAR *name, const WCHAR *expected)
{
    VARIANT value;
    CIMTYPE type = 0;
    HRESULT hr;

    VariantInit(&value);
    hr = IWbemClassObject_Get(object, name, 0, &value, &type, NULL);
    ok(hr == S_OK, "%ls: Get returned %#lx\n", name, hr);
    if(hr == S_OK)
    {
        ok(type == CIM_STRING, "%ls: CIM type %lu, expected string\n", name, (DWORD)type);
        ok(V_VT(&value) == VT_BSTR, "%ls: variant type %u, expected BSTR\n", name, V_VT(&value));
        if(V_VT(&value) == VT_BSTR)
        {
            ok(V_BSTR(&value) != NULL, "%ls: missing string\n", name);
            if(expected && V_BSTR(&value))
                ok(SysStringLen(V_BSTR(&value)) == wcslen(expected)
                   && !memcmp(V_BSTR(&value), expected, wcslen(expected) * sizeof(WCHAR)),
                   "%ls: string differs from the measured public-API source\n", name);
        }
    }
    VariantClear(&value);
}

static void timezone_row(IWbemClassObject *object, const TIME_ZONE_INFORMATION *tz,
                         const WCHAR *display, BOOL compare)
{
    timezone_number(object, L"Bias", CIM_SINT32, VT_I4, -(LONGLONG)tz->Bias, compare);
    timezone_number(object, L"StandardBias", CIM_UINT32, VT_I4, tz->StandardBias, compare);
    timezone_number(object, L"DaylightBias", CIM_SINT32, VT_I4, tz->DaylightBias, compare);
#define TIMEZONE_DATE(prefix, field) \
    timezone_number(object, L##prefix L"Day", CIM_UINT32, VT_I4, tz->field.wDay, compare); \
    timezone_number(object, L##prefix L"DayOfWeek", CIM_UINT8, VT_UI1, tz->field.wDayOfWeek, compare); \
    timezone_number(object, L##prefix L"Hour", CIM_UINT32, VT_I4, tz->field.wHour, compare); \
    timezone_number(object, L##prefix L"Minute", CIM_UINT32, VT_I4, tz->field.wMinute, compare); \
    timezone_number(object, L##prefix L"Month", CIM_UINT32, VT_I4, tz->field.wMonth, compare); \
    timezone_number(object, L##prefix L"Second", CIM_UINT32, VT_I4, tz->field.wSecond, compare); \
    timezone_number(object, L##prefix L"Year", CIM_UINT32, VT_I4, tz->field.wYear, compare)
    TIMEZONE_DATE("Standard", StandardDate);
    TIMEZONE_DATE("Daylight", DaylightDate);
#undef TIMEZONE_DATE
    timezone_string(object, L"StandardName", compare ? tz->StandardName : NULL);
    timezone_string(object, L"DaylightName", compare ? tz->DaylightName : NULL);
    timezone_string(object, L"Caption", compare ? display : NULL);
    timezone_string(object, L"Description", compare ? display : NULL);
}

static void test_current_timezone(void)
{
    TIME_ZONE_INFORMATION before = {0}, after = {0};
    DYNAMIC_TIME_ZONE_INFORMATION dynamic_before = {0}, dynamic_after = {0};
    IWbemLocator *locator = NULL;
    IWbemServices *services = NULL;
    IEnumWbemClassObject *enumerator = NULL;
    IWbemClassObject *object = NULL;
    BSTR name = NULL, wql = NULL, query = NULL;
    WCHAR display[512];
    DWORD state, dynamic_state, final_state, final_dynamic_state;
    ULONG got;
    HRESULT hr;
    BOOL comparable, have_display;

    state = GetTimeZoneInformation(&before);
    dynamic_state = GetDynamicTimeZoneInformation(&dynamic_before);
    ok(state != TIME_ZONE_ID_INVALID, "Failed to get Win32 timezone\n");
    ok(dynamic_state != TIME_ZONE_ID_INVALID, "Failed to get dynamic timezone\n");
    if(state == TIME_ZONE_ID_INVALID || dynamic_state == TIME_ZONE_ID_INVALID) return;
    have_display = timezone_display(&dynamic_before, display);
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if(!timezone_hr(hr, "CoInitializeEx"))
    {
        if(SUCCEEDED(hr)) CoUninitialize();
        return;
    }
    if(!timezone_hr(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                     RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL), "CoInitializeSecurity")) goto done;
    hr = CoCreateInstance(&CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator);
    if(!timezone_hr(hr, "CoCreateInstance")) goto done;
    ok(locator != NULL, "Missing locator\n");
    if(!locator) goto done;
    name = SysAllocString(L"ROOT\\CIMV2");
    wql = SysAllocString(L"WQL");
    query = SysAllocString(L"SELECT * FROM Win32_TimeZone");
    ok(name && wql && query, "Query allocation failed\n");
    if(!name || !wql || !query) goto done;
    hr = IWbemLocator_ConnectServer(locator, name, NULL, NULL, NULL, 0, NULL, NULL, &services);
    if(!timezone_hr(hr, "ConnectServer")) goto done;
    ok(services != NULL, "Missing services\n");
    if(!services) goto done;
    hr = CoSetProxyBlanket((IUnknown *)services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                          RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if(!timezone_hr(hr, "CoSetProxyBlanket")) goto done;
    hr = IWbemServices_ExecQuery(services, wql, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                NULL, &enumerator);
    if(!timezone_hr(hr, "ExecQuery")) goto done;
    ok(enumerator != NULL, "Missing enumerator\n");
    if(!enumerator) goto done;
    got = 0;
    hr = IEnumWbemClassObject_Next(enumerator, 5000, 1, &object, &got);
    ok(hr == S_OK, "First Next returned %#lx\n", hr);
    ok(got == 1 && object != NULL, "Expected one timezone object, count %lu\n", got);
    if(hr != S_OK || got != 1 || !object) goto done;

    final_state = GetTimeZoneInformation(&after);
    final_dynamic_state = GetDynamicTimeZoneInformation(&dynamic_after);
    comparable = state == final_state && dynamic_state == final_dynamic_state
                 && !memcmp(&before, &after, sizeof(before))
                 && !memcmp(&dynamic_before, &dynamic_after, sizeof(dynamic_before));
    /* The native sample cannot distinguish base from active bias outside
     * standard time with zero standard adjustment. Never guess those values. */
    comparable = comparable && state == TIME_ZONE_ID_STANDARD && state == dynamic_state
                 && !dynamic_before.DynamicDaylightTimeDisabled && before.StandardBias == 0;
    if(!comparable) win_skip("Only property types/presence checked outside measured stable standard-time conditions\n");
    if(comparable && !have_display) win_skip("Display source unavailable; label equality is not measured for this setup\n");
    timezone_row(object, &before, have_display ? display : NULL, comparable);
    IWbemClassObject_Release(object); object = NULL;
    got = 0;
    hr = IEnumWbemClassObject_Next(enumerator, 5000, 1, &object, &got);
    ok(hr == S_FALSE, "Final Next returned %#lx\n", hr);
    ok(!got && !object, "Expected end of timezone enumeration, count %lu\n", got);

done:
    if(object) IWbemClassObject_Release(object);
    if(enumerator) IEnumWbemClassObject_Release(enumerator);
    if(services) IWbemServices_Release(services);
    if(locator) IWbemLocator_Release(locator);
    SysFreeString(query); SysFreeString(wql); SysFreeString(name);
    CoUninitialize();
}
