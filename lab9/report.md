# Lab 9 – Watching System Calls with strace

Ina Pancenco, FAF-242. Ubuntu 24.04 (VirtualBox), gcc 13.3.0, strace 6.8. The
program was compiled with `gcc -Wall -Wextra` without warnings.

## Part 1 – See the calls of a real command

```
$ strace -f -e trace=openat,read,write,close ls -l      (burst of openat/read/close, then the writes)
$ strace -o filtered.txt -e trace=openat,read,write,close ls -l
$ grep "^write" filtered.txt | cut -c1-70
write(1, "total 8\n", 8)                = 8
write(1, "-rw-rw-r-- 1 ina ina 2214 Nov  2"..., 52) = 52
write(1, "-rw-rw-r-- 1 ina ina 1056 Nov  2"..., 50) = 50
```

Counted with `strace -o`, one `ls -l` in a folder of two files made 67 calls of
these four kinds (31 openat, 20 close, 13 read, 3 write), and 156 system calls
of all kinds.

**Which system call does ls use to print to the screen? To which descriptor
does it write?**
`write`, on descriptor 1 (standard output). In the terminal it made one write
per line of the listing.

## Part 2 – A program that calls the kernel directly (`syscalls.c`)

```
open returned descriptor 3
write sent 14 bytes
read got 14 bytes: hello, kernel
```

```
openat(AT_FDCWD, "note.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 3
write(3, "hello, kernel\n", 14)         = 14
close(3)                                = 0
openat(AT_FDCWD, "note.txt", O_RDONLY)  = 3
read(3, "hello, kernel\n", 63)          = 14
close(3)                                = 0
```

**The descriptor returned by the first openat is 3, not 0, 1 or 2. Why are those
three already taken?**
Every process starts with 0 (standard input), 1 (standard output) and 2
(standard error) already open: it inherits them from the shell, here all three
pointing to the terminal. The kernel always gives the lowest free descriptor, so
the first file the program opens gets 3.

## Part 3 – Count one kind of call

```
$ strace -e trace=openat ls -l 2>&1 >/dev/null | grep -c "^openat"
31
```

strace writes its lines to standard error, so `2>&1` sends them into the pipe,
and `>/dev/null` throws away the listing of `ls`. `grep -c` counts the lines that
start with `openat`. `strace -c -e trace=openat ls -l` confirms it: 31 calls, of
which 17 failed.
