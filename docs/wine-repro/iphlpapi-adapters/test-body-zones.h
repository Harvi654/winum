/* Windows 10 (docs/wine-repro/iphlpapi-adapters, format 2): for every
 * adapter with an IP interface, the zone indices of the interface,
 * link and subnet levels (and index 0) are the interface index, the
 * global level is 0 and every other level 1, the same in
 * GetAdaptersAddresses() and GetIpInterfaceEntry() for both families;
 * Ipv6IfIndex is the interface index when there is an IPv6 interface. */
static void test_GetAdaptersAddresses_zones(void)
{
    static const ADDRESS_FAMILY families[] = { AF_INET, AF_INET6 };
    IP_ADAPTER_ADDRESSES *list, *aa;
    MIB_IPINTERFACE_ROW row;
    ULONG size = 0, ret, expect, f, z;
    BOOL has_v6, has_any;

    ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_DNS_SERVER, NULL, NULL, &size);
    ok(ret == ERROR_BUFFER_OVERFLOW, "got %lu\n", ret);
    if (ret != ERROR_BUFFER_OVERFLOW) return;
    list = malloc(size);
    ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_DNS_SERVER, NULL, list, &size);
    ok(!ret, "got %lu\n", ret);
    for (aa = ret ? NULL : list; aa; aa = aa->Next)
    {
        has_v6 = has_any = FALSE;
        for (f = 0; f < ARRAY_SIZE(families); f++)
        {
            memset(&row, 0, sizeof(row));
            row.Family = families[f];
            row.InterfaceLuid = aa->Luid;
            if (GetIpInterfaceEntry(&row)) continue;
            has_any = TRUE;
            if (families[f] == AF_INET6) has_v6 = TRUE;
            for (z = 0; z < ARRAY_SIZE(row.ZoneIndices); z++)
            {
                expect = z <= ScopeLevelSubnet ? aa->IfIndex : z == ScopeLevelGlobal ? 0 : 1;
                ok(row.ZoneIndices[z] == expect, "if %lu family %u zone %lu: got %lu, expected %lu\n",
                   aa->IfIndex, families[f], z, row.ZoneIndices[z], expect);
            }
        }
        if (!has_any) continue;
        ok(aa->Ipv6IfIndex == (has_v6 ? aa->IfIndex : 0), "if %lu: Ipv6IfIndex %lu, ipv6 %d\n",
           aa->IfIndex, aa->Ipv6IfIndex, has_v6);
        for (z = 0; z < ARRAY_SIZE(aa->ZoneIndices); z++)
        {
            expect = z <= ScopeLevelSubnet ? aa->IfIndex : z == ScopeLevelGlobal ? 0 : 1;
            ok(aa->ZoneIndices[z] == expect, "if %lu adapter zone %lu: got %lu, expected %lu\n",
               aa->IfIndex, z, aa->ZoneIndices[z], expect);
        }
    }
    free(list);
}
