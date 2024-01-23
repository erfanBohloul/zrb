#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define debug(x) printf("%s:%d\n", #x, x)
#define debug_s(x) printf("%s:%s\n", #x, x)
#define alias_path "/Users/erfan/Codes/zrb/alias_ha.txt"
#define global_conf "/Users/erfan/Codes/zrb/author.txt"

typedef int (*func_ptr)(int, char **);

// input:
func_ptr input_finder(int argc, char **argv);

// repo:
char *get_repo_path();
int create_repo(int argc, char **argv);

// alias
char *search_alias(char *command);
char **handle_alias(int *argc, char **command);

// general:
char *get_parent_folder(char *path);
int count_word(char *str);
int is_file_empty(FILE *file);
char *get_local_config_path();
void copy_file(FILE *src, FILE *dest);

// errors:
void Undefined_Behaviour();
void Invalid_Command();
void Not_repo();

// config
int change_config(int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        // no command provided
        printf("Do you need help?\n");
        return 0;
    }

    argv = handle_alias(&argc, argv);
    for (int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");

    func_ptr command_func = input_finder(argc, &argv[1]);
    if (!command_func)
    {
        fprintf(stderr, "there is no command named: \"%s\".\nyou may need some help.\n", argv[1]);
        return 0;
    }

    int status = command_func(argc - 1, &argv[1]);
    return 0;
}

int count_word(char *str)
{

    int count = 0;
    while (*str == ' ')
        ++str;

    char last_char = '\0';
    while (1)
    {
        if (*str == '\0')
            break;

        if (*str == ' ' && last_char != ' ')
        {
            count++;
        }
        last_char = *str;
        ++str;
    }

    return count + 1;
}

int is_file_empty(FILE *file)
{
    fseek(file, 0, SEEK_END);
    long sz = ftell(file);
    rewind(file);
    return sz == 0;
}

char *search_alias(char *command)
{
    FILE *alias = fopen(alias_path, "r");
    if (!alias)
    {
        fprintf(stderr, "[ERR] can not open alias file.\n");
        return NULL;
    }
    if (command == NULL)
        return NULL;

    char str[1000], key[100], *val;
    // char *tmp = fgets(str, sizeof(str), alias);
    // printf("tmp:%s\n", tmp);
    while (fgets(str, sizeof(str), alias))
    {

        sscanf(str, "%s", key);
        if (!strcmp(key, command))
        {

            // get val
            val = strstr(str, ":") + 2;

            if (val[strlen(val) - 1] == '\n')
                val[strlen(val) - 1] = '\0';

            printf("found: %s -> %s\n", key, val);

            return strdup(val);
        }
    }

    return NULL;
}

char **handle_alias(int *argc, char **command)
{
    // try to alloc argv:
    char **argv = (char **)malloc(sizeof(char *) * *argc);
    for (int i = 0; i < *argc; i++)
    {
        argv[i] = command[i];
    }

    // try every word and replace it with alias one
    for (int i = 1; i < *argc; i++)
    {
        if (!argv[i])
            continue;

        char *val = search_alias(argv[i]);

        // there is no value for key
        if (!val)
            continue;

        int count = count_word(val);

        // try to put the alias one among other word;
        argv = (char **)realloc(argv, sizeof(char *) * (*argc + count - 1));

        // shift the rest of the word to right:
        for (int j = *argc - 1; j > i; j--)
        {
            if (argv[j] == NULL)
                continue;

            char tmp[100];
            strcpy(tmp, argv[j]);
            argv[j + count - 1] = strdup(tmp);
        }
        *argc = *argc + count - 1;
        // replace the alias ones:
        char *token = strtok(val, " ");

        while (token != NULL)
        {
            argv[i] = token;

            token = strtok(NULL, " ");
            i++;
        }
        i--;
    }
    return argv;
}

func_ptr input_finder(int argc, char **argv)
{
    char *command = argv[0];
    // looking for command in valid commands and then return a sutable function
    if (command == NULL)
    {
        fprintf(stderr, "[ERROR] Command is null.\n");
        return NULL;
    }

    if (!strcmp(command, "init"))
    {
        return &create_repo;
    }

    if (!strcmp(command, "config"))
    {
        return &change_config;
    }

    // else:
    Invalid_Command();

    return NULL;
}

int change_config_file(FILE *conf_file, int argc, char **argv, int idx)
{
    // file must be in write mode
    if (conf_file == NULL)
    {
        printf("Error opening config file\n");
        return -1;
    }

    int line_num = 0;
    char line[100] = "user.";
    if (!strcmp(argv[idx], "user.name"))
    {
        strcat(line, "name = ");
        strcat(line, argv[idx + 1]);
        line_num = 1;
    }
    else if (!strcmp(argv[idx], "user.name"))
    {
        strcat(line, "email = ");
        strcat(line, argv[idx + 1]);
        line_num = 2;
    }

    else
    {
        Invalid_Command();
        return -1;
    }

    // change file:
    int count = 0;
    char old_line[100];
    while (fgets(old_line, sizeof line, conf_file))
    {
        count++;
        if (count == line_num)
        {
            fprintf(conf_file, "%s", line);
        }

        fprintf(conf_file, "%s", old_line);
    }

    fclose(conf_file);
    return 0;
}

char *get_local_config_path()
{
    char *repo_path = get_repo_path();
    if (repo_path == NULL)
    {
        Not_repo();
        return NULL;
    }

    strcat(repo_path, "/author.txt");
    return strdup(repo_path);
}

int change_config(int argc, char **argv)
{
    char *repo_path = get_repo_path();
    if (!repo_path)
    {
        Not_repo();
        return -1;
    }

    FILE *conf_file;

    int idx = 2;
    if (!strcmp(argv[2], "-global"))
    {
        printf("Global configuration\n");
        conf_file = fopen(global_conf, "w");
        change_config_file(conf_file, argc, argv, idx + 1);
        idx++;
    }

    printf("Local configuration\n");
    conf_file = fopen(get_local_config_path(), "w");
    change_config_file(conf_file, argc, argv, idx);

    fclose(conf_file);
    free(repo_path);

    return 0;
}

void copy_file(FILE *src, FILE *dest)
{
    int ch;
    while ((ch = fgetc(src)) != EOF)
    {
        fputc(ch, dest);
    }
}

// int newCommit()
// {
//     // create a helper FILE
//     FILE *tmp_w = fopen("./.zrb/help", "w");
//     if (!tmp_w)
//     {
//         perror("fopen tmp_w");
//         return -1;
//     }

//     FILE *tmp_r = fopen("./.zrb/help", "r");
//     if (!tmp_r)
//     {
//         perror("fopen tmp_r");
//         return -1;
//     }
// }

void write_to_file(FILE *file, char *str)
{
    // the file is in the write mode
    fputs(str, file);
}

int create_repo(int argc, char **argv)
{
    if (get_repo_path() != NULL)
    {
        printf("[ERR] a repo already exists\n");
        return 0;
    }

    if (mkdir(".zrb", 0777) == -1)
    {
        printf("[ERR] couldn't create the repo file\n");
        return 0;
    }

    printf("[SUCC] .zrb file has create\n");

    // handle_author
    FILE *author = fopen("./.zrb/author.txt", "w");
    if (!author)
    {
        perror("[ERROR] fopen author.txt");
        return -1;
    }
    FILE *global_author = fopen(global_conf, "r");
    if (!global_author)
    {
        perror("[ERROR] fopen global_author.txt");
        return -1;
    }

    copy_file(global_author, author);
    fclose(author);
    fclose(global_author);

    printf("[SUCC] author has set\n");

    // files
    if (mkdir("./.zrb/files", 0777) == -1)
    {
        perror("[ERROR] mkdir .zrb/files");
        return -1;
    }

    // create a root commit:
    FILE *root_commit = fopen("./.zrb/files/root.txt", "w");
    write_to_file(root_commit, "commit root\nroot\n");

    printf("[SUCC] folder of file has created\n");

    // create stage folder:
    if (mkdir("./.zrb/stage", 0777) == -1)
    {
        perror("[ERROR] mkdir ./.zrb/stage");
        return -1;
    }

    printf("[SUCC] the stage has created\n");
    return 0;
}

char *get_parent_folder(char *path)
{
    int len = strlen(path);

    for (int i = len - 1; i >= 0; i--)
    {
        if (path[i] == '/')
        {
            path[i] = '\0';
            return path;
        }

        path[i] = '\0';
    }

    return path;
}

char *get_repo_path()
{
    char *path = getcwd(NULL, 0);
    DIR *folder;
    while (strlen(path))
    {
        folder = opendir(path);

        struct dirent *entry;
        while ((entry = readdir(folder)) != NULL)
        {
            if (!strcmp(".zrb", entry->d_name))
            {
                strcat(path, "/.zrb");
                return path;
            }
        }

        path = get_parent_folder(path);
    }

    return NULL;
}

void Undefined_Behaviour()
{
    fprintf(stderr, "[ERR] undefined behaviour\n");
}

void Invalid_Command()
{
    fprintf(stderr, "[ERROR] invalid command\n");
}

void Not_repo()
{
    fprintf(stderr, "[ERROR] not a zrb repo\n");
}
