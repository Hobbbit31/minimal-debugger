# Mini Debugger – Technical Report

## 1. Introduction
This technical report explains the internal design, algorithms, and system programming concepts used to build a **Mini Debugger** for UNIX-like operating systems.

The Mini Debugger provides a simplified but functionally accurate model of a real debugger such as `gdb`.  
Instead of focusing on source-level debugging, the project emphasizes **low-level process control and debugging mechanisms provided by the operating system**.

The debugger demonstrates the following core concepts:

- Process tracing using `ptrace()`  
- Tracer–tracee process relationship  
- Program execution control (`run`, `continue`, `step`)  
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
- `test.c` - Basic C code for testing purpose

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
   - Waits for child to stop
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

1. Read original instruction using `PTRACE_PEEKDATA`
2. Save original instruction word
3. Replace lowest byte with `0xCC`
4. Write modified word using `PTRACE_POKEDATA`

This causes the CPU to raise `SIGTRAP` when execution reaches the breakpoint.

---

### 6.3 Breakpoint Hit Handling
When a breakpoint is hit:

1. CPU stops execution with `SIGTRAP`
2. RIP points to `(breakpoint address + 1)`
3. Debugger:
   - Restores original instruction
   - Rewinds RIP
   - Single-steps original instruction
   - Reinserts breakpoint

This precise lifecycle ensures correct execution without corrupting program behavior.

---

### 6.4 Removing a Breakpoint
Breakpoint removal involves:

- Restoring the original instruction
- Removing the breakpoint entry from the table

Memory is modified **only when the program is STOPPED**, ensuring safety.

---

## 7. Register Inspection
The debugger supports inspecting CPU registers using:

- ptrace(PTRACE_GETREGS)


Registers such as `RIP`, `RSP`, `RBP`, and `RAX` can be displayed, enabling low-level program analysis.

---

## 8. Signal Handling

### 8.1 SIGTRAP
`SIGTRAP` is used to detect:

- Breakpoint hits
- Single-step completion

The debugger distinguishes breakpoint-related traps by checking the instruction pointer.

---

### 8.2 Program Exit Handling
When the program exits:

- The debugger detects `WIFEXITED`
- Updates state to `EXITED`
- Prevents further invalid operations

This avoids illegal `ptrace()` calls and ensures graceful termination.

---

## 9. Error Handling and Robustness

The debugger performs error checking on critical system calls:

- `ptrace()`
- `waitpid()`
- `execvp()`

Invalid command sequences (e.g., stepping after exit) are safely rejected with clear messages.

---

## 10. Design Choices and Simplifications

| Feature | Support |
|------|---------|
| Instruction-level debugging | Supported |
| Software breakpoints | Supported |
| Multiple breakpoints | Supported |
| Register inspection | Supported |
| Source-level debugging | Not supported |
| Symbol resolution | Not supported |
| Multi-threaded debugging | Not supported |

These simplifications keep the debugger focused on **core OS-level concepts**.

---

## 11. Testing and Verification

### 11.1 Basic Execution

- run ./test
- continue

### 11.2 Breakpoints

- break 0x40113e
- continue

### 11.3 Single-Stepping

- step
- step


### 11.4 Error Cases
- `break` before `run`
- `step` after exit
- Invalid addresses
- Deleting non-existent breakpoints

All errors are handled gracefully.

---

## 12. Conclusion
The Mini Debugger provides a clean and accurate implementation of a low-level UNIX debugger.  
It demonstrates how debuggers interact with the operating system using:

- `ptrace()`  
- `fork()`  
- `execvp()`  
- `waitpid()`  
- CPU registers and signals  

The project serves as a strong educational tool for understanding **process control, debugging internals, and operating system fundamentals**.

---

## 13. Git Commit History

