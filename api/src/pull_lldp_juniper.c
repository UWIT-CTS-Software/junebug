#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <regex.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

struct PortRecord {
    char *portid;
    char *ipaddr;
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
    msleep(20);
}

char *write_struct_to_csv(struct PortRecord src) {
    int str_len = strlen(src.portid) + strlen(src.ipaddr) + strlen(src.chassisid) + strlen(src.hostname) + 4;
    char *dest = malloc(str_len);
    strcpy(dest, src.portid);
    strcat(dest, ",");
    strcat(dest, src.hostname);
    strcat(dest, ",");
    strcat(dest, src.chassisid);
    strcat(dest, ",");
    strcat(dest, src.ipaddr);
    strcat(dest, "\n");

    return dest;
}

void strslice(const char* src, char* dest, size_t start, size_t end) {
    strncpy(dest, src+start, end-start);
}

int main(void) {
    FILE *w_out = freopen("output.txt", "w", stdout);
    FILE *process = initProcess("python3 sample_cli.py");
    if (!(process && w_out)) {
        perror("[-] F_ERR: Error initializing.");
        return 1;
    }

    write_string("y\n", process);
    write_string("show lldp neighbors\n", process);

    fflush(process);
    // fclose(process);
    fclose(w_out);

    struct PortRecord records[12];
    int records_len = 0;
    char output[20];
    char *regex_string = "^([A-Za-z0-9./-]+).*([A-Za-z0-9:]{17}).* ([A-Za-z0-9]+-[A-Za-z0-9]+-[A-Za-z0-9]+)";
    size_t max_groups = 4;
    regex_t regex_rule;
    regmatch_t matches[max_groups];
    if (regcomp(&regex_rule, regex_string, REG_EXTENDED)) {
        perror("R_ERR: Could not compile regex.");
        return 1;
    }

    FILE *r_out = fopen("output.txt", "r");
    if (!r_out) {
        perror("[-] F_ERR: Error initializing.");
        return 1;
    }

    char line[256];
    int pass;
    while(fgets(line, sizeof(line), r_out) != NULL) {
        pass = regexec(&regex_rule, line, max_groups, matches, 0);
        if (!pass) {
            char *pack_struct[3];
            for (unsigned g=1; g<max_groups; g++) {
                if (matches[g].rm_so == (size_t)-1) {
                    break;
                }

                strslice(line, output, matches[g].rm_so, matches[g].rm_eo);
                output[matches[g].rm_eo-matches[g].rm_so] = 0;
                pack_struct[g-1] = malloc(matches[g].rm_eo-matches[g].rm_so);
                strcpy(pack_struct[g-1], output);
            }
            records[records_len].portid = malloc(strlen(pack_struct[0]));
            strcpy(records[records_len].portid, pack_struct[0]);
            records[records_len].chassisid = malloc(strlen(pack_struct[1]));
            strcpy(records[records_len].chassisid, pack_struct[1]);
            records[records_len].hostname = malloc(strlen(pack_struct[2]));
            strcpy(records[records_len].hostname, pack_struct[2]);

            records_len++;

        }
    }

    fclose(r_out);
    regfree(&regex_rule);

    w_out = freopen("output.txt", "w", stdout);
    if (!w_out) {
        perror("[-] F_ERR: Error opening write-only file.");
    }
    /* process = initProcess("python3 sample_cli.py");
    if (w_out == NULL || process == NULL) {
        perror("[-] F_ERR: Error initializing.");
        return 1;
    } */

    char *com_base = "show lldp neighbors interface ";
    int test = strlen(com_base);
    char *command = malloc(strlen(com_base)+strlen(records[0].portid)+3);
    for (int i=0; i<records_len; i++) {
        strcpy(command, com_base);
        strcat(command, records[i].portid);
        strcat(command, "\n");
        write_string(command, process);
    }
    free(command);
    write_string("quit\n", process);

    fclose(w_out);
    pclose(process);
    process = NULL;


    r_out = fopen("output.txt", "r");
    if (r_out == NULL) {
        perror("[-] F_ERR: Error opening read-only file.");
    }

    char *hn_regex_string = "^System name.*: ([A-Za-z0-9-]*)";
    size_t hn_max_groups = 2;
    regex_t hn_regex_rule;
    regmatch_t hn_matches[hn_max_groups];
    if (regcomp(&hn_regex_rule, hn_regex_string, REG_EXTENDED)) {
        perror("R_ERR: Could not compile regex.");
        return 1;
    }

    char *ip_regex_string = "^.*Address.*: ([0-9.]*)";
    size_t ip_max_groups = 2;
    regex_t ip_regex_rule;
    regmatch_t ip_matches[ip_max_groups];
    if (regcomp(&ip_regex_rule, ip_regex_string, REG_EXTENDED)) {
        perror("R_ERR: Could not compile regex.");
        return 1;
    }

    char *last_hn;
    int hn_pass;
    int ip_pass;
    while(fgets(line, sizeof(line), r_out) != NULL) {
        hn_pass = regexec(&hn_regex_rule, line, hn_max_groups, hn_matches, 0);
        ip_pass = regexec(&ip_regex_rule, line, ip_max_groups, ip_matches, 0);
        if (!hn_pass) {
            last_hn = malloc((hn_matches[1].rm_eo-hn_matches[1].rm_so)+1);
            strslice(line, last_hn, hn_matches[1].rm_so, hn_matches[1].rm_eo);
        } else if (!ip_pass) {
            for (int i=0; i<records_len; i++) {
                int result = strcmp(records[i].hostname, last_hn);
                if (!result) {
                    records[i].ipaddr = malloc((ip_matches[1].rm_eo-ip_matches[1].rm_so)+1);
                    strslice(line, records[i].ipaddr, ip_matches[1].rm_so, ip_matches[1].rm_eo);
                    break;
                }
            }
            free(last_hn);
        }
    }

    fclose(r_out);
    regfree(&hn_regex_rule);
    regfree(&ip_regex_rule);

    FILE *csv = fopen("records_out.csv", "w");
    char *csv_line;

    for (int i=0; i<records_len; i++) {
        csv_line = write_struct_to_csv(records[i]);
        fputs(csv_line, csv);
        free(records[i].portid);
        free(records[i].chassisid);
        free(records[i].hostname);
        free(records[i].ipaddr);
        free(csv_line);
    }
    fclose(csv);
    return 0;
}
