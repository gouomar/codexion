# Queue Logic — FIFO & EDF for Codexion

---

## 0. Your Roadmap (read this first)

You only need to touch **three things** to finish the queue:

```
STEP 1 ── Add t_waiter + heap[] fields to t_dongle in codexion.h
STEP 2 ── Create queue.c with the 5 heap helpers
STEP 3 ── Rewrite take_dongle() in routine.c to use the heap
```

The two schedulers (FIFO and EDF) share the **exact same heap code**.
The only thing that differs between them is **one line**: how you compute
the `priority` value before inserting into the heap.

Read FIFO first. Once you understand it, EDF is just a different number.

---

## 1. The Problem With Your Current Code

Your current `take_dongle` does this:

```c
/* Current code — the thundering herd problem */
while (dongle->is_taken == 1 || dongle->available_at > get_time_ms())
    pthread_cond_wait(&dongle->cond, &dongle->mutex);
/* whoever the OS wakes up first wins — totally random */
dongle->is_taken = 1;
```

When `release_dongle` calls `broadcast`, **every** waiting coder wakes up
and races. The OS decides who wins. You have no control over order.

You need a **waiting list** inside the dongle. When the dongle is free,
only the coder that is **first in the list** is allowed to take it.
Everyone else checks, sees it is not their turn, and goes back to sleep.

```
Without queue:           With queue (FIFO example):
                         Heap (sorted by arrival time):
  release_dongle()         [0] Coder-3  arrived t=10  ← WINS
  broadcast()              [1] Coder-1  arrived t=15
  OS picks anyone          [2] Coder-5  arrived t=22

                         Only Coder-3 passes the check. Others sleep again.
```

---

## 2. Shared Data Structure (used by both FIFO and EDF)

### 2.1 The waiter entry

Each coder that wants a dongle registers itself as a `t_waiter`:

```c
/* One slot in the waiting list.
   The 'priority' field means different things for FIFO vs EDF — see below. */
typedef struct s_waiter
{
    int     coder_id;   /* which coder is waiting              */
    t_ll    priority;   /* the key used to sort the heap       */
}   t_waiter;
```

### 2.2 Add the heap inside `t_dongle`

In `codexion.h`, add two fields to `s_dongle`:

```c
#define MAX_CODERS 200   /* add this near the top of codexion.h */

struct s_dongle
{
    t_mutex     mutex;
    t_cond      cond;
    int         is_taken;
    t_ll        available_at;

    /* The waiting list — a min-heap sorted by priority.
       heap[0] is always the coder with the highest right to take the dongle. */
    t_waiter    heap[MAX_CODERS];
    int         heap_size;          /* how many coders are currently waiting */
};
```

---

## 3. The Heap Helpers — What They Are and When They Are Called

There are 5 helpers. Here is **when** each one is called during a dongle
acquire/release cycle, before you see any code:

```
CODER WANTS DONGLE
  └─► heap_insert()        ← called once: "I am in the queue now"

CODER WAKES UP AFTER cond_wait
  └─► checks heap[0].coder_id == my id  ← no helper needed, just an if

CODER IS THE WINNER AND TAKES THE DONGLE
  └─► heap_pop()           ← called once: "I am leaving the queue"

SIMULATION ENDS WHILE CODER IS STILL WAITING
  └─► heap_remove_by_id()  ← called once: "I need to leave the queue early"

heap_insert and heap_pop use two internal helpers:
  └─► heap_push_up()       ← restores order after insert  (bubble UP)
  └─► heap_push_down()     ← restores order after pop     (bubble DOWN)
```

### 3.1 `heap_push_up` — used ONLY after `heap_insert`

After you add a new entry at the end of the array, it might have a smaller
priority than its parent. push-up swaps it upward until the order is restored.

```
Before insert of Coder-2 (priority=5):
  [0]=Coder-1(10)  [1]=Coder-3(15)

After insert at end (index 2):
  [0]=Coder-1(10)  [1]=Coder-3(15)  [2]=Coder-2(5)
                                             ↑ smaller than parent[0]=10

push_up swaps [2] with [0]:
  [0]=Coder-2(5)   [1]=Coder-3(15)  [2]=Coder-1(10)   ← correct!
```

```c
/* Moves the element at index 'i' upward until the heap property is restored.
   Called only by heap_insert — never call this directly. */
static void heap_push_up(t_waiter *heap, int i)
{
    int parent;

    while (i > 0)
    {
        parent = (i - 1) / 2;  /* formula: parent of node i */
        if (heap[parent].priority <= heap[i].priority)
            break ;             /* parent is already smaller — stop */
        /* parent is bigger than child — swap them */
        t_waiter tmp   = heap[parent];
        heap[parent]   = heap[i];
        heap[i]        = tmp;
        i = parent;             /* continue checking from the new position */
    }
}
```

### 3.2 `heap_push_down` — used ONLY after `heap_pop`

After you remove the root (index 0), you put the last element there.
It might be bigger than its children. push-down pushes it downward.

```
Before pop (Coder-2 with priority=5 wins):
  [0]=Coder-2(5)   [1]=Coder-3(15)  [2]=Coder-1(10)

Pop: move last element to root:
  [0]=Coder-1(10)  [1]=Coder-3(15)   heap_size = 2
         ↑ bigger than child[1]=15? No. Bigger than child[2]? No child.
         → already correct here (this was a lucky example)

Harder example — if root becomes (20):
  [0]=20  [1]=8  [2]=12
  push_down: 20 > 8 → swap → [0]=8  [1]=20  [2]=12
  continue from index 1: no children bigger → done
```

```c
/* Moves the element at index 'i' downward until the heap property is restored.
   Called only by heap_pop — never call this directly. */
static void heap_push_down(t_waiter *heap, int size, int i)
{
    int left;
    int right;
    int smallest;

    while (1)
    {
        left     = 2 * i + 1;  /* left child index  */
        right    = 2 * i + 2;  /* right child index */
        smallest = i;

        /* check if left child exists and is smaller */
        if (left < size && heap[left].priority < heap[smallest].priority)
            smallest = left;

        /* check if right child exists and is even smaller */
        if (right < size && heap[right].priority < heap[smallest].priority)
            smallest = right;

        if (smallest == i)
            break ;             /* already smallest among children — stop */

        /* swap with the smallest child and continue from there */
        t_waiter tmp      = heap[i];
        heap[i]           = heap[smallest];
        heap[smallest]    = tmp;
        i                 = smallest;
    }
}
```

### 3.3 `heap_insert` — called when a coder starts waiting

```c
/* Adds a new coder to the waiting list with the given priority.
   After adding at the end, push_up restores the heap order.
   The mutex on the dongle must be held when calling this. */
void heap_insert(t_dongle *d, int coder_id, t_ll priority)
{
    int i;

    i = d->heap_size;               /* insert at the end of the array  */
    d->heap[i].coder_id = coder_id;
    d->heap[i].priority = priority;
    d->heap_size++;
    heap_push_up(d->heap, i);       /* bubble up to the correct position */
}
```

### 3.4 `heap_pop` — called when the winner takes the dongle

```c
/* Removes the root (the winner — heap[0]) from the waiting list.
   Replaces root with the last element, shrinks the array, then
   push_down restores the heap order.
   The mutex on the dongle must be held when calling this. */
void heap_pop(t_dongle *d)
{
    /* move the last element to the root position */
    d->heap[0] = d->heap[d->heap_size - 1];
    d->heap_size--;
    if (d->heap_size > 0)
        heap_push_down(d->heap, d->heap_size, 0); /* push new root down */
}
```

### 3.5 `heap_remove_by_id` — called only on early exit

This is the cleanup helper. It is only needed when `is_end_program()` returns
true while a coder is still waiting in the queue. Without it, the heap would
hold a ghost entry forever.

```c
/* Removes the entry for 'coder_id' from any position in the heap.
   This is only called when the simulation ends while a coder is mid-wait.
   After removing, both push operations are run to fix order from that spot. */
void heap_remove_by_id(t_dongle *d, int coder_id)
{
    int i;

    i = 0;
    while (i < d->heap_size)
    {
        if (d->heap[i].coder_id == coder_id)
        {
            /* overwrite with the last element and shrink */
            d->heap[i] = d->heap[d->heap_size - 1];
            d->heap_size--;
            if (i < d->heap_size)
            {
                /* the replacement could go either up or down — try both */
                heap_push_down(d->heap, d->heap_size, i);
                heap_push_up(d->heap, i);
            }
            return ;
        }
        i++;
    }
}
```

---

## 4. PART 1 — FIFO (First In, First Out)

### Concept

The dongle is given to whichever coder **asked for it first**. Priority is
the timestamp in milliseconds at the moment the coder joins the queue.
The coder who called `heap_insert` earliest has the smallest timestamp
and therefore sits at `heap[0]`.

```
t=10ms  Coder-3 joins queue   heap: [Coder-3(10)]
t=15ms  Coder-1 joins queue   heap: [Coder-3(10), Coder-1(15)]
t=22ms  Coder-5 joins queue   heap: [Coder-3(10), Coder-1(15), Coder-5(22)]

Dongle released → broadcast → everyone wakes up
  Coder-3 checks: heap[0].coder_id == 3? YES → takes dongle ✓
  Coder-1 checks: heap[0].coder_id == 1? NO  → back to sleep
  Coder-5 checks: heap[0].coder_id == 5? NO  → back to sleep
```

### The one FIFO line in `take_dongle`

```c
/* FIFO: priority = the moment we join the queue.
   Earlier arrival → smaller number → closer to heap[0] → wins sooner. */
priority = get_time_ms();
```

### Full `take_dongle` for FIFO (with comments)

```c
/* Called from acquire_pair().
   PRECONDITION: dongle->mutex must already be locked by the caller.
   POSTCONDITION: mutex is unlocked before this function returns. */
int take_dongle(t_coder *coder, t_dongle *dongle)
{
    t_ll    priority;
    t_rules *rules;

    rules = coder->rules;

    /* FIFO: use arrival time as priority.
       Whoever calls this first gets the smallest number → wins. */
    priority = get_time_ms();

    /* Join the waiting list. From this moment we are "in the queue".
       heap_insert adds us at the end and pushs us up if needed. */
    heap_insert(dongle, coder->id, priority);

    /* Wait loop. We sleep until THREE conditions are all true:
       1. We are at the top of the heap (heap[0] == us)      → it is our turn
       2. The dongle is not currently held by someone else   → it is free
       3. The cooldown timer has expired                     → it is available
       If any condition fails we go back to sleep. */
    while (!is_end_program(rules))
    {
        if (dongle->heap[0].coder_id == coder->id  /* condition 1 */
            && dongle->is_taken == 0               /* condition 2 */
            && dongle->available_at <= get_time_ms())  /* condition 3 */
            break ;   /* all three true — we can take the dongle */

        /* At least one condition failed. Sleep until a broadcast wakes us.
           pthread_cond_wait automatically unlocks the mutex while sleeping
           and re-locks it before returning. */
        pthread_cond_wait(&dongle->cond, &dongle->mutex);
    }

    /* Check if we were woken up because the simulation ended, not because
       it is our turn. If so, clean up and leave. */
    if (is_end_program(rules))
    {
        heap_remove_by_id(dongle, coder->id); /* remove our ghost entry */
        pthread_mutex_unlock(&dongle->mutex);
        return (0);
    }

    /* We are the winner. Remove ourselves from the heap and claim the dongle. */
    heap_pop(dongle);
    dongle->is_taken = 1;
    pthread_mutex_unlock(&dongle->mutex); /* unlock before printing */
    safe_print(coder, "has taken a dongle");
    return (1);
}
```

---

## 5. PART 2 — EDF (Earliest Deadline First)

### Concept

EDF does not care who arrived first. It cares who is **closest to burning out**.
The coder with the nearest deadline (smallest `last_compile + time_to_burnout`)
gets the dongle first. This prevents starvation: a coder who has not compiled
in a long time will eventually have the closest deadline and jump to the front.

```
Deadlines (last_compile + time_to_burnout):
  Coder-3: 500 + 800 = 1300ms
  Coder-1: 200 + 800 = 1000ms  ← closest deadline → wins
  Coder-5: 350 + 800 = 1150ms

heap after all three join:
  [0] Coder-1 (1000)   ← root = most urgent
  [1] Coder-3 (1300)
  [2] Coder-5 (1150)

Dongle released → broadcast → Coder-1 is at heap[0] → takes dongle ✓
```

### Why EDF prevents burnout better than FIFO

With FIFO, if Coder-1 keeps arriving first, Coder-5 might wait a long time.
With EDF, as Coder-5 waits longer, its `last_compile` gets older, so its
deadline shrinks and it naturally rises to the top of the heap.

### The one EDF line in `take_dongle`

```c
/* EDF: priority = when this coder must compile by or it burns out.
   Smaller deadline → more urgent → closer to heap[0] → wins sooner. */
priority = coder->last_compile + rules->time_to_burnout;
```

### Full `take_dongle` for EDF (only the priority line changes)

```c
/* Called from acquire_pair().
   PRECONDITION: dongle->mutex must already be locked by the caller.
   POSTCONDITION: mutex is unlocked before this function returns. */
int take_dongle(t_coder *coder, t_dongle *dongle)
{
    t_ll    priority;
    t_rules *rules;

    rules = coder->rules;

    /* EDF: priority = absolute deadline for this coder.
       last_compile is set at the START of compile_step (before sleeping),
       so it represents the last time this coder began compiling.
       At simulation start, last_compile is initialized to rules->start_time. */
    priority = coder->last_compile + rules->time_to_burnout;

    /* Join the waiting list. The heap will place us at heap[0] if we
       have the smallest deadline among all currently waiting coders. */
    heap_insert(dongle, coder->id, priority);

    /* Wait loop — identical to FIFO.
       The only difference was already applied above (priority value).
       We still wait until: our turn AND free AND cooldown expired. */
    while (!is_end_program(rules))
    {
        if (dongle->heap[0].coder_id == coder->id
            && dongle->is_taken == 0
            && dongle->available_at <= get_time_ms())
            break ;

        pthread_cond_wait(&dongle->cond, &dongle->mutex);
    }

    /* Same exit-on-simulation-end logic as FIFO. */
    if (is_end_program(rules))
    {
        heap_remove_by_id(dongle, coder->id);
        pthread_mutex_unlock(&dongle->mutex);
        return (0);
    }

    /* Winner: pop from heap, claim the dongle, unlock. */
    heap_pop(dongle);
    dongle->is_taken = 1;
    pthread_mutex_unlock(&dongle->mutex);
    safe_print(coder, "has taken a dongle");
    return (1);
}
```

### Making both modes work in one function

In practice you write a single `take_dongle` and branch on the scheduler:

```c
/* Single function that handles both modes.
   The ONLY branching point is priority computation — everything else is shared. */
int take_dongle(t_coder *coder, t_dongle *dongle)
{
    t_ll    priority;
    t_rules *rules;

    rules = coder->rules;

    /* ── ONLY DIFFERENCE between FIFO and EDF ── */
    if (strcmp(rules->scheduler, "fifo") == 0)
        priority = get_time_ms();                               /* FIFO */
    else
        priority = coder->last_compile + rules->time_to_burnout; /* EDF  */
    /* ─────────────────────────────────────────── */

    heap_insert(dongle, coder->id, priority);
    while (!is_end_program(rules))
    {
        if (dongle->heap[0].coder_id == coder->id
            && dongle->is_taken == 0
            && dongle->available_at <= get_time_ms())
            break ;
        pthread_cond_wait(&dongle->cond, &dongle->mutex);
    }
    if (is_end_program(rules))
    {
        heap_remove_by_id(dongle, coder->id);
        pthread_mutex_unlock(&dongle->mutex);
        return (0);
    }
    heap_pop(dongle);
    dongle->is_taken = 1;
    pthread_mutex_unlock(&dongle->mutex);
    safe_print(coder, "has taken a dongle");
    return (1);
}
```

---

## 6. EDF — Initializing `last_compile` Correctly

This is a detail that will break EDF silently if you get it wrong.

`last_compile` must be set to `rules->start_time` when each coder is created,
**not 0**. If it is 0, the first deadline is `0 + time_to_burnout`, which might
already be in the past by the time the simulation starts.

```c
/* In your init_coders() or wherever you set up t_coder structs: */
coders[i].last_compile = rules->start_time; /* ← critical for EDF */
```

And in `compile_step`, it must be updated **before** sleeping, not after:

```c
static void compile_step(t_coder *coder, t_rules *rules)
{
    pthread_mutex_lock(&coder->state_mutex);
    coder->last_compile = get_time_ms(); /* ← set BEFORE sleep, not after */
    pthread_mutex_unlock(&coder->state_mutex);

    safe_print(coder, "is compiling");
    my_sleep(rules->time_to_compile);

    pthread_mutex_lock(&coder->state_mutex);
    coder->compiles_done++;
    pthread_mutex_unlock(&coder->state_mutex);
}
```

---

## 7. `acquire_pair` — Mutex Must Be Locked Before Calling `take_dongle`

Your current `acquire_pair` unlocks the mutex inside `take_dongle` at the
start. The new design is: **lock before, `take_dongle` unlocks at the end**.

```c
static int acquire_pair(t_coder *coder, t_dongle *first, t_dongle *second)
{
    t_rules *rules;

    rules = coder->rules;

    /* Lock first dongle BEFORE calling take_dongle.
       take_dongle will unlock it before returning. */
    pthread_mutex_lock(&first->mutex);
    if (!take_dongle(coder, first))
        return (0);  /* take_dongle already unlocked first->mutex */

    /* Now lock second dongle. */
    pthread_mutex_lock(&second->mutex);
    if (!take_dongle(coder, second))
    {
        /* Second failed — release the first we already hold. */
        release_dongle(first, rules->dongle_cooldown);
        return (0);
    }
    if (is_end_program(rules))
    {
        release_dongle(first, rules->dongle_cooldown);
        release_dongle(second, rules->dongle_cooldown);
        return (0);
    }
    return (1);
}
```

---

## 8. `init_mutex` — Zero the Heap

```c
int init_mutex(t_dongle *dongles, int n)
{
    int i;

    i = 0;
    while (i < n)
    {
        if (pthread_mutex_init(&dongles[i].mutex, NULL) != 0)
            return (0);
        if (pthread_cond_init(&dongles[i].cond, NULL) != 0)
            return (0);
        dongles[i].is_taken     = 0;
        dongles[i].available_at = 0;
        dongles[i].heap_size    = 0;               /* ← NEW: empty queue */
        memset(dongles[i].heap, 0,                 /* ← NEW: zeroed slots */
               sizeof(dongles[i].heap));
        i++;
    }
    return (1);
}
```

---

## 9. Summary: FIFO vs EDF Side by Side

```
                   FIFO                          EDF
                   ────                          ───
Priority key:      get_time_ms()                 last_compile + time_to_burnout
Meaning:           when I joined the queue        when I will burn out
Winner:            arrived first                  burns out soonest
Fairness:          strict order                   urgency-based
Anti-starvation:   no (a coder can be skipped)   yes (deadline always shrinks)

Both use:          same heap struct
                   same heap_insert / heap_pop
                   same take_dongle wait loop
                   same release_dongle / broadcast

Only difference:   one line — the priority= assignment
```

---

## 10. Files to Create / Modify

| File | What to do |
|---|---|
| `codexion.h` | Add `t_waiter`, `MAX_CODERS`, and `heap[]` + `heap_size` to `s_dongle` |
| `queue.c` | New file: all 5 helpers (`push_up`, `push_down`, `insert`, `pop`, `remove_by_id`) |
| `routine.c` | Rewrite `take_dongle` + fix `acquire_pair` mutex ordering |
| `init.c` | Add `heap_size = 0` and `memset(heap, 0, ...)` inside `init_mutex` |
