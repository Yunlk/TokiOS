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

## 已知攻击面 & 缓解

写设计的时候先把自己当攻击者看了一圈。

| 问题 | 缓解 | 状态 |
|------|------|------|
| 无分页 → 任意物理内存读写 | 分页（页表 + R/W + U/S 位） | TODO |
| 异常处理器打行字继续跑 | 非键盘异常 → dump → kill 进程 → halt | TODO |
| 4KB 栈无守护页/canary | 栈底 canary + 分页后守护页 | TODO |
| .tk 头解析无边界检查 | hook_count ≤ 64，名表指针在代码区间内 | TODO |
| 钩子一次性信任，无法热撤销 | hook_dispatch 中间层 + `revoke_hook()` | TODO |
| 键盘无认证 → ring 1 | 启动密钥 / U 盘物理密钥 | TODO |
| 导出钩子无边界检查 | 每个钩子自检输出区（如 vga_write 限 VGA_END） | TODO |

总工作量约 150 行 C。前 3 项分页前做，第 4 项跟 .tk 加载一起，后 3 项分页后顺手带掉。

## 与之相近的 OS

**Exokernel（MIT, 1995）** — 最像。
内核不抽象硬件，只做安全多路复用。应用拿函数指针直接操作资源。
TokiOS 的"内核导出 hook、.tk 声明依赖"就是这套。
麻省理工写了论文，我们写了实现。

**Plan 9（Bell Labs）** — 精神祖先。
"一切皆文件"推到极致。`/proc` 里每个运行中的进程就是一个文件。
TokiOS 的"进程 = .tk 文件，删除即杀死"直接从这里来的。
不过 Plan 9 把网络协议栈也当文件，TokiOS 还没想那么远。

**xv6（MIT）** — 同是教学内核，方向相反。
xv6 教你怎么做一个小型 Unix。fork+exec、管道、文件描述符、POSIX。
TokiOS 教你怎么不学 Unix。没有 fork，没有管道，一切是文件名不是 fd。
都 ~10k 行 C，但设计哲学互相否定。

**TempleOS** — 同是一个人的内核，隔离相反。
Terry Davis 一个人写的 x86 内核，所有代码 ring 0，
HolyC 编译器嵌在内核里，上帝通过随机数跟你说话。
TokiOS 也是一个人写，但有 ring 分层、进程隔离、公钥提权。
算是"TempleOS 但偏执于安全"的版本。

**Unikernel（MirageOS, IncludeOS）** — 钩子精神像，架构不像。
应用编译进内核地址空间，直接调硬件函数。
TokiOS 的 hook 模型有相同的 bare-metal 感，
但 TokiOS 是多进程的——多个 .tk 共享同一个内核。

**明确拒绝的**
- Unix/POSIX — 没有 fork，没有 exec，没有 fd
- Linux — 不要 3000 万行，不要设备树，不要 ELF 唯一格式
- Windows — 不是闭源

## 许可

MIT。拿去拆，别署名。
