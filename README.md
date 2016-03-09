# bare_core
Bare metal core dump will be able to dump registers and RAM from a bare metal mcu app for transfer to a workstation where the simple format (a memory region and a registers structure) are converted into a core file for analysis with cross tools.

First step is on server side, to take two trivial inputs, an all-zero register file and an all-zero RAM, and generate a core file that can be used by arm-none-eabi-gdb. The cross gdb should show all zero registers and variables in this trivial test.

elfcore.c and elfcore.h are derived from https://sourceforge.net/projects/goog-coredumper/

libelf folder is from libelf project with no changes. I wouldn't include it here if installers didn't install it differently on different platforms. Keeping a copy here helps me compile on both mac and linux.

