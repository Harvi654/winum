/*
 * MEASURE: the loopback interface's rows in the IP helper tables that a
 * service reads for every client connection.
 *
 * Original AI-assisted (Claude), LGPL-2.1-or-later. Public API
 * observations only; prints values and judges nothing.
 *
 * CodeMeter's service (ABB RobotStudio) reads, for each client connection,
 * the unicast address tables, the interface table (index, GUID), the IP
 * interface tables and the IPv6 route table (Wine +relay of its NSI calls,
 * Harvi654/ABB-RobotStudio-on-Linux step 13). Under Wine a local client
 * connecting to ::1 gets "en-/decryption of the communication failed";
 * on Windows the same client works. This prints what the documented
 * functions over the same tables return for the loopback interface, and
 * the route rows that concern loopback addresses.
 *
 * Privacy: only rows of the loopback interface, loopback/unspecified
 * prefixes and multicast prefixes are printed; other interfaces only as
 * a count. No names, GUID values, MACs or LAN addresses.
 *
 * Exit code: 0 when every call ran, 2 otherwise.
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <stdio.h>
#include <string.h>

static int incomplete;

static const char *addr_class(const SOCKADDR_INET *a, char *buf, size_t size)
{
    if (a->si_family == AF_INET)
    {
        const BYTE *b = (const BYTE *)&a->Ipv4.sin_addr;
        if (b[0] == 127) snprintf(buf, size, "v4 %u.%u.%u.%u", b[0], b[1], b[2], b[3]);
        else if (!b[0] && !b[1] && !b[2] && !b[3]) snprintf(buf, size, "v4 0.0.0.0");
        else if (b[0] >= 224 && b[0] <= 239) snprintf(buf, size, "v4 multicast");
        else if (b[0] == 255) snprintf(buf, size, "v4 broadcast");
        else if (b[0] == 169 && b[1] == 254) snprintf(buf, size, "v4 link-local");
        else snprintf(buf, size, "v4 other");
    }
    else if (a->si_family == AF_INET6)
    {
        const BYTE *b = a->Ipv6.sin6_addr.s6_addr;
        static const BYTE zero[16], loop[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
        if (!memcmp(b, loop, 16)) snprintf(buf, size, "v6 ::1 scope %lu", (unsigned long)a->Ipv6.sin6_scope_id);
        else if (!memcmp(b, zero, 16)) snprintf(buf, size, "v6 ::");
        else if (b[0] == 0xff) snprintf(buf, size, "v6 multicast ff%02x", b[1]);
        else if (b[0] == 0xfe && (b[1] & 0xc0) == 0x80) snprintf(buf, size, "v6 link-local");
        else snprintf(buf, size, "v6 other");
    }
    else snprintf(buf, size, "family %u", a->si_family);
    return buf;
}

static BOOL interesting(const SOCKADDR_INET *a)
{
    char buf[64];
    addr_class(a, buf, sizeof(buf));
    return strncmp(buf + 3, "other", 5) && strncmp(buf + 3, "link-local", 10);
}

static void measure_if(NET_LUID luid)
{
    MIB_IF_ROW2 row;
    DWORD ret;
    static const GUID null_guid;

    memset(&row, 0, sizeof(row));
    row.InterfaceLuid = luid;
    ret = GetIfEntry2(&row);
    printf("if ret=%lu", ret);
    if (!ret)
        printf(" index=%lu type=%lu tunnel=%d media=%d phys_medium=%d access=%d direction=%d oper=%d admin=%d"
               " media_conn=%d conn_type=%d guid_set=%d net_guid_set=%d hw=%d filter=%d conn_present=%d"
               " mtu=%lu phys_len=%lu",
               row.InterfaceIndex, row.Type, row.TunnelType, row.MediaType, row.PhysicalMediumType,
               row.AccessType, row.DirectionType, row.OperStatus, row.AdminStatus, row.MediaConnectState,
               row.ConnectionType, memcmp(&row.InterfaceGuid, &null_guid, sizeof(GUID)) != 0,
               memcmp(&row.NetworkGuid, &null_guid, sizeof(GUID)) != 0, row.InterfaceAndOperStatusFlags.HardwareInterface,
               row.InterfaceAndOperStatusFlags.FilterInterface, row.InterfaceAndOperStatusFlags.ConnectorPresent,
               row.Mtu, row.PhysicalAddressLength);
    printf("\n");
}

static void measure_unicast(ADDRESS_FAMILY family, NET_LUID luid)
{
    MIB_UNICASTIPADDRESS_TABLE *table;
    DWORD ret, i, others = 0;
    char buf[64];

    ret = GetUnicastIpAddressTable(family, &table);
    printf("unicast_%s ret=%lu\n", family == AF_INET ? "v4" : "v6", ret);
    if (ret) { if (ret != ERROR_NOT_FOUND) incomplete = 1; return; }
    for (i = 0; i < table->NumEntries; i++)
    {
        MIB_UNICASTIPADDRESS_ROW *r = &table->Table[i];
        if (r->InterfaceLuid.Value != luid.Value) { others++; continue; }
        printf("unicast_%s %s prefix_origin=%d suffix_origin=%d valid=%lu preferred=%lu onlink=%u skip_as_source=%d"
               " dad=%d scope_level=%u scope_zone=%lu index_is_lo=1\n",
               family == AF_INET ? "v4" : "v6", addr_class(&r->Address, buf, sizeof(buf)), r->PrefixOrigin,
               r->SuffixOrigin, r->ValidLifetime, r->PreferredLifetime, r->OnLinkPrefixLength, r->SkipAsSource,
               r->DadState, r->ScopeId.Level, (unsigned long)r->ScopeId.Zone);
    }
    printf("unicast_%s other_interfaces_rows=%lu\n", family == AF_INET ? "v4" : "v6", others);
    FreeMibTable(table);
}

static void measure_routes(ADDRESS_FAMILY family, NET_LUID luid)
{
    MIB_IPFORWARD_TABLE2 *table;
    DWORD ret, i, others = 0, other_prefix = 0;
    char buf[64], hop[64];

    ret = GetIpForwardTable2(family, &table);
    printf("routes_%s ret=%lu\n", family == AF_INET ? "v4" : "v6", ret);
    if (ret) { if (ret != ERROR_NOT_FOUND) incomplete = 1; return; }
    for (i = 0; i < table->NumEntries; i++)
    {
        MIB_IPFORWARD_ROW2 *r = &table->Table[i];
        BOOL lo = r->InterfaceLuid.Value == luid.Value;
        if (!lo && !interesting(&r->DestinationPrefix.Prefix)) { others++; continue; }
        if (!interesting(&r->DestinationPrefix.Prefix)) { other_prefix++; continue; }
        printf("routes_%s %s/%u on_lo=%d hop=%s metric=%lu protocol=%d origin=%d loopback=%d autoconf=%d publish=%d"
               " immortal=%d valid=%lu\n",
               family == AF_INET ? "v4" : "v6", addr_class(&r->DestinationPrefix.Prefix, buf, sizeof(buf)),
               r->DestinationPrefix.PrefixLength, lo, addr_class(&r->NextHop, hop, sizeof(hop)), r->Metric,
               r->Protocol, r->Origin, r->Loopback, r->AutoconfigureAddress, r->Publish, r->Immortal,
               r->ValidLifetime);
    }
    printf("routes_%s other_rows=%lu lo_rows_with_other_prefix=%lu\n", family == AF_INET ? "v4" : "v6", others,
           other_prefix);
    FreeMibTable(table);
}

static void measure_ipif(ADDRESS_FAMILY family, NET_LUID luid)
{
    MIB_IPINTERFACE_ROW r;
    DWORD ret;

    memset(&r, 0, sizeof(r));
    r.Family = family;
    r.InterfaceLuid = luid;
    ret = GetIpInterfaceEntry(&r);
    printf("ipif_%s ret=%lu", family == AF_INET ? "v4" : "v6", ret);
    if (!ret)
        printf(" forwarding=%d weak_send=%d weak_recv=%d advertising=%d use_auto_metric=%d use_nud=%d"
               " managed=%d other_stateful=%d router_discovery=%d dad_transmits=%lu reachable=%lu retransmit=%lu"
               " pmtu_timeout=%lu ll_behavior=%d ll_timeout=%lu metric=%lu nlmtu=%lu connected=%d"
               " wakeup=%d site_prefix=%lu support_ll=%d disable_default_routes=%d",
               r.ForwardingEnabled, r.WeakHostSend, r.WeakHostReceive, r.AdvertisingEnabled, r.UseAutomaticMetric,
               r.UseNeighborUnreachabilityDetection, r.ManagedAddressConfigurationSupported,
               r.OtherStatefulConfigurationSupported, r.RouterDiscoveryBehavior, r.DadTransmits, r.BaseReachableTime,
               r.RetransmitTime, r.PathMtuDiscoveryTimeout, r.LinkLocalAddressBehavior, r.LinkLocalAddressTimeout,
               r.Metric, r.NlMtu, r.Connected, r.SupportsWakeUpPatterns, r.SitePrefixLength,
               r.SupportsNeighborDiscovery, r.DisableDefaultRoutes);
    printf("\n");
}

int main(void)
{
    MIB_IF_TABLE2 *ifs;
    NET_LUID lo;
    DWORD ret, i, count = 0;
    BOOL found = FALSE;

    printf("measure iphlpapi-loopback-tables format=1 arch=%s\n", sizeof(void *) == 8 ? "x86_64" : "i686");
    ret = GetIfTable2(&ifs);
    printf("iftable ret=%lu\n", ret);
    if (ret) return 2;
    for (i = 0; i < ifs->NumEntries; i++)
    {
        if (ifs->Table[i].Type == IF_TYPE_SOFTWARE_LOOPBACK && !found) { lo = ifs->Table[i].InterfaceLuid; found = TRUE; }
        count++;
    }
    printf("iftable rows=%lu loopback_found=%d\n", count, found);
    FreeMibTable(ifs);
    if (!found) return 2;
    measure_if(lo);
    measure_ipif(AF_INET, lo);
    measure_ipif(AF_INET6, lo);
    measure_unicast(AF_INET, lo);
    measure_unicast(AF_INET6, lo);
    measure_routes(AF_INET, lo);
    measure_routes(AF_INET6, lo);
    printf("complete %d\n", !incomplete);
    return incomplete ? 2 : 0;
}
