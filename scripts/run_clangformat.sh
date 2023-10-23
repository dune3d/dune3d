#!/usr/bin/env bash
#
# This script tries to detect a suitable clang-format program by searching for
# common names. The user can override the detection by specifying the
# clang-format binary in the environment variable CLANG_FORMAT.
#
# When clang-format has been found, the source tree is processed. Any
# corrections are inserted in-place.

# Find clang-format, in priority order, unless overridden by user.
CLANG_FORMAT=${CLANG_FORMAT:-$(command -v clang-format-16)}
CLANG_FORMAT=${CLANG_FORMAT:-$(command -v clang-format-16.0)}
CLANG_FORMAT=${CLANG_FORMAT:-$(command -v clang-format-160)}
CLANG_FORMAT=${CLANG_FORMAT:-$(command -v clang-format)}

if [ ! -z $CLANG_FORMAT ]; then
    CLANG_FORMAT_VERSION="$(${CLANG_FORMAT} -version | cut -d " " -f 4 | cut -d "." -f 1)"

    # Check version
    if [ "$CLANG_FORMAT_VERSION" = "16" ]; then
    # clang-format major version is 16.0
	CLANG_BIN=$CLANG_FORMAT
    else
        echo "clang-format version 16 required. The following was found:"
        $CLANG_FORMAT -version
        exit -1
    fi
else
    # error messages
    echo "$0: could not find a suitable clang-format"
    exit -1
fi
echo "Using clang-format command: ${CLANG_BIN}"

# Search using the binary found
find ./src \( -iname *.h -o -iname *.cpp -o -iname *.hpp -o -iname *.cc -o -iname *.c \) ! -iname bitmap_font_*.c | xargs $CLANG_BIN -style=file -i
exit $?
