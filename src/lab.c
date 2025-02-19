#include "lab.h"
#include <stdio.h>
#include <getopt.h>

char *get_prompt(const char *env) {
    // Implementation here
    return NULL;
}

int change_dir(char **dir) {
    // Implementation here
    return 0;
}

char **cmd_parse(char const *line) {
    // Implementation here
    return NULL;
}

void cmd_free(char **line) {
    // Implementation here
}

char *trim_white(char *line) {
    // Implementation here
    return line;
}

bool do_builtin(struct shell *sh, char **argv) {
    // Implementation here
    return false;
}

void sh_init(struct shell *sh) {
    // Implementation here
}

void sh_destroy(struct shell *sh) {
    // Implementation here
}

void parse_args(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                printf("Shell version %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}