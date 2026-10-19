# Lab 8 – The Same Calls, Inside Your Own OS (xv6)

Ina Pancenco, FAF-242. xv6-riscv in QEMU (`make qemu`, 3 harts). New programs:
`user/run.c`, `user/pingpong.c`, `user/primes.c`, registered in `UPROGS` as
`$U/_run\`, `$U/_pingpong\`, `$U/_primes\`.

## Part 1 – run, inside xv6

```
$ run
parent: my pid is 3
child: my pid is 4
hello from the child
parent: child 4 ended with status 0
```

With `{ "ls", 0 }` / `exec("ls", args)` the child printed the listing of `/`
and the status was 0. With `exec("nosuchprog", args)`:

```
exec failed
parent: child 4 ended with status 1
```

1. **The three calls in xv6 and on Linux. What is the same?**
   xv6: `fork()`, `exec("echo", args)`, `wait(&status)`.
   Linux (Lab 7): `fork()`, `execlp("ls", "ls", "-l", NULL)`,
   `waitpid(pid, &status, 0)`.
   The model is identical: `fork` copies the process and returns the child's
   PID to the parent and 0 to the child; `exec` replaces the child's memory
   with a new program and returns only on failure; `wait` blocks the parent
   until the child exits and gives back its exit status. Only the surface
   differs: `execlp` is a glibc wrapper (around the `execve` system call) that
   takes a variable argument list and searches `PATH`, while xv6 `exec` takes
   an `argv` array and a path (here relative to `/`); `waitpid` can choose
   which child, xv6 `wait` takes any child.
2. **Who prints "hello from the child"?**
   `echo`. After a successful `exec` the code of `run` is gone from the child;
   the process (still PID 4) now runs `/echo`, which prints its arguments
   `hello from the child`. The line before it, "child: my pid is 4", is the
   last thing printed by `run`'s code in the child.
3. **Why status 1 here and 127 on Linux?**
   Because each program chose that number itself: the xv6 `run.c` has
   `exit(1)` after `exec`, the Linux `run.c` had `exit(127)`. The kernel just
   passes the value given to `exit` to the parent's `wait` (xv6 stores it in
   `p->xstate`; Linux encodes it, and `WEXITSTATUS` extracts it).

## Part 2 – pingpong

```
$ pingpong
4: received ping
3: received pong
$ pingpong
6: received ping
5: received pong
```

1. **Why TWO pipes?**
   A pipe is one-way: data written to `p[1]` comes out of `p[0]`. With a single
   pipe for both directions, the parent, after writing the ping, could read
   its own byte back from the same pipe before the child got it (and the
   child could read its own pong), because both processes would hold both
   ends. One pipe per direction makes it unambiguous who reads what.
2. **Which call makes the parent wait for the reply?**
   `read(c2p[0], &b, 1)`. Reading from an empty pipe blocks (the kernel puts
   the process to sleep in `piperead`) until the child writes the byte, or
   until all write ends are closed. So "ping" is always printed before "pong".
   The final `wait(0)` only collects the child after it has exited.

## Optional – primes

`user/primes.c` builds a chain of processes connected by pipes; each stage
prints its first number `p` and forwards only numbers not divisible by `p`:

```
$ primes
prime 2
prime 3
prime 5
...
prime 31
```

Screenshots of every run are in `screenshots/` (l8_01 … l8_08); the full report
for Labs 7 and 8 is `Lab7-8_InaPancenco_FAF-242.docx`.
