```
████████╗ ██████╗ ██╗  ██╗██╗ ██████╗ ███████╗
╚══██╔══╝██╔═══██╗██║ ██╔╝██║██╔═══██╗██╔════╝
   ██║   ██║   ██║█████╔╝ ██║██║   ██║███████╗
   ██║   ██║   ██║██╔═██╗ ██║██║   ██║╚════██║
   ██║   ╚██████╔╝██║  ██╗██║╚██████╔╝███████║
   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚═╝ ╚═════╝ ╚══════╝
```

> *A kernel so small it fits in your L1 cache. A shell so clean you could teach your mother.*

---

## What

TokiOS is a **from-scratch x86 kernel** written in pure C and AT&T assembly. No libc. No multithreading. No bullshit.

It boots. It takes commands. It gets out of your way.
Natural language verbs. Single-syllable English. Just type what you mean.

```
TokiOS> cout hello world
hello world

TokiOS> where
row=3, col=0
```

## Build

You need a 32-bit x86 toolchain. That's it.

```bash
as --32 boot.S -o boot.o
gcc -m32 -c *.c -ffreestanding -nostdlib -fno-pie -fno-stack-protector
ld -m elf_i386 -T linker.ld *.o -o tokios.bin
```

No cmake. No autotools. No package.json. **Four lines.**

## Run

```bash
qemu-system-i386 -machine pc -kernel tokios.bin
```

QEMU 7 through 11. PVH. Multiboot. Whatever. It just works.

## Anatomy

```
boot.S      Where it all begins. GDT. IDT. IRQ stubs. The real shit.
kernel.c    One function: clear the screen, print a banner, hand off to shell.
gdt.c/h     Flat memory model. Ring 0. No protection, no problem.
idt.c/h     Interrupt vectors. PIC remap. Exceptions politely ignored.
isr.c/h     Dispatch. Only IRQ1 gets a handler — the keyboard.
keyboard.c  Scan codes → ASCII. 128-entry lookup. Echo. Buffer. Cursor.
shell.c/h   strncmp. atoi. write_int. Command dispatch. All in one file.
linker.ld   ELF layout. Multiboot header goes first, or QEMU won't touch it.
```

## Command Reference

### Now
| Verb | Short | Does |
|------|-------|------|
| `cout` | | Print arguments to screen |
| `clear` | `cls` | Blank the terminal |
| `where` | `w` | Cursor coordinates |
| `go <row>` | | Jump to row |
| `up [n]` | | Cursor up |
| `down [n]` | | Cursor down |
| `home` | | Cursor to origin |
| `help` | `?` | List commands |
| `shutdown` | | Halt. Goodbye. |

### Soon™
`show` `to` `new` `del` `copy` `move` — filesystem stubs. The disk driver isn't written yet. You're looking at it at the exact wrong moment.

## Philosophy

- **One file per concept.** Not one file per function.
- **No dynamic allocation.** You want heap? Write it.
- **ASCII art is documentation.**
- **If a command needs more than two syllables, rename it.**

## License

MIT. Take it. Break it. Learn from it. Rewrite it in Rust if you must.
