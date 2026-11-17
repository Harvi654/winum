/* An adapter without DNS servers of the requested family keeps an empty
 * list; the call itself does not fail (Windows 10: GetAdaptersAddresses
 * (AF_INET6, 0) succeeds with adapters whose FirstDnsServerAddress is
 * NULL, docs/wine-repro/iphlpapi-adapters). */
static void test_GetAdaptersAddresses_dns_family(void)
{
    static const ULONG families[] = { AF_UNSPEC, AF_INET, AF_INET6 };
    IP_ADAPTER_ADDRESSES *list, *aa;
    ULONG i, ret, ret_skip, size, size_skip;

    for (i = 0; i < ARRAY_SIZE(families); i++)
    {
        size_skip = 0;
        ret_skip = GetAdaptersAddresses(families[i], GAA_FLAG_SKIP_DNS_SERVER, NULL, NULL, &size_skip);
        ok(ret_skip == ERROR_BUFFER_OVERFLOW || ret_skip == ERROR_NO_DATA,
           "family %lu, no DNS servers: got %lu\n", families[i], ret_skip);
        size = 0;
        ret = GetAdaptersAddresses(families[i], 0, NULL, NULL, &size);
        ok(ret == ret_skip, "family %lu: got %lu, without DNS servers %lu\n", families[i], ret, ret_skip);
        if (ret != ERROR_BUFFER_OVERFLOW) continue;

        list = malloc(size);
        ret = GetAdaptersAddresses(families[i], 0, NULL, list, &size);
        ok(!ret, "family %lu: got %lu\n", families[i], ret);
        if (!ret)
            for (aa = list; aa; aa = aa->Next)
                ok(aa->Length >= sizeof(IP_ADAPTER_ADDRESSES_LH), "family %lu: length %lu\n",
                   families[i], aa->Length);
        free(list);
    }
}
