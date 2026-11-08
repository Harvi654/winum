# shellcheck shell=bash
# Check a downloaded Wine source tarball against the SHA-256 pinned in
# wine/version.env before anything extracts it.
#
#   wine_verify_tarball <tarball> <version>
#
# For the pinned WINE_VERSION the expected value is WINE_SHA256. For another
# version (WINE_VER_OVERRIDE experiments), or for an old ref whose
# version.env predates WINE_SHA256, pass the expected value in
# WINE_SHA256_OVERRIDE. Prints the verified hash on stdout.
wine_verify_tarball() {
    local tarball="$1" version="$2" expected actual
    if [ "$version" = "$WINE_PINNED_VERSION" ]; then
        expected="${WINE_SHA256:-${WINE_SHA256_OVERRIDE:-}}"
    else
        expected="${WINE_SHA256_OVERRIDE:-}"
    fi
    actual="$(sha256sum "$tarball" | cut -d' ' -f1)"
    if [ -z "$expected" ]; then
        printf 'ERROR: no pinned SHA-256 for wine-%s.tar.xz (computed %s);\n' "$version" "$actual" >&2
        printf '       set WINE_SHA256 in wine/version.env (or WINE_SHA256_OVERRIDE for another version)\n' >&2
        return 1
    fi
    if [ "$actual" != "$expected" ]; then
        printf 'ERROR: wine-%s.tar.xz SHA-256 mismatch: expected %s, got %s\n' "$version" "$expected" "$actual" >&2
        return 1
    fi
    printf '%s\n' "$actual"
}
