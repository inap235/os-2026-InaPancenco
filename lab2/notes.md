# Lab 2 – Meet the OS You Will Build (xv6)

Ina Pancenco, FAF-242. xv6-riscv in QEMU (`make qemu`, 3 harts). New program:
`user/sleep.c`, registered in `UPROGS` as `$U/_sleep\`.

## Part 2 – Using xv6 as the Unix it is

```
$ ls
$ cat README
$ echo hello xv6
$ ls | grep c
$ wc README
48 336 2441 README
$ usertests -q
...
ALL TESTS PASSED
```

1. **List three programs xv6 ships with.**
   `cat` (prints the contents of files), `grep` (prints the lines that match a
   pattern) and `wc` (counts lines, words and bytes). `ls` also shows `echo`,
   `ls`, `mkdir`, `rm`, `ln`, `kill`, `sh`, `init`, `usertests`, `forktest`,
   `stressfs`, `grind` and `zombie`.
2. **You just used a pipe ( | ) inside xv6. Which two OS features must exist
   for a pipe to work?**
   First, process management: the kernel has to create new processes (`fork`)
   and load programs into them (`exec`), so that `ls` and `grep` run at the
   same time as two separate processes. Second, inter-process communication
   through the kernel: the `pipe()` system call creates a kernel buffer with a
   read end and a write end. File descriptors (with `dup`/`close`) connect the
   output of the first process to the input of the second, and the kernel
   blocks and wakes the two processes as data flows.
3. **In one sentence: how does the xv6 shell compare to the Linux shell from
   Lab 1?**
   The xv6 shell works on the same principles as bash (it runs commands as
   child processes and supports pipes, redirection, `;` and `&`), but it is a
   minimal program of a few hundred lines with no variables, scripting,
   history, tab completion, wildcards or job control, and its prompt is just
   `$`.

## Part 3 – Reading the source code

```
sed -n "1,40p" user/ls.c
sed -n "1,30p" user/cat.c
cat user/user.h
grep -n "sys_read\|sys_write" kernel/sysfile.c | head
```

1. **Which system calls does user/cat.c use, and what does each one ask the
   kernel to do?**
   - `read(fd, buf, n)`: copy up to `n` bytes from the open file (or console)
     `fd` into `buf` in the program's memory. It returns the number of bytes
     read, or 0 at end of file.
   - `write(fd, buf, n)`: copy `n` bytes from `buf` to the file or device
     `fd`. `cat` writes to descriptor 1, standard output (the console).
   - `open(path, O_RDONLY)`: find the file by name in the file system and give
     the process a new file descriptor for reading it.
   - `close(fd)`: release a file descriptor that is no longer needed.
   - `exit(status)`: terminate the process and report the status to its
     parent (the shell).

   Error messages printed with `fprintf(2, ...)` also reach the kernel as
   `write()` calls on descriptor 2.
2. **In which kernel file, and on which line, is sys_read implemented?**
   In `kernel/sysfile.c`, line 69. `sys_write` comes right after it, on
   line 83.
3. **In one sentence: what is the difference between the code in kernel/ and
   the code in user/?**
   The code in `kernel/` is the operating system itself. It runs in privileged
   supervisor mode with direct access to all memory and hardware and
   implements the services. The code in `user/` is ordinary programs running
   in unprivileged user mode, which can get those services only by asking the
   kernel through system calls.

## Part 4 – sleep, inside xv6

In this xv6 version the sleep system call is exposed to user programs as
`pause(int)` (see `user/user.h`), so `user/sleep.c` calls `pause()`. Calling
`sleep()` fails to build with `implicit declaration of function 'sleep'`.

```
$ sleep 10
$ sleep
usage: sleep <ticks>
$ ls sleep
sleep          2 23 34032
```

`sleep 10` returns to the prompt after 10 ticks (about one second). Without an
argument it prints the usage message and exits with status 1.
