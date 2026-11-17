/*
 * MEASURE: what GetAdaptersAddresses and GetIpAddrTable return, as a
 * service that lists its local addresses sees it.
 *
 * Original AI-assisted (Claude), LGPL-2.1-or-later. Public API
 * observations only; prints values and judges nothing.
 *
 * CodeMeter's runtime (ABB RobotStudio) logs "Found IPv4 address:
 * 127.0.0.1 | ..." and "Found IPv6 address: ::1 | ..." at every start on
 * Windows, and nothing under Wine, after calling exactly these functions
 * (Harvi654/ABB-RobotStudio-on-Linux, docs/robotstudio-2026-first-try.md,
 * steps 11-12). This prints, for the same calls, every field such a
 * caller can test, so the Windows and the Wine output can be compared
 * line by line.
 *
 * Privacy: no adapter names, descriptions, GUIDs, MAC addresses, DNS
 * suffixes or non-loopback addresses are printed. Loopback addresses
 * (127.0.0.0/8, ::1) are printed in full; link-local ones only as
 * "link-local" (with their scope id); every other address as
 * "other-v4"/"other-v6". Adapters appear in the order the API returns
 * them, numbered from 0.
 *
 * Exit code: 0 when every call ran, 2 when a buffer could not be got.
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int incomplete;

static const char *lifetime(ULONG value)
{
    if (value == 0xffffffff) return "infinite";
    if (value == 0) return "zero";
    return "finite";
}

static void describe_addr(const SOCKADDR *sa, int len, char *out, size_t size)
{
    if (!sa)
    {
        snprintf(out, size, "null");
        return;
    }
    if (sa->sa_family == AF_INET && len >= (int)sizeof(SOCKADDR_IN))
    {
        const BYTE *b = (const BYTE *)&((const SOCKADDR_IN *)sa)->sin_addr;
        if (b[0] == 127) snprintf(out, size, "v4 loopback %u.%u.%u.%u", b[0], b[1], b[2], b[3]);
        else if (b[0] == 169 && b[1] == 254) snprintf(out, size, "v4 link-local");
        else if (!b[0] && !b[1] && !b[2] && !b[3]) snprintf(out, size, "v4 any 0.0.0.0");
        else snprintf(out, size, "v4 other-v4");
    }
    else if (sa->sa_family == AF_INET6 && len >= (int)sizeof(SOCKADDR_IN6))
    {
        const SOCKADDR_IN6 *s6 = (const SOCKADDR_IN6 *)sa;
        const BYTE *b = s6->sin6_addr.s6_addr;
        static const BYTE loop[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1};
        if (!memcmp(b, loop, 16)) snprintf(out, size, "v6 loopback ::1 scope %lu", (unsigned long)s6->sin6_scope_id);
        else if (b[0] == 0xfe && (b[1] & 0xc0) == 0x80)
            snprintf(out, size, "v6 link-local scope %lu", (unsigned long)s6->sin6_scope_id);
        else snprintf(out, size, "v6 other-v6 scope %lu", (unsigned long)s6->sin6_scope_id);
    }
    else snprintf(out, size, "family %u len %d", sa->sa_family, len);
}

static unsigned int count_unicast(IP_ADAPTER_UNICAST_ADDRESS *u) { unsigned int n = 0; for (; u; u = u->Next) n++; return n; }
static unsigned int count_anycast(IP_ADAPTER_ANYCAST_ADDRESS *a) { unsigned int n = 0; for (; a; a = a->Next) n++; return n; }
static unsigned int count_multicast(IP_ADAPTER_MULTICAST_ADDRESS *m) { unsigned int n = 0; for (; m; m = m->Next) n++; return n; }
static unsigned int count_dns(IP_ADAPTER_DNS_SERVER_ADDRESS *d) { unsigned int n = 0; for (; d; d = d->Next) n++; return n; }
static unsigned int count_gateway(IP_ADAPTER_GATEWAY_ADDRESS_LH *g) { unsigned int n = 0; for (; g; g = g->Next) n++; return n; }

static void measure_adapters(const char *key, ULONG family, ULONG flags)
{
    IP_ADAPTER_ADDRESSES *list, *a;
    ULONG size = 0, ret, ret2;
    unsigned int i = 0;

    ret = GetAdaptersAddresses(family, flags, NULL, NULL, &size);
    printf("%s size_query ret=%lu size_nonzero=%d\n", key, ret, size != 0);
    if (ret != ERROR_BUFFER_OVERFLOW || !size) return; /* e.g. ERROR_NO_DATA: printed, nothing to list */
    size += 4096;
    list = malloc(size);
    if (!list)
    {
        incomplete = 1;
        return;
    }
    ret2 = GetAdaptersAddresses(family, flags, NULL, list, &size);
    printf("%s call ret=%lu\n", key, ret2);
    if (ret2)
    {
        free(list);
        incomplete = 1;
        return;
    }
    for (a = list; a; a = a->Next, i++)
    {
        IP_ADAPTER_UNICAST_ADDRESS *u;
        IP_ADAPTER_PREFIX *p;
        unsigned int j = 0, prefixes = 0;

        printf("%s adapter %u length=%lu iftype=%lu oper=%d flags=%#lx ddns=%d register_suffix=%d dhcp=%d"
               " receive_only=%d no_multicast=%d ipv4=%d ipv6=%d\n",
               key, i, a->Length, a->IfType, a->OperStatus, a->Flags,
               !!(a->Flags & IP_ADAPTER_DDNS_ENABLED), !!(a->Flags & IP_ADAPTER_REGISTER_ADAPTER_SUFFIX),
               !!(a->Flags & IP_ADAPTER_DHCP_ENABLED), !!(a->Flags & IP_ADAPTER_RECEIVE_ONLY),
               !!(a->Flags & IP_ADAPTER_NO_MULTICAST), !!(a->Flags & IP_ADAPTER_IPV4_ENABLED),
               !!(a->Flags & IP_ADAPTER_IPV6_ENABLED));
        printf("%s adapter %u ifindex_nonzero=%d ipv6ifindex_nonzero=%d same_index=%d mtu=%lu phys_len=%lu"
               " tunnel=%d connection=%d luid_iftype=%u ipv4_metric=%lu ipv6_metric=%lu\n",
               key, i, a->IfIndex != 0, a->Ipv6IfIndex != 0, a->IfIndex == a->Ipv6IfIndex, a->Mtu,
               a->PhysicalAddressLength, a->TunnelType, a->ConnectionType,
               (unsigned int)a->Luid.Info.IfType, a->Ipv4Metric, a->Ipv6Metric);
        printf("%s adapter %u zone_link=%lu zone_site=%lu name_set=%d friendly_set=%d description_set=%d"
               " dns_suffix_set=%d unicast=%u anycast=%u multicast=%u dns=%u gateways=%u\n",
               key, i, a->ZoneIndices[ScopeLevelLink], a->ZoneIndices[ScopeLevelSite],
               a->AdapterName && a->AdapterName[0], a->FriendlyName && a->FriendlyName[0],
               a->Description && a->Description[0], a->DnsSuffix && a->DnsSuffix[0],
               count_unicast(a->FirstUnicastAddress), count_anycast(a->FirstAnycastAddress),
               count_multicast(a->FirstMulticastAddress), count_dns(a->FirstDnsServerAddress),
               count_gateway(a->FirstGatewayAddress));
        for (p = a->FirstPrefix; p; p = p->Next)
        {
            char desc[64];
            describe_addr(p->Address.lpSockaddr, p->Address.iSockaddrLength, desc, sizeof(desc));
            printf("%s adapter %u prefix %u %s/%lu\n", key, i, prefixes++, desc, p->PrefixLength);
        }
        for (u = a->FirstUnicastAddress; u; u = u->Next, j++)
        {
            char desc[64];
            describe_addr(u->Address.lpSockaddr, u->Address.iSockaddrLength, desc, sizeof(desc));
            printf("%s adapter %u unicast %u %s sockaddr_len=%d length=%lu flags=%#lx prefix_origin=%d"
                   " suffix_origin=%d dad=%d onlink_prefix=%u valid=%s preferred=%s lease=%s\n",
                   key, i, j, desc, u->Address.iSockaddrLength, u->Length, u->Flags,
                   u->PrefixOrigin, u->SuffixOrigin, u->DadState, u->OnLinkPrefixLength,
                   lifetime(u->ValidLifetime), lifetime(u->PreferredLifetime), lifetime(u->LeaseLifetime));
        }
    }
    printf("%s adapters %u\n", key, i);
    free(list);
}

static void measure_ipaddrtable(const char *key, BOOL sort)
{
    MIB_IPADDRTABLE *table;
    ULONG size = 0, ret, ret2, i;

    ret = GetIpAddrTable(NULL, &size, sort);
    printf("%s size_query ret=%lu size_nonzero=%d\n", key, ret, size != 0);
    if (ret != ERROR_INSUFFICIENT_BUFFER || !size) return;
    table = malloc(size);
    if (!table)
    {
        incomplete = 1;
        return;
    }
    ret2 = GetIpAddrTable(table, &size, sort);
    printf("%s call ret=%lu entries=%lu\n", key, ret2, ret2 ? 0 : table->dwNumEntries);
    if (!ret2)
        for (i = 0; i < table->dwNumEntries; i++)
        {
            MIB_IPADDRROW *row = &table->table[i];
            SOCKADDR_IN sin;
            char desc[64];
            ULONG mask = ntohl(row->dwMask), bits = 0;

            while (mask & 0x80000000) { bits++; mask <<= 1; }
            memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_addr.s_addr = row->dwAddr;
            describe_addr((SOCKADDR *)&sin, sizeof(sin), desc, sizeof(desc));
            printf("%s row %lu %s mask_bits=%lu mask_contiguous=%d index_nonzero=%d bcast=%lu reasm=%lu type=%#x\n",
                   key, i, desc, bits, mask == 0, row->dwIndex != 0, row->dwBCastAddr, row->dwReasmSize,
                   row->wType);
        }
    else incomplete = 1;
    free(table);
}

int main(void)
{
    WSADATA wsa;

    printf("measure iphlpapi-adapters format=1 arch=%s\n", sizeof(void *) == 8 ? "x86_64" : "i686");
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) return 2;
    /* the calls CodeMeter's service makes at its start (Wine +iphlpapi trace) */
    measure_adapters("gaa_unspec_0", AF_UNSPEC, 0);
    measure_adapters("gaa_unspec_2e", AF_UNSPEC, 0x2e);
    measure_adapters("gaa_inet6_0", AF_INET6, 0);
    measure_adapters("gaa_inet_0", AF_INET, 0);
    measure_ipaddrtable("ipaddrtable_unsorted", FALSE);
    measure_ipaddrtable("ipaddrtable_sorted", TRUE);
    WSACleanup();
    printf("complete %d\n", !incomplete);
    return incomplete ? 2 : 0;
}
