/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * MEASURE (follow-up): EM_SETCHARFORMAT(SCF_SELECTION | SCF_WORD) bold at an
 * empty insertion point in the boundary cases the first measurement did not
 * cover: paragraph starts and ends inside the text, format changes inside a
 * word and before punctuation, a change of colour only, word starts with
 * non-ASCII letters, digits or punctuation. riched20 and msftedit; own
 * windows and text only. Output per case: bold per character (B/.), the
 * marked range per character (I = italic or red, as set up). */
#include "../batch3-measure/measure-support.h"
#include <richedit.h>

static const struct { const char *name; const WCHAR *dll, *cls; } dlls[] =
{
    { "riched20", L"riched20.dll", L"RichEdit20W" },
    { "msftedit", L"msftedit.dll", L"RICHEDIT50W" },
};

/* mark: 0 none, 1 italic, 2 red (colour only); the marked range is
 * [mark_start, mark_end) */
static const struct
{
    const char *name;
    const WCHAR *text;
    int mark, mark_start, mark_end;
    int points[6];
} cases[] =
{
    { "two_paragraphs", L"ab\rcd ef", 0, 0, 0, { 2, 3, 4, 5, 8, -1 } },
    { "italic_inside_word", L"abcd ef", 1, 0, 2, { 1, 2, 3, 4, -1 } },
    { "italic_word_end", L"abc def", 1, 0, 3, { 2, 3, 4, -1 } },
    { "red_word_end", L"abc def", 2, 0, 3, { 2, 3, 4, -1 } },
    { "italic_space_and_word", L"abc def", 1, 3, 7, { 3, 4, 5, -1 } },
    { "italic_before_dot", L"abc.def", 1, 0, 3, { 2, 3, 4, -1 } },
    { "red_before_dot", L"abc.def", 2, 0, 3, { 3, -1 } },
    { "start_accent", L"a \x00e9z b", 0, 0, 0, { 2, 3, -1 } },
    { "start_accent_ascii_after", L"a z\x00e9 b", 0, 0, 0, { 2, 3, -1 } },
    { "start_digit", L"a 9z b", 0, 0, 0, { 2, 3, -1 } },
    { "start_dot", L"a .z b", 0, 0, 0, { 2, 3, -1 } },
    { "start_underscore", L"a _z b", 0, 0, 0, { 2, 3, -1 } },
    { "two_spaces", L"a  bc d", 0, 0, 0, { 2, 3, 4, -1 } },
    { "tab", L"a\tbc d", 0, 0, 0, { 1, 2, 3, -1 } },
    { "text_start_space", L" ab c", 0, 0, 0, { 0, 1, -1 } },
    { "text_start_dot", L".ab c", 0, 0, 0, { 0, 1, -1 } },
    { "text_start_accent", L"\x00e9" L"ab c", 0, 0, 0, { 0, 1, -1 } },
    { "text_end_space", L"ab ", 0, 0, 0, { 2, 3, -1 } },
    { "single_letters", L"a b c", 0, 0, 0, { 1, 2, 3, 5, -1 } },
};

static HWND edit;

static void setup(unsigned int c)
{
    CHARFORMAT2W cf;

    SendMessageW(edit, WM_SETTEXT, 0, (LPARAM)cases[c].text);
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_COLOR;
    cf.crTextColor = RGB(0, 0, 0);
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    if (cases[c].mark)
    {
        SendMessageW(edit, EM_SETSEL, cases[c].mark_start, cases[c].mark_end);
        memset(&cf, 0, sizeof(cf));
        cf.cbSize = sizeof(cf);
        if (cases[c].mark == 1) { cf.dwMask = CFM_ITALIC; cf.dwEffects = CFE_ITALIC; }
        else { cf.dwMask = CFM_COLOR; cf.crTextColor = RGB(255, 0, 0); }
        SendMessageW(edit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    }
}

static void dump(unsigned int c, LONG len, char *bold, char *mark)
{
    CHARFORMAT2W cf;
    LONG i;

    for (i = 0; i < len; i++)
    {
        memset(&cf, 0, sizeof(cf));
        cf.cbSize = sizeof(cf);
        SendMessageW(edit, EM_SETSEL, i, i + 1);
        SendMessageW(edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        bold[i] = (cf.dwMask & CFM_BOLD) && (cf.dwEffects & CFE_BOLD) ? 'B' : '.';
        if (cases[c].mark == 1) mark[i] = (cf.dwEffects & CFE_ITALIC) ? 'I' : '.';
        else if (cases[c].mark == 2) mark[i] = (cf.crTextColor & 0xff) == 0xff ? 'I' : '.';
        else mark[i] = '-';
    }
    bold[len] = mark[len] = 0;
}

int main(void)
{
    char bold[32], mark[32];
    unsigned int d, c, p;
    CHARFORMAT2W cf;
    HWND parent;
    LRESULT ret;
    LONG len;

    if (!b3_start("riched20-scf-word-boundary", 120000)) return 2;
    printf("LEGEND bold: B/. per character; mark: I = the italic or red range, - none;"
           " text length counts a paragraph break as one character\n");
    for (d = 0; d < ARRAYSIZE(dlls); d++)
    {
        if (!LoadLibraryW(dlls[d].dll)) { b3_error("LoadLibrary", GetLastError()); continue; }
        parent = CreateWindowExW(0, L"STATIC", L"measure", WS_OVERLAPPEDWINDOW, 0, 0, 500, 200,
                                 NULL, NULL, GetModuleHandleW(NULL), NULL);
        edit = CreateWindowExW(0, dlls[d].cls, L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE,
                               0, 0, 480, 150, parent, (HMENU)1, GetModuleHandleW(NULL), NULL);
        if (!parent || !edit) { b3_error("CreateWindow", GetLastError()); continue; }
        for (c = 0; c < ARRAYSIZE(cases); c++)
        {
            setup(c);
            len = (LONG)SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0);
            dump(c, len, bold, mark);
            printf("SETUP dll=%s case=%s len=%ld bold=%s mark=%s\n", dlls[d].name, cases[c].name, len, bold, mark);
            for (p = 0; p < ARRAYSIZE(cases[c].points) && cases[c].points[p] >= 0; p++)
            {
                LONG point = cases[c].points[p];
                int insert_bold;

                setup(c);
                SendMessageW(edit, EM_SETSEL, point, point);
                memset(&cf, 0, sizeof(cf));
                cf.cbSize = sizeof(cf);
                cf.dwMask = CFM_BOLD;
                cf.dwEffects = CFE_BOLD;
                ret = SendMessageW(edit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD, (LPARAM)&cf);
                memset(&cf, 0, sizeof(cf));
                cf.cbSize = sizeof(cf);
                SendMessageW(edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
                insert_bold = (cf.dwMask & CFM_BOLD) ? !!(cf.dwEffects & CFE_BOLD) : -1;
                dump(c, len, bold, mark);
                printf("POINT dll=%s case=%s point=%ld ret=%ld bold=%s mark=%s insert_bold=%d\n",
                       dlls[d].name, cases[c].name, point, (long)ret, bold, mark, insert_bold);
            }
        }
        DestroyWindow(parent);
    }
    return b3_finish();
}
