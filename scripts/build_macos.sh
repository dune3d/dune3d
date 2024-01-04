#!/usr/bin/env bash

# on macOS we have a couple of issues (at this time), this script works around those with some hacks.

# first of all, llvm coming with XCode is not new enough for the C++ features that dune3d uses.
# because of this, we have to use llvm from homebrew, and tell meson to use it using a cross file.
meson setup --cross-file macos-$(uname -m).ini build

# the other issue is with meson (https://github.com/mesonbuild/meson/issues/12622) which causes the frameworks to be added incorrectly in the link command.
# to be able to get the (broken) link command from the output, we write the last 10 lines to a error.log file for manipulation later on
meson compile -C build | tee >(tail -n 10 > error.log)

# look for the line saying "FAILED" and then take the next one, which should be the link command
LINK_COMMAND="$(awk '/FAILED/{getline; print}' error.log)"

# remove all words ending with .framework, and repetitions of "-lobjc"
LINK_COMMAND_CLEANED=$(echo "${LINK_COMMAND}" | sed 's/[^ ]*\.framework[^ ]*//g' | sed s/-lobjc//g)

# add those back in the correct way
LINK_COMMAND_RESTORED="$LINK_COMMAND_CLEANED -lobjc -framework IOKit -framework AppKit -framework OpenGL"

cd build

echo ""
echo "Ignoring the error above and running the fixed command in the build folder, hang in there..."
echo ""

# eval the fixed link command
echo "${LINK_COMMAND_RESTORED}"
eval "${LINK_COMMAND_RESTORED}"
