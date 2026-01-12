# Pwn College Advent of Pwn Day 3

## Context

We are provided with a description of the challenge and two shell scripts: init-stuff-stocking.sh and stuff-stocking. The description of the challenge is lengthy (and honestly didn't really help in the understanding and solution of the challenge) so I'll skip pasting it below.

## Background Information: Niceness of processes

Processes should have some way of signalling to the CPU that it should be prioritized over other processes. All operating systems have some version of this, and in Linux it's called "niceness".
Each process has a nice value, where the higher the value, the more willing that process is to, for example, share CPU resources with another process. This process is "nicer" than other processes because it's saying that it's lower in priority than other processes.
The "ni" option for the "ps" command can be used to see the niceness of a given process:

```
ubuntu@2025~day-03:~$ ps -eo pid,ni,comm
    PID  NI COMMAND
      1   0 docker-init
      7   0 /run/dojo/bin/s
    135   0 stuff-stocking
    208   0 ssh-entrypoint
    214   0 bash
    313   0 sleep
    314   0 ps
ubuntu@2025~day-03:~$
```

## Vulnerability

Let's take a closer look at the files we're given and identify the vulnerability at hand. 

Running ls -alF, we see that .init is a symlink to the init-stuff-stocking.sh shell script:

```
ubuntu@2025~day-03:~$ ls -alF /challenge/
total 24
drwxr-xr-x 1 root root 4096 Jan  5 03:25 ./
drwxr-xr-x 1 root root 4096 Jan  5 03:25 ../
lrwxrwxrwx 1 root root   24 Dec  2 23:49 .init -> ./init-stuff-stocking.sh*
-rwsr-xr-x 1 root root 3152 Dec  2 23:54 DESCRIPTION.md*
-rwsr-xr-x 1 root root   39 Dec  2 23:49 init-stuff-stocking.sh*
-rwsr-xr-x 1 root root  338 Dec  2 23:49 stuff-stocking*
```

init-stuff-stocking.sh essentially starts the stuff-stocking program in the background. 

```
ubuntu@2025~day-03:~$ cat /challenge/init-stuff-stocking.sh 
#!/bin/sh

/challenge/stuff-stocking &
```

Let's take a look at the stuff-stocking program.

```
ubuntu@2025~day-03:~$ cat /challenge/stuff-stocking 
#!/bin/sh

set -eu

GIFT="$(cat /flag)"
rm /flag

touch /stocking

sleeping_nice() {
    ps ao ni,comm --no-headers \
        | awk '$1 > 0' \
        | grep -q sleep
}

# Only when children sleep sweetly and nice does Santa begin his flight
until sleeping_nice; do
    sleep 0.1
done

chmod 400 /stocking
printf "%s" "$GIFT" > /stocking
ubuntu@2025~day-03:~$ 
```

The program grabs the flag from /flag, removes the file, and then checks if any currently running process has a positive "niceness" and originates from a sleep command. 
If not, the program will sleep for 0.1 seconds and then check once again. If a process that meets these requirements is found, the stocking file will become read-only for its owner and the flag
will be printed in the file. As we can see, the stocking file is owned by root, so "chmod 400" would render the file un-readable for a non-root user.

```
ubuntu@2025~day-03:~$ ls -l /stocking
-rw-r--r-- 1 root root 0 Jan  5 03:25 /stocking

```
We need some way 
to be able to maintain reading permission inside the stocking file even after the stuff-stocking program locks us out of reading that file. That means we'll also have to spawn a process that
originated with the sleep command and has a positive niceness.

## Exploitation

The tail command with the -f option allows data from that file to be appended as it is recieved. 

```
TAIL(1)                       User Commands                       TAIL(1)

NAME         top

       tail - output the last part of files
...

       -f, --follow[={name|descriptor}]
              output appended data as the file grows; an absent option
              argument means 'descriptor'
```

SSHing into the challenge in two different terminals, I started a tail command in one terminal: tail -f /stocking. In the second terminal, I spawned a process with a positive niceness that 
will sleep for 5 seconds: nice -n 1 sleep 5. Since this process is found by the stuff-stocking program, it will continue. It locks up the stocking file and then prints the flag. Since we have
a tail -f command running, we can see the flag being printed into the stocking command.

```
ubuntu@2025~day-03:~$ tail -f /stocking 
pwn.college{ flag redacted :) }
```

## Remediation

The easiest way to remediate this problem of being able to read a file even after privileges have been removed is give it the necessary permissions in the first place. If the stocking
file was only readable by root to begin with stuff-stocking ran as root, we would not be able to use the tail command to read the file.

# Sources/Credits

Written by Madalina Stoicov
