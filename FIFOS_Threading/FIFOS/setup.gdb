# Load the ELF executable
file ./memos_ELF-1

# Then configure TUI
tui enable
layout regs

# Connect to QEMU instance
target remote localhost:1234
set architecture i8086
break *_start
continue
