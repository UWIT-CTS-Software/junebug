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

FILE *pinit(char *command) {
    FILE *process = popen(command, "w");
    if (!process) {
        printf("[-] P_ERR: Process not established.");
        exit(1);
    }

    return process;
}

FILE *open(const char *filename, const char *mode) {
    FILE *file = fopen(filename, mode);
    if (!file) {
        perror("[-] F_ERR: Error opening file.");
        exit(1);
    }

    return file;
}

FILE *reopen(const char *filename, const char *mode) {
    FILE *file = freopen(filename, mode, stdout);
    if (!file) {
        perror("R_ERR: freopen failed.");
    }

    return file;
}

int msleep(long msec) {
    struct timespec ts;
    
    if (msec < 0) {
        exit(1);
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    int ret_val = nanosleep(&ts, &ts);

    return ret_val;
}

void wstring(char *string, FILE *process) {
    size_t written = fwrite(string, sizeof(char), strlen(string), process);
    if (written != strlen(string)) {
        perror("[-] W_ERR: Failed to write.");
        pclose(process);
        exit(1);
    }
    msleep(100);
}

char *prtocsv(struct PortRecord src) {
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

int jexec(const char* hostname) {
    FILE *out = reopen("output.txt", "w+");
    FILE *process = pinit("python3 sample_cli_juniper.py");

    wstring("y\n", process);
    wstring("show lldp neighbors\n", process);

    fflush(process);
    msleep(200);
    fflush(out);
    rewind(out);

    struct PortRecord records[12];
    int records_len = 0;
    char output[20];
    char *regex_string = "^([A-Za-z0-9./-]+).*([A-Za-z0-9:]{17}).* ([A-Za-z0-9]+-[A-Za-z0-9]+-[A-Za-z0-9]+)";
    size_t max_groups = 4;
    regex_t regex_rule;
    regmatch_t matches[max_groups];
    if (regcomp(&regex_rule, regex_string, REG_EXTENDED)) {
        perror("R_ERR: Could not compile regex.");
        exit(1);
    }

    char line[256];
    int pass;
    while(fgets(line, sizeof(line), out) != NULL) {
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

    regfree(&regex_rule);

    char *com_base = "show lldp neighbors interface ";
    char command[128];
    for (int i=0; i<records_len; i++) {
        strcpy(command, com_base);
        strcat(command, records[i].portid);
        strcat(command, "\n");
        wstring(command, process);
    }
    wstring("quit\n", process);


    fflush(process);
    msleep(200);
    fflush(out);
    rewind(out);

    char *hn_regex_string = "^System name.*: ([A-Za-z0-9-]*)";
    size_t hn_max_groups = 2;
    regex_t hn_regex_rule;
    regmatch_t hn_matches[hn_max_groups];
    if (regcomp(&hn_regex_rule, hn_regex_string, REG_EXTENDED)) {
        perror("R_ERR: Could not compile regex.");
        exit(1);
    }

    char *ip_regex_string = "^.*Address.*: ([0-9.]*)";
    size_t ip_max_groups = 2;
    regex_t ip_regex_rule;
    regmatch_t ip_matches[ip_max_groups];
    if (regcomp(&ip_regex_rule, ip_regex_string, REG_EXTENDED)) {
        perror("R_ERR: Could not compile regex.");
        exit(1);
    }

    char *last_hn;
    int hn_pass;
    int ip_pass;
    while(fgets(line, sizeof(line), out) != NULL) {
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

    regfree(&hn_regex_rule);
    regfree(&ip_regex_rule);

    FILE *csv = open("records_out.csv", "w");
    char *csv_line;

    for (int i=0; i<records_len; i++) {
        csv_line = prtocsv(records[i]);
        fputs(csv_line, csv);
        free(records[i].portid);
        free(records[i].chassisid);
        free(records[i].hostname);
        free(records[i].ipaddr);
        free(csv_line);
    }
    fclose(csv);
    fclose(out);
    pclose(process);
    process = NULL;

    return 0;
}

int nexec(const char* hostname) {
    FILE *out = reopen("output.txt", "w+");
    FILE *process = pinit("python3 sample_cli_netgear.py");

    wstring("enable\n", process);
    wstring("show lldp interface all\n", process);

    fflush(process);
    msleep(200);
    fflush(out);
    rewind(out);

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

    char line[256];
    int pass;
    while(fgets(line, sizeof(line), out) != NULL) {
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

    regfree(&regex_rule);

    char *com_base = "show lldp interface ";
    char command[128];
    for (int i=0; i<records_len; i++) {
        strcpy(command, com_base);
        strcat(command, records[i].portid);
        strcat(command, "\n");
        wstring(command, process);
    }
    // free(command);
    wstring("quit\n", process);

    fflush(process);
    msleep(200);
    fflush(out);
    rewind(out);

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
    while(fgets(line, sizeof(line), out) != NULL) {
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

    regfree(&hn_regex_rule);
    regfree(&ip_regex_rule);

    FILE *csv = open("records_out.csv", "w");
    char *csv_line;

    for (int i=0; i<records_len; i++) {
        csv_line = prtocsv(records[i]);
        fputs(csv_line, csv);
        free(records[i].portid);
        free(records[i].chassisid);
        free(records[i].hostname);
        free(records[i].ipaddr);
        free(csv_line);
    }
    fclose(csv);
    fclose(out);
    pclose(process);
    process = NULL;

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        perror("[-] ARG_ERR: Insufficient number of arguments passed.");
        printf("    Arguments recieved: %d\n", argc);
        exit(1);
    }

    if (!strcmp(argv[1],"juniper")) {
        jexec(argv[2]);
    } else if (!strcmp(argv[1],"netgear")) {
        nexec(argv[2]);
    } else {
        perror("[-] DEV_ERR: Incorrect device type passed as the first argument.");
        exit(1);
    }
    
    return 0;
}
