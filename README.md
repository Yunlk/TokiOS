<div align="center">
<pre>
  ::::::::::: ::::::::  :::    ::: ::::::::::: ::::::::   ::::::::
     :+:    :+:    :+: :+:   :+:      :+:    :+:    :+: :+:    :+:
    +:+    +:+    +:+ +:+  +:+       +:+    +:+    +:+ +:+
   +#+    +#+    +:+ +#++:++        +#+    +#+    +:+ +#++:++#++
  +#+    +#+    +#+ +#+  +#+       +#+    +#+    +#+        +#+
 #+#    #+#    #+# #+#   #+#      #+#    #+#    #+# #+#    #+#
###     ########  ###    ### ########### ########   ########
</pre>
</div>

---

一个 x86 内核。大一学生写的。跑在 QEMU 上，还没上真机。

MVP 目标：最小内核（~10k 行 C）+ 丰富的中文 Shell。

## 当前状态

分页开着。Ring 3 用户态能跑 `.tk` 程序。`int 0x80` 系统调用框架搭好了，9 个调用号。ToyFS v0.1 线性分配，16 槽位，1MB RAM 盘。Shell 有 8 个命令。

```
TokiOS> ls
  hello.tk  206

TokiOS> run hello.tk
Entered Ring 3 (CS=0x1B) [OK]
entry=0x200000
press any key, q to exit
Hello from TokiOS Ring 3!
TokiOS>
```

内核 21 个源文件，~2k 行 C + ~120 行 ASM。

## 命令

| 命令 | 效果 |
|------|------|
| `ls` | 列出 TFS 文件 |
| `run <file>` | 加载并运行 `.tk`（ring 3） |
| `show <file>` | 打印文件内容 |
| `new <file>` | 新建空文件 |
| `del <file>` | 删除文件 |
| `clear` | 清屏 |
| `info` | CR0 / CR2 / CR3 |
| `help` | 列出命令 |

## 系统调用（当前实现）

`int 0x80`，eax 分发。寄存器传参。

| 号 | 名 | 功能 |
|----|-----|------|
| 1 | `SYS_COUT` | 输出字符串（ebx=地址） |
| 2 | `SYS_GETCH` | 阻塞读一个按键 |
| 3 | `SYS_CUR_SET` | 设光标（ebx=位置） |
| 4 | `SYS_CUR_GET` | 取光标位置 |
| 5 | `SYS_TFS_LIST` | 列文件 |
| 6 | `SYS_TFS_READ` | 读文件 |
| 7 | `SYS_TFS_CREATE` | 创建文件 |
| 8 | `SYS_TFS_DELETE` | 删除文件 |
| 9 | `SYS_EXIT` | 退出进程 |

## 内存布局

```
0x000000    IVT / BDA
0x100000    页目录
0x101000    页表（恒等映射 0-4MB）
0x200000    用户代码（map 进来的）
0x300000    ToyFS 数据区（1MB）
0xB8000     VGA 文本缓冲
```

## 设计共识

### 进程模型（已规划）

- `run myapp.tk` 一步启动，不学 fork+exec
- 进程即文件：删除 `.tk` = 进程结束，无僵尸/匿名进程
- `show /proc` 列出运行中的 `.tk`

### .tk 文件格式（已规划）

```
tok\0(4B) | 入口地址(4B) | 代码大小(4B) | 权限(1B) | 保留(3B) | [钩子名表] | 机器码
```

权限：`0`=ring3, `1`=ring1, `2`=kami。

### 系统调用：函数钩子（已规划，当前临时用 int 0x80）

不走 `int 0x80`。内核维护符号表，加载 `.tk` 时查表填跳转区。`.tk` 直接 `call [跳转表+偏移]`。同 ring 约 3 周期开销。

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
- 继承单向往上：子死→父，父死→祖
- 内存冲突按进程树深度仲裁

### 崩溃模型

进程崩溃 → dump → kill 进程 → reboot 进程（非全系统）

## 进度

**2026-06-15**
Ring 3 用户态打通。`proc_load` 从 TFS 读 `.tk`→`page_map_user` 映射→`proc_start` 用 `iret` 跳 ring 3。`int 0x80` 系统调用框架 + 9 个调用号。ToyFS v0.1（线性分配，删除仅标记）。修了三个典型内核 bug：键盘 IRQ 在 ring 3 时泄露到 shell 输入缓冲、`info` 命令 CR0 打印溢出、二进制嵌入字符串 off-by-one。

**2026-06-14**
分页启用（恒等映射前 4MB + 用户空间 2MB-3MB）。GDT 扩展到 6 条目（含 ring 3 + TSS）。Shell 命令分发从 if-else 重构为函数指针表。键盘环形缓冲区（32 字节）实现。

**2026-06-13**
Multiboot → GDT → IDT → PIC 重映射 → 键盘中断 → scancode→ASCII → VGA 回显 → 光标 → 输入缓冲 → Shell 命令分发。

用了一整天跟 QEMU 11 斗。Multiboot1 的 `-kernel` 被废弃了，补了 Multiboot2 头，不行。最后加了 PVH ELF Note 才过。下次先写 `qemu-system-i386 --version`，少走弯路。

## 构建

```bash
as --32 boot.S -o boot.o
gcc -m32 -c *.c -ffreestanding -nostdlib -fno-pie -fno-stack-protector
ld -m elf_i386 -T linker.ld *.o -o tokios.elf
```

QEMU 11 必须用 ELF 格式（`-kernel tokios.elf`），flat binary 不支持。

## 文件

### 内核

| 文件 | 内容 |
|------|------|
| `boot.S` | Multiboot/PVH 头，GDT/IDT flush，IRQ/系统调用存根 |
| `kernel.c` | 入口：清屏→初始化→TFS→预载 hello.tk→HLT |
| `gdt.c/h` | 6 条目 GDT（ring0 代码/数据，ring3 代码/数据，TSS） |
| `idt.c/h` | 中断向量，PIC 重映射，0x80 trap gate（DPL=3） |
| `isr.c/h` | 中断分派，`regs` 结构体，键盘/光标/系统调用 API |
| `keyboard.c` | 128 项 scancode 查表→VGA 回显+环形缓冲+ring3 隔离 |
| `paging.c/h` | 恒等映射 0–4MB，`page_map_user()` 用户页 |
| `proc.c/h` | 进程加载（TFS→0x200000），`iret` 跳 ring3 |
| `shell.c/h` | 命令分发（函数指针表），`write_int`/`parse_args` |
| `syscall.c/h` | `int 0x80` 分发（SYS_COUT=1 ~ SYS_EXIT=9） |
| `tfs.c/h` | ToyFS v0.1（16 槽位，线性分配，删除标记，1MB RAM 盘） |
| `linker.ld` | 链接脚本（Multiboot 布局，`.bss` ALIGN 4096） |

### ring3 demo

| 文件 | 内容 |
|------|------|
| `hello_tk.c` | 用户态程序源码（cout+getch+exit，编译为 `.tk`） |

## 已知攻击面

| 问题 | 缓解 | 状态 |
|------|------|------|
| 分页启用但无 ring3 异常处理 | 异常→dump→kill 进程→reboot | TODO |
| 栈无守护页/canary | 栈底 canary + 分页后守护页 | TODO |
| .tk 头解析无边界检查 | hook_count ≤ 64，名表指针在代码区间内 | TODO |
| 钩子一次性信任，无法热撤销 | hook_dispatch 中间层 + `revoke_hook()` | TODO |
| 键盘无认证→ring 1 | 启动密钥 / U 盘物理密钥 | TODO |
| TFS 删除不回收空间 | 紧凑 / 链表空闲块 | TODO |
| Ring 3 IOPB 未设置 | TSS IOPB ≥ sizeof(tss_t) | TODO |

## 与之相近的 OS

**Exokernel（MIT, 1995）** — 最像。
内核不抽象硬件，只做安全多路复用。应用拿函数指针直接操作资源。
TokiOS 的"内核导出 hook、.tk 声明依赖"就是这套。

**Plan 9（Bell Labs）** — 精神祖先。
"一切皆文件"推到极致。`/proc` 里每个运行中的进程就是一个文件。
TokiOS 的"进程 = .tk 文件，删除即杀死"直接从这里来的。

**xv6（MIT）** — 同是教学内核，方向相反。
xv6 教你怎么做一个小型 Unix。fork+exec、管道、文件描述符、POSIX。
TokiOS 教你怎么不学 Unix。没有 fork，没有管道，一切是文件名不是 fd。

**TempleOS** — 同是一个人的内核，隔离相反。
Terry Davis 一个人写的 x86 内核，所有代码 ring 0。
TokiOS 也有 ring 分层、进程隔离、公钥提权。

**明确拒绝的**
- Unix/POSIX — 没有 fork，没有 exec，没有 fd
- Linux — 不要 3000 万行，不要设备树
- Windows — 不是闭源

## 许可

MIT。拿去拆，别署名。
