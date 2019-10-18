GECKO_OBJDIR=${GECKO_OBJDIR:-objdir-gecko}
SYSROOT=$GECKO_OBJDIR/dist/sysroot/
GONK_PATH=${GONK_PATH:-.}
GDB=$GONK_PATH/gonk-misc/arm-unknown-linux-androideabi-gdb

if [ -z "$GECKO_OBJDIR" -o ! -e "$GECKO_OBJDIR/config.log" ]; then
    echo "GECKO_OBJDIR is invalid!" >&2
    exit 255
fi

if [ ! -e "$SYSROOT" ]; then
    echo "Run prepare-gdb-syms.sh at first." >&2
    exit 255
fi

if [ -z "$GONK_PATH" -o ! -e "$GDB" ]; then
    echo "GONK_PATH is invalid!" >&2
    exit 255
fi

echo -e "Please run b2g with \e[31mgdbserver at localhost:8859\e[0m"
echo -e "For example, \e[92madb shell 'COMMAND_PREFIX=\"gdbserver localhost:8859\" b2g.sh'\e[0m"
echo
echo

adb forward tcp:8859 tcp:8859
$GDB -n -ex "set sysroot $SYSROOT" -ex "set debug-file-directory $SYSROOT" -ex 'target remote localhost:8859'
