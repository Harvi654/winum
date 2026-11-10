/* Original, AI-assisted (Claude), LGPL-2.1-or-later.
 * L3 reproducer for winum rpcrt4-ndrstubcall3.patch (Wine only, x86_64):
 * the measurement program of this directory, with the server metadata
 * changed to the scenario below. Expected results are Wine with the patch;
 * a valid native NdrStubCall3 call needs NDR64 tables (see README). */
#define STUB_CALL_VERSION 3
#define CONFIGURE_SERVER_INFO l3_configure
static void l3_configure(void);
#include "com-measure.c"
static const RPC_SYNTAX_IDENTIFIER ndr64_id = {
    {0x71710533,0xbeba,0x4937,{0x83,0x19,0xb5,0xdb,0xef,0x9c,0xcc,0x36}},{1,0}};
static MIDL_SYNTAX_INFO entries[2];

/* No syntax entries, NDR tables in the server info itself.
 * Expected: the patch falls back to them; S_OK, value 42. */
static void l3_configure(void)
{
    (void)entries; (void)ndr64_id;
    server_info.nCount = 0;
    server_info.pSyntaxInfo = NULL;
    server_info.pTransferSyntax = NULL;
    server_info.ProcString = IWinumNdrProbe_ServerInfo.ProcString;
    server_info.FmtStringOffset = IWinumNdrProbe_ServerInfo.FmtStringOffset;
    printf("L3 scenario=fallback expect=functional\n");
}
