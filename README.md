# TokiOS

一个从头写的极小 x86 内核 + 自然语言 Shell。

## 构建

需要 i386 交叉编译链（`as`、`gcc -m32`、`ld -m elf_i386`）。

```bash
as --32 boot.S -o boot.o
gcc -m32 -c kernel.c gdt.c idt.c isr.c keyboard.c shell.c \
    -ffreestanding -nostdlib -fno-pie -fno-stack-protector
ld -m elf_i386 -T linker.ld boot.o kernel.o gdt.o idt.o isr.o keyboard.o shell.o \
    -o tokios.bin
```

## 运行

```bash
qemu-system-i386 -machine pc -kernel tokios.bin
```

支持 QEMU 7 ~ 11（含 PVH ELF Note）。

## 结构

```
boot.S      入口 + Multiboot 头 + GDT/IDT flush + IRQ 存根
kernel.c    内核入口（清屏，显示 banner，设光标）
gdt.c/h     GDT（Null + Code + Data）
idt.c/h     IDT 初始化 + PIC 重映射
isr.c/h     中断分派：异常 → 未处理，IRQ1 → 键盘
keyboard.c  键盘中断 → scancode→ASCII → 回显 / 缓冲
shell.c/h   Shell 命令解析与执行
linker.ld   链接脚本
```

## 命令

| 命令 | 简写 | 说明 |
|------|------|------|
| `cout <text>` | | 输出文本 |
| `clear` | `cls` | 清屏 |
| `where` | `w` | 打印光标坐标 |
| `go <row>` | | 跳转到指定行 |
| `up [n]` | | 光标上移 n 行 |
| `down [n]` | | 光标下移 n 行 |
| `home` | | 光标归零 |
| `help` | `?` | 列出所有命令 |
| `shutdown` | | 停机 |
| `show` `to` `new` `del` `copy` `move` | | 文件操作（fs 未实现） |

## 许可

MIT
