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

---

一个 x86 内核。大一学生写的。跑在 QEMU 上，还没上真机。

MVP 目标：最小内核（~10k 行 C）+ 丰富的中文 Shell。

## 当前状态

能启动，能打字，有个 shell。没了。

```
TokiOS> cout yo
yo

TokiOS> where
row=3, col=0
```

九个命令，文件系统的全都还没写。现在就是个带键盘中断的打字机。

## 设计共识（2026-06-14）

### 进程模型

- `run myapp.tk` 一步启动，不学 fork+exec
- 进程即文件：删除 `.tk` = 进程结束，无僵尸/匿名进程
- `show /proc` 列出运行中的 `.tk`

### .tk 文件格式

```
tok\0(4B) | 入口地址(4B) | 代码大小(4B) | 权限(1B) | 保留(3B) | [钩子名表] | 机器码
```

权限：`0`=ring3, `1`=ring1, `2`=kami。保留区放钩子数量 + 钩子名表指针。

### 系统调用：函数钩子

不走 `int 0x80`。内核维护符号表，加载 `.tk` 时查表填跳转区。`.tk` 直接 `call [跳转表+偏移]`。链接器约 50 行 C。

### 权限模型

- 默认 ring 3
- ring 1（本地键盘用户），ring 3（SSH 用户，密钥验证→ring 1）
- 提权走公钥签名/助记词/物理密钥，内核最终判定
- 只降不升

### 进程依赖

- `.tk` 文件头声明子进程依赖 → 进程树 = 文件树
- 父作用于子（杀/重启/读状态）天然成立

### 共享内存

- `tk_share` 钩子握手，显式声明
- 跨 ring 共享需签名+审计
- 继承单向往上：子死→父，父死→祖（内存不掉给低 ring）
- 内存冲突按进程树深度仲裁（靠近 `kami` 的优先）

### 崩溃模型

进程崩溃 → dump → kill 进程 → reboot 进程（非全系统）

## 进度记录

**2026-06-13**
Multiboot → GDT → IDT → PIC 重映射 → 键盘中断 → scancode→ASCII → VGA 回显 → 光标 → 输入缓冲 → Shell 命令分发。

用了一整天跟 QEMU 11 斗。Multiboot1 的 `-kernel` 被废弃了，补了 Multiboot2 头，不行。最后加了 PVH ELF Note 才过。下次先写 `qemu-system-i386 --version`，少走弯路。

Shell 目前是 `strncmp` + `if-else`。没有 lexer 没有 parser，就这样先跑着。命令设计成自然语言动词——新手敲 `cout` 就能用，不需要先学 `echo`。

**TODO**
- 分页。没写。现在跑在物理地址上，像裸奔。
- 文件系统。`show` `new` `del` `copy` `move` 全是空的，等磁盘驱动。
- 用户态。Ring 3 + Ring 1 的门都还没画。
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
