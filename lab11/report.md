# Lab 11 – The Race and the Semaphore

Ina Pancenco, FAF-242. Ubuntu 24.04 (VirtualBox, 6 CPUs), gcc 13.3.0. All
programs compiled with `gcc -Wall -pthread`, without -O2 and without warnings.

## Part 1 – See the race (`race.c`)

```
$ ./race
counter = 1496491 (expected 2000000)
$ ./race
counter = 1245639 (expected 2000000)
$ ./race
counter = 1325839 (expected 2000000)
```

**1. counter++ looks like one step. Write the three steps the CPU really does.**
1. load: read `counter` from memory into a register;
2. add: add 1 to the register;
3. store: write the register back into `counter`.

**2. Describe, with those steps, how two threads can lose one addition.**
Say `counter` is 100. Thread A loads 100. Before A stores, thread B also loads
100. A adds 1 and stores 101, then B adds 1 and stores 101 too. Two additions
were made, but the counter went up only by one: B's store overwrote A's.

**3. Why is the result different at every run?**
It depends on when the scheduler switches the threads and how the two CPUs
overlap, and that timing is never the same twice. So the number of lost
additions changes from run to run (here between 500 000 and 750 000 of them).

## Part 2 – Fix it: mutex, then semaphore (`mutex.c`, `sem.c`)

```
$ ./mutex                    (3 times)
counter = 2000000 (expected 2000000)
counter = 2000000 (expected 2000000)
counter = 2000000 (expected 2000000)
$ ./sem                      (3 times)
counter = 2000000 (expected 2000000)
counter = 2000000 (expected 2000000)
counter = 2000000 (expected 2000000)
```

`time` on my machine:

| program | result          | real   | sys    |
|---------|-----------------|--------|--------|
| race    | 1691888 (wrong) | 0.040 s | 0.021 s |
| mutex   | 2000000         | 0.144 s | 0.067 s |
| sem     | 2000000         | 0.232 s | 0.163 s |
| atomic  | 2000000         | 0.078 s | 0.037 s |

**1. Run mutex and sem three times each. Is the answer always 2 000 000?**
Yes, all six runs gave exactly 2000000. Only one thread can be between
lock/unlock (or sem_wait/sem_post) at a time, so the three steps of counter++
are never cut in two.

**2. What would happen in sem.c with sem_init(&s, 0, 2)? Try it and explain.**
```
counter = 1987101 (expected 2000000)
counter = 1994665 (expected 2000000)
counter = 1995304 (expected 2000000)
```
The result is wrong again. With start value 2 the semaphore lets two threads in
at the same time, and we only have two threads, so in practice there is no
protection. It is less wrong than race (fewer losses) because the sem_wait/
sem_post calls make the threads overlap less often, but the race is back.

**3. Why is the locked version slower?**
Every addition now also costs a lock and an unlock (atomic instructions on
the lock word), and when the lock is taken the other thread must wait, often
through a futex system call into the kernel (see the `sys` column: 0.07 s for
mutex and 0.16 s for sem, against 0.02 s for race). The two threads also take turns instead of running
in parallel. sem is a bit slower than mutex because sem_post/sem_wait do a bit
more work.

## Part 3 – A semaphore as a signal (`order.c`)

```
$ ./order
B: waiting for the message...
A: message written
B: I read "hello"
```
B printed its first line at once, then waited one second for A.

**1. Change the start value to 1, compile, run. What does B read now, and why?**
```
B: waiting for the message...
B: I read ""
A: message written
```
B reads an empty string. The semaphore already has 1 unit, so sem_wait does not
block: B takes the unit and reads `message` at once, while A is still sleeping
and has not written "hello" yet (a global array starts full of zeros).

**2. In one sentence: what does a semaphore that starts at 0 mean?**
"Nothing has happened yet": whoever calls sem_wait has to wait until another
thread calls sem_post to say the event happened.

## Part 4 – A semaphore as a counter: the parking lot (`parking.c`)

```
car 1 enters (inside: 1)
car 2 enters (inside: 2)
car 3 enters (inside: 3)
car 2 leaves (inside: 2)
car 4 enters (inside: 3)
car 3 leaves (inside: 2)
car 5 enters (inside: 3)
car 1 leaves (inside: 2)
car 6 enters (inside: 3)
car 5 leaves (inside: 2)
car 4 leaves (inside: 1)
car 6 leaves (inside: 0)
```

**1. What is the largest value of "inside" you see? Which line of code guarantees it?**
3, never more. `sem_wait(&places);` guarantees it: the semaphore starts at 3, every
car that enters takes one unit, and the 4th car blocks there until some car
leaves and calls `sem_post(&places)`.

**2. Set PLACES to 1. What does the semaphore become?**
A mutex (a binary semaphore): only one car can be inside at a time.
```
car 1 enters (inside: 1)
car 1 leaves (inside: 0)
car 2 enters (inside: 1)
car 2 leaves (inside: 0)
...
car 6 enters (inside: 1)
car 6 leaves (inside: 0)
```
The cars go in strictly one after another, and `time` shows 6.1 s instead of 2 s.

## Optional – no lock at all (`atomic.c`)

```
$ time ./atomic
counter = 2000000 (expected 2000000)
real    0m0.078s
```
Correct, and about 2 times faster than the mutex (0.144 s) in my run: atomic_fetch_add is one
indivisible instruction, with no lock and no waiting in the kernel.
