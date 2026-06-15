#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_COUT       1     /* void cout(const char *msg)         */
#define SYS_GETCH      2     /* char getch(void)                   */
#define SYS_CUR_SET    3     /* void cursor_set(int pos)           */
#define SYS_CUR_GET    4     /* int  cursor_get(void)              */
#define SYS_TFS_LIST   5     /* void tfs_list(void)                */
#define SYS_TFS_READ   6     /* int  tfs_read(name, buf, bufsize)  */
#define SYS_TFS_CREATE 7     /* int  tfs_create(name, data, len)   */
#define SYS_TFS_DELETE 8     /* int  tfs_delete(name)              */
#define SYS_EXIT       9     /* void exit(void)                    */

#endif
