# Codexion — Master the race for resources before the deadline masters you

> *This project has been created as part of the 42 curriculum.*

---

## Description

Codexion is a concurrency simulation in C. N coders sit in a circle, sharing N USB dongles. Each coder needs **two dongles at once** to compile quantum code. If a coder goes too long without compiling, they **burn out** — and the simulation ends. Your job: orchestrate threads, mutexes, condition variables, and a scheduler so every coder gets fair, timely access — no deadlocks, no starvation, no burnout.

The program stops when either:
- A coder **burns out** (didn't compile within `time_to_burnout` ms), or
- Every coder has compiled at least `number_of_compiles_required` times.

This is a direct analogue of the **Dining Philosophers Problem** — but with cooldown timers, scheduling policies (FIFO / EDF), and a precise burnout monitor thread.

---

## Usage

```bash
make
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

**Example:**
```bash
./codexion 5 800 200 200 200 3 50 fifo
```

**Arguments:**

| Argument | Type | Description |
|---|---|---|
| `number_of_coders` | int | Number of coders (= number of dongles) |
| `time_to_burnout` | ms | Max time between compile starts before burnout |
| `time_to_compile` | ms | Time to compile (holds both dongles during this) |
| `time_to_debug` | ms | Time spent debugging after compile |
| `time_to_refactor` | ms | Time spent refactoring after debug |
| `number_of_compiles_required` | int | Compiles needed per coder to finish cleanly |
| `dongle_cooldown` | ms | How long a dongle is locked after being released |
| `scheduler` | string | `fifo` or `edf` |

**Expected log format:**
```
0 1 has taken a dongle
1 1 has taken a dongle
1 1 is compiling
201 1 is debugging
401 1 is refactoring
402 2 has taken a dongle
403 2 has taken a dongle
403 2 is compiling
603 2 is debugging
803 2 is refactoring
1204 3 burned out
```

---

## Project Architecture

### Data Structures

```
t_rules  (stack — lives in main)
  ├── number_of_coders
  ├── time_to_burnout / compile / debug / refactor
  ├── number_of_compiles_required
  ├── dongle_cooldown
  ├── start_time          ← master clock: set once before threads spawn
  ├── scheduler           ← "fifo" or "edf"
  └── print_mutex         ← serializes ALL log output

t_coder[N]  (heap array)
  ├── id                  ← 1..N
  ├── thread_id           ← pthread_t
  ├── *left_dongle        → dongles[i]
  ├── *right_dongle       → dongles[(i+1) % N]
  ├── last_compile        ← ms timestamp (EDF deadline = last_compile + time_to_burnout)
  ├── compiles_done       ← counter
  └── *rules              → t_rules

t_dongle[N]  (heap array, circular layout)
  ├── mutex               ← protects is_taken + available_at + wait queue
  ├── cond                ← wait queue: coders block here when dongle is busy
  ├── is_taken            ← 0 = free, 1 = in use
  └── available_at        ← ms cooldown deadline (0 if no cooldown active)
```

### Circular Dongle Layout (N=5 example)

```
        Coder 1
       /       \
  Dongle 5   Dongle 1
    /               \
Coder 5           Coder 2
    \               /
  Dongle 4   Dongle 2
       \       /
        Coder 3 — Dongle 3 — Coder 4

Each coder shares one dongle with each neighbor.
Coder i: left=dongles[i-1], right=dongles[i % N]
```

### Coder Lifecycle (per thread)

```
THREAD START
     │
     ▼
[waiting / thinking]  ←─────────────────────────────────┐
     │                                                   │
     │  try to acquire left_dongle                       │
     │  (blocks on cond if busy or in cooldown)          │
     ▼                                                   │
[has left dongle]                                        │
     │                                                   │
     │  try to acquire right_dongle                      │
     │  (blocks on cond if busy or in cooldown)          │
     ▼                                                   │
[has BOTH dongles]                                       │
     │                                                   │
     │  log: "X is compiling"                            │
     │  sleep(time_to_compile)                           │
     │  update last_compile = now                        │
     │  compiles_done++                                  │
     ▼                                                   │
[release both dongles]                                   │
     │  set is_taken=0, available_at=now+cooldown        │
     │  broadcast on cond (wake waiting coders)          │
     ▼                                                   │
[debugging]  sleep(time_to_debug)                        │
     ▼                                                   │
[refactoring]  sleep(time_to_refactor)                   │
     │                                                   │
     └───────────────────────────────────────────────────┘
                    (loop until simulation_over flag is set)
```

If at any check: `now - last_compile > time_to_burnout` → **burned out** → monitor sets stop flag.

---

## Where I Stand — What Is Done

### Completed

**`parsing.c`** — Full argument validation and population of `t_rules`. Handles missing args, non-numeric values, and invalid scheduler strings.

**`utils.c`**
- `get_time_ms()` — wall-clock ms via `gettimeofday`
- `my_sleep()` — busy-wait loop with `usleep(500)` to avoid oversleeping
- `safe_print()` — mutex-protected log output in the correct format

**`main.c`**
- Parses args, allocates `t_coder[N]` and `t_dongle[N]` on the heap
- Inits all mutexes and condition variables via `init_mutex()`
- Assigns circular dongle pointers via `assigne_dongles()`
- Spawns one thread per coder, joins all threads, destroys all mutexes, frees all memory

**`init.c`**
- `init_mutex()` — inits `pthread_mutex_t` + `pthread_cond_t` per dongle, rollback on failure
- `destroy_mutex()` — destroys mutex + cond per dongle
- `assigne_dongles()` — circular left/right assignment
- `create_threads()` — spawns threads with rollback if one fails
- `join_threads()` — joins all N threads

**`codexion.h`** — All structs, forward declarations, and prototypes complete.

### Started but Incomplete

**`routine.c`** — `is_end_program()` is stubbed with only comments. Needs: a `simulation_over` flag in `t_rules`, a dedicated mutex to protect it, and the logic to check it.

**`coder_routine()` in `init.c`** — Currently prints "is thinking" and returns immediately. The full lifecycle (take dongles → compile → debug → refactor → loop) is not implemented.

---

## What Remains — Next Steps

### 1. Stop flag + monitor thread
Add `int simulation_over` and a `stop_mutex` to `t_rules`. Spawn one **monitor thread** that polls each coder's `last_compile` every ~1ms. When it detects burnout, it logs the event (within 10ms of actual burnout) and sets the flag. Every coder thread checks this flag at the top of each loop iteration.

### 2. Full `coder_routine()` implementation
Replace the stub with the complete lifecycle loop: acquire left dongle → acquire right dongle → compile → release both → debug → refactor → repeat. Each state transition calls `safe_print()`. The loop runs `while (!is_end_program(rules))`.

### 3. Dongle acquisition with `pthread_cond_wait`
Inside a mutex lock, check `is_taken` AND `available_at`. If unavailable, enqueue the coder and call `pthread_cond_wait()`. **Always re-check in a `while` loop** — never `if` — because of spurious wakeups. On release, `pthread_cond_broadcast()` wakes all waiters; the scheduler decides who proceeds.

```c
pthread_mutex_lock(&dongle->mutex);
while (dongle->is_taken || get_time_ms() < dongle->available_at)
    pthread_cond_wait(&dongle->cond, &dongle->mutex);
dongle->is_taken = 1;
pthread_mutex_unlock(&dongle->mutex);
```

### 4. Priority queue (min-heap) for FIFO / EDF
The subject **requires a manual heap implementation in C**. Each dongle holds a heap of waiting coders. On `pthread_cond_broadcast`, the correct coder (earliest arrival for FIFO, nearest deadline for EDF) is given priority. You need: `heap_push()`, `heap_pop()`, `sift_up()`, `sift_down()`.

### 5. Deadlock prevention
If all N coders grab their left dongle simultaneously, they all wait for the right one → deadlock. Fix: have **even-numbered coders pick up right dongle first** (asymmetric ordering). This breaks the circular wait — one of Coffman's four deadlock conditions.

---

## Blocking Cases to Handle

| Problem | Cause | Solution |
|---|---|---|
| **Deadlock** | All coders hold left dongle, wait for right | Asymmetric pickup order (odd: left-first, even: right-first) |
| **Starvation (FIFO)** | A coder is always skipped | FIFO queue guarantees arrival-order service |
| **Starvation (EDF)** | Subject requires liveness for feasible params | EDF naturally prioritizes the most urgent coder |
| **Cooldown bypass** | Coder retakes dongle before cooldown expires | Check `available_at` inside mutex before taking dongle |
| **Interleaved logs** | Two threads print simultaneously | `print_mutex` in `safe_print()` — already done |
| **Burnout detection lag** | Monitor thread too slow | Poll at ~1ms intervals, print within 10ms |
| **Spurious wakeups** | `pthread_cond_wait` returns without signal | Always `while(condition)`, never `if(condition)` |
| **Race on stop flag** | Two threads read/write `simulation_over` unsafely | Protect with `stop_mutex` |

---

## Thread Synchronization Mechanisms

| Primitive | Where | Purpose |
|---|---|---|
| `pthread_mutex_t print_mutex` | `t_rules` | Serializes all log output — no interleaving |
| `pthread_mutex_t mutex` (per dongle) | `t_dongle` | Protects `is_taken`, `available_at`, and the wait queue |
| `pthread_cond_t cond` (per dongle) | `t_dongle` | Blocks waiting coders; wakes them when dongle is released |
| `pthread_mutex_t stop_mutex` *(TODO)* | `t_rules` | Protects the `simulation_over` flag |

**How `print_mutex` prevents interleaved output:**
```
Without mutex:             With mutex:
"10 1 is comp"             "10 1 is compiling"      ← full line, clean
"10 2 is comp"     →       "10 2 is compiling"      ← full line, clean
"iling\niling\n"
```

**Why `while` not `if` on `pthread_cond_wait`:**
```c
// WRONG — if OS wakes this thread spuriously, it proceeds without checking
if (dongle->is_taken)
    pthread_cond_wait(&dongle->cond, &dongle->mutex);

// CORRECT — always re-verify the condition after waking
while (dongle->is_taken || get_time_ms() < dongle->available_at)
    pthread_cond_wait(&dongle->cond, &dongle->mutex);
```

---

## Resources

- [POSIX Threads Programming — Lawrence Livermore](https://hpc-tutorials.llnl.gov/posix/)
- [The Little Book of Semaphores — Allen Downey](https://greenteapress.com/wp/semaphores/)
- [Dining Philosophers Problem — Wikipedia](https://en.wikipedia.org/wiki/Dining_philosophers_problem)
- [Earliest Deadline First Scheduling — Wikipedia](https://en.wikipedia.org/wiki/Earliest_deadline_first_scheduling)
- [pthread_cond_wait man page](https://man7.org/linux/man-pages/man3/pthread_cond_wait.3p.html)
- [Binary Heap / Priority Queue in C](https://www.geeksforgeeks.org/binary-heap/)

**AI usage:** Used to review struct design, clarify `pthread_cond_wait` spurious wakeup semantics, and generate this README. All code is written and fully understood by the author.
