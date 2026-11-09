# Lab 12 – Producer, Consumer and the Other Tools

Ina Pancenco, FAF-242. Ubuntu 24.04 (VirtualBox, 6 CPUs), gcc 13.3.0. All
programs compiled with `gcc -Wall -pthread`, without warnings.

## Part 1 – Producer and consumer (`prodcons.c`)

```
$ ./prodcons
put 1      (in buffer: 1)
put 2      (in buffer: 2)
put 3      (in buffer: 3)
put 4      (in buffer: 4)
            take 1   (in buffer: 3)
put 5      (in buffer: 4)
            take 2   (in buffer: 3)
put 6      (in buffer: 4)
            take 3   (in buffer: 3)
put 7      (in buffer: 4)
            take 4   (in buffer: 3)
put 8      (in buffer: 4)
            take 5   (in buffer: 3)
            take 6   (in buffer: 2)
            take 7   (in buffer: 1)
            take 8   (in buffer: 0)
```

**1. The producer is fast, yet "in buffer" never goes above 4. Which call stops it?**
`sem_wait(&empty)` in producer(). `empty` starts at 4 (the free slots); after 4
puts it is 0, so the producer sleeps there until the consumer takes an item and
calls `sem_post(&empty)`.

**2. Who waits at the start: the consumer or the producer? Which call makes it wait?**
The consumer. It is created first, but `full` starts at 0 (nothing to take), so
it blocks in `sem_wait(&full)` until the producer has put item 1 and called
`sem_post(&full)`.

**3. Lock the mutex BEFORE sem_wait(&empty). What happens when the buffer is full, and why?**
```
$ timeout 5 ./prodcons_swap; echo "exit code: $?"
put 1      (in buffer: 1)
put 2      (in buffer: 2)
put 3      (in buffer: 3)
put 4      (in buffer: 4)
exit code: 124
```
It hangs forever (a deadlock). When the buffer is full the producer takes the
mutex and then sleeps in `sem_wait(&empty)` while still holding the mutex. The
consumer gets `full`, but then blocks on `pthread_mutex_lock(&lock)`, so it can
never take an item and post `empty`. Each one waits for the other. Rule: wait on
the semaphore first, lock the mutex after. (I tested it in a copy,
variants/prodcons_swap.c, so prodcons.c stays correct.)

## Part 2 – The condition variable (`condvar.c`)

```
$ ./condvar
B: not ready, I sleep
A: message written
B: I read "hello"
```

**1. In order.c the semaphore itself remembered that the message was ready. Here, which variable remembers it?**
The normal variable `ready` (0 -> 1), protected by `lock`. The condition variable
`changed` remembers nothing: a signal sent when nobody is waiting is lost.
That is why B checks `ready` and does not just wait on `changed`.

**2. Replace "while (!ready)" with "if (!ready)". Does it still work here?**
Yes, the output is the same three lines. Here there is only one writer, one
reader and one signal, and ready is never set back to 0, so when B wakes up
ready really is 1.

**3. Then why do we write while? Give one reason why a thread can wake up and still find ready == 0.**
- *Spurious wakeups*: POSIX lets `pthread_cond_wait` return even when nobody
  signalled.
- With several waiters (or with `pthread_cond_broadcast`), another thread can
  wake first, take the mutex and use the data (set ready back to 0) before
  this one gets the mutex.

With `while`, the thread checks again and goes back to sleep; with `if` it would
go on with data that is not there.

## Part 3 – Readers and writers (`readers.c`)

```
$ ./readers
writer 1 writes 10  (alone)
reader 2 reads 10   (readers inside: 1)
reader 3 reads 10   (readers inside: 2)
reader 1 reads 10   (readers inside: 3)
writer 2 writes 20  (alone)
```

**1. Three readers are inside at the same time, but a writer is always alone. Which two lines of reader() make that possible?**
```
if (++readers == 1)  sem_wait(&room);    /* the first reader in locks out the writers */
if (--readers == 0)  sem_post(&room);    /* the last reader out lets the writers in */
```
Only the first reader takes `room` and only the last one gives it back, so the
readers in between come in without waiting on room. A writer needs `room` for
itself, so it waits until no reader is inside.

**2. Why does the counter readers need its own lock (rlock)? What could go wrong without it?**
`++readers` and `--readers` are load/add/store, the same race as counter++ in
Lab 11. Without rlock two readers coming in together could both see `readers`
go to 1 and both call `sem_wait(&room)`: the second one blocks forever. Or two
readers leaving together could both miss the 0, and nobody calls
`sem_post(&room)`, so the writers wait forever. The counter can also just end
up wrong.

**3. Readers that keep arriving without a pause. What happens to writer 2?**
It may never get in. As long as at least one reader is inside, `readers` never
goes back to 0, so `room` is never posted, and new readers keep coming in past
the waiting writer. Writer 2 waits forever: that is starvation. This solution
favours readers.

## Part 4 – Build a deadlock, then remove it (`deadlock.c`)

Original version (variants/deadlock_orig.c):
```
$ timeout 5 ./deadlock; echo "exit code: $?"
one: I have A, I want B
two: I have B, I want A
exit code: 124
```

Fixed version (deadlock.c, in two() A is locked first, then B; unlocks swapped too):
```
$ timeout 5 ./deadlock; echo "exit code: $?"
one: I have A, I want B
one: I have both
two: I have B, I want A
two: I have both
done
exit code: 0
```
(The printf text in two() was left as in the handout.)

**1. In the stuck run, which thread holds which mutex, and which one is each thread waiting for? Draw the circle.**
Thread one holds A and waits for B; thread two holds B and waits for A.
```
        holds
  one ────────► A
   ▲            │
   │ wanted by  │ wanted by
   │            ▼
   B ◄──────── two
        holds
```

**2. Remove both sleep(1) from the ORIGINAL version and run it ten times. Does it still always hang? Why is that worse, not better?**
No. In 10 runs 9 finished with "done" (exit 0) and only run 5 hung (exit 124);
in 20 more runs it hung once. Without the sleep, thread one usually takes A and B before thread
two even starts, so the bad interleaving is rare. It is worse because the bug
is still there but hidden: it passes the tests almost every time and then
freezes once in a while, on another machine or under load, and is very hard to
reproduce and debug.

**3. In one sentence: why can taking the locks in the same order never deadlock?**
Because a thread that holds B always holds A already, nobody can hold B and wait
for A, so the circle of waiting can never close.
