/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Observe current timezone label sources and bias relationships, read-only. */
#define COBJMACROS
#include "../batch3-measure/measure-support.h"
#include <ole2.h>
#include <wbemidl.h>

struct label_source {
    const char *name;
    WCHAR text[512];
    size_t length;
    BOOL available;
};

static size_t bounded_length(const WCHAR *text, size_t capacity)
{
    size_t length;
    for(length = 0; length < capacity && text[length]; ++length);
    return length;
}

static void read_label_source(HKEY key, const WCHAR *value, BOOL mui, struct label_source *source)
{
    DWORD bytes = sizeof(source->text), type = 0;
    LONG result;
    memset(source->text, 0, sizeof(source->text));
    if(mui)
        result = RegLoadMUIStringW(key, value, source->text, sizeof(source->text), &bytes, 0, NULL);
    else
        result = RegQueryValueExW(key, value, NULL, &type, (BYTE *)source->text, &bytes);
    source->length = bounded_length(source->text, sizeof(source->text)/sizeof(source->text[0]));
    source->available = result == ERROR_SUCCESS
                        && (mui || type == REG_SZ || type == REG_EXPAND_SZ)
                        && source->length < sizeof(source->text)/sizeof(source->text[0]);
    printf("SOURCE name=%s result=%ld type=%lu available=%u length=%u\n",
           source->name, result, type, source->available,
           (unsigned int)(source->available ? source->length : 0));
    /* Optional source failures are observations, not guessed API contracts. */
}

static BOOL same_string(const VARIANT *value, const WCHAR *text, size_t length)
{
    return V_VT(value) == VT_BSTR && V_BSTR(value)
           && SysStringLen(V_BSTR(value)) == length
           && !memcmp(V_BSTR(value), text, length * sizeof(WCHAR));
}

static void inspect_labels(IWbemClassObject *object, const TIME_ZONE_INFORMATION *tz,
                           const struct label_source *sources)
{
    static const WCHAR *const names[] = {L"Caption", L"Description", L"StandardName", L"DaylightName"};
    VARIANT values[4];
    CIMTYPE types[4] = {0};
    unsigned int i, j;
    HRESULT hr;
    for(i = 0; i < 4; ++i) {
        VariantInit(&values[i]);
        hr = IWbemClassObject_Get(object, names[i], 0, &values[i], &types[i], NULL);
        printf("LABEL name=%ls hr=%08lx vt=%u cim=%lu present=%u length=%u\n",
               names[i], (DWORD)hr, V_VT(&values[i]), (DWORD)types[i],
               V_VT(&values[i]) == VT_BSTR && V_BSTR(&values[i]) != NULL,
               V_VT(&values[i]) == VT_BSTR && V_BSTR(&values[i]) ? SysStringLen(V_BSTR(&values[i])) : 0);
        if(FAILED(hr)) b3_error("GetTimezoneLabel", (DWORD)hr);
        for(j = 0; j < 4; ++j)
            printf("LABEL_SOURCE name=%ls source=%s available=%u equal=%u\n", names[i], sources[j].name,
                   sources[j].available, sources[j].available
                   && same_string(&values[i], sources[j].text, sources[j].length));
        printf("LABEL_API name=%ls equal_standard=%u equal_daylight=%u\n", names[i],
               same_string(&values[i], tz->StandardName, bounded_length(tz->StandardName, 32)),
               same_string(&values[i], tz->DaylightName, bounded_length(tz->DaylightName, 32)));
    }
    printf("LABEL_PAIR caption_description_equal=%u\n",
           V_VT(&values[1]) == VT_BSTR && V_BSTR(&values[1])
           && same_string(&values[0], V_BSTR(&values[1]), SysStringLen(V_BSTR(&values[1]))));
    for(i = 0; i < 4; ++i) VariantClear(&values[i]);
}

static void inspect_bias(IWbemClassObject *object, const TIME_ZONE_INFORMATION *tz, DWORD state)
{
    static const WCHAR *const names[] = {L"Bias", L"StandardBias", L"DaylightBias"};
    LONGLONG active = tz->Bias, value;
    unsigned int i;
    VARIANT data;
    CIMTYPE type;
    HRESULT hr;
    BOOL numeric;
    if(state == TIME_ZONE_ID_STANDARD) active += tz->StandardBias;
    if(state == TIME_ZONE_ID_DAYLIGHT) active += tz->DaylightBias;
    for(i = 0; i < 3; ++i) {
        VariantInit(&data);
        type = 0;
        value = 0;
        numeric = TRUE;
        hr = IWbemClassObject_Get(object, names[i], 0, &data, &type, NULL);
        if(V_VT(&data) == VT_I4) value = V_I4(&data);
        else if(V_VT(&data) == VT_UI4) value = V_UI4(&data);
        else numeric = FALSE;
        printf("BIAS name=%ls hr=%08lx vt=%u cim=%lu numeric=%u value=%lld\n",
               names[i], (DWORD)hr, V_VT(&data), (DWORD)type, numeric, value);
        if(FAILED(hr)) b3_error("GetTimezoneBias", (DWORD)hr);
        if(!i) printf("BIAS_RELATION numeric=%u equal_base=%u equal_negated_base=%u equal_negated_standard_total=%u equal_negated_active_total=%u base=%ld standard_adjust=%ld daylight_adjust=%ld active_total=%lld\n",
                      numeric, numeric && value == tz->Bias, numeric && value == -(LONGLONG)tz->Bias,
                      numeric && value == -((LONGLONG)tz->Bias + tz->StandardBias),
                      numeric && value == -active, tz->Bias, tz->StandardBias, tz->DaylightBias, active);
        VariantClear(&data);
    }
}

static void query_timezone(const TIME_ZONE_INFORMATION *tz, DWORD state, const struct label_source *sources)
{
    IWbemLocator *locator = NULL;
    IWbemServices *services = NULL;
    IEnumWbemClassObject *enumerator = NULL;
    IWbemClassObject *object = NULL;
    BSTR name = NULL, wql = NULL, query = NULL;
    ULONG got;
    HRESULT hr;
    if(!b3_hr(CoInitializeEx(NULL, COINIT_MULTITHREADED), "CoInitializeEx")) return;
    if(!b3_hr(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
              RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL), "CoInitializeSecurity")) goto done;
    if(!b3_hr(CoCreateInstance(&CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
              &IID_IWbemLocator, (void **)&locator), "CreateLocalWbemLocator")) goto done;
    if(!locator) { b3_error("MissingWbemLocator", ERROR_INVALID_DATA); goto done; }
    name = SysAllocString(L"ROOT\\CIMV2");
    wql = SysAllocString(L"WQL");
    query = SysAllocString(L"SELECT * FROM Win32_TimeZone");
    if(!name || !wql || !query) { b3_error("AllocateQuery", ERROR_OUTOFMEMORY); goto done; }
    if(!b3_hr(IWbemLocator_ConnectServer(locator, name, NULL, NULL, NULL, 0, NULL, NULL, &services),
              "ConnectLocalNamespace")) goto done;
    if(!services) { b3_error("MissingWbemServices", ERROR_INVALID_DATA); goto done; }
    if(!b3_hr(CoSetProxyBlanket((IUnknown *)services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
              NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE), "CoSetProxyBlanket")) goto done;
    if(!b3_hr(IWbemServices_ExecQuery(services, wql, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
              NULL, &enumerator), "QueryTimezone")) goto done;
    if(!enumerator) { b3_error("MissingTimezoneEnumerator", ERROR_INVALID_DATA); goto done; }
    got = 0;
    hr = IEnumWbemClassObject_Next(enumerator, 5000, 1, &object, &got);
    printf("NEXT phase=first hr=%08lx returned=%lu object=%u\n", (DWORD)hr, got, object != NULL);
    if(FAILED(hr) || hr == WBEM_S_TIMEDOUT || got != 1 || !object) {
        b3_error("FirstTimezoneObject", FAILED(hr) ? (DWORD)hr : ERROR_INVALID_DATA); goto done;
    }
    inspect_labels(object, tz, sources);
    inspect_bias(object, tz, state);
    IWbemClassObject_Release(object); object = NULL;
    got = 0;
    hr = IEnumWbemClassObject_Next(enumerator, 5000, 1, &object, &got);
    printf("NEXT phase=end hr=%08lx returned=%lu object=%u\n", (DWORD)hr, got, object != NULL);
    if(FAILED(hr) || hr == WBEM_S_TIMEDOUT || got || object)
        b3_error("TimezoneEnumerationEnd", FAILED(hr) ? (DWORD)hr : ERROR_INVALID_DATA);
done:
    if(object) IWbemClassObject_Release(object);
    if(enumerator) IEnumWbemClassObject_Release(enumerator);
    if(services) IWbemServices_Release(services);
    if(locator) IWbemLocator_Release(locator);
    SysFreeString(query); SysFreeString(wql); SysFreeString(name);
    CoUninitialize();
}

int main(void)
{
    TIME_ZONE_INFORMATION before = {0}, after = {0};
    DYNAMIC_TIME_ZONE_INFORMATION dynamic_before = {0}, dynamic_after = {0};
    struct label_source sources[4] = {
        {"display_raw", {0}, 0, FALSE}, {"display_resolved", {0}, 0, FALSE},
        {"mui_display_raw", {0}, 0, FALSE}, {"mui_display_resolved", {0}, 0, FALSE}
    };
    WCHAR path[256];
    HKEY key = NULL;
    LONG result;
    DWORD state, dynamic_state, final_state, final_dynamic_state;
    BOOL unchanged;
    int length;
    if(!b3_start("wbemprox_timezone_labels", 45000)) return 1;
    state = GetTimeZoneInformation(&before);
    dynamic_state = GetDynamicTimeZoneInformation(&dynamic_before);
    printf("TIMEZONE_API state=%lu dynamic_state=%lu key_present=%u dynamic_daylight_disabled=%u\n",
           state, dynamic_state, dynamic_before.TimeZoneKeyName[0] != 0, dynamic_before.DynamicDaylightTimeDisabled);
    if(state == TIME_ZONE_ID_INVALID || dynamic_state == TIME_ZONE_ID_INVALID) {
        b3_error("GetTimezoneSnapshot", ERROR_INVALID_DATA); return b3_finish();
    }
    if(!dynamic_before.TimeZoneKeyName[0]) { b3_error("NoCurrentTimezoneKey", ERROR_NOT_FOUND); return b3_finish(); }
    if(bounded_length(dynamic_before.TimeZoneKeyName, 128) == 128) {
        b3_error("UnterminatedTimezoneKey", ERROR_INVALID_DATA); return b3_finish();
    }
    length = swprintf(path, sizeof(path)/sizeof(path[0]),
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\%ls", dynamic_before.TimeZoneKeyName);
    if(length < 0 || (size_t)length >= sizeof(path)/sizeof(path[0])) {
        b3_error("TimezoneKeyLength", ERROR_BUFFER_OVERFLOW); return b3_finish();
    }
    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key);
    printf("REGISTRY current_timezone_open=%ld\n", result);
    if(result != ERROR_SUCCESS) { b3_error("OpenCurrentTimezoneKey", result); return b3_finish(); }
    read_label_source(key, L"Display", FALSE, &sources[0]);
    read_label_source(key, L"Display", TRUE, &sources[1]);
    read_label_source(key, L"MUI_Display", FALSE, &sources[2]);
    read_label_source(key, L"MUI_Display", TRUE, &sources[3]);
    result = RegCloseKey(key);
    if(result != ERROR_SUCCESS) b3_error("CloseTimezoneKey", result);
    if(!sources[0].available && !sources[1].available && !sources[2].available && !sources[3].available)
        b3_error("NoComparableLabelSource", ERROR_NOT_FOUND);
    query_timezone(&before, state, sources);
    final_state = GetTimeZoneInformation(&after);
    final_dynamic_state = GetDynamicTimeZoneInformation(&dynamic_after);
    unchanged = state == final_state && dynamic_state == final_dynamic_state
                && !memcmp(&before, &after, sizeof(before))
                && !memcmp(&dynamic_before, &dynamic_after, sizeof(dynamic_before));
    printf("SNAPSHOT state=%lu dynamic_state=%lu unchanged=%u\n", final_state, final_dynamic_state, unchanged);
    if(!unchanged) b3_error("TimezoneChangedDuringObservation", ERROR_INVALID_DATA);
    return b3_finish();
}
