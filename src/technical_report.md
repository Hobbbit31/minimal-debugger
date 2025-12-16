# Mini Debugger – Technical Report

## 1. Introduction
This technical report explains the internal design, algorithms, and system programming concepts used to build a **Mini Debugger** for UNIX-like operating systems.

The Mini Debugger provides a simplified but functionally accurate model of a real debugger such as `gdb`.  
Instead of focusing on source-level debugging, the project emphasizes **low-level process control and debugging mechanisms provided by the operating system**.

The debugger demonstrates the following core concepts:

- Process tracing using `ptrace()`  
- Tracer–tracee process relationship  
- Program execution control (`run`, `continue`, `step`, `break <hexaddr>`, `delete <hexaddr>`)  
- Software breakpoint implementation using `INT3 (0xCC)`  
- Breakpoint lifecycle handling  
- CPU register inspection  
- Signal handling using `SIGTRAP`  
- Debugger state management  
- Graceful error handling  

The Mini Debugger is implemented in modular C source files:

- `main.c` – Interactive command-line interface (REPL)
- `include/debugger.c` – Process control, stepping, breakpoint handling
- `include/breakpoint.c` – Breakpoint table management and memory patching
- `include/debug.h` – Debugger data structures and APIs
- `include/breakpoint.h` – Breakpoint data structures and APIs
- `tests/...` - Basic C code's for testing purpose

This modular structure closely resembles the internal architecture of real debuggers while remaining minimal and educational.

---

## 2. System Architecture Overview
The Mini Debugger follows a **control–observe–resume execution model**.  
The debugged program (tracee) executes **only when explicitly allowed by the debugger (tracer)**.

High-level execution flow:

- User Command → Debugger Logic → ptrace() Request → Kernel → Debuggee Process

At any moment, the debugger maintains a strict internal state machine to ensure safe and correct operations.

### Debugger State Machine

- NOT_STARTED → STOPPED → RUNNING → STOPPED → EXITED


Each command validates the current state before issuing `ptrace()` requests.

---

## 3. Tracer–Tracee Model

The debugger is built using the **ptrace tracer–tracee relationship**:

- **Tracer**: Debugger process (parent)
- **Tracee**: Program being debugged (child)

The relationship is established when the child process executes:

- ptrace(PTRACE_TRACEME)


This allows the debugger to:

- Pause and resume execution
- Inspect and modify registers
- Read and write program memory
- Receive execution-related signals

Only one tracer is allowed per tracee, ensuring exclusive debugger control.

---

## 4. Command-Line Interface (REPL)

The debugger provides an interactive command-line interface similar to `gdb`.

### Supported Commands

| Command | Description |
|------|------------|
| `run <program>` | Start program under debugger |
| `continue` | Resume execution |
| `step` | Execute one instruction |
| `break <hexaddr>` | Set breakpoint |
| `delete <hexaddr>` | Remove breakpoint |
| `regs -s ` | print the stack register value|
| `regs -g ` | print the general register value|
| `regs ` | print all register value|
| `status` | show process execution state|
| `help` | Show command list |
| `quit` | Exit debugger |

The REPL parses user input, validates debugger state, and invokes the corresponding debugger functions.

---

## 5. Program Execution Control

### 5.1 Program Launch (`run`)
Program execution begins by:

1. Calling `fork()`  
2. Child process:
   - Calls `ptrace(PTRACE_TRACEME)`
   - Executes target program using `execvp()`
3. Parent process:
   - Waits for child to stop with `waitpid()`
   - Verifies the expected initial `SIGTRAP` stop
   - Marks debugger state as `STOPPED`

This ensures the debugger gains control **before the program executes any user code**.

---

### 5.2 Continue Execution (`continue`)
Execution is resumed using:

- ptrace(PTRACE_CONT)


The debugger then waits using `waitpid()` until:

- A breakpoint is hit (`SIGTRAP`)
- The program exits normally

State transitions are updated accordingly.

---

### 5.3 Single-Step Execution (`step`)
Single-step execution allows precise instruction-level control.

Steps:
1. Issue `ptrace(PTRACE_SINGLESTEP)`
2. Wait for the process to stop
3. Update debugger state

This enables fine-grained debugging similar to stepping in real debuggers.

---

## 6. Software Breakpoint Implementation

Breakpoints are implemented using **software breakpoints** via the `INT3` instruction (`0xCC`).

### 6.1 Breakpoint Data Structure
Each breakpoint stores:

- Address of breakpoint
- Original instruction word
- Active/inactive state

A fixed-size breakpoint table is used for simplicity.

---

### 6.2 Setting a Breakpoint
To set a breakpoint:

1. Validate hexadecimal address format
2. Ensure breakpoint is not already registered
3. Read original instruction using `PTRACE_PEEKDATA` 
4. Save original instruction word
5. Replace lowest byte with `0xCC`
6. Write modified word using `PTRACE_POKEDATA`

This causes the CPU to raise `SIGTRAP` when execution reaches the breakpoint.

---

### 6.3 Breakpoint Hit Handling

When a breakpoint is hit, the following sequence occurs:

- The CPU raises a **SIGTRAP** signal
- The instruction pointer (**RIP**) points to  
  `breakpoint_address + 1` due to execution of the `INT3` instruction

Upon detecting a breakpoint hit, the debugger performs these steps:

1. **Restore the original instruction**  
   - The original instruction byte at the breakpoint address is written back

2. **Rewind the instruction pointer (RIP)**  
   - RIP is adjusted back to the exact breakpoint address

3. **Execute the original instruction once**  
   - The instruction is executed using **single-step** execution

4. **Reinsert the breakpoint**  
   - The `INT3` instruction is reinserted at the breakpoint address


---

### 6.4 Removing a Breakpoint
Breakpoint removal involves:

- Restoring the original instruction
- Removing the breakpoint entry from the table

Memory is modified **only when the program is STOPPED**, ensuring safety.

---

## 7. Logical Instruction Pointer Correction
- When a software breakpoint is hit, the CPU advances the instruction pointer by one byte due to the INT3 instruction.
- To present accurate information, the debugger maps the real RIP value back to the logical breakpoint address before displaying it to the user.
- This ensures register inspection reflects the true instruction location, matching real debugger behavior.

---

## 8. Register Inspection

The debugger supports CPU register inspection using:

ptrace(PTRACE_GETREGS);

---

### Register Display Modes

- **Stack-related registers**
  - RIP
  - RSP
  - RBP

- **General-purpose registers**
  - RAX
  - RBX
  - RCX
  - RDX
  - RSI
  - RDI
  - R8
  - R9
  - R10
  - R11
  - R12
  - R13
  - R14
  - R15

---

### Notes

- Registers are displayed **only when the program is in the STOPPED state**.
- This ensures register values reflect a consistent and inspectable CPU context.


---

### 8.2 Program Exit Handling
When the program exits:

- The debugger detects `WIFEXITED`
- Updates state to `EXITED`
- Prevents further invalid operations

This avoids illegal `ptrace()` calls and ensures graceful termination.

### 8.3 Other Signals
- `WIFSIGNALED`: reports the terminating signal and marks `EXITED`.
- `WIFSTOPPED` with non-`SIGTRAP` signals: reports the stopping signal and keeps the state `STOPPED`.

---
## 9. Signal Handling

The debugger relies on UNIX signal reporting via `waitpid()` to manage execution control and state transitions.

---

### 9.1 SIGTRAP

`SIGTRAP` is generated in the following situations:

- When a **software breakpoint** (`INT3`) is hit
- When **single-step execution** completes

Breakpoint-related traps are detected by examining the **instruction pointer (RIP) offset**, since `INT3` advances RIP by one byte.

---

### 9.2 Program Exit Handling

When the debugged program exits normally:

- `waitpid()` reports `WIFEXITED`
- The debugger state transitions to **EXITED**
- Further execution-related commands (`continue`, `step`) are safely rejected

---

### 9.3 Other Signals

- **WIFSIGNALED**  
  - The program was terminated by a signal  
  - Debugger state is set to **EXITED**

- **WIFSTOPPED (non-SIGTRAP)**  
  - The program received a signal other than `SIGTRAP`  
  - The debugger reports the signal
  - Program remains in the **STOPPED** state

---

## 10. Process State Reporting

The debugger provides a `status` command to display:

- The **current execution state** of the program:
  - NOT_STARTED
  - RUNNING
  - STOPPED
  - EXITED
- The **stop signal**, when applicable

This information improves debugging transparency and helps ensure correct command usage.

---

## 11. Error Handling and Robustness

The debugger performs strict error checking on all critical system calls, including:

- fork()
- ptrace()
  - PTRACE_TRACEME
  - PTRACE_CONT
  - PTRACE_SINGLESTEP
  - PTRACE_GETREGS
  - PTRACE_PEEKDATA / PTRACE_POKEDATA
- waitpid()
- execvp()

Invalid command sequences are safely rejected with clear and descriptive error messages.

---

### Robustness Guarantees

The debugger enforces the following safety guarantees:

- Breakpoints are inserted **only when it is safe to do so**
- Original instructions are **always restored** after breakpoint handling
- Tracee memory is **never modified while the program is running**
- Unexpected or invalid child process states cause the debugger to abort execution cleanly

---

## 12. Design Choices and Simplifications

| Feature                     | Support        |
|----------------------------|----------------|
| Instruction-level debugging| Supported      |
| Software breakpoints       | Supported      |
| Multiple breakpoints       | Supported      |
| Register inspection        | Supported      |
| Source-level debugging     | Not supported  |
| Symbol resolution          | Not supported  |
| Multi-threaded debugging   | Not supported  |

These design decisions keep the debugger focused on **core OS-level debugging mechanisms**, ensuring simplicity, correctness, and educational value.

---

## 13. Testing and Verification

The following tests were performed to verify correct debugger behavior across normal and error scenarios.

---

### 13.1 Basic Execution

\begin{lstlisting}
run ./test
continue
\end{lstlisting}

Verifies that a program can be launched and executed to completion under debugger control.

---

### 13.2 Breakpoints

\begin{lstlisting}
run ./test
break 0x40113e
continue
\end{lstlisting}

Confirms correct breakpoint insertion and `SIGTRAP` handling.

---

### 13.3 Single-Stepping

\begin{lstlisting}
step
step
\end{lstlisting}

Ensures that single-instruction execution functions correctly when the program is stopped.

---

### 13.4 Breakpoint Removal While Running

\begin{lstlisting}
run ./test
break 0x40113e
continue
delete 0x40113e
continue
\end{lstlisting}

Verifies safe breakpoint removal with proper instruction restoration.

---

### 13.5 Error Cases

The debugger correctly handles the following error scenarios:

- Setting a breakpoint before `run`
- Stepping after program exit
- Providing an invalid breakpoint address
- Deleting a non-existent breakpoint

All error cases are handled gracefully without crashing the debugger.

---

## 14. Conclusion

The Mini Debugger provides a clean and accurate implementation of a low-level UNIX debugger.

It demonstrates how debuggers interact with the operating system using:

- ptrace()
- fork()
- execvp()
- waitpid()
- CPU registers and UNIX signals

This project serves as a strong educational foundation for understanding **process control**, **debugging internals**, and **operating system fundamentals**.

---

## 13. Git Commit History

