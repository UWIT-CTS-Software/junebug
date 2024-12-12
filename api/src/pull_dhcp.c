#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <regex.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

struct PortRecord {
    char *portid;
    char *chassisid;
    char *hostname;
};

FILE *initProcess(char *command) {
    FILE *process = popen(command, "w");

    if (process == NULL) {
        printf("[-] P_ERR: Process not established.");
        exit(1);
    }
    return process;
}

int msleep(long msec) {
    struct timespec ts;
    
    if (msec < 0) {
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    int ret_val = nanosleep(&ts, &ts);

    return ret_val;
}

void write_string(char *string, FILE *process) {
    if (fputs(string, process) == EOF) {
        perror("[-] W_ERR: Failed to write.");
        pclose(process);
        exit(1);
    }
    // msleep(10);
}

void strslice(const char* src, char* dest, size_t start, size_t end) {
    strncpy(dest, src+start, end-start);
}

int main(void) {
    FILE *w_out = freopen("output.txt", "w+", stdout);
    FILE *process = initProcess("python3 sample_cli.py");
    if (w_out == NULL || process == NULL) {
        perror("[-] F_ERR: Error initializing.");
        return 1;
    }

    write_string("y\n", process);
    write_string("show lldp neighbors\n", process);
    write_string("quit\n", process);

    pclose(process);
    process = NULL;
    fclose(w_out);

    FILE *r_out = fopen("output.txt", "r");
    if (r_out == NULL) {
        perror("[-] F_ERR: Error opening read-only file.");
    }

    // struct PortRecord records[12];
    char *records[12][3];
    int records_len = 0;
    char output[20];
    char *regex_string = "^([A-Za-z0-9./-]+).*([A-Za-z0-9:]{17}).*([A-Za-z0-9]+-[A-Za-z0-9]+-[A-Za-z0-9]+)";
    size_t max_groups = 4;
    regex_t regex_rule;
    regmatch_t matches[max_groups];
    if (regcomp(&regex_rule, regex_string, REG_EXTENDED)) {
        perror("R_ERR: Could not compile regex.");
        return 1;
    }

    char line[256];
    int pass;
    while(fgets(line, sizeof(line), r_out) != NULL) {
        pass = regexec(&regex_rule, line, max_groups, matches, 0);
        if (!pass) {

            unsigned offset = 0;
            for (unsigned g=1; g<max_groups; g++) {
                if (matches[g].rm_so == (size_t)-1) {
                    break;
                }

                strslice(line, output, matches[g].rm_so, matches[g].rm_eo);
                output[matches[g].rm_eo-matches[g].rm_so] = 0;
                records[records_len][g-1] = malloc(matches[g].rm_eo-matches[g].rm_so);
                strcpy(records[records_len][g-1], output);
            }
            records_len++;
        }
    }

    fclose(r_out);
    regfree(&regex_rule);

    w_out = freopen("output.txt", "w", stdout);
    if (w_out == NULL) {
        perror("[-] F_ERR: Error opening write-only file.");
    }
    process = initProcess("python3 sample_cli.py");
    if (w_out == NULL || process == NULL) {
        perror("[-] F_ERR: Error initializing.");
        return 1;
    }

    write_string("y\n", process);
    char *com_base = "show lldp neighbors interface ";
    for (int i=0; i<records_len; i++) {
        char *command = malloc(strlen(com_base)+strlen(records[i][0])+2);
        strcpy(command, com_base);
        strcat(command, records[i][0]);
        strcat(command, "\n");
        write_string(command, process);
        free(command);
    }

    pclose(process);
    process = NULL;
    fclose(w_out);

    for (int i=0; i<records_len; i++) {
        for (int j=0; j<3; j++) {
            free(records[i][j]);
        }
    }
    return 0;
}
