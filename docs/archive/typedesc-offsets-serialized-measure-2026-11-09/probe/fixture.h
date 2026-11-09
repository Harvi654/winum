/* Original, AI-assisted (Codex), LGPL-2.1-or-later.
 * Rebuild only our generated PTR(I4) seed using the open Wine MSFT layout.
 * No arbitrary/vendor file input. This does not use Wine's nested writer. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint32_t offsets_get32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void offsets_put32(unsigned char *p, uint32_t value)
{
    p[0] = value;
    p[1] = value >> 8;
    p[2] = value >> 16;
    p[3] = value >> 24;
}

static int offsets_span(size_t start, size_t length, size_t size)
{
    return start <= size && length <= size - start;
}

static int offsets_fixture(const unsigned char *seed, size_t size, uint32_t boundary,
                           unsigned char **output, size_t *output_size)
{
    uint32_t dir, typeinfo, desc, desc_size, scalar, arrays_size;
    size_t new_desc, new_arrays, total, i;
    unsigned char *data, *td, *ad;

    *output = NULL;
    *output_size = 0;
    if (boundary != 0x8000 && boundary != 0x10000) return 0;
    if (!seed || size < 0x54 || size > 1024 * 1024 || memcmp(seed, "MSFT", 4)) return 0;
    if (offsets_get32(seed + 0x20) != 1) return 0;
    dir = 0x58 + ((offsets_get32(seed + 0x14) & 0x100) ? 4 : 0);
    if (!offsets_span(dir, 15 * 16, size)) return 0;
    typeinfo = offsets_get32(seed + dir);
    if (!offsets_span(typeinfo, 0x64, size) || (offsets_get32(seed + typeinfo) & 15) != 6 ||
        offsets_get32(seed + typeinfo + 0x54) != 0) return 0;
    desc = offsets_get32(seed + dir + 9 * 16);
    desc_size = offsets_get32(seed + dir + 9 * 16 + 4);
    arrays_size = offsets_get32(seed + dir + 10 * 16 + 4);
    if (desc_size != 8 || arrays_size || !offsets_span(desc, desc_size, size)) return 0;
    scalar = offsets_get32(seed + desc + 4);
    if ((offsets_get32(seed + desc) & 0xffff) != 26 ||
        (scalar & 0xffff) != 3 || !(scalar & 0x80000000)) return 0;

    new_desc = (size + 3) & ~(size_t)3;
    new_arrays = new_desc + boundary + 24;
    total = new_arrays + boundary + 32;
    data = calloc(1, total);
    if (!data) return 0;
    memcpy(data, seed, size);
    offsets_put32(data + dir + 9 * 16, (uint32_t)new_desc);
    offsets_put32(data + dir + 9 * 16 + 4, boundary + 24);
    offsets_put32(data + dir + 10 * 16, (uint32_t)new_arrays);
    offsets_put32(data + dir + 10 * 16 + 4, boundary + 32);
    td = data + new_desc;
    ad = data + new_arrays;
    /* Fill unused entries with the already generated valid PTR(I4) record.
     * Entry zero remains the alias root; existing typeinfo metadata stays. */
    for (i = 0; i < boundary + 24; i += 8) memcpy(td + i, seed + desc, 8);
    offsets_put32(td, 0x7ffe001a);             /* PTR, complex pointee */
    offsets_put32(td + 4, boundary);          /* outer CARRAY typedesc */
    offsets_put32(td + boundary, 0x7ffe001c);
    offsets_put32(td + boundary + 4, boundary); /* outer array description */
    offsets_put32(td + boundary + 8, 0x7ffe001a);
    offsets_put32(td + boundary + 12, boundary + 16); /* PTR to inner CARRAY */
    offsets_put32(td + boundary + 16, 0x7ffe001c);
    offsets_put32(td + boundary + 20, boundary + 16); /* inner array description */
    /* Keep unused array records valid too, in case the native loader scans
     * this segment sequentially rather than only following references. */
    for (i = 0; i < boundary + 32; i += 16)
    {
        offsets_put32(ad + i, scalar);
        offsets_put32(ad + i + 4, 0x00080001);
        offsets_put32(ad + i + 8, 2);
    }
    offsets_put32(ad + boundary, boundary + 8); /* outer element: PTR typedesc */
    offsets_put32(ad + boundary + 4, 0x00080001); /* one bound, eight bytes */
    offsets_put32(ad + boundary + 8, 2);
    offsets_put32(ad + boundary + 16, scalar);  /* inner element: inline I4 */
    offsets_put32(ad + boundary + 20, 0x00080001);
    offsets_put32(ad + boundary + 24, 2);
    *output = data;
    *output_size = total;
    return 1;
}
