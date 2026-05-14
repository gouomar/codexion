# Agent Notes

## Verified Reality
- Real project is the pthread C simulator under `coders/`; entrypoint is `coders/main.c`.
- Trust source over docs: `README.md`/`understading.md` describe intended phases and are not fully aligned with current code.
- No CI/lint/test runner config is present; `Makefile` is the only build contract.

## Commands That Matter
- Build: `make` (compiles with `cc -Wall -Wextra -Werror -pthread`).
- Clean rebuild: `make re`.
- Run: `./coders/codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler`.

## Current Build Blockers (as of repo state)
- `make` currently fails at `coders/codexion.h` because `struct s_dongle` uses `t_waiter *heap` before `t_waiter` is defined.
- `Makefile` `SRC` does not include `coders/queue.c` even though heap APIs are declared/used (`heap_insert`, `heap_pop`, `heap_remove_by_id`).
- `coders/routine.c` contains duplicate `take_dongle` definitions; this will fail once compilation reaches that file.

## High-Value File Map
- `coders/parsing.c`: hard-validates CLI contract (`argc == 9`, numeric `argv[1..7]`, scheduler must be `fifo` or `edf`).
- `coders/main.c`: lifecycle wiring (parse -> init mutex/state -> create coder threads -> run monitor -> join -> cleanup).
- `coders/monitor.c`: global stop flag (`simulation_over`) synchronization and burnout/completion stop conditions.
- `coders/routine.c`: coder work loop + dongle acquire/release behavior; most likely file for runtime/concurrency bugs.
- `coders/init.c` + `coders/cleanup.c`: thread/mutex/cond setup and teardown; also where dongle waiter heap is allocated/freed.

## Scope Guardrails
- Ignore root experiments like `data_race.c`, `a.out`, and docs like `queue.md` for build decisions.
- When fixing behavior, validate against actual executable path (`make` + `./coders/codexion ...`), not roadmap prose.
