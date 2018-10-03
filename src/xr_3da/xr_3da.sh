#!/bin/sh

SCRIPT_NAME=$(basename "$0")
GAMEROOT=$(dirname -- "$(readlink -f -- "$0")")
if [ -z "$GAMEEXE" ]; then
	GAMEEXE=${SCRIPT_NAME%.*} # strip extension(not required, but do anyway)
fi

#determine platform
UNAME=$(uname)
if [ "$UNAME" = "Darwin" ]; then
	# prepend our lib path to DYLD_LIBRARY_PATH
	export DYLD_LIBRARY_PATH=${GAMEROOT}:$DYLD_LIBRARY_PATH
else
	# prepend our lib path to LD_LIBRARY_PATH
	export LD_LIBRARY_PATH=${GAMEROOT}:$LD_LIBRARY_PATH
fi

# and launch the game
if ! cd "$GAMEROOT"; then
	echo "Failed cd to $GAMEROOT"
	exit
fi

STATUS=42
while [ $STATUS -eq 42 ]; do
	${DEBUGGER} "${GAMEROOT}"/"${GAMEEXE}" "$@"
	STATUS=$?
done
exit $STATUS
