#!/bin/sh
set -eu

dylib="$1"

rewrite_deps() {
  from_prefix="$1"
  to_prefix="$2"
  exclude_pattern="${3:-}"

  otool -L "$dylib" \
    | awk -v from="$from_prefix" -v exclude="$exclude_pattern" '
        NR > 1 && index($1, from) == 1 {
          if (exclude != "" && $1 ~ exclude) next
          old = $1
          name = $1
          sub(from, "", name)
          print old, name
        }
      ' \
    | while read old name; do
        install_name_tool -change "$old" "${to_prefix}${name}" "$dylib"
      done
}

rewrite_deps '@loader_path/' '@loader_path/../../../macosx/lib/' 'SDKExtension'
rewrite_deps '@rpath/'       '@loader_path/../../../macosx/lib/'

install_name_tool \
  -change '@loader_path/libSDKExtension.1.0.0.dylib' \
          '@loader_path/../../SDKExtension/macosx/libSDKExtension.1.0.0.dylib' \
          "$dylib"
