// Annotated claus.c

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char gift[256];

//Replace the flag (gift array) with # characters and print the progress to the screen
void wrap(char *gift, size_t size)
{
    fprintf(stdout, "Wrapping gift: [          ] 0%%");
    for (int i = 0; i < size; i++) {
        sleep(1);
        gift[i] = "#####\n"[i % 6];
        int progress = (i + 1) * 100 / size;
        int bars = progress / 10;
        fprintf(stdout, "\rWrapping gift: [");
        for (int j = 0; j < 10; j++) {
            fputc(j < bars ? '=' : ' ', stdout);
        }
        fprintf(stdout, "] %d%%", progress);
        fflush(stdout);
    }
    fprintf(stdout, "\n🎁 Gift wrapped successfully!\n\n");
}

//When the program recieves SIGTSTP, this message will print
void sigtstp_handler(int signum)
{
    puts("🎅 Santa won't stop!");
}

int main(int argc, char **argv, char **envp)
{
    uid_t ruid, euid, suid;

    
    //Retrieves the values of the process's RUID, EUID, and SUID
    if (getresuid(&ruid, &euid, &suid) == -1) {
        perror("getresuid");
        return 1;
    }

    //Ensures that EUID is root
    if (euid != 0) {
        fprintf(stderr, "❌ Error: Santa must wrap as root!\n");
        return 1;
    }

    //Ensures that the user who ran the program is not root
    if (ruid != 0) {

        //Changes the RUID and EUID of the program
        //RUID is changed to 0 (root)
        //EUID is unchanged (stays as root)
        if (setreuid(0, -1) == -1) {
            perror("setreuid");
            return 1;
        }

        fprintf(stdout, "🦌 Now, Dasher! now, Dancer! now, Prancer and Vixen!\nOn, Comet! on Cupid! on, Donder and Blitzen!\n\n");

        //Relaunches the program under the same PID with the new RUID
        execve("/proc/self/exe", argv, envp);

        perror("execve");
        return 127;
    }

    //A signal handle is registered for when the program recieves a SIGTSTP signal
    //Thus, when the program recieves SIGSTP, the sigtstp_handler function will execute
    if (signal(SIGTSTP, sigtstp_handler) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    //Open a file descriptor for the flag
    int fd = open("/flag", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    //Read in the flag to the global char array gift
    int count = read(fd, gift, sizeof(gift));
    if (count == -1) {
        perror("read");
        return 1;
    }

    //Call the wrap function on the flag
    wrap(gift, count);

    puts("🎄 Merry Christmas!\n");

    //Print the new flag
    puts(gift);

    return 0;
}
