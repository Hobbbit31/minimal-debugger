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
- Breakpoint removal  
- Basic CPU register inspection  
- Safe handling of invalid commands  
- Graceful program termination handling  

---

## 5. Running a Program

### Command: `run`
Starts a program under debugger control.

Usage:

- run <program>

Example:

- run ./test

Notes:
- The program must exist and be executable
- Only one program can be debugged at a time
- Running a program initializes a fresh breakpoint table

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
- If the program is stopped, the breakpoint is activated immediately
- If the program is running, the breakpoint is registered and activated on the next stop

---

### 8.2 Removing a Breakpoint

### Command: `delete`
Removes a previously set breakpoint.

Usage:

- delete <hexaddr>

Example:

- delete 0x40113e


Notes:
- The original instruction is restored when the program is stopped
- Removing a non-existent breakpoint reports an error

---

## 9. Register Inspection
The debugger supports inspection of selected CPU registers when the program is stopped.

Registers displayed include:
- Instruction Pointer (RIP)
- Stack Pointer (RSP)
- Base Pointer (RBP)
- Accumulator Register (RAX)

(Available internally; can be exposed via a command if enabled.)

---

## 10. Program Exit Handling
When the debugged program exits:

- The debugger detects normal termination
- The debugger state is updated to `EXITED`
- Further execution commands are safely rejected

Example output:

- [dbg] child exited with status 0


---

## 11. Error Handling
The Mini Debugger reports common user and execution errors, including:

- Attempting to debug before `run`
- Invalid hexadecimal breakpoint addresses
- Stepping when the program is running or exited
- Deleting non-existent breakpoints
- Continuing an already exited program

All errors are handled gracefully without crashing the debugger.

---

## 12. Usage Examples

### 12.1 Basic Debugging

- run ./test
- continue

---

### 12.2 Breakpoint Debugging

- run ./test
- break 0x40113e
- continue
- continue

---

### 12.3 Single-Stepping

- run ./test
- step
- step
- continue

---

### 12.4 Breakpoint Removal

- run ./test
- break 0x40113e
- delete 0x40113e
- continue


---

## 13. Limitations
The Mini Debugger intentionally keeps scope minimal:

- No source-level debugging
- No symbol resolution (function/line names)
- No command-line argument parsing for debugged program
- No multi-threaded debugging
- No memory inspection commands

These limitations help maintain clarity and focus on core OS-level debugging concepts.

---

## 14. Exiting the Debugger

### Command: `quit`
Ends the debugger session.

Usage:

- quit


---

## 15. Conclusion
The Mini Debugger provides a clean and educational demonstration of how debuggers interact with the operating system kernel.  
It exposes the internal mechanics behind breakpoints, single-stepping, and execution control, serving as a strong foundation for understanding real-world debuggers like `gdb`.

---
