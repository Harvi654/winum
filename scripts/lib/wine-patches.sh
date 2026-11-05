# Shared by scripts/build-wine.sh and scripts/make-wine-source.sh: apply the
# patches listed in patches/series and leave the notices the LGPL asks for
# (2a: modified files carry a notice that they were changed, and when).
# Taken over from Autodesk-Inventor-for-Linux (scripts/lib/wine-patches.sh at
# a41b275); the change: the patches and their order come from patches/series.
#
#   wine_apply_patches SRC PATCHDIR [GITDIR COMMIT]
#
# SRC      unpacked vanilla Wine source tree
# PATCHDIR directory with series and the *.patch files it names
# GITDIR, COMMIT  where the date of each patch comes from (last commit that
#          touched patches/<name> up to COMMIT); without them "unknown"
#
# Writes SRC/PATCHES.txt and appends a short comment to the end of every
# changed C/IDL/spec/Makefile file (at the end, so line numbers in messages
# such as asserts stay the same as in the patched code). Sets PATCH_LIST
# (sha256 prefix + name per line) for BUILDINFO.

wine_patch_date() {   # name -> YYYY-MM-DD of the last change of the patch
    local name="$1"
    [ -n "${_WP_GITDIR:-}" ] || { echo unknown; return; }
    git -C "$_WP_GITDIR" log -1 --format=%cs "$_WP_COMMIT" -- "patches/$name" 2>/dev/null | grep . || echo unknown
}

wine_series() {   # PATCHDIR -> the names in series, in order, one per line
    local line
    [ -f "$1/series" ] || { echo "no $1/series" >&2; return 1; }
    while IFS= read -r line || [ -n "$line" ]; do
        line="${line%%#*}"
        line="${line//[[:space:]]/}"
        [ -n "$line" ] && echo "$line"
    done < "$1/series"
}

wine_apply_patches() {
    local src="$1" pdir p name sum date f
    pdir="$(cd "$2" && pwd)" || return 1
    _WP_GITDIR="${3:-}" _WP_COMMIT="${4:-HEAD}"
    PATCH_LIST=""
    declare -A _wp_files=()
    local patches=()
    while IFS= read -r name; do
        case "$name" in */*|.*) echo "series: not a file in patches/: $name" >&2; return 1 ;; esac
        [ -f "$pdir/$name" ] || { echo "series names $name, which is missing" >&2; return 1; }
        patches+=("$pdir/$name")
    done < <(wine_series "$pdir")
    [ "${#patches[@]}" -gt 0 ] || { echo "patches/series is empty" >&2; return 1; }

    {
        echo "Patches applied to this Wine source by winum"
        echo "https://github.com/Harvi654/winum"
        echo
        echo "The patches were written with AI help (Claude by Anthropic, Codex by"
        echo "OpenAI) and tested by people on real machines. They are not part of Wine."
        grep -qx 'wbemprox-timezone.patch' < <(wine_series "$pdir") && \
            echo "wbemprox-timezone.patch comes from https://github.com/cakelesscoder/inventor-on-linux."
        echo "License: LGPL 2.1 or later, like the files they change (COPYING.LIB)."
        echo
        echo "Order of application (patches/series), SHA-256, date of the last change,"
        echo "files changed:"
        echo
    } > "$src/PATCHES.txt"

    for p in "${patches[@]}"; do
        name="$(basename "$p")"
        (cd "$src" && patch -p1 -s -N --fuzz=0 --no-backup-if-mismatch < "$p") || { echo "$name does not apply (without fuzz)" >&2; return 1; }
        echo "    $name"
        sum="$(sha256sum "$p" | cut -d' ' -f1)"
        date="$(wine_patch_date "$name")"
        PATCH_LIST+="${sum:0:16}  $name"$'\n'
        {
            echo "$name"
            echo "    sha256 $sum"
            echo "    date   $date"
            sed -n 's#^+++ b/\([^[:space:]]*\).*#    file   \1#p' "$p"
            echo
        } >> "$src/PATCHES.txt"
        while IFS= read -r f; do
            [ -n "$f" ] && _wp_files["$f"]+="${_wp_files[$f]:+, }$name ($date)"
        done < <(sed -n 's#^+++ b/\([^[:space:]]*\).*#\1#p' "$p")
    done

    for f in "${!_wp_files[@]}"; do
        [ -f "$src/$f" ] || continue
        case "$f" in
            *.c|*.h|*.idl|*.rc|*.y|*.l)
                printf '\n/* Modified by winum: %s.\n * See PATCHES.txt at the top of this source tree. */\n' \
                    "${_wp_files[$f]}" >> "$src/$f" ;;
            *.spec|*Makefile.in)
                printf '\n# Modified by winum: %s.\n# See PATCHES.txt at the top of this source tree.\n' \
                    "${_wp_files[$f]}" >> "$src/$f" ;;
            *) ;;   # other file types: listed in PATCHES.txt only
        esac
    done
}
