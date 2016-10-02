#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

FILE *ostream;

void sig_handler(int signum) {
    fprintf(ostream, "received signal: %d\n", signum);
}

int handle_input(const char * const line, size_t bytes) {
    if (bytes > 0) {
        fprintf(ostream, "Received command '%.*s'\n", (int)bytes, line);
    } else {
        fprintf(ostream, "Received empty command!\n");
    }
    static int nmoves = 0;

    if (strcmp(line, "xboard") == 0) {
        return 0;
    } else if (strncmp(line, "protover", strlen("protover")) == 0) {
        if (line[9] != '2') {
            fprintf(stderr, "Unrecognized protocol version:\n");
            return 1;
        }
        printf("feature myname=\"experiment\"\n");
        printf("feature reuse=0\n");
        printf("feature analyze=0\n");
        printf("feature done=1\n");
    } else if (strcmp(line, "new") == 0) {
        // reset board
    } else if (strcmp(line, "random") == 0) {
        // no-op
    } else if (strncmp(line, "level", strlen("level")) == 0) {
        // read time control format: "level MPS BASE INC" so "level 0 5 0" means 5 minutes for whole game
        // "level 0 2 12" means 2 minutes with 12 second increment
        // "level 0 0:30 0" means 30 seconds for whole game
        // "level 40 5 0" means 5 minutes for every 40 moves
    } else if (strcmp(line, "post") == 0) {
        // turn on thinking/pondering output

        // thinking output should be in the format "ply score time nodes pv"
        // E.g. "9 156 1084 48000 Nf3 Nc6 Nc3 Nf6" ==> "9 ply, score=1.56, time = 10.84 seconds, nodes=48000, PV = "Nf3 Nc6 Nc3 Nf6""
    } else if (strncmp(line, "accepted", strlen("accepted")) == 0) {
        // no-op
    } else if (strcmp(line, "post") == 0) {
        // no-op
    } else if (strcmp(line, "hard") == 0) {
        // no-op
    } else if (strcmp(line, "easy") == 0) {
        // no-op
    } else if (strcmp(line, "white") == 0) {
        // no-op
    } else if (strcmp(line, "black") == 0) {
        // no-op
    } else if (strncmp(line, "time", strlen("time")) == 0) {
        // no-op
    } else if (strncmp(line, "otim", strlen("otim")) == 0) {
        // no-op
    } else if (strcmp(line, "force") == 0) {
        // no-op
    } else if (strcmp(line, "go") == 0) {
        fprintf(ostream, "sending move to xboard...\n");
        printf("move e2e4\n");
        ++nmoves;
        fprintf(ostream, "sent move to xboard\n");
    } else if (strcmp(line, "e7e5") == 0) {
        printf("move d2d4\n");
    } else if (nmoves > 0) {
        fprintf(ostream, "trying to resign...\n");
        printf("resign\n");
    } else {
        fprintf(ostream, "Unrecognized command: '%s'\n", line);
        printf("Error (unknown command: %.*s\n", (int)bytes, line);
        return 0;
    }

    return 0;
}

int
main(void)
{
    FILE *istream;
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;

    signal(SIGINT, &sig_handler);

    istream = fdopen(STDIN_FILENO, "rb");
    if (istream == NULL)
        exit(EXIT_FAILURE);
    ostream = fopen("/tmp/output.txt", "w");
    if (ostream == NULL)
        exit(EXIT_FAILURE);
    /* turn off buffering */
    setbuf(stdout , NULL);
    setbuf(istream, NULL);
    setbuf(ostream, NULL);

    while ((read = getline(&line, &len, istream)) > 0) {
        fprintf(ostream, "Read new line... %d\n", (int)read);
        line[read-1] = 0;
        if (handle_input(line, read-1) != 0) {
            break;
        }
    }

    fprintf(ostream, "Bye.\n");
    fprintf(ostream, "Last received command: '%s'\n", line);
    free(line);
    fclose(istream);
    fclose(ostream);
    exit(EXIT_SUCCESS);
}


