# Mini Debugger – Technical Report
==================================================

## 1. Introduction
--------------------------------------------------

This technical report documents the internal design, algorithms, and system-level mechanisms used to implement a **Mini Debugger** for UNIX-like operating systems.

The Mini Debugger is a deliberately simplified yet realistic representation of a production debugger such as **GDB**. Instead of focusing on source-level debugging, the project concentrates on **instruction-level execution**, **process control**, and **kernel-assisted debugging primitives**.

### Core Concepts Demonstrated

- Process tracing using `ptrace()`
- Program execution control (`run`, `continue`, `step`)
- Software breakpoints using the `INT3` instruction
- Correct breakpoint lifecycle management
- CPU register inspection and modification
- Signal-based execution control using `SIGTRAP`
- Explicit debugger state management

### Project Organization

The implementation follows a clean, modular structure:

- **main.c**  
  Interactive command-line interface (REPL)

- **debugger.c**  
  Core debugger logic: execution control, stepping, and event handling

- **breakpoint.c**  
  Breakpoint storage, insertion, and instruction patching

- **include/debug.h**  
  Debugger data structures and public APIs

- **include/breakpoint.h**  
  Breakpoint-specific data structures

This modular organization closely mirrors the internal architecture of real-world debuggers while remaining compact and educational.

---

## 2. System Architecture Overview
--------------------------------------------------

The Mini Debugger follows a **control–observe–resume** execution model.  
The debugged program executes *only* when explicitly permitted by the debugger.

### High-Level Architecture

User Command  
→ Debugger  
→ `ptrace` (kernel interface)  
→ Target Program  
→ Signal (`SIGTRAP`)  
→ Debugger

### Execution Semantics

- **run**  
  Loads the program and stops execution before the first instruction

- **continue**  
  Resumes execution until a breakpoint or program termination

- **step**  
  Executes exactly one CPU instruction

---

## 3. Tracer–Tracee Model
--------------------------------------------------

The debugger is implemented using the `ptrace` **tracer–tracee** relationship:

- **Tracer**  
  The debugger process (parent)

- **Tracee**  
  The program being debugged (child)

The relationship is established when the child executes:

    ptrace(PTRACE_TRACEME);

Once established, the debugger gains the ability to:

- Pause and resume execution
- Inspect and modify CPU registers
- Read and write the tracee’s memory
- Receive execution-related signals

Only one tracer may be attached to a tracee at a time, guaranteeing exclusive control.

---

## 4. Program Launch and Execution Control
--------------------------------------------------

### 4.1 Program Launch (`run` Command)

When the user enters:

    run ./program

the debugger performs the following sequence:

1. `fork()` creates a child process
2. The child enables tracing using `PTRACE_TRACEME`
3. The child replaces its image using `execvp()`
4. The parent waits using `waitpid()`
5. The kernel stops the child and delivers `SIGTRAP`

At this point, the program is fully loaded into memory but has not yet executed, allowing safe breakpoint insertion.

---

### 4.2 Debugger State Machine

The debugger maintains a strict internal state machine:

- **NOT_STARTED**  
  No program has been launched

- **STOPPED**  
  Program is paused and inspectable

- **RUNNING**  
  Program is actively executing

- **EXITED**  
  Program has terminated

This prevents invalid operations such as stepping a running program or continuing after termination.

---

## 5. Software Breakpoints
--------------------------------------------------

### 5.1 Breakpoint Representation

Each breakpoint is represented by the following structure:

    typedef struct {
        unsigned long addr;
        long orig_word;
        int used;
    } Breakpoint;

**Field Description**

- `addr`  
  Address where the breakpoint is set

- `orig_word`  
  Original instruction word overwritten by `INT3`

- `used`  
  Indicates whether the breakpoint entry is active

Breakpoints are stored in a fixed-size table for simplicity.

---

### 5.2 Breakpoint Insertion

To insert a breakpoint, the debugger:

- Reads the original instruction using `PTRACE_PEEKDATA`
- Saves the original instruction word
- Replaces the least significant byte with `0xCC` (`INT3`)
- Writes the modified word using `PTRACE_POKEDATA`

The `INT3` instruction causes the CPU to raise a trap when executed.

---

### 5.3 Breakpoint Hit Detection

When execution reaches a breakpoint:

- The CPU executes `INT3`
- The kernel delivers `SIGTRAP`
- Control returns to the debugger via `waitpid()`
- The debugger identifies the breakpoint using its internal table

This approach avoids unreliable instruction-pointer heuristics.

---

### 5.4 Breakpoint Lifecycle

The debugger follows the complete breakpoint lifecycle:

1. Save original instruction
2. Insert `INT3`
3. Resume execution
4. Receive `SIGTRAP`
5. Restore original instruction
6. Correct the instruction pointer (RIP)
7. Single-step the original instruction
8. Reinsert the breakpoint

This ensures consistent and repeatable breakpoint behavior.

---

## 6. Continue and Single-Step Execution
--------------------------------------------------

### 6.1 Continue Execution (`continue`)

Execution is resumed using:

    ptrace(PTRACE_CONT);

The program runs until a breakpoint is encountered or execution terminates.

---

### 6.2 Single-Step Execution (`step`)

Instruction-level execution is performed using:

    ptrace(PTRACE_SINGLESTEP);

This executes exactly one CPU instruction.

---

## 7. CPU Register Handling
--------------------------------------------------

CPU registers are accessed via:

    ptrace(PTRACE_GETREGS);
    ptrace(PTRACE_SETREGS);

Register manipulation is required to:

- Correct the instruction pointer after `INT3`
- Resume execution at the correct location
- Inspect the execution state of the program

---

## 8. Signal Handling
--------------------------------------------------

### 8.1 SIGTRAP

`SIGTRAP` is the primary signal used by the debugger. It is generated when:

- A breakpoint is hit
- A single-step operation completes
- A traced program begins execution

The debugger correctly distinguishes breakpoint-related traps from other stop events.

---

## 9. Error Handling and Safety
--------------------------------------------------

The debugger includes defensive checks to:

- Prevent stepping while execution is in progress
- Prevent continuing after program termination
- Validate breakpoint addresses
- Avoid duplicate breakpoint insertion

These checks ensure predictable and stable behavior.

---

## 10. Design Choices and Simplifications
--------------------------------------------------

Supported and unsupported features:

- Software breakpoints   : Supported
- Multiple breakpoints   : Supported
- Continue execution     : Supported
- Single-step execution  : Supported
- Register inspection    : Supported
- Hardware breakpoints   : Not supported
- Source-level debugging : Not supported
- Symbol resolution      : Not supported

---

## 11. Testing and Verification
--------------------------------------------------

### Basic Execution Test

    run ./test
    continue

### Breakpoint Test

    break 0x40113e
    continue

### Single-Step Test

    step

---

## 12. Conclusion
--------------------------------------------------

The Mini Debugger provides a clean and accurate implementation of a basic UNIX debugger.

It demonstrates how debuggers fundamentally rely on:

- `ptrace()` for process control
- Software breakpoints using `INT3`
- Signal-driven execution management
- Direct CPU register manipulation

This project serves as a strong educational foundation for understanding operating system internals and low-level debugging techniques.

---

## 13. Git Commit History
--------------------------------------------------

- Your Name
- Teammate Name
