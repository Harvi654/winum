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

/* Only an NDR64 entry and no NDR tables at all.
 * Expected: the stub raises RPC_X_WRONG_STUB_VERSION, the client gets
 * HRESULT 80070725, the server is not called; Wine logs
 * "NDR64 syntax is not supported". */
static void l3_configure(void)
{
    entries[0].TransferSyntax = ndr64_id;
    server_info.nCount = 1;
    server_info.pSyntaxInfo = entries;
    server_info.pTransferSyntax = &entries[0].TransferSyntax;
    printf("L3 scenario=ndr64-only expect=serverfault\n");
}
