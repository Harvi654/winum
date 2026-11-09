/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE: EM_SETCHARFORMAT with SCF_WORD at an empty insertion point (and
 * with a selection), at every position of own sample texts, riched20 and
 * msftedit; then the format of each character, of the insertion point and
 * of a character typed there. Own windows and text only. */
#include "../batch3-measure/measure-support.h"
#include <richedit.h>

static const struct { const char *name; const WCHAR *dll, *cls; } dlls[] =
{
    { "riched20", L"riched20.dll", L"RichEdit20W" },
    { "msftedit", L"msftedit.dll", L"RICHEDIT50W" },
};
static const WCHAR *texts[] =
{
    L"ab cd  e_f1 g.h i'j k-l",
    L"x \x00e9\x00e8z 12,5 m\tn ?! o",
};
static const struct { const char *name; WPARAM flags; } flag_sets[] =
{
    { "sel_word", SCF_SELECTION | SCF_WORD },
    { "word", SCF_WORD },
    { "sel", SCF_SELECTION },
};

static HWND edit;

static BOOL is_bold(LONG start, LONG end)
{
    CHARFORMAT2W cf;
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    SendMessageW(edit, EM_SETSEL, start, end);
    SendMessageW(edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    return (cf.dwMask & CFM_BOLD) && (cf.dwEffects & CFE_BOLD);
}

static void format_string(char *out, LONG len)
{
    LONG i;
    for (i = 0; i < len; i++) out[i] = is_bold(i, i + 1) ? 'B' : '.';
    out[len] = 0;
}

static void reset(const WCHAR *text)
{
    CHARFORMAT2W cf;
    SendMessageW(edit, WM_SETTEXT, 0, (LPARAM)text);
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_BOLD;
    cf.dwEffects = 0;
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

static void bold_case(const char *dll, unsigned int t, unsigned int f, LONG start, LONG end)
{
    char after[64], typed[64];
    LONG len = lstrlenW(texts[t]);
    CHARFORMAT2W cf;
    CHARRANGE sel;
    LRESULT ret;
    int insert_bold;

    reset(texts[t]);
    SendMessageW(edit, EM_SETSEL, start, end);
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_BOLD;
    cf.dwEffects = CFE_BOLD;
    ret = SendMessageW(edit, EM_SETCHARFORMAT, flag_sets[f].flags, (LPARAM)&cf);
    SendMessageW(edit, EM_EXGETSEL, 0, (LPARAM)&sel);
    /* format of the selection or insertion point, before the selection moves */
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    SendMessageW(edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    insert_bold = (cf.dwMask & CFM_BOLD) ? !!(cf.dwEffects & CFE_BOLD) : -1;
    format_string(after, len);
    typed[0] = 0;
    if (start == end)
    {
        /* repeat and type one character at the insertion point */
        reset(texts[t]);
        SendMessageW(edit, EM_SETSEL, start, end);
        memset(&cf, 0, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_BOLD;
        cf.dwEffects = CFE_BOLD;
        SendMessageW(edit, EM_SETCHARFORMAT, flag_sets[f].flags, (LPARAM)&cf);
        SendMessageW(edit, EM_REPLACESEL, FALSE, (LPARAM)L"X");
        format_string(typed, len + 1);
    }
    printf("WORD dll=%s text=%u flags=%s sel=%ld,%ld ret=%ld sel_after=%ld,%ld format=%s insert_bold=%d typed=%s\n",
           dll, t, flag_sets[f].name, start, end, (long)ret, sel.cpMin, sel.cpMax, after, insert_bold,
           typed[0] ? typed : "-");
}

/* the Inventor sequence: letters set in another face, then SCF_SELECTION |
 * SCF_WORD with the normal face at the empty insertion point after them */
static char face_letter(LONG start, LONG end)
{
    CHARFORMAT2W cf;
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    if (start != -2) SendMessageW(edit, EM_SETSEL, start, end);
    SendMessageW(edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    if (!(cf.dwMask & CFM_FACE)) return '*';
    return !lstrcmpW(cf.szFaceName, L"Arial") ? 'A' : !lstrcmpW(cf.szFaceName, L"Courier New") ? 'C' :
           !lstrcmpW(cf.szFaceName, L"Times New Roman") ? 'T' : '?';
}

static void face_case(const char *dll, LONG point, WPARAM flags, const char *flags_name)
{
    static const WCHAR text[] = L"ab gaj cd";
    char faces[16];
    CHARFORMAT2W cf;
    LRESULT ret;
    LONG i, len = lstrlenW(text);
    char insert;

    SendMessageW(edit, WM_SETTEXT, 0, (LPARAM)text);
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE;
    lstrcpyW(cf.szFaceName, L"Arial");
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessageW(edit, EM_SETSEL, 3, 6);
    lstrcpyW(cf.szFaceName, L"Courier New");
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageW(edit, EM_SETSEL, point, point);
    lstrcpyW(cf.szFaceName, L"Times New Roman");
    ret = SendMessageW(edit, EM_SETCHARFORMAT, flags, (LPARAM)&cf);
    /* the insertion point's format, before the selection moves */
    insert = face_letter(-2, -2);
    for (i = 0; i < len; i++) faces[i] = face_letter(i, i + 1);
    faces[len] = 0;
    printf("FACE dll=%s point=%ld flags=%s ret=%ld faces=%s insert=%c\n", dll, point, flags_name, (long)ret,
           faces, insert);
}

/* as face_case, then one character typed right after the SCF_WORD call */
static void face_typed_case(const char *dll, LONG point, WPARAM flags, const char *flags_name)
{
    static const WCHAR text[] = L"ab gaj cd";
    char faces[16];
    CHARFORMAT2W cf;
    LONG i, len = lstrlenW(text) + 1;

    SendMessageW(edit, WM_SETTEXT, 0, (LPARAM)text);
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE;
    lstrcpyW(cf.szFaceName, L"Arial");
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessageW(edit, EM_SETSEL, 3, 6);
    lstrcpyW(cf.szFaceName, L"Courier New");
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageW(edit, EM_SETSEL, point, point);
    lstrcpyW(cf.szFaceName, L"Times New Roman");
    SendMessageW(edit, EM_SETCHARFORMAT, flags, (LPARAM)&cf);
    SendMessageW(edit, EM_REPLACESEL, FALSE, (LPARAM)L"X");
    for (i = 0; i < len; i++) faces[i] = face_letter(i, i + 1);
    faces[len] = 0;
    printf("FACE_TYPED dll=%s point=%ld flags=%s faces=%s\n", dll, point, flags_name, faces);
}

int main(void)
{
    unsigned int d, t, f;
    HWND parent;
    LONG p, len;

    if (!b3_start("riched20-scf-word-insertion-point", 120000)) return 2;
    printf("LEGEND A=Arial C=Courier New T=Times New Roman ?=other face *=mixed B=bold .=not bold\n");
    for (d = 0; d < ARRAYSIZE(dlls); d++)
    {
        if (!LoadLibraryW(dlls[d].dll)) { b3_error("LoadLibrary", GetLastError()); continue; }
        parent = CreateWindowExW(0, L"STATIC", L"measure", WS_OVERLAPPEDWINDOW, 0, 0, 500, 200,
                                 NULL, NULL, GetModuleHandleW(NULL), NULL);
        edit = CreateWindowExW(0, dlls[d].cls, L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE,
                               0, 0, 480, 150, parent, (HMENU)1, GetModuleHandleW(NULL), NULL);
        if (!parent || !edit) { b3_error("CreateWindow", GetLastError()); continue; }
        for (t = 0; t < ARRAYSIZE(texts); t++)
        {
            len = lstrlenW(texts[t]);
            printf("TEXT dll=%s text=%u len=%ld\n", dlls[d].name, t, len);
            for (f = 0; f < ARRAYSIZE(flag_sets); f++)
            {
                for (p = 0; p <= len; p++) bold_case(dlls[d].name, t, f, p, p);
                /* selections: inside a word, across a space, a whole word */
                bold_case(dlls[d].name, t, f, 1, 1 + 1);
                bold_case(dlls[d].name, t, f, 1, 4);
                bold_case(dlls[d].name, t, f, 3, 5);
            }
        }
        for (p = 2; p <= 7; p++)
        {
            face_case(dlls[d].name, p, SCF_SELECTION | SCF_WORD, "sel_word");
            face_case(dlls[d].name, p, SCF_SELECTION, "sel");
            face_typed_case(dlls[d].name, p, SCF_SELECTION | SCF_WORD, "sel_word");
            face_typed_case(dlls[d].name, p, SCF_SELECTION, "sel");
        }
        DestroyWindow(parent);
    }
    return b3_finish();
}
