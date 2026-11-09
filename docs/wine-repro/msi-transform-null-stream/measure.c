/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Own compound-storage transform: nullable object plus populated control. */
#define COBJMACROS
#define UNICODE
#define _WIN32_MSI 300
#include <objbase.h>
#include <msi.h>
#include <msiquery.h>
#include "../batch3-measure/measure-support.h"
static BOOL msi_api(UINT result, const char *name)
{
    printf("MSI operation=%s result=%u\n", name, result);
    if(result) b3_error(name, result);
    return result == ERROR_SUCCESS;
}
static UINT query(MSIHANDLE db, const char *sql)
{
    MSIHANDLE view = 0;
    UINT result = MsiDatabaseOpenViewA(db, sql, &view);
    if(!result) result = MsiViewExecute(view, 0);
    if(view) { MsiViewClose(view); MsiCloseHandle(view); }
    return result;
}
/* MSI's documented compressed stream-name alphabet, table prefix 0x4840. */
static void encode_name(const char *name, BOOL table, WCHAR *out)
{
    static const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._";
    if(table) *out++ = 0x4840;
    while(*name) {
        const char *a = strchr(alphabet, *name++), *b = *name ? strchr(alphabet, *name) : NULL;
        if(!a) { *out++ = (unsigned char)name[-1]; continue; }
        if(b) { *out++ = 0x3800 + (a - alphabet) + ((b - alphabet) << 6); name++; }
        else *out++ = 0x4800 + (a - alphabet);
    }
    *out = 0;
}
static BOOL write_stream(IStorage *storage, const char *name, BOOL table, const void *data, ULONG size)
{
    WCHAR encoded[80];
    IStream *stream = NULL;
    ULONG written = 0;
    HRESULT hr;
    encode_name(name, table, encoded);
    hr = IStorage_CreateStream(storage, encoded, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stream);
    if(!b3_hr(hr, "CreateOwnTransformStream")) return FALSE;
    hr = IStream_Write(stream, data, size, &written);
    IStream_Release(stream);
    if(!b3_hr(hr, "WriteOwnTransformStream")) return FALSE;
    if(written != size) { b3_error("ShortTransformWrite", ERROR_WRITE_FAULT); return FALSE; }
    return TRUE;
}
static BOOL create_transform(const WCHAR *path)
{
    static const CLSID transform_class = {0x000c1082,0,0,{0xc0,0,0,0,0,0,0,0x46}};
    static const WORD pool[] = {1252,0, 8,1, 8,1};
    static const char strings[] = "null-rowblob-row";
    static const WORD rows[] = {0x0301,0x8001,1,0, 0x0301,0x8002,2,1};
    static const BYTE blob[] = {0x57,0x33,0x4d,0x21};
    IStorage *storage = NULL;
    BOOL ok;
    if(!b3_hr(StgCreateDocfile(path, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                              0, &storage), "StgCreateDocfile")) return FALSE;
    ok = b3_hr(IStorage_SetClass(storage, &transform_class), "SetTransformClass")
         && write_stream(storage, "_StringPool", TRUE, pool, sizeof(pool))
         && write_stream(storage, "_StringData", TRUE, strings, sizeof(strings) - 1)
         && write_stream(storage, "WinumNull", TRUE, rows, sizeof(rows))
         && write_stream(storage, "WinumNull.2", FALSE, blob, sizeof(blob));
    IStorage_Release(storage);
    return ok;
}
static BOOL inspect(MSIHANDLE db, const char *phase)
{
    MSIHANDLE view = 0, row = 0;
    unsigned int rows = 0;
    BOOL control = FALSE;
    UINT result;
    if(!msi_api(MsiDatabaseOpenViewA(db, "SELECT `Key`, `Note`, `Payload` FROM `WinumNull` ORDER BY `Key`", &view), "OpenResultView")) return FALSE;
    if(!msi_api(MsiViewExecute(view, 0), "ExecuteResultView")) goto done;
    while((result = MsiViewFetch(view, &row)) == ERROR_SUCCESS) {
        char text[64] = {0}, bytes[8] = {0};
        DWORD len = sizeof(text), size = sizeof(bytes);
        UINT text_result = MsiRecordGetStringA(row, 2, text, &len);
        UINT stream_result = MsiRecordReadStream(row, 3, bytes, &size);
        printf("ROW phase=%s key=%d text_result=%u note_is_null_row=%u note_is_blob_row=%u payload_null=%u read_result=%u bytes=%lu blob_matches=%u\n",
               phase, MsiRecordGetInteger(row, 1), text_result, !strcmp(text,"null-row"), !strcmp(text,"blob-row"),
               MsiRecordIsNull(row, 3), stream_result, size, size == 4 && !memcmp(bytes,"W3M!",4));
        if(MsiRecordGetInteger(row, 1) == 2 && !text_result && !strcmp(text,"blob-row")
           && !stream_result && size == 4 && !memcmp(bytes,"W3M!",4)) control = TRUE;
        rows++;
        MsiCloseHandle(row); row = 0;
    }
    if(result != ERROR_NO_MORE_ITEMS) b3_error("FetchResult", result);
    printf("ROWS phase=%s count=%u\n", phase, rows);
done:
    if(row) MsiCloseHandle(row);
    MsiViewClose(view); MsiCloseHandle(view);
    return control;
}
int main(void)
{
    WCHAR directory[MAX_PATH], database[MAX_PATH], transform[MAX_PATH];
    MSIHANDLE db = 0;
    UINT result;
    BOOL own_directory = FALSE;
    if(!b3_start("msi_transform_null_stream", 30000)) return 1;
    if(!b3_hr(CoInitialize(NULL), "CoInitialize")) return b3_finish();
    if(!b3_tempdir(directory)) goto uninit;
    own_directory = TRUE;
    if(!b3_path(database,directory,L"fixture.msi") || !b3_path(transform,directory,L"fixture.mst")) goto cleanup;
    if(!msi_api(MsiOpenDatabaseW(database, MSIDBOPEN_CREATE, &db), "CreateDatabase")) goto cleanup;
    if(!msi_api(query(db, "CREATE TABLE `WinumNull` (`Key` SHORT NOT NULL, `Note` CHAR(40), `Payload` OBJECT PRIMARY KEY `Key`)"), "CreateTable")) goto cleanup;
    if(!msi_api(MsiDatabaseCommit(db), "CommitDatabase")) goto cleanup;
    MsiCloseHandle(db); db = 0;
    if(!msi_api(MsiOpenDatabaseW(database, MSIDBOPEN_DIRECT, &db), "ReopenDatabase")) goto cleanup;
    inspect(db, "before");
    if(!create_transform(transform)) goto cleanup;
    result = MsiDatabaseApplyTransformW(db, transform, 0);
    printf("APPLY result=%u\n", result);
    if(!inspect(db, "after")) b3_error("PopulatedBlobControl", ERROR_INVALID_DATA);
    if(result) b3_error("ApplyOwnTransform", result);
    printf("CASES completed=%u\n", b3_errors == 0);
cleanup:
    if(db) MsiCloseHandle(db);
    if(own_directory) {
        if(b3_path(database,directory,L"fixture.msi")) b3_delete_file(database);
        if(b3_path(transform,directory,L"fixture.mst")) b3_delete_file(transform);
        b3_remove_dir(directory);
    }
uninit:
    CoUninitialize();
    return b3_finish();
}
