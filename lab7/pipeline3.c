#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/* does the same as the shell command:   ls /usr/bin | grep x | wc -l   */

static void close_all(int a[2], int b[2])
{
    close(a[0]); close(a[1]);
    close(b[0]); close(b[1]);
}

int main(void)
{
    int p[2], q[2];                              /* p: ls -> grep,  q: grep -> wc */
    if (pipe(p) < 0 || pipe(q) < 0) { perror("pipe"); return 1; }

    if (fork() == 0) {                           /* ls: its 1 becomes p[1] */
        close(1);
        dup(p[1]);
        close_all(p, q);
        execlp("ls", "ls", "/usr/bin", (char *)NULL);
        perror("exec ls");
        exit(127);
    }

    if (fork() == 0) {                           /* grep: 0 <- p[0], 1 -> q[1] */
        close(0);
        dup(p[0]);
        close(1);
        dup(q[1]);
        close_all(p, q);
        execlp("grep", "grep", "x", (char *)NULL);
        perror("exec grep");
        exit(127);
    }

    if (fork() == 0) {                           /* wc: its 0 becomes q[0] */
        close(0);
        dup(q[0]);
        close_all(p, q);
        execlp("wc", "wc", "-l", (char *)NULL);
        perror("exec wc");
        exit(127);
    }

    close_all(p, q);                             /* the parent uses no end */
    wait(NULL);
    wait(NULL);
    wait(NULL);
    return 0;
}
