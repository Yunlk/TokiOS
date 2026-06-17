#include "auth.h"
#include "tfs.h"
#include "isr.h"
#include <stdint.h>

/* ================================================================
 * SHA-256 (FIPS 180-4)
 * ================================================================ */

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
    0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
    0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
    0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
    0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
    0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
    0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
    0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
    0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR(x,n)  (((x)>>(n))|((x)<<(32-(n))))
#define SHR(x,n)   ((x)>>(n))
#define CH(x,y,z)  (((x)&(y))^(~(x)&(z)))
#define MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define EP0(x)     (ROTR(x,2)^ROTR(x,13)^ROTR(x,22))
#define EP1(x)     (ROTR(x,6)^ROTR(x,11)^ROTR(x,25))
#define SIG0(x)    (ROTR(x,7)^ROTR(x,18)^SHR(x,3))
#define SIG1(x)    (ROTR(x,17)^ROTR(x,19)^SHR(x,10))

static void sha256_transform(uint32_t *S, const uint8_t *blk) {
    uint32_t w[64];
    for (int i=0;i<16;i++)
        w[i] = ((uint32_t)blk[i*4]<<24)|((uint32_t)blk[i*4+1]<<16)
              |((uint32_t)blk[i*4+2]<<8)|(uint32_t)blk[i*4+3];
    for (int i=16;i<64;i++)
        w[i]=SIG1(w[i-2])+w[i-7]+SIG0(w[i-15])+w[i-16];

    uint32_t a=S[0],b=S[1],c=S[2],d=S[3],e=S[4],f=S[5],g=S[6],h=S[7];
    for (int i=0;i<64;i++) {
        uint32_t t1=h+EP1(e)+CH(e,f,g)+K[i]+w[i];
        uint32_t t2=EP0(a)+MAJ(a,b,c);
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    S[0]+=a;S[1]+=b;S[2]+=c;S[3]+=d;
    S[4]+=e;S[5]+=f;S[6]+=g;S[7]+=h;
}

void sha256(const uint8_t *data, uint32_t len, uint8_t *hash) {
    uint32_t S[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                   0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    uint8_t blk[64]; uint32_t bl=0;
    uint64_t bits=(uint64_t)len*8;

    for (uint32_t i=0;i<len;i++) {
        blk[bl++]=data[i];
        if (bl==64) { sha256_transform(S,blk); bl=0; }
    }
    blk[bl++]=0x80;
    if (bl>56) { while(bl<64) blk[bl++]=0; sha256_transform(S,blk); bl=0; }
    while(bl<56) blk[bl++]=0;
    for (int i=7;i>=0;i--) { blk[56+i]=(uint8_t)(bits&0xFF); bits>>=8; }
    sha256_transform(S,blk);

    for (int i=0;i<8;i++) {
        hash[i*4]  =(S[i]>>24)&0xFF; hash[i*4+1]=(S[i]>>16)&0xFF;
        hash[i*4+2]=(S[i]>>8)&0xFF;  hash[i*4+3]=S[i]&0xFF;
    }
}

/* ================================================================
 * Multi-user auth — user table + /etc/passwd 格式 "user:hash\n"
 * ================================================================ */

static user_entry_t users[MAX_USERS];
static int  current_uid = -1;    // -1 = 未登录
static int  auth_locked  = 0;
static int  fail_cnt     = 0;

/* ---- tiny helpers ---- */
static int slen(const char *s) { int n=0; while(*s++)n++; return n; }

static int scmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void hex_encode(const uint8_t *raw, char *hex) {
    const char *d="0123456789abcdef";
    for (int i=0;i<32;i++) { hex[i*2]=d[raw[i]>>4]; hex[i*2+1]=d[raw[i]&0xF]; }
    hex[64]=0;
}

/* ---- 重建 /etc/passwd 并写入 TFS ---- */
static void passwd_sync(void) {
    char buf[512];
    int pos = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        if (!users[i].used) continue;
        for (int j = 0; users[i].name[j]; j++)
            buf[pos++] = users[i].name[j];
        buf[pos++] = ':';
        for (int j = 0; j < 64; j++)
            buf[pos++] = users[i].hash[j];
        buf[pos++] = '\n';
    }
    tfs_delete("/etc/passwd");
    tfs_create("/etc/passwd", buf, pos);
}

/* ---- 查找用户, 返回 uid 或 -1 ---- */
static int user_find(const char *name) {
    for (int i = 0; i < MAX_USERS; i++)
        if (users[i].used && scmp(name, users[i].name) == 0)
            return i;
    return -1;
}

/* ---- 初始化：读 /etc/passwd，没有则创建 root:toki ---- */
void auth_init(void) {
    for (int i = 0; i < MAX_USERS; i++)
        users[i].used = 0;

    char buf[512];
    int n = tfs_read("/etc/passwd", buf, sizeof(buf));
    if (n <= 0) {
        cursor_write("[auth] new system -- creating tokisora (pass: tokisora)\n");
        /* 直接写入 users[0]，默认用户名/密码均为 tokisora */
        uint8_t raw[32];
        sha256((const uint8_t*)"tokisora", 8, raw);
        char hx[65]; hex_encode(raw, hx);
        for (int j = 0; j < 65; j++)
            users[0].hash[j] = hx[j];
        users[0].name[0] = 't'; users[0].name[1] = 'o';
        users[0].name[2] = 'k'; users[0].name[3] = 'i';
        users[0].name[4] = 's'; users[0].name[5] = 'o';
        users[0].name[6] = 'r'; users[0].name[7] = 'a';
        users[0].name[8] = '\0';
        users[0].used = 1;
        passwd_sync();
        return;
    }

    /* 解析 user:hash\n ... */
    int uid = 0;
    char *p = buf, *end = buf + n;
    while (p < end && uid < MAX_USERS) {
        /* 找 ':' */
        char *colon = p;
        while (colon < end && *colon != ':') colon++;
        if (colon >= end) break;
        int nl = colon - p;
        if (nl > MAX_USERNAME) nl = MAX_USERNAME;
        for (int i = 0; i < nl; i++)
            users[uid].name[i] = p[i];
        users[uid].name[nl] = '\0';

        p = colon + 1;

        /* 读 64 hex chars */
        int h = 0;
        while (p < end && *p != '\n' && h < 64)
            users[uid].hash[h++] = *p++;
        users[uid].hash[h] = '\0';
        if (p < end && *p == '\n') p++;

        if (h == 64) {
            users[uid].used = 1;
            uid++;
        }
    }
}

/* ================================================================
 * 公开 API
 * ================================================================ */

int auth_login(const char *user, const char *pass) {
    if (auth_locked)
        goto do_freeze;

    int uid = user_find(user);
    if (uid < 0) {
        fail_cnt++;
        cursor_write("[auth] no such user\n");
        if (fail_cnt >= 3) { auth_locked = 1; goto do_freeze; }
        return 0;
    }

    uint8_t raw[32];
    sha256((const uint8_t*)pass, slen(pass), raw);
    char hx[65]; hex_encode(raw, hx);

    for (int i = 0; i < 64; i++) {
        if (hx[i] != users[uid].hash[i]) {
            fail_cnt++;
            char fb[4] = {'0' + fail_cnt, 0, 0, 0};
            cursor_write("[auth] wrong password (");
            cursor_write(fb); cursor_write("/3)\n");
            if (fail_cnt >= 3) {
                auth_locked = 1;
                cursor_write("[auth] 3 fails => 30s lockout\n");
                goto do_freeze;
            }
            return 0;
        }
    }

    /* 密码正确 */
    current_uid = uid;
    fail_cnt = 0;
    auth_locked = 0;
    return 1;

do_freeze:
    cursor_write("[auth] locked.  30s freeze...\n");
    for (volatile int i = 0; i < 300; i++)
        for (volatile int j = 0; j < 10000000; j++)
            __asm__ volatile("pause");
    auth_locked = 0;
    fail_cnt = 0;
    cursor_write("[auth] unlocked\n");
    return 0;
}

void auth_logout(void) {
    current_uid = -1;
}

int auth_passwd(const char *new_pass) {
    if (current_uid < 0) {
        cursor_write("passwd: not logged in\n");
        return -1;
    }
    uint8_t raw[32];
    sha256((const uint8_t*)new_pass, slen(new_pass), raw);
    hex_encode(raw, users[current_uid].hash);
    passwd_sync();
    return 0;
}

int auth_adduser(const char *user, const char *pass) {
    if (user_find(user) >= 0) {
        cursor_write("useradd: user exists\n");
        return -1;
    }
    /* 找空槽 */
    int slot = -1;
    for (int i = 0; i < MAX_USERS; i++) {
        if (!users[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        cursor_write("useradd: table full\n");
        return -1;
    }

    int nl = slen(user);
    if (nl > MAX_USERNAME) nl = MAX_USERNAME;
    for (int i = 0; i < nl; i++)
        users[slot].name[i] = user[i];
    users[slot].name[nl] = '\0';

    uint8_t raw[32];
    sha256((const uint8_t*)pass, slen(pass), raw);
    hex_encode(raw, users[slot].hash);
    users[slot].used = 1;

    passwd_sync();
    return 0;
}

const char *auth_whoami(void) {
    if (current_uid < 0) return "";
    return users[current_uid].name;
}

int auth_logged_in(void) {
    return current_uid >= 0;
}
