<div align="center">

```
  ::::::::::: ::::::::  :::    ::: ::::::::::: ::::::::   :::::::: 
     :+:    :+:    :+: :+:   :+:      :+:    :+:    :+: :+:    :+: 
    +:+    +:+    +:+ +:+  +:+       +:+    +:+    +:+ +:+         
   +#+    +#+    +:+ +#++:++        +#+    +#+    +:+ +#++:++#++   
  +#+    +#+    +#+ +#+  +#+       +#+    +#+    +#+        +#+    
 #+#    #+#    #+# #+#   #+#      #+#    #+#    #+# #+#    #+#     
###     ########  ###    ### ########### ########   ########       
```

</div>

<div align="center">

![license](https://img.shields.io/badge/license-MIT-blue)
![status](https://img.shields.io/badge/status-half--baked-orange)
![arch](https://img.shields.io/badge/arch-x86-lightgrey)

</div>

---

一个 x86 内核。大一学生写的。跑在 QEMU 上，还没上真机。

## 当前状态

能启动，能打字，有个 shell。没了。

```
TokiOS> cout yo
yo

TokiOS> where
row=3, col=0
```

九个命令，文件系统的全都还没写。现在就是个带键盘中断的打字机。

## 进度记录

**2026-06-13**
Multiboot → GDT → IDT → PIC 重映射 → 键盘中断 → scancode→ASCII → VGA 回显 → 光标 → 输入缓冲 → Shell 命令分发。

用了一整天跟 QEMU 11 斗。Multiboot1 的 `-kernel` 被废弃了，补了 Multiboot2 头，不行。最后加了 PVH ELF Note 才过。下次先写 `qemu-system-i386 --version`，少走弯路。

Shell 目前是 `strncmp` + `if-else`。没有 lexer 没有 parser，就这样先跑着。命令设计成自然语言动词——新手敲 `cout` 就能用，不需要先学 `echo`。

**TODO**
- 分页。没写。现在跑在物理地址上，像裸奔。
- 文件系统。`show` `new` `del` `copy` `move` 全是空的，等磁盘驱动。
- 用户态。Ring 3 的门都还没画。
- 真机启动。UEFI 还没碰。

## 构建

```bash
as --32 boot.S -o boot.o
gcc -m32 -c *.c -ffreestanding -nostdlib -fno-pie -fno-stack-protector
ld -m elf_i386 -T linker.ld *.o -o tokios.bin
```

没有构建系统。就四行 shell。

## 文件

```
boot.S      Multiboot/PVH 头，GDT/IDT flush，IRQ 汇编存根
kernel.c    清屏，打名字，交棒给 shell。就这么点。
gdt.c/h     平坦内存，Ring 0。两个段。
idt.c/h     中断向量，PIC 重映射。
isr.c/h     中断分派。只接了键盘，其他的打行字继续跑。
keyboard.c  128 项查表 → VGA 回显 + 缓冲 + 光标。一个文件。
shell.c/h   命令分发。strncmp + atoi。没有抽象语法树。
linker.ld   放好 Multiboot 头的位置。
```

## 当前命令

| 命令 | 效果 |
|------|------|
| `cout <text>` | 回显 |
| `clear` / `cls` | 清屏 |
| `where` / `w` | 光标坐标 |
| `go <row>` | 光标跳到第 row 行 |
| `up [n]` / `down [n]` | 光标上/下移 |
| `home` | 光标归零 |
| `help` / `?` | 列出命令 |
| `shutdown` | `hlt` |

## 顺手记的

- 中断是内核唯一的异步入口。也只应该是唯一的。
- 内核不 panic，它真的停机。`hlt`。
- 堆栈 4KB。不够说明你在上面干不该堆栈干的事。
- 不需要 `malloc`，因为还没写。
- 一个文件一个概念。键盘驱动就该一个文件。

## 许可

MIT。拿去拆，别署名。
