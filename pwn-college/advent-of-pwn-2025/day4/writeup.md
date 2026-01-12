# Pwn College Advent of Pwn Day 4

## Context

We are provided with a description of the challenge, an init shell script, a northpole executable with its .c sourcecode, and a tracker.bpf.o object file. The description for the challenge is as follows:

```
Every Christmas Eve, Santa’s reindeer take to the skies—but not through holiday magic. Their whole flight control stack runs on pure eBPF, uplinked straight into the North Pole, a massive kprobe the reindeer feed telemetry into mid-flight. The ever-vigilant eBPF verifier rejects anything even slightly questionable, which is why the elves spend most of December hunched over terminals, running llvm-objdump on sleigh binaries and praying nothing in the control path gets inlined into oblivion again. It’s all very festive, in a high-performance-kernel-engineering sort of way. Ho ho .ko!
```

## Background Information: eBPF

There is a lot to eBPF, but I'll only cover what's relevant to this challenge! AI was really helpful to understand just a glimpse of what eBPF is, especially as documentation for some of this stuff was hard to find from just a Google search. I made sure to double check the accuracy of everything written here and my sources are cited at the end of the writeup.

Berkeley Packet Filter (BPF) was initially created to capture and filter networks packets (think of programs like tcpdump). Eventually, the functionality was expanded into eBPF (extended BPF). eBPF allows users to write small programs and attach them to certain kernel hooks to expand the functionality of the kernel without changing source code or writing modules. The exmaple we'll see in this challenge is the use of a kprobe, which allows us to attach a program onto a specific syscall. The security of the program that the kernel will execute is verified through static analysis (we won't go into how this static analysis works, but I'm wondering how effective it actually is...). The kernel can communicate the results of the program to user-space with a map.

eBPF programs are written in C and are compiled with clang, and the resulting object file is linked with a user-space program. libbpf is often used to prepare and attach the eBPF program to the kernel hook it will use. Functions include:

- bpf_object__open_file(...) to open the BPF file
- bpf_object__load(...) to load the BPF object
- bpf_object__find_program_by_name(...) to find the desired BPF program
- bpf_program__attach_kprobe(...) to attach the program to a certain syscall (use of the kprobe kernel hook)
- bpf_object__find_map_by_name(...) to find the map that the kernel and user-space use to communicate the results of the program
- bpf_map_lookup_elem(...) lookup an element in the shared mapping
- bpf_link__destroy(...) and bpf_object__close(...) to clean up

A .bpf.o file can be analyzed for the behavior of the BPF program if you don't have the source code. Since the object file is in the ELF file format, we can see what section might be most valuable to analyze.

```
hacker@2025~day-04:/challenge$ file tracker.bpf.o 
tracker.bpf.o: setuid ELF 64-bit LSB relocatable, eBPF, version 1 (SYSV), not stripped
hacker@2025~day-04:/challenge$ readelf -a tracker.bpf.o 

...

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .strtab           STRTAB           0000000000000000  00001df4
       000000000000008f  0000000000000000           0     0     1
  [ 2] .text             PROGBITS         0000000000000000  00000040
       0000000000000000  0000000000000000  AX       0     0     4
  [ 3] kprobe/__x64[...] PROGBITS         0000000000000000  00000040
       00000000000008a8  0000000000000000  AX       0     0     8
  [ 4] .relkprobe/_[...] REL              0000000000000000  00001860
       0000000000000030  0000000000000010   I      12     3     8
  [ 5] license           PROGBITS         0000000000000000  000008e8
       000000000000000d  0000000000000000  WA       0     0     1
  [ 6] .maps             PROGBITS         0000000000000000  000008f8
       0000000000000040  0000000000000000  WA       0     0     8
  [ 7] .BTF              PROGBITS         0000000000000000  00000938
       0000000000000937  0000000000000000           0     0     4
  [ 8] .rel.BTF          REL              0000000000000000  00001890
       0000000000000030  0000000000000010   I      12     7     8
  [ 9] .BTF.ext          PROGBITS         0000000000000000  00001270
       0000000000000560  0000000000000000           0     0     4
  [10] .rel.BTF.ext      REL              0000000000000000  000018c0
       0000000000000530  0000000000000010   I      12     9     8
  [11] .llvm_addrsig     LLVM_ADDRSIG     0000000000000000  00001df0
       0000000000000004  0000000000000000   E       0     0     1
  [12] .symtab           SYMTAB           0000000000000000  000017d0
       0000000000000090  0000000000000018           1     2     8

...

```

The file will have different sections depending on how the eBPF program is used/what kernel hook is used. If the program will act upon a syscall, the kprobe kernel hook will be used, generating the kprobe section (seen above). Other examples include tracepoint, XDP, etc. These sections will show the assembly of the program if obj dumped. This leads us to talk about how eBPF assembly is organized...

eBPF has 11 registers: R0 through R10. R10 is read-only and is the stack frame pointer, R0 is used for return values, R1 - R5 are arguments for function calls, and R6 - R9 are callee-saved registers (see Armosec blog linked below for more details). For each kind of section mentioned above (kprobe, tracepoint, XDP, etc.) the R1 register is the context register. When the kprobe kernel hook is used, the context register points to a pt_regs struct, which contains the state of the registers upon that syscall being used. This will also contain the arguments passed to the syscall since those value are stored in rdi, rsi, rdx, etc. (on 64-bit Linux). 

A .bpf.o file can be objdumped using llvm-objdump:

```
hacker@2025~day-04:/challenge$ llvm-objdump --no-show-raw-insn -d tracker.bpf.o 

tracker.bpf.o:  file format elf64-bpf

Disassembly of section kprobe/__x64_sys_linkat:

0000000000000000 <handle_do_linkat>:
       0:       r6 = *(u64 *)(r1 + 0x70)

...
```


## Vulnerability

Let's take a closer look at the files we're given and identify the vulnerability at hand. 

Running ls -alF, we see that .init is a symlink to the init-northpole.sh shell script which essentially starts the northpole program in the background and discards its output.

```
hacker@2025~day-04:/challenge$ cat init-northpole.sh 
#!/bin/sh

set -eu

/challenge/northpole > /dev/null 2>&1 &
```

An annotated version of northpole.c is available in the same directory as this writeup. The program essentially hooks the eBPF program in the tracker.bpf.o file to the syscall linkat() (uses a kprobe kernel hook). This means that immediatley when linkat() is called, the program starts up, does its thing, and writes its output in 0th index of the map array that the kernel and user-space use to communicate. When northpole sees that the output is non-zero, the function broadcast_cheer() is called, which just prints the flag on any connected terminal.

Thus, to get the flag, we need to use the linkat() syscall in the way the eBPF program expects that allows it to output a non-zero value, and therefore printing the flag. _How can we analyze the eBPF program if we only have it in .bpf.o format?_ As hinted in the challenge description, we can use llvm-objdump to dump the assembly of the program and reverse engineer our way to understanding how the eBPF program functions. 

## Exploitation

Before diving into the assembly, there's one thing we already know. The eBPF program is attached to the linkat() syscall which creates a hard link between two file paths (they refer to the same inode). We can do this on the command line with `ln /filepath/one /filepath/two`. Thus, we can assume that we'll probably have to hard link some files together to get the flag.

A fully annotated version of the disassembly is available in the same directory as this writeup, but I'll give an overview here. The first thing to note is that we have some data being compared to some words with ASCII letters: sleigh, dasher, vixen, cupid, blitzen, dancer, comet, donner, prancer. 

Since the kernel hook being used is a kprobe, we know that the r1 register is a pointer to a pt_regs struct.

```
//Source: https://elixir.bootlin.com/linux/v3.0/source/arch/x86/include/asm/ptrace.h
struct pt_regs {
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long bp;
	unsigned long bx;
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long ax;
	unsigned long cx;
	unsigned long dx;
	unsigned long si;
	unsigned long di;
	unsigned long orig_ax;
	unsigned long ip;
	unsigned long cs;
	unsigned long flags;
	unsigned long sp;
	unsigned long ss;
};
```

Register r6 contains the value 0x70 into the struct, which is register rdi. In x86-64 calling convention, the first argument is in rdi. Thus, we can assume that there's some argument checking going; perhaps, the names of the files being linked are checked against "sleigh" and the reindeer names. I noticed that each of reindeer names have a "return value" after them, assigning the name of a reindeer with a value. In order, the reindeer are: dasher, dancer, prancer, vixen, comet, cupid, donner, and blitzen. I also noticed "sleigh" didn't have one of these values. 

At this point, I took an educated guess and decided to try hard linking a file "sleigh" with each of the reindeer names...

```
hacker@2025~day-04:~$ vim sleigh
hacker@2025~day-04:~$ ls
sleigh
hacker@2025~day-04:~$ ln sleigh dasher
hacker@2025~day-04:~$ ln sleigh dancer
hacker@2025~day-04:~$ ln sleigh prancer
hacker@2025~day-04:~$ ln sleigh vixen
hacker@2025~day-04:~$ ln sleigh comet
hacker@2025~day-04:~$ ln sleigh cupid
hacker@2025~day-04:~$ ln sleigh donner
hacker@2025~day-04:~$ ln sleigh blitzen
🎅 🎄 🎁 Ho Ho Ho, Merry Christmas!
pwn.college{flag!}
^C
``` 

## Remediation

Being able to look into the disassembly of the eBPF program and gain clues about how it works led us to figure out how to trigger the printing of the flag. Thus, we could use classic ways of making our compiled binary more resistant to reversing, such as obfuscation.

# Sources/Credits

Written by Madalina Stoicov

- https://www.armosec.io/blog/ebpf-reverse-engineering-programs/
- https://en.wikipedia.org/wiki/EBPF
- https://www.sysdig.com/blog/the-art-of-writing-ebpf-programs-a-primer
- https://docs.ebpf.io/linux/program-type/BPF_PROG_TYPE_KPROBE/
- https://cs4157.github.io/www/2025-1/lect/18-unix-fs.html
- https://www.cs.cornell.edu/courses/cs4120/2022sp/notes.html?id=callconv
- https://elixir.bootlin.com/linux/v3.0/source/arch/x86/include/asm/ptrace.h
- ChatGPT to help connect the dots for understanding 






