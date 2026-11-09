/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Local WMI timezone properties compared with the public Win32 snapshot. */
#define COBJMACROS
#include "../batch3-measure/measure-support.h"
#include <ole2.h>
#include <wbemidl.h>
static void number(IWbemClassObject *object,const WCHAR *name,LONGLONG reference)
{
    VARIANT value;
    CIMTYPE type=0;
    HRESULT hr;
    LONGLONG n=0;
    BOOL numeric=TRUE;
    VariantInit(&value);
    hr=IWbemClassObject_Get(object,name,0,&value,&type,NULL);
    switch(V_VT(&value)) {
    case VT_I2:n=V_I2(&value);break;
    case VT_I4:n=V_I4(&value);break;
    case VT_UI1:n=V_UI1(&value);break;
    case VT_UI4:n=V_UI4(&value);break;
    default:numeric=FALSE;break;
    }
    printf("NUMBER name=%ls hr=%08lx vt=%u cim=%lu numeric=%u value=%lld api=%lld equal_api=%u\n",
           name,(DWORD)hr,V_VT(&value),(DWORD)type,numeric,n,reference,numeric&&n==reference);
    VariantClear(&value);
}
static void string_value(IWbemClassObject *object,const WCHAR *name,const WCHAR *reference)
{
    VARIANT value;
    CIMTYPE type=0;
    HRESULT hr;
    BOOL present;
    VariantInit(&value);
    hr=IWbemClassObject_Get(object,name,0,&value,&type,NULL);
    present=V_VT(&value)==VT_BSTR && V_BSTR(&value)!=NULL;
    printf("STRING name=%ls hr=%08lx vt=%u cim=%lu present=%u length=%u equal_api=%u\n",
           name,(DWORD)hr,V_VT(&value),(DWORD)type,present,present?SysStringLen(V_BSTR(&value)):0,
           present&&reference&&!lstrcmpW(V_BSTR(&value),reference));
    VariantClear(&value);
}
static void inspect(IWbemClassObject *object,const TIME_ZONE_INFORMATION *tz)
{
    number(object,L"Bias",tz->Bias);
    number(object,L"DaylightBias",tz->DaylightBias);
    number(object,L"StandardBias",tz->StandardBias);
#define DATE_FIELDS(prefix,field) \
    number(object,L##prefix L"Day",tz->field.wDay); \
    number(object,L##prefix L"DayOfWeek",tz->field.wDayOfWeek); \
    number(object,L##prefix L"Hour",tz->field.wHour); \
    number(object,L##prefix L"Minute",tz->field.wMinute); \
    number(object,L##prefix L"Month",tz->field.wMonth); \
    number(object,L##prefix L"Second",tz->field.wSecond); \
    number(object,L##prefix L"Year",tz->field.wYear)
    DATE_FIELDS("Daylight",DaylightDate);
    DATE_FIELDS("Standard",StandardDate);
#undef DATE_FIELDS
    string_value(object,L"DaylightName",tz->DaylightName);
    string_value(object,L"StandardName",tz->StandardName);
    string_value(object,L"Caption",NULL);
    string_value(object,L"Description",NULL);
}
static void run_query(IWbemServices *services,const WCHAR *text,const char *label,const TIME_ZONE_INFORMATION *tz)
{
    BSTR wql=SysAllocString(L"WQL"),query=SysAllocString(text);
    IEnumWbemClassObject *enumerator=NULL;
    IWbemClassObject *object=NULL;
    HRESULT hr;
    ULONG got;
    unsigned int rows=0;
    if(!wql||!query) { b3_error("AllocateQuery",ERROR_OUTOFMEMORY); goto done; }
    hr=IWbemServices_ExecQuery(services,wql,query,WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,NULL,&enumerator);
    printf("QUERY case=%s hr=%08lx\n",label,(DWORD)hr);
    if(FAILED(hr)) { b3_error("ExecQuery",(DWORD)hr); goto done; }
    for(;;) {
        got=0;
        hr=IEnumWbemClassObject_Next(enumerator,5000,1,&object,&got);
        printf("NEXT case=%s hr=%08lx returned=%lu\n",label,(DWORD)hr,got);
        if(FAILED(hr)||hr==WBEM_S_TIMEDOUT) { b3_error("NextTimezone",(DWORD)hr); break; }
        if(!got) break;
        if(!object) { b3_error("MissingTimezoneObject",ERROR_INVALID_DATA); break; }
        printf("OBJECT case=%s index=%u\n",label,rows);
        inspect(object,tz);
        IWbemClassObject_Release(object);object=NULL;
        rows++;
        if(rows==8) { b3_error("TimezoneEnumerationBound",ERROR_MORE_DATA); break; }
    }
    printf("ROWS case=%s count=%u\n",label,rows);
done:
    if(object) IWbemClassObject_Release(object);
    if(enumerator) IEnumWbemClassObject_Release(enumerator);
    SysFreeString(query);SysFreeString(wql);
}
int main(void)
{
    TIME_ZONE_INFORMATION before={0},after={0};
    IWbemLocator *locator=NULL;
    IWbemServices *services=NULL;
    BSTR namespace=NULL;
    WCHAR filtered[100];
    DWORD state;
    if(!b3_start("wbemprox_timezone",45000)) return 1;
    state=GetTimeZoneInformation(&before);
    printf("TIMEZONE_API state=%lu\n",state);
    if(state==TIME_ZONE_ID_INVALID) { b3_error("GetTimeZoneInformation",GetLastError()); return b3_finish(); }
    if(!b3_hr(CoInitializeEx(NULL,COINIT_MULTITHREADED),"CoInitializeEx")) return b3_finish();
    if(!b3_hr(CoInitializeSecurity(NULL,-1,NULL,NULL,RPC_C_AUTHN_LEVEL_DEFAULT,RPC_C_IMP_LEVEL_IMPERSONATE,NULL,EOAC_NONE,NULL),"CoInitializeSecurity")) goto done;
    if(!b3_hr(CoCreateInstance(&CLSID_WbemLocator,NULL,CLSCTX_INPROC_SERVER,&IID_IWbemLocator,(void**)&locator),"CoCreateWbemLocator")) goto done;
    namespace=SysAllocString(L"ROOT\\CIMV2");
    if(!namespace) { b3_error("AllocateNamespace",ERROR_OUTOFMEMORY); goto done; }
    if(!b3_hr(IWbemLocator_ConnectServer(locator,namespace,NULL,NULL,NULL,0,NULL,NULL,&services),"ConnectLocalNamespace")) goto done;
    if(!b3_hr(CoSetProxyBlanket((IUnknown*)services,RPC_C_AUTHN_WINNT,RPC_C_AUTHZ_NONE,NULL,RPC_C_AUTHN_LEVEL_CALL,RPC_C_IMP_LEVEL_IMPERSONATE,NULL,EOAC_NONE),"CoSetProxyBlanket")) goto done;
    run_query(services,L"SELECT * FROM Win32_TimeZone","all",&before);
    swprintf(filtered,100,L"SELECT * FROM Win32_TimeZone WHERE Bias = %ld",before.Bias);
    run_query(services,filtered,"matching_bias",&before);
    state=GetTimeZoneInformation(&after);
    printf("SNAPSHOT state=%lu unchanged=%u\n",state,state!=TIME_ZONE_ID_INVALID&&!memcmp(&before,&after,sizeof(before)));
done:
    SysFreeString(namespace);
    if(services) IWbemServices_Release(services);
    if(locator) IWbemLocator_Release(locator);
    CoUninitialize();
    return b3_finish();
}
