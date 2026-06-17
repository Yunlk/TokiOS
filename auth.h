#ifndef AUTH_H
#define AUTH_H
#include <stdint.h>

#define MAX_USERS    8
#define MAX_USERNAME 31

/* user table entry */
typedef struct {
    char name[MAX_USERNAME + 1];
    char hash[65];        // 64 hex + null
    int  used;
} user_entry_t;

void auth_init(void);

/* login: 1=ok, 0=fail/locked */
int  auth_login(const char *user, const char *pass);
void auth_logout(void);

/* change current user's password: 0=ok, -1=fail */
int  auth_passwd(const char *new_pass);

/* add a user: 0=ok, -1=fail (full or dup) */
int  auth_adduser(const char *user, const char *pass);

/* whoami: returns name ("" if not logged in) */
const char *auth_whoami(void);
int  auth_logged_in(void);

/* raw SHA-256 */
void sha256(const uint8_t *data, uint32_t len, uint8_t *hash);

#endif
