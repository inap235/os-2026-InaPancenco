# Lab 7 – Three Small Programs: Processes, Redirection, a Pipe

Ina Pancenco, FAF-242. Ubuntu 24.04 (VirtualBox), gcc 13.3.0. All programs were
compiled with `gcc -Wall -Wextra` without warnings.

## Part 1 – fork + exec + wait (`run.c`)

```
parent: my PID is 7186
child:  my PID is 7187, my parent is 7186
total 108
...
parent: child 7187 ended with status 0
```

With `execlp("date", ...)` the child printed the date and the status was 0.
With `execlp("nosuchprog", ...)`:

```
exec: No such file or directory
parent: child 7262 ended with status 127
```

1. **Why is `perror("exec")` printed only with nosuchprog?**
   A successful `exec` never returns: it replaces the whole memory image of the
   child (code, data, stack) with the new program, so the lines after `execlp`
   no longer exist in that process. `ls` and `date` were found, so the child
   became them. `nosuchprog` was not found in any directory of `PATH`, so
   `execlp` returned -1 (errno = ENOENT) and the old code continued with
   `perror` and `exit(127)`.
2. **Which number does the child print as "my parent"?**
   7186, which is the PID printed by the parent on the first line, i.e. the PID
   of the `./run` process that called `fork`. (The shell itself had PID 7173; it
   is the parent of `./run`, not of the child.)
3. **Status with ls and with nosuchprog?**
   With `ls` the status is 0 (ls succeeded and called `exit(0)`). With
   `nosuchprog` it is 127, the value passed to `exit(127)` after the failed
   exec – the same code the shell uses for "command not found".

## Part 2 – A zombie, on purpose (`zombie.c`)

```
    PID    PPID STAT CMD
   7278    5546 Ss   bash
   7292    7278 S    ./zombie
   7294    7292 Z    [ls] <defunct>
   7296    7278 R+   ps -o pid,ppid,stat,cmd
```

After 20 seconds: `[1]+ Done ./zombie > /dev/null`, and `ps` shows only
`bash` and `ps`.

1. **Which letter does ls have in STAT?**
   `Z` – zombie (defunct). `ls` has already finished: its memory and open files
   were released, but the kernel keeps its process-table entry (PID and exit
   status) until the parent collects it with `wait`. The parent (7292) is
   sleeping (`S`) in `sleep(20)`.
2. **After 20 seconds, is `[ls] <defunct>` still there?**
   No. The line `waitpid(pid, &status, 0);` reaped it: `waitpid` read the exit
   status and the kernel freed the entry. Then `./zombie` itself ended and was
   reaped by bash (`[1]+ Done`).

## Part 3 – Redirection: `ls -l > out.txt` (`redirect.c`)

```
open gave descriptor 1
done: the listing is in out.txt
```
`head -3 out.txt` shows `total 156`, `out.txt` (size 0) and `pipeline`.

1. **Which number did open give, and why?**
   1. `open` always returns the lowest-numbered free descriptor. In the child
   0 (stdin) and 2 (stderr) were open, and 1 had just been closed, so 1 was
   the smallest free number.
2. **Why does the listing of ls not appear on the screen?**
   `ls` writes to descriptor 1, as always. Descriptors survive `exec`, so in
   the child 1 still refers to `out.txt`, not to the terminal. `ls` does not
   know or care where 1 leads. (The message "open gave descriptor 1" is on the
   screen because it went to descriptor 2.) `out.txt` itself is listed with
   size 0 because `open` created it before `ls` started writing.
3. **To redirect the errors (`2> err.txt`)?**
   Close descriptor 2 before `open`; then `open` returns 2.

## Part 4 – A pipe: `ls /etc | wc -l` (`pipeline.c`)

```
$ ./pipeline
237
$ ls /etc | wc -l
237
```

With `/* close(p[1]); */` the program hangs; `ps` shows `./pipeline_noclose`
and `wc -l` both in state `S`, while `ls` is gone. Ctrl-C (`kill -INT %1`)
stops it.

1. **Same number?** Yes, both print 237.
2. **Which child never ends?** `wc`. `ls` writes its listing and exits
   normally (the first `wait` reaps it); `wc` stays blocked in `read`.
3. **What is wc waiting for?** End of file on the pipe, which `read` returns
   only when every write end of the pipe is closed – and the parent still holds
   `p[1]` open (while waiting for wc), so EOF never comes.

## Optional – three stages (`pipeline3.c`)

`ls /usr/bin | grep x | wc -l` with two pipes and three children; every child
closes all four pipe ends after its `dup`, and so does the parent.

```
$ ./pipeline3
260
$ ls /usr/bin | grep x | wc -l
260
```

Screenshots of every run are in `screenshots/` (l7_01 … l7_10); the full report
for Labs 7 and 8 is `Lab7-8_InaPancenco_FAF-242.docx`.
