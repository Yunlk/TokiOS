# TokiOS

> 一个 x86 实验内核。  
> 设计目标：最小内核 + 丰富的中文 Shell。  
> MIT。拿去拆，别署名。

---

## 当前进度

| 模块 | 状态 |
|------|------|
| Multiboot/PVH 启动 | ✅ |
| GDT（ring 0 code/data） | ✅ |
| IDT + PIC 重映射 | ✅ |
| 键盘中断 + VGA 回显 | ✅ |
| Shell（9 命令） | ✅ |
| 分页 | ⬜ |
| 文件系统 | ⬜ |
| 用户态（ring 1/3） | ⬜ |

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
- 无孤儿进程

### 共享内存

- `tk_share` 钩子握手，显式声明
- 跨 ring 共享需签名+审计
- 继承单向往上：子死→父，父死→祖（内存不掉给低 ring）
- 内存冲突按进程树深度仲裁（靠近 `kami` 的优先）

### 崩溃模型

进程崩溃 → dump → kill 进程 → reboot 进程（非全系统）

### 工程原则

- **中断是唯一的异步来源** — 没有多线程、没有抢占
- **没有 malloc** — 无动态内存分配，4KB 固定栈
- **内核 halt** — 没活干就停，不等事
- **代码即文档** — 注释解释"为什么"，不重复"做什么"

---

## 编译 & 运行

在 Arch Linux（或任何有 gcc-multilib 的环境）编译，在 Windows/QEMU 运行。

```sh
# 编译 (Arch VM)
as --32 boot.S -o boot.o
gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -c kernel.c -o kernel.o
gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -c gdt.c -o gdt.o
gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -c idt.c -o idt.o
gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -c isr.c -o isr.o
gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -c keyboard.c -o keyboard.o
ld -m elf_i386 -T linker.ld -o tokios.bin boot.o kernel.o gdt.o idt.o isr.o keyboard.o

# 运行 (Windows)
qemu-system-i386 -kernel tokios.bin
```

QEMU 11+ 用 PVH ELF Note 启动，兼容 Multiboot 1。

---

## 命名体系

| 类型 | 说明 |
|------|------|
| 根用户 | `kami` |
| 文件后缀 | `.tk` |
| Shell 命令 | `cout` `clear` `where` `go` `up` `down` `home` `help` `shutdown` |
| 别名 | `to=cd` `show=cat` `where=pwd` `say=echo` `del=rm` `new=统一创建` |
