/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Observe own MSI SelectionTree direct-directory and browse-indirection bindings. */
#define UNICODE
#define _WIN32_MSI 300
#include "../batch3-measure/measure-support.h"
#include <msi.h>
#include <msiquery.h>
#include <commctrl.h>
#include <ole2.h>
static BOOL msi_api(UINT result, const char *name)
{
    printf("MSI operation=%s result=%u\n", name, result);
    if(result) b3_error(name, result);
    return result == ERROR_SUCCESS;
}
static BOOL sql(MSIHANDLE db, const char *text)
{
    MSIHANDLE view = 0;
    UINT result = MsiDatabaseOpenViewA(db, text, &view);
    if(!result) result = MsiViewExecute(view, 0);
    if(view) { MsiViewClose(view); MsiCloseHandle(view); }
    return msi_api(result, "OwnSql");
}
static BOOL create_database(const WCHAR *path, BOOL indirect)
{
    static const char *const statements[] = {
        "CREATE TABLE `Property` (`Property` CHAR(72) NOT NULL, `Value` CHAR(0) PRIMARY KEY `Property`)",
        "INSERT INTO `Property` VALUES ('ProductCode','{CA2646FC-983A-4D87-B376-D7E75BA36CCA}')",
        "INSERT INTO `Property` VALUES ('ProductName','Winum Selection Measure')",
        "INSERT INTO `Property` VALUES ('ProductVersion','1.0.0')",
        "INSERT INTO `Property` VALUES ('Manufacturer','Winum fixture')",
        "INSERT INTO `Property` VALUES ('ProductLanguage','1033')",
        "INSERT INTO `Property` VALUES ('INSTALLLEVEL','1')",
        "INSERT INTO `Property` VALUES ('BROWSE','INSTALLDIR')",
        "CREATE TABLE `Directory` (`Directory` CHAR(72) NOT NULL, `Directory_Parent` CHAR(72), `DefaultDir` CHAR(255) NOT NULL PRIMARY KEY `Directory`)",
        "INSERT INTO `Directory` VALUES ('TARGETDIR',NULL,'SourceDir')",
        "INSERT INTO `Directory` VALUES ('INSTALLDIR','TARGETDIR','First')",
        "INSERT INTO `Directory` VALUES ('ALTDIR','TARGETDIR','Second')",
        "CREATE TABLE `Feature` (`Feature` CHAR(38) NOT NULL, `Feature_Parent` CHAR(38), `Title` CHAR(64), `Description` CHAR(255), `Display` SHORT, `Level` SHORT NOT NULL, `Directory_` CHAR(72), `Attributes` LONG NOT NULL PRIMARY KEY `Feature`)",
        "INSERT INTO `Feature` VALUES ('Root',NULL,'First feature','First feature',1,1,'INSTALLDIR',0)",
        "INSERT INTO `Feature` VALUES ('Child','Root','Second feature','Second feature',2,1,'ALTDIR',0)",
        "CREATE TABLE `Component` (`Component` CHAR(72) NOT NULL, `ComponentId` CHAR(38), `Directory_` CHAR(72) NOT NULL, `Attributes` SHORT NOT NULL, `Condition` CHAR(255), `KeyPath` CHAR(72) PRIMARY KEY `Component`)",
        "INSERT INTO `Component` VALUES ('FirstComponent','{719907D3-A10D-4AF8-A0ED-35AB579F58F1}','INSTALLDIR',0,NULL,NULL)",
        "INSERT INTO `Component` VALUES ('SecondComponent','{5F4F050B-C779-4515-93B0-166CBEFA0A29}','ALTDIR',0,NULL,NULL)",
        "CREATE TABLE `FeatureComponents` (`Feature_` CHAR(38) NOT NULL, `Component_` CHAR(72) NOT NULL PRIMARY KEY `Feature_`,`Component_`)",
        "INSERT INTO `FeatureComponents` VALUES ('Root','FirstComponent')",
        "INSERT INTO `FeatureComponents` VALUES ('Child','SecondComponent')",
        "CREATE TABLE `Dialog` (`Dialog` CHAR(72) NOT NULL, `HCentering` SHORT NOT NULL, `VCentering` SHORT NOT NULL, `Width` SHORT NOT NULL, `Height` SHORT NOT NULL, `Attributes` LONG, `Title` CHAR(128), `Control_First` CHAR(50) NOT NULL, `Control_Default` CHAR(50), `Control_Cancel` CHAR(50) PRIMARY KEY `Dialog`)",
        "INSERT INTO `Dialog` VALUES ('WinumSelect',50,50,300,175,3,'Winum Selection Measure','Tree','Done','Done')",
        "CREATE TABLE `Control` (`Dialog_` CHAR(72) NOT NULL, `Control` CHAR(50) NOT NULL, `Type` CHAR(20) NOT NULL, `X` SHORT NOT NULL, `Y` SHORT NOT NULL, `Width` SHORT NOT NULL, `Height` SHORT NOT NULL, `Attributes` LONG, `Property` CHAR(50), `Text` CHAR(0), `Control_Next` CHAR(50), `Help` CHAR(255) PRIMARY KEY `Dialog_`,`Control`)",
        "INSERT INTO `Control` VALUES ('WinumSelect','Done','PushButton',220,145,65,18,3,NULL,'Done','Tree',NULL)",
        "CREATE TABLE `ControlEvent` (`Dialog_` CHAR(72) NOT NULL, `Control_` CHAR(50) NOT NULL, `Event` CHAR(50) NOT NULL, `Argument` CHAR(255) NOT NULL, `Condition` CHAR(255), `Ordering` SHORT PRIMARY KEY `Dialog_`,`Control_`,`Event`,`Argument`,`Condition`)",
        "INSERT INTO `ControlEvent` VALUES ('WinumSelect','Done','EndDialog','Return','1',1)",
        "CREATE TABLE `TextStyle` (`TextStyle` CHAR(72) NOT NULL, `FaceName` CHAR(32) NOT NULL, `Size` SHORT NOT NULL, `Color` LONG, `StyleBits` SHORT PRIMARY KEY `TextStyle`)",
        "INSERT INTO `TextStyle` VALUES ('Default','Tahoma',9,0,0)",
        "INSERT INTO `Property` VALUES ('DefaultUIFont','Default')"
    };
    MSIHANDLE db = 0, summary = 0;
    unsigned int i;
    BOOL ok = FALSE;
    char text[512];
    if(!msi_api(MsiOpenDatabaseW(path, MSIDBOPEN_CREATE, &db), "CreateDatabase")) return FALSE;
    for(i = 0; i < sizeof(statements)/sizeof(statements[0]); i++)
        if(!sql(db, statements[i])) goto done;
    snprintf(text,sizeof(text),"INSERT INTO `Control` VALUES ('WinumSelect','Tree','SelectionTree',10,10,280,105,3,'%s',NULL,'Path',NULL)",indirect?"BROWSE":"INSTALLDIR");
    if(!sql(db,text)) goto done;
    snprintf(text,sizeof(text),"INSERT INTO `Control` VALUES ('WinumSelect','Path','PathEdit',10,120,280,18,%u,'%s',NULL,'Done',NULL)",indirect?11:3,indirect?"BROWSE":"INSTALLDIR");
    if(!sql(db,text)) goto done;
    if(!msi_api(MsiGetSummaryInformationW(db, NULL, 3, &summary), "GetSummaryInformation")) goto done;
    if(!msi_api(MsiSummaryInfoSetPropertyW(summary,7,VT_LPSTR,0,NULL,L"Intel;1033"), "SetTemplate")) goto done;
    if(!msi_api(MsiSummaryInfoSetPropertyW(summary,9,VT_LPSTR,0,NULL,L"{2B8659BC-F0EF-4F5E-960D-E4D589BA7F74}"), "SetPackageCode")) goto done;
    if(!msi_api(MsiSummaryInfoSetPropertyW(summary,14,VT_I4,200,NULL,NULL), "SetSchemaVersion")) goto done;
    if(!msi_api(MsiSummaryInfoPersist(summary), "PersistSummary")) goto done;
    ok = msi_api(MsiDatabaseCommit(db), "CommitDatabase");
done:
    if(summary) MsiCloseHandle(summary);
    MsiCloseHandle(db);
    return ok;
}
struct automation { DWORD main_thread; HWND dialog; BOOL selected; };
static BOOL CALLBACK find_dialog(HWND window, LPARAM parameter)
{
    struct automation *state = (struct automation *)parameter;
    WCHAR title[80];
    DWORD pid;
    GetWindowThreadProcessId(window,&pid);
    if(pid != GetCurrentProcessId()) return TRUE;
    GetWindowTextW(window,title,80);
    if(!lstrcmpW(title,L"Winum Selection Measure")) { state->dialog=window; return FALSE; }
    return TRUE;
}
static BOOL tree_message(HWND window, UINT message, WPARAM wp, LPARAM lp, DWORD_PTR *result)
{
    return SendMessageTimeoutW(window,message,wp,lp,SMTO_ABORTIFHUNG|SMTO_BLOCK,3000,result) != 0;
}
static DWORD WINAPI automate(void *parameter)
{
    struct automation *state = parameter;
    DWORD start = GetTickCount();
    HWND tree = NULL, button = NULL;
    DWORD_PTR root = 0, child = 0, result;
    while(GetTickCount()-start < 15000) {
        EnumThreadWindows(state->main_thread,find_dialog,(LPARAM)state);
        if(state->dialog) {
            tree = FindWindowExW(state->dialog,NULL,WC_TREEVIEWW,NULL);
            button = FindWindowExW(state->dialog,NULL,L"Button",L"Done");
            if(tree && button && tree_message(tree,TVM_GETNEXTITEM,TVGN_ROOT,0,&root) && root
               && tree_message(tree,TVM_GETNEXTITEM,TVGN_CHILD,(LPARAM)root,&child) && child) break;
        }
        Sleep(25);
    }
    if(tree && button && root && child) {
        state->selected = tree_message(tree,TVM_SELECTITEM,TVGN_CARET,(LPARAM)root,&result)
                          && tree_message(tree,TVM_SELECTITEM,TVGN_CARET,(LPARAM)child,&result)
                          && tree_message(tree,TVM_GETNEXTITEM,TVGN_CARET,0,&result) && result == child;
        printf("UI found=1 selected_child=%u\n",state->selected);
        PostMessageW(state->dialog,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(button),BN_CLICKED),(LPARAM)button);
    } else {
        printf("UI found=%u selected_child=0\n",state->dialog != NULL);
        if(state->dialog) PostMessageW(state->dialog,WM_CLOSE,0,0);
    }
    return 0;
}
static BOOL get_property(MSIHANDLE package, const WCHAR *name, WCHAR *value)
{
    DWORD size = 1024;
    return msi_api(MsiGetPropertyW(package,name,value,&size), "GetOwnProperty");
}
static void observe(const char *phase, BOOL indirect, const WCHAR *install, const WCHAR *browse,
                    const WCHAR *initial_install, const WCHAR *initial_browse)
{
    printf("PROPERTY phase=%s indirect=%u install_same_before=%u install_is_identifier=%u browse_same_before=%u browse_is_install=%u browse_is_alt=%u\n",
           phase,indirect,!lstrcmpW(install,initial_install),
           !lstrcmpW(install,L"INSTALLDIR")||!lstrcmpW(install,L"ALTDIR"),
           !lstrcmpW(browse,initial_browse),!lstrcmpW(browse,L"INSTALLDIR"),!lstrcmpW(browse,L"ALTDIR"));
}
static void run_case(const WCHAR *directory, BOOL indirect)
{
    WCHAR database[MAX_PATH], install[1024], browse[1024], initial_install[1024], initial_browse[1024];
    struct automation state = {GetCurrentThreadId(),NULL,FALSE};
    HANDLE thread = NULL;
    MSIHANDLE package = 0, message = 0;
    int result;
    printf("CASE indirect=%u\n",indirect);
    if(!b3_path(database,directory,indirect?L"indirect.msi":L"direct.msi")) return;
    if(!create_database(database,indirect)) goto done;
    if(!msi_api(MsiOpenPackageExW(database,MSIOPENPACKAGEFLAGS_IGNOREMACHINESTATE,&package), "OpenOwnPackage")) goto done;
    if(!msi_api(MsiSetPropertyW(package,L"TARGETDIR",directory), "SetOwnTargetDir")) goto done;
    if(!msi_api(MsiDoActionW(package,L"CostInitialize"), "CostInitialize")) goto done;
    if(!msi_api(MsiDoActionW(package,L"FileCost"), "FileCost")) goto done;
    if(!msi_api(MsiDoActionW(package,L"CostFinalize"), "CostFinalize")) goto done;
    if(!get_property(package,L"INSTALLDIR",initial_install) || !get_property(package,L"BROWSE",initial_browse)) goto done;
    if(!initial_install[0] || lstrcmpW(initial_browse,L"INSTALLDIR")) {
        b3_error("InitialDirectoryBinding",ERROR_INVALID_DATA); goto done;
    }
    observe("before",indirect,initial_install,initial_browse,initial_install,initial_browse);
    message = MsiCreateRecord(0);
    if(!message) { b3_error("CreateDialogRecord",ERROR_OUTOFMEMORY); goto done; }
    if(!msi_api(MsiRecordSetStringW(message,0,L"WinumSelect"), "SetDialogRecord")) goto done;
    thread = CreateThread(NULL,0,automate,&state,0,NULL);
    if(!thread) { b3_error("CreateAutomationThread",GetLastError()); goto done; }
    result = MsiProcessMessage(package,INSTALLMESSAGE_SHOWDIALOG,message);
    printf("DIALOG indirect=%u result=%d\n",indirect,result);
    if(WaitForSingleObject(thread,20000) != WAIT_OBJECT_0) { TerminateProcess(GetCurrentProcess(),2); ExitProcess(2); }
    if(!CloseHandle(thread)) b3_error("CloseAutomationThread",GetLastError());
    if(!state.selected) b3_error("SelectionAutomation",ERROR_INVALID_DATA);
    if(result == -1) b3_error("OwnDialog",ERROR_FUNCTION_FAILED);
    if(get_property(package,L"INSTALLDIR",install) && get_property(package,L"BROWSE",browse))
        observe("after",indirect,install,browse,initial_install,initial_browse);
    printf("CASE_END indirect=%u\n",indirect);
done:
    if(message) MsiCloseHandle(message);
    if(package) MsiCloseHandle(package);
    b3_delete_file(database);
}
int main(void)
{
    WCHAR directory[MAX_PATH];
    INITCOMMONCONTROLSEX controls = {sizeof(controls),ICC_TREEVIEW_CLASSES};
    INSTALLUILEVEL previous;
    if(!b3_start("msi_selection_tree_property",60000)) return 1;
    if(!b3_hr(OleInitialize(NULL), "OleInitialize")) return b3_finish();
    if(!InitCommonControlsEx(&controls)) { b3_error("InitCommonControlsEx",GetLastError()); OleUninitialize(); return b3_finish(); }
    previous = MsiSetInternalUI(INSTALLUILEVEL_FULL,NULL);
    if(b3_tempdir(directory)) {
        run_case(directory,FALSE);
        run_case(directory,TRUE);
        b3_remove_dir(directory);
    }
    MsiSetInternalUI(previous,NULL);
    OleUninitialize();
    return b3_finish();
}
