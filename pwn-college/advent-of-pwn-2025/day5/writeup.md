# Pwn College Advent of Pwn Day 5 (in progress)

## Context

We are provided with a description of the challenge, a file sleigh.c, and the compiled and linked executable sleigh. The challenge description is as follows:

```
Did you ever wonder how Santa manages to deliver sooo many presents in one night?
Dashing through the code,
In a one-ring I/O sled,
O’er the syscalls go,
No blocking lies ahead!
Buffers queue and spin,
Completions shining bright,
What fun it is to read and write,
Async I/O tonight — hey!
```

## Background Information: seccomp

Linux seccomp (secure computing mode) restricts the number of syscalls and what specific syscalls are allowed or denied for a certain process to execute. seccomp_init(...) is used to specify
the action that will be taken if a syscall is executed that strays from the allowed syscalls. seccomp_rule_add(...) can be used to specify which syscalls are allowed or denied. To activate the
filter, seccomp_load(...) is used.

## Vulnerability

Let's take a closer look at sleigh.c. An annotated version is availabilty in the same directory as this writeup.

The program starts by creating a memory mapping at a specific address that is 0x1000 bytes in size. The mapping is readable, writeable, and executable. Then, a random number generator is set 
up with the seed of `time(NULL)` and a random number is generated between 1 and 100. The program reads from stdin into the memory mapping for 0x1000 bytes and then sets up a sandbox using pctrl and seccomp. First,
it is specified that the process running the program cannot gain new privileges. Then, the sandbox
specifies that any process or thread that uses syscalls other than these specified ones will be killed: io_uring_setup, io_uring_enter, io_uring_register, exit_group. Finally, the program
uses the randomly generated number specified earlier to execute code in the memory mapping at that random number:

```
srand(time(NULL));
int offset = (rand() % 100) + 1;

...

((void (*)())(code + offset))();
```

So, we'll need to feed in a program into the memory mapped region via stdin that uses io_uring functions to print our flag. _How can we bypass the fact that a random number will be used to decide
where execution begins?_ Note that the random number is only between 1 and 100, while the memory mapped region is much larger. This means we can use something like a NOP sled for the first 100 bytes
to ensure that execution always begins at the same place. _How can we properly feed in our program through stdin?_ Recall that we're basically dealing with shellcode here. Just like how shellcode
is used in malware, we'll need to feed in a bunch of raw bytes. When we write our program, we can strip away uncesessary parts of the binary and feed it the raw bytes.

There's one more hurdle we have to consider. According to the constraints of our sandbox, our process cannot gain elevated privileges, but /flag is only readable by root. Additionally, the io_uring 
syscalls we have access to cannot give us elevated privileges in the first place. One way we may be able to gain access to the flag is through a passed down file descriptor when our bash process
was exec'd. I used the `ps` command to get the PID of my current bash process, and then looked in the `/proc/<PID>/fd` directory to see what file descriptors my bash process has access to. I found that
the bash process has access to file descriptor 255 in addition to the expected stdin, stdout, and stderr:

```
ubuntu@2025~day-05:/proc/114/fd$ ls
0  1  2  255
```

Although we don't know for sure if this file descriptor has root access, we can direct our reading and writing commands through this fd and see if we do get access.


## Exploitation

Let's write a program that uses io_uring functions to read and print our flag and direct those reading and writing command through file descriptor 255.

[Okay, this is harder than I thought it would be. I tried using LLMs to write the payload for me, but I don't think they're succeeding very well... I also am not sure if I'm converting this to 
shellcode correctly...? What I have so far is in payload.c and the binary in payload.bin. I'm putting to rest for now lol, I'll pick it up again one day...]

## Remediation


# Sources/Credits

Written by Madalina Stoicov

- https://medium.com/google-cloud/understanding-seccomp-a-comprehensive-guide-b1c519167df3
- https://kernel.dk/io_uring.pdf
- https://unixism.net/loti/
- 


