/**
 * hello_tk.c — TokiOS ring3 demo
 *
 * Build on Arch VM:
 *   gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -Ttext=0x200000 \
 *       hello_tk.c -o hello.elf
 *   objcopy -O binary hello.elf hello.tk
 *
 * In QEMU-TokiOS:
 *   run hello.tk
 */

/* ---- syscall numbers (match syscall.h) ---- */
enum {
    SYS_COUT       = 1,
    SYS_GETCH      = 2,
    SYS_CUR_SET    = 3,
    SYS_CUR_GET    = 4,
    SYS_TFS_LIST   = 5,
    SYS_TFS_READ   = 6,
    SYS_TFS_CREATE = 7,
    SYS_TFS_DELETE = 8,
    SYS_EXIT       = 9,
};

/* syscall: int 0x80, eax=num, ebx/ecx/edx=args, ret in eax */
static inline int syscall3(int num, int a1, int a2, int a3)
{
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(a1), "c"(a2), "d"(a3)
    );
    return ret;
}

static inline int  syscall1(int num, int a1)      { return syscall3(num, a1, 0, 0); }
static inline int  syscall2(int num, int a1, int a2) { return syscall3(num, a1, a2, 0); }
static inline void syscall0(int num)               { syscall3(num, 0, 0, 0); }

/* wrappers */
#define cout(msg)        syscall1(SYS_COUT, (int)(msg))
#define getch()          syscall0(SYS_GETCH)
#define cursor_set(pos)  syscall1(SYS_CUR_SET, (pos))
#define cursor_get()     syscall0(SYS_CUR_GET)
#define tfs_list()       syscall0(SYS_TFS_LIST)
#define tfs_read(n,b,sz) syscall3(SYS_TFS_READ, (int)(n), (int)(b), (sz))
#define tfs_create(n,d,l) syscall3(SYS_TFS_CREATE, (int)(n), (int)(d), (l))
#define tfs_delete(n)    syscall1(SYS_TFS_DELETE, (int)(n))
#define exit()           syscall0(SYS_EXIT)

/* ---- entry ---- */
__attribute__((section(".text")))
void _start(void)
{
    cout("Hello from TokiOS Ring 3!\n");
    cout("Press any key to exit...\n");
    getch();
    exit();
}
