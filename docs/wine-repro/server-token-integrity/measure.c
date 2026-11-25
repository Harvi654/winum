/* Token integrity MEASURE program.
 * Original code, AI-assisted (Claude), LGPL-2.1-or-later.
 *
 * Prints, as key=value lines, what Windows reports about the integrity level
 * of the process token and of tokens derived from it: the linked token,
 * duplicates, an ImpersonateSelf thread token, a restricted token, lowered
 * and raised duplicates, and a CreateProcess child. Only mandatory label SIDs
 * (S-1-16-*) are printed; no user or group SIDs, names or paths. */
#define _WIN32_WINNT 0x0a00
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_label_sid(const char *p, const char *key, PSID sid)
{
    SID_IDENTIFIER_AUTHORITY *auth;
    if (!sid || !IsValidSid(sid))
    {
        printf("%s.%s=invalid\n", p, key);
        return;
    }
    auth = GetSidIdentifierAuthority(sid);
    if (auth->Value[5] == 16 && !auth->Value[0] && !auth->Value[1] && !auth->Value[2] &&
        !auth->Value[3] && !auth->Value[4] && *GetSidSubAuthorityCount(sid) == 1)
        printf("%s.%s=S-1-16-%lu\n", p, key, *GetSidSubAuthority(sid, 0));
    else /* never print other SIDs, they may identify the user */
        printf("%s.%s=non-label(authority=%u,subauthorities=%u)\n", p, key, auth->Value[5],
               *GetSidSubAuthorityCount(sid));
}

static BOOL is_label_sid(PSID sid)
{
    SID_IDENTIFIER_AUTHORITY *auth = GetSidIdentifierAuthority(sid);
    return auth->Value[5] == 16 && !auth->Value[0] && !auth->Value[1] && !auth->Value[2] &&
           !auth->Value[3] && !auth->Value[4];
}

static void dump_dword_info(const char *p, const char *key, HANDLE token, TOKEN_INFORMATION_CLASS cls)
{
    DWORD value = 0xdeadbeef, size = 0;
    BOOL ret;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, cls, &value, sizeof(value), &size);
    if (ret) printf("%s.%s=%lu\n", p, key, value);
    else printf("%s.%s=fail(error=%lu)\n", p, key, GetLastError());
}

static void dump_integrity(const char *p, HANDLE token)
{
    BYTE buffer[256];
    TOKEN_MANDATORY_LABEL *tml = (TOKEN_MANDATORY_LABEL *)buffer;
    DWORD size = 0xdeadbeef;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenIntegrityLevel, NULL, 0, &size);
    printf("%s.integrity_probe=ret=%d error=%lu size=%lu\n", p, ret, GetLastError(), size);

    size = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenIntegrityLevel, buffer, sizeof(buffer), &size);
    if (!ret)
    {
        printf("%s.integrity=fail(error=%lu)\n", p, GetLastError());
        return;
    }
    printf("%s.integrity_size=%lu\n", p, size);
    printf("%s.integrity_sid_in_buffer=%d\n", p,
           (BYTE *)tml->Label.Sid >= buffer && (BYTE *)tml->Label.Sid < buffer + size);
    print_label_sid(p, "integrity", tml->Label.Sid);
    printf("%s.integrity_attributes=0x%lx\n", p, tml->Label.Attributes);
}

static void dump_groups(const char *p, HANDLE token)
{
    DWORD size = 0, i, labels = 0, flagged = 0;
    TOKEN_GROUPS *groups;
    char key[64];

    GetTokenInformation(token, TokenGroups, NULL, 0, &size);
    if (!size || !(groups = malloc(size)))
    {
        printf("%s.groups=fail(error=%lu)\n", p, GetLastError());
        return;
    }
    if (!GetTokenInformation(token, TokenGroups, groups, size, &size))
    {
        printf("%s.groups=fail(error=%lu)\n", p, GetLastError());
        free(groups);
        return;
    }
    printf("%s.groups_count=%lu\n", p, groups->GroupCount);
    for (i = 0; i < groups->GroupCount; i++)
    {
        if (groups->Groups[i].Attributes & (SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED)) flagged++;
        if (!is_label_sid(groups->Groups[i].Sid)) continue;
        snprintf(key, sizeof(key), "groups_label%lu", labels++);
        print_label_sid(p, key, groups->Groups[i].Sid);
        printf("%s.%s_index=%lu\n", p, key, i);
        printf("%s.%s_attributes=0x%lx\n", p, key, groups->Groups[i].Attributes);
    }
    printf("%s.groups_label_count=%lu\n", p, labels);
    printf("%s.groups_with_integrity_flags=%lu\n", p, flagged);
    free(groups);
}

static void dump_object_label(const char *p, HANDLE object)
{
    SECURITY_DESCRIPTOR_CONTROL control = 0;
    DWORD size = 0, revision, i;
    BOOL present = FALSE, defaulted = FALSE;
    PSECURITY_DESCRIPTOR sd;
    ACL *sacl = NULL;
    char key[64];

    SetLastError(0xdeadbeef);
    GetKernelObjectSecurity(object, LABEL_SECURITY_INFORMATION, NULL, 0, &size);
    if (!size || !(sd = malloc(size)))
    {
        printf("%s.label=fail(error=%lu)\n", p, GetLastError());
        return;
    }
    if (!GetKernelObjectSecurity(object, LABEL_SECURITY_INFORMATION, sd, size, &size))
    {
        printf("%s.label=fail(error=%lu)\n", p, GetLastError());
        free(sd);
        return;
    }
    GetSecurityDescriptorControl(sd, &control, &revision);
    GetSecurityDescriptorSacl(sd, &present, &sacl, &defaulted);
    printf("%s.label_control=0x%x\n", p, control);
    printf("%s.label_sacl_present=%d\n", p, present);
    printf("%s.label_ace_count=%u\n", p, present && sacl ? sacl->AceCount : 0);
    for (i = 0; present && sacl && i < sacl->AceCount; i++)
    {
        SYSTEM_MANDATORY_LABEL_ACE *ace;
        if (!GetAce(sacl, i, (void **)&ace)) continue;
        snprintf(key, sizeof(key), "label_ace%lu", i);
        printf("%s.%s_type=%u flags=0x%x mask=0x%lx\n", p, key, ace->Header.AceType,
               ace->Header.AceFlags, ace->Mask);
        if (ace->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE)
            print_label_sid(p, key, (PSID)&ace->SidStart);
    }
    free(sd);
}

static void dump_token(const char *p, HANDLE token)
{
    DWORD type = 0, size;

    printf("%s.open=1\n", p);
    if (GetTokenInformation(token, TokenType, &type, sizeof(type), &size))
    {
        printf("%s.type=%lu\n", p, type);
        if (type == TokenImpersonation)
            dump_dword_info(p, "impersonation_level", token, TokenImpersonationLevel);
    }
    dump_dword_info(p, "elevation_type", token, TokenElevationType);
    dump_dword_info(p, "elevated", token, TokenElevation);
    dump_dword_info(p, "mandatory_policy", token, TokenMandatoryPolicy);
    dump_integrity(p, token);
    dump_groups(p, token);
    dump_object_label(p, token);
}

static DWORD label_rid(HANDLE token)
{
    BYTE buffer[256];
    TOKEN_MANDATORY_LABEL *tml = (TOKEN_MANDATORY_LABEL *)buffer;
    DWORD size;
    if (!GetTokenInformation(token, TokenIntegrityLevel, buffer, sizeof(buffer), &size)) return ~0u;
    return *GetSidSubAuthority(tml->Label.Sid, 0);
}

static void set_level(const char *p, HANDLE process_token, DWORD rid)
{
    SID_IDENTIFIER_AUTHORITY auth = {SECURITY_MANDATORY_LABEL_AUTHORITY};
    TOKEN_MANDATORY_LABEL tml;
    HANDLE dup;
    PSID sid;
    BOOL ret;

    if (!DuplicateTokenEx(process_token, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &dup))
    {
        printf("%s.duplicate=fail(error=%lu)\n", p, GetLastError());
        return;
    }
    AllocateAndInitializeSid(&auth, 1, rid, 0, 0, 0, 0, 0, 0, 0, &sid);
    tml.Label.Sid = sid;
    tml.Label.Attributes = SE_GROUP_INTEGRITY;
    SetLastError(0xdeadbeef);
    ret = SetTokenInformation(dup, TokenIntegrityLevel, &tml, sizeof(tml) + GetLengthSid(sid));
    printf("%s.set=ret=%d error=%lu\n", p, ret, ret ? 0 : GetLastError());
    dump_integrity(p, dup);
    dump_groups(p, dup);
    dump_object_label(p, dup);
    FreeSid(sid);
    CloseHandle(dup);
}

static int run_child(const char *exe)
{
    STARTUPINFOA si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;
    char cmd[MAX_PATH + 32];
    DWORD code = ~0u;

    snprintf(cmd, sizeof(cmd), "\"%s\" --child", exe);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    fflush(stdout);
    if (!CreateProcessA(exe, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        printf("child.create=fail(error=%lu)\n", GetLastError());
        return 1;
    }
    WaitForSingleObject(pi.hProcess, 60000);
    GetExitCodeProcess(pi.hProcess, &code);
    printf("child.exit=%lu\n", code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code != 0;
}

int main(int argc, char **argv)
{
    static const struct { const char *name; DWORD rid; } levels[] =
    {
        {"set_untrusted", SECURITY_MANDATORY_UNTRUSTED_RID},
        {"set_low", SECURITY_MANDATORY_LOW_RID},
        {"set_medium", SECURITY_MANDATORY_MEDIUM_RID},
        {"set_medium_plus", SECURITY_MANDATORY_MEDIUM_PLUS_RID},
        {"set_high", SECURITY_MANDATORY_HIGH_RID},
        {"set_system", SECURITY_MANDATORY_SYSTEM_RID},
    };
    TOKEN_LINKED_TOKEN linked, linked2;
    const char *(CDECL *wine_get_version)(void);
    LONG (WINAPI *rtl_get_version)(OSVERSIONINFOW *);
    OSVERSIONINFOW os = {.dwOSVersionInfoSize = sizeof(os)};
    HANDLE token, dup, thread_token;
    char exe[MAX_PATH];
    BOOL wow64 = FALSE;
    DWORD size, i;
    int failures = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc > 1 && !strcmp(argv[1], "--child"))
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | READ_CONTROL, &token))
        {
            printf("child.open=fail(error=%lu)\n", GetLastError());
            return 1;
        }
        dump_token("child", token);
        CloseHandle(token);
        return 0;
    }

    printf("measure=token-integrity-v1\n");
    printf("pointer_bits=%u\n", (unsigned)(sizeof(void *) * 8));
    IsWow64Process(GetCurrentProcess(), &wow64);
    printf("wow64=%d\n", wow64);
    rtl_get_version = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
    if (rtl_get_version && !rtl_get_version(&os))
        printf("windows=%lu.%lu.%lu\n", os.dwMajorVersion, os.dwMinorVersion, os.dwBuildNumber);
    wine_get_version = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version");
    printf("wine=%s\n", wine_get_version ? wine_get_version() : "none");

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | READ_CONTROL | TOKEN_DUPLICATE, &token))
    {
        printf("process.open=fail(error=%lu)\n", GetLastError());
        return 1;
    }
    dump_token("process", token);

    dump_integrity("pseudo_process", GetCurrentProcessToken());
    dump_integrity("pseudo_effective", GetCurrentThreadEffectiveToken());
    dump_object_label("process_object", GetCurrentProcess());

    SetLastError(0xdeadbeef);
    thread_token = NULL;
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &thread_token))
    {
        printf("thread_before.open=1\n");
        CloseHandle(thread_token);
    }
    else printf("thread_before.open=fail(error=%lu)\n", GetLastError());

    if (GetTokenInformation(token, TokenLinkedToken, &linked, sizeof(linked), &size))
    {
        dump_token("linked", linked.LinkedToken);
        if (GetTokenInformation(linked.LinkedToken, TokenLinkedToken, &linked2, sizeof(linked2), &size))
        {
            dump_token("linked_linked", linked2.LinkedToken);
            CloseHandle(linked2.LinkedToken);
        }
        else printf("linked_linked.open=fail(error=%lu)\n", GetLastError());
        CloseHandle(linked.LinkedToken);
    }
    else printf("linked.open=fail(error=%lu)\n", GetLastError());

    if (DuplicateTokenEx(token, MAXIMUM_ALLOWED, NULL, SecurityAnonymous, TokenPrimary, &dup))
    {
        dump_token("dup_primary", dup);
        CloseHandle(dup);
    }
    else printf("dup_primary.open=fail(error=%lu)\n", GetLastError());

    if (DuplicateTokenEx(token, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenImpersonation, &dup))
    {
        dump_token("dup_impersonation", dup);
        CloseHandle(dup);
    }
    else printf("dup_impersonation.open=fail(error=%lu)\n", GetLastError());

    if (DuplicateToken(token, SecurityIdentification, &dup))
    {
        dump_token("dup_identification", dup);
        CloseHandle(dup);
    }
    else printf("dup_identification.open=fail(error=%lu)\n", GetLastError());

    if (CreateRestrictedToken(token, DISABLE_MAX_PRIVILEGE, 0, NULL, 0, NULL, 0, NULL, &dup))
    {
        dump_token("restricted", dup);
        CloseHandle(dup);
    }
    else printf("restricted.open=fail(error=%lu)\n", GetLastError());

    if (ImpersonateSelf(SecurityImpersonation))
    {
        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | READ_CONTROL, TRUE, &thread_token))
        {
            dump_token("impersonate_self", thread_token);
            CloseHandle(thread_token);
        }
        else printf("impersonate_self.open=fail(error=%lu)\n", GetLastError());
        dump_integrity("impersonate_self_effective", GetCurrentThreadEffectiveToken());
        RevertToSelf();
    }
    else printf("impersonate_self.open=fail(error=%lu)\n", GetLastError());

    CloseHandle(token);
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | READ_CONTROL | TOKEN_DUPLICATE, &token))
        return 1;
    printf("process.rid=0x%lx\n", label_rid(token));
    for (i = 0; i < sizeof(levels) / sizeof(levels[0]); i++)
        set_level(levels[i].name, token, levels[i].rid);
    CloseHandle(token);

    GetModuleFileNameA(NULL, exe, sizeof(exe));
    failures += run_child(exe);
    printf("measure_complete=1\n");
    return failures;
}
