What this line does:

char **target_argv = &argv[1];
This simply says:
“target_argv should point to argv[1], not argv[0].”
So now:
target_argv[0] == "/bin/ls"
target_argv[1] == "-l"
target_argv[2] == "/tmp"
target_argv[3] == NULL
This is exactly the format needed for:
execvp(target_argv[0], target_argv);

# version 0.1.0 Implement dbg_launch (fork + simple exec)

### 1) `fork()`

**What it is:**  
`fork()` creates a new process by duplicating the current process. After `fork()`, two processes exist:

- `-1` → error  
- `0` → returned in the **child** process  
- `>0` → returned in the **parent**, value is the child's PID  

**Why we need it:**  
To run a target program under a debugger, we must execute the target in a **separate process**.  
The **debugger remains in the parent** and uses `ptrace()` to control/monitor the child.

**What happens if we skip it:**  
If you call `exec*()` without a prior `fork()`, the debugger process is **replaced** by the target program.  
This means the debugger disappears — it can no longer inspect or control anything.

### 2) `execvp()` (from the exec*() family)

**What it is:**  
`execvp(path, argv)` replaces the **current process image** with a new program specified by `argv[0]`.  
It searches for the executable using the system `PATH`.  
If `execvp()` succeeds, **it never returns** — the current process becomes the target program.

**Why we need it:**  
After `fork()`, the **child** must start running the target program (e.g., `/bin/ls`).  
`execvp()` loads that program into the child's memory and begins execution of the actual target binary.

**What happens if we skip it:**  
If the child does **not** call `exec*()` and continues running the debugger’s code, then:

- The child is **not** running the target program.
- You end up with **two copies** of the debugger logic instead of debugger + debuggee.

So calling `execvp()` is essential to turn the child into the program that will be debugged.


### 3) Parent vs Child Responsibilities

**Child:**  
- Calls `execvp()` → becomes the target program.  
- After `execvp()`, it no longer runs debugger code.  
- Its entire memory/code is replaced by the target binary.

**Parent (Debugger):**  
- Receives the child’s PID from `fork()`.  
- Uses `waitpid()` to detect child state changes (e.g., stopped on exec).  
- Uses `ptrace()` to inspect or control the child (set breakpoints, read registers, etc.).  
- May set up I/O redirection or manage signals.  
- Continues running separately as the controlling debugger process.

**Why responsibilities are split:**  
The debugger must stay **alive, separate, and unchanged** so it can observe and control the target.  
The child must transform into the **target program** so the debugger has something to debug.  
Without this separation, debugging would be impossible — you cannot debug yourself once you `exec()`.


### 4) `waitpid()` (used in `main_test`)

**What it is:**  
`waitpid(pid, &status, options)` waits for a **specific child process** to change state  
(exit, stop, or continue).  
It returns the PID of the child whose state changed and stores information in `status`.

**Why we use it in the test:**  
The test program wants to:

- Run the child briefly  
- Ensure the child **does not become a zombie**  
- Exit cleanly  

For `/bin/true`, the child exits immediately.  
`waitpid()` lets the parent block until the child terminates and then **reaps** it.

**What happens if we don’t wait:**  

- If the parent never calls `waitpid()`, the child may become a **zombie** until `init` cleans it up.  
- If the parent exits too early, the child is reparented to `init`, which eventually reaps it — but this is messy and undesirable in tests.  
- In real debugging, the parent usually does **not** wait immediately because it needs to use `ptrace()` first.  
  In tests, we wait simply to keep the process table clean.

So, `waitpid()` ensures controlled and clean process management.


### 5) Error Handling: `perror()` and `_exit(127)`

**Why print errors (`perror()`):**  
If `execvp()` fails in the child, the child should print a clear diagnostic message.  
`perror()` prints a human-readable error based on `errno`  
(e.g., `ENOENT` → “No such file or directory”).  
This helps you understand *why* the target program could not be started.

**Why use `_exit(127)` (not `exit()`) after exec fails:**  
- `exit()` performs **stdio buffering cleanup**, but after `fork()`, the child's buffers are **copies** of the parent's.  
  Calling `exit()` would flush them a second time → output duplication or corruption.  
- `_exit()` immediately terminates the process **without touching stdio buffers**, making it safe in the fork-child.  
- `127` is the conventional return code meaning **exec failed** (used by shells and POSIX tools).

So:  
- Print the error with `perror()`  
- Terminate safely with `_exit(127)`  


### 6) `errno`

**Why it is relevant:**  
Many system calls and library functions return `-1` (or another sentinel value) on failure **and set `errno`** to indicate the specific error.  
Typical workflow:

1. Function returns `-1`  
2. Caller checks return value  
3. Caller uses `perror()` or `strerror(errno)` to print a meaningful message  

This pattern allows clean error propagation without mixing error handling logic into every function.

### 7) Why separate `dbg_launch` into its own file

**Modularity:**  
`dbg_launch` will eventually grow to include:

- `ptrace(PTRACE_TRACEME)` in the child before `execvp()`  
- Setup for pipes / file descriptor redirection  
- Additional child preparation steps  

Keeping launch logic isolated means:

- `main.c` stays small and readable  
- Future changes to launching don't break unrelated code  
- Debugger features can be added incrementally without rewriting main

**Testability:**  
By isolating the launch code:

- We can write a `main_test.c` that links only against `dbg_launch`  
- Tests focus on fork/exec behavior **without ptrace complexity**  
- Easier debugging, easier unit testing, cleaner development workflow

This separation is a standard practice in debugger and systems programming design.

---

# V0.1.2 — Add ptrace(TRACEME) in child (basic)

### 1) Why do we call `ptrace(PTRACE_TRACEME)` in the child?

`ptrace(PTRACE_TRACEME)` tells the kernel:

> “This process (the child) allows its parent to trace, inspect, and control it.”

What this does:

- Marks the **child** as traceable by its **parent**.  
- Causes the kernel to send an **initial stop** to the parent right after the child executes `exec()`.  
- Enables the parent to use `ptrace()` operations later:
  - read/write registers  
  - set breakpoints  
  - single-step  
  - inspect memory  
  - intercept system calls  

Without `PTRACE_TRACEME`, the parent would not be allowed to debug or control the child.  
This is the fundamental mechanism that turns the child into a **debuggable process**.

### 2) Components Used and Why

**1. `fork()`**  
Creates a **child process**, giving us:
- Parent → debugger  
- Child → will become the target  

Why needed:  
If we skip `fork()`, calling `execvp()` would **replace the debugger itself** with the target program.  
We need two separate processes so the debugger can control the target.

---

**2. `ptrace(PTRACE_TRACEME)`**  
Called **in the child before exec**.  
This tells the kernel:

> “My parent is allowed to trace and debug me.”

Effects:
- Kernel delivers an initial **exec stop** to the parent.  
- Parent can then perform ptrace ops (breakpoints, register reads, stepping, etc.).

---

**3. `execvp()`**  
Replaces the **child’s program image** with the actual target binary (e.g., `/bin/ls`).  
After `execvp()` succeeds:
- The child **becomes** the target program.  
- It never returns to the debugger code.

Without this, the child would run duplicate debugger logic instead of the program being debugged.

---

**4. `perror()` and `_exit()`**  
Used for **error handling** in the child when `execvp()` fails.

- `perror()` prints a human-readable error message (based on `errno`) so we know why exec failed.  
- `_exit()` safely terminates the child **without flushing stdio buffers**.

Why `_exit()` instead of `exit()`?
- After `fork()`, the child inherits **copies of parent’s stdio buffers**.  
- Using `exit()` could flush them twice → duplicated or corrupted output.  
- `_exit()` avoids this entirely.

These components together form the minimal, correct structure for launching a traceable target program under a debugger.


### 3) What Happens if `ptrace(PTRACE_TRACEME)` Fails?

If `ptrace(PTRACE_TRACEME)` fails in the child:

- The child prints an error message using `perror()`.
- The child exits immediately with `_exit(127)` (the conventional "exec/trace setup failed" code).

**Why can it fail?**

Common causes include:

- **Permission restrictions**  
  Some systems (e.g., Ubuntu with Yama security module) restrict ptrace so that a process cannot be traced unless certain conditions are met.

- **Wrong order of operations**  
  `PTRACE_TRACEME` must be called *before* `execvp()`.

- **Resource or kernel errors**  
  Rare, but `errno` will tell you the reason.

**How to diagnose it?**

- Check `errno` after failure.
- `perror()` will print messages like:
  - `Operation not permitted`
  - `Permission denied`

**On a normal development machine:**  
Child-to-parent tracing almost always succeeds unless system-wide ptrace restrictions are configured.

So:  
Calling `PTRACE_TRACEME` should normally work, but if it fails, the child must abort cleanly to avoid running in a half-debuggable state.

---

# V0.1.3 — Implement dbg_wait() and initial Stop detection


- When a traced child does ptrace(PTRACE_TRACEME) and exec, the kernel stops the child briefly and notifies the parent. The parent (debugger) must wait for that stop so it can begin tracing from the very start of the target program. dbg_wait() performs that waiting and tells the caller what happened.

### 1) `waitpid(pid, &status, 0)`

**What it is:**  
A system call that waits for a specific child process (`pid`) to **change state**.  
State changes include:
- the child stopping (e.g., due to `PTRACE_TRACEME` + `exec`)
- the child exiting normally
- the child being killed by a signal

When the state change happens, `waitpid()` returns the child’s PID and fills `status`.

---

**Why we need it (in a debugger):**  
After the child calls `ptrace(PTRACE_TRACEME)` and then `execvp()`, the kernel sends an **initial stop** to the parent.  
This stop is *crucial* — it signals:

- the child is now traceable  
- the target program has loaded  
- the debugger may now read registers, set breakpoints, etc.

`waitpid()` is how the debugger **blocks until this moment** and retrieves this event.

---

**If we didn’t use it:**  
- The parent would not detect the child’s initial exec-stop.  
- The debugger might try to call `ptrace()` before the child is ready → causing errors.  
- Breakpoints could not be set early enough (the child may run ahead).  
- The debugger could completely **miss** the first stop, making debugging impossible.

So `waitpid()` is the synchronization point that ensures the debugger and child stay in sync during startup.

### 2) `WIFSTOPPED(status)` and `WSTOPSIG(status)`

**What they do:**  
These are macros used to interpret the `status` value returned by `waitpid()`.

- **`WIFSTOPPED(status)`**  
  Returns true if the child process is **stopped**, not exited.  
  (Stopped events include the initial `SIGTRAP` from `PTRACE_TRACEME + exec`.)

- **`WSTOPSIG(status)`**  
  Returns the **signal number** that caused the stop.  
  For debuggers, the most common initial stop signal is `SIGTRAP`.

---

**Why they are required:**  
When debugging, we need to know:

1. **Did the child actually stop?** (`WIFSTOPPED`)  
2. **Which signal caused the stop?** (`WSTOPSIG`)  

This tells the debugger whether:
- The exec-stop occurred (`SIGTRAP`)
- The child hit a breakpoint
- The child got a real signal (e.g., `SIGSEGV`, `SIGINT`)
- Some other ptrace event happened

Without correctly interpreting the status, the debugger cannot react appropriately.

---

**If they weren’t used:**  
We would only have a raw integer `status` from `waitpid()` → unreadable and non-portable.  
Manually decoding it is platform-specific and error-prone.

These macros make stop detection **clean, safe, and portable across Unix-like systems**.

### 3) `SIGTRAP`

**What it is:**  
`SIGTRAP` is a signal generated by the kernel to notify a process (usually a debugger) of **tracing-related events**.  
Common situations that produce `SIGTRAP`:

- The child called `ptrace(PTRACE_TRACEME)` and then executed `exec()`  
- The program hits a breakpoint instruction (`int3` on x86)  
- Single-stepping events  
- Other ptrace-related notifications

The signal number is commonly **5**, but should not be hard-coded.

---

**Why it matters in a debugger:**  
For a debugger, the **initial stop** after `PTRACE_TRACEME + exec` is delivered as `SIGTRAP`.  
When the parent sees:

- `WIFSTOPPED(status) == true`
- `WSTOPSIG(status) == SIGTRAP`

it knows:

1. The child’s exec completed.  
2. The child is now being traced correctly.  
3. The debugger may safely:
   - read registers  
   - insert breakpoints  
   - continue execution under control  

`SIGTRAP` is therefore the “handshake signal” that tells the debugger:  
**“You now have control — the target is ready.”**

---

**If not handled:**  
If the debugger ignores or misinterprets this stop:

- It may miss the opportunity to set early breakpoints.  
- It may attempt ptrace operations too early → errors.  
- The target may run ahead uncontrolled.  
- The debugger loses synchronization with the debuggee.

Correct handling of `SIGTRAP` is essential for reliable debugger startup.

### 4) `dbg_event` struct

**What it is:**  
A small, custom struct that encodes **what happened to the child process**.  
Typical fields include:

- whether the child **stopped**, **exited**, or was **signaled**  
- which **signal** caused the stop  
- the **exit code** (if the child exited)  
- raw wait status if needed

It is a higher-level, human-friendly representation of a `waitpid()` result.

---

**Why we use it:**  
The debugger’s `dbg_wait()` can convert the messy, low-level `waitpid()` status into a **clean, structured event**.  
This provides a nicer API for the rest of the debugger code:

- No need to repeatedly call `WIFSTOPPED`, `WSTOPSIG`, `WIFEXITED`, etc.  
- All interpretations happen in **one place**.  
- Other modules (breakpoints, stepping, REPL, UI, logging) simply:  
- dbg_event ev = dbg_wait(pid);


### 5) What Happens if These Components Are Missing or Misused?

**1. No `waitpid()`**  
If the parent does not wait for the child’s initial stop:
- The debugger never sees the `SIGTRAP` exec-stop.
- Breakpoints cannot be installed *before* the child starts running.
- The child may run ahead uncontrolled.
- You **lose deterministic control** of the program from the very start.

This breaks the fundamental requirement of a debugger: controlled execution.

---

**2. Not checking `WIFSTOPPED(status)`**  
If the parent assumes the child stopped when it actually **exited**:
- The debugger may call `ptrace()` on a **dead process** → undefined behavior.
- Errors become confusing (“No such process”, inconsistent state).
- You cannot reliably distinguish stop events, exits, or signals.

Correct stop detection is mandatory for stable debugging.

---

**3. Ignoring `SIGTRAP`**  
If the debugger does not check for or understand `SIGTRAP`:
- It will miss the normal ptrace **handshake event** after `PTRACE_TRACEME + exec`.
- It may incorrectly assume tracing failed.
- It may continue without realizing it already has control.
- Early breakpoints, register reads, or state inspection may occur too late or fail.

`SIGTRAP` is essential for synchronizing the debugger with the child.

---

**4. Not using a `dbg_event` (or equivalent abstraction)**  
If the code directly processes raw `waitpid()` statuses everywhere:
- Each module repeats low-level decoding logic.
- Bugs multiply due to duplicated logic.
- The code becomes harder to maintain, read, or extend.
- Adding new event types (breakpoints, signals, single-step stops) becomes messy.

A structured event object keeps the debugger clean, consistent, and scalable.

---

**Summary:**  
Misusing or omitting these components breaks the debugger’s ability to:
- start tracing correctly  
- maintain control  
- understand child events  
- provide clean abstractions  

Every piece is essential for a reliable debugging pipeline.





















