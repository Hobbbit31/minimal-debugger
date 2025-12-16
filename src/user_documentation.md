# Mini Debugger – User Documentation

## 1. Problem Statement
Design and implement a minimal UNIX-style debugger capable of controlling and inspecting a running program using operating system–level debugging primitives, without relying on existing debuggers such as `gdb`.

---

## 2. Introduction
The Mini Debugger is a lightweight command-line debugger built for UNIX-like systems.  
It demonstrates fundamental operating system concepts such as **process tracing, execution control, signal handling, and software breakpoints** using the `ptrace()` system call.

The debugger allows users to start a program under debugger control, pause and resume execution, set breakpoints at specific memory addresses, single-step instructions, and observe program termination behavior.

---

## 3. Starting the Debugger

### Compilation

- make

### Running the Debugger

- ./mydbg

### Prompt Example

- mydbg>


---

## 4. Feature Overview
The Mini Debugger supports the following features:

- Launching a program under debugger control  
- Continuing program execution  
- Single-step instruction execution  
- Software breakpoints using `INT3 (0xCC)`  
- Breakpoint removal with safe instruction restoration
- CPU register inspection 
- Process state reporting
- Safe handling of invalid commands  
- Graceful program termination handling  

---

## 5. Running a Program

### Command: `run`
Starts a program under debugger control, with optional arguments.

Usage:

- run <program> 

Example:

- run ./test 

Notes:
- The program must exist and be executable
- Only one program can be debugged at a time
- Running a program initializes a fresh breakpoint table
- Argument parsing is minimal; currently only the program path is supported

---

## 6. Continuing Program Execution

### Command: `continue`
Resumes execution of the debugged program until:

- A breakpoint is hit  
- The program exits  

Usage:

- continue

Example:

- continue

If the program has already exited:

- [dbg] program has already exited


---

## 7. Single-Step Execution

### Command: `step`
Executes exactly **one CPU instruction** and then stops again.

Usage:

- step


Rules:
- The program must be in a **stopped** state
- Stepping is not allowed after program exit

Example:

- step
- step


---

## 8. Breakpoints

### 8.1 Setting a Breakpoint

### Command: `break`
Sets a software breakpoint at a specific **hexadecimal memory address**.

Usage:

- break <hexaddr>

Example:

- break 0x40113e

Notes:
- Addresses must be provided in hexadecimal
- Breakpoints can only be set after the program is started
- If the program is stopped, the breakpoint is inserted immediately
- If the program is running, the breakpoint is recorded and inserted the next time the program stops
- Duplicate or invalid breakpoint addresses are rejected
- reakpoints are implemented using INT3 (0xCC) instruction patching

---

### 8.2 Removing a Breakpoint

### Command: `delete`
Removes a previously set breakpoint.

Usage:

- delete <hexaddr>

Example:

- delete 0x40113e


Notes:
- If the program is running, it is first stopped safely
- The original instruction is restored before breakpoint removal
- Removing a non-existent breakpoint reports an error

---

## 9. Register Inspection

**Command:** `regs`

Displays CPU register values when the program is **stopped** (for example, after hitting a breakpoint or during single-step execution).

---

### Usage

regs  
regs -s  
regs -g  

---

### Description

- `regs`  
  Display **all supported CPU registers**.

- `regs -s`  
  Display **stack-related registers** only:
  - RIP (Instruction Pointer)
  - RSP (Stack Pointer)
  - RBP (Base Pointer)

- `regs -g`  
  Display **general-purpose registers** only.

---

### Supported Registers

The debugger currently supports inspection of the following registers:

- **Instruction Pointer**
  - RIP

- **Stack Registers**
  - RSP
  - RBP

- **General-Purpose Registers**
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

- Register inspection is only allowed when the debugged program is in the **STOPPED** state.
- Internally, register values are retrieved using `ptrace(PTRACE_GETREGS)`.
- This command is useful for understanding program execution flow, stack state, and CPU context at breakpoints.

---
## 10. Process State Reporting

The debugger internally tracks the **execution state** of the debugged program to ensure valid command handling and correct execution flow.

---

### Possible Process States

- **NOT_STARTED**  
  The program has not been launched yet using the `run` command.

- **RUNNING**  
  The program is currently executing.

- **STOPPED**  
  The program execution is paused.  
  This can occur due to:
  - Hitting a breakpoint
  - Single-step execution
  - Receiving a signal (e.g., `SIGTRAP`)

- **EXITED**  
  The program has finished execution or terminated.

---

### Command

**status**

---

### Usage

Process state: NOT_STARTED

---

### Description

- Displays the **current process state** of the debugged program.
- If the program is in the **STOPPED** state, the debugger also reports the **reason for stopping** (such as a breakpoint hit or signal received).

---

### Notes

- The process state is maintained internally by the debugger and updated after every `ptrace`-controlled event.
- This command helps users understand whether the program can be continued, stepped, or restarted.
- Invalid commands are safely rejected based on the current process state.

---

## 11. Program Exit Handling
When the debugged program exits:

- Normal termination is detected via waitpid
- The debugger state is updated to `EXITED`
- Further execution commands are safely rejected

Example output:

- [dbg] child exited with status 0


---

## 12. Error Handling
The Mini Debugger reports common user and execution errors, including:

- Attempting to debug before `run`
- Invalid hexadecimal breakpoint addresses
- Stepping when the program is running or exited
- Deleting non-existent breakpoints
- Continuing an already exited program

All errors are handled gracefully without crashing the debugger.

---

## 13. Usage Examples

### 13.1 Basic Debugging

- run ./test
- continue

---

### 13.2 Breakpoint Debugging

- run ./test
- break 0x40113e
- continue

---

### 13.3 Single-Stepping

- run ./test
- step
- continue

---

### 13.4 Breakpoint Removal

- run ./test
- break 0x40113e
- delete 0x40113e
- continue


---

## 14. Limitations
The Mini Debugger intentionally keeps scope minimal:

- No source-level debugging
- No symbol resolution (function/line names)
- No multi-threaded debugging
- No memory inspection commands
- Simple command parsing without quoting or escaping

These limitations help maintain clarity and focus on core OS-level debugging concepts.

---

## 15. Exiting the Debugger

### Command: `quit`
Ends the debugger session.

Usage:

- quit


---

## 16. Conclusion
The Mini Debugger provides a clean and educational demonstration of how debuggers interact with the operating system kernel.  
It exposes the internal mechanics behind breakpoints, single-stepping, and execution control, serving as a strong foundation for understanding real-world debuggers like `gdb`.

---