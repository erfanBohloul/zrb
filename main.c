#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define debug(x) printf("%s:%d\n", #x, x)
#define debug_s(x) printf("%s:%s\n", #x, x)
#define alias_path "/Users/erfan/Codes/zrb/alias_ha.txt"

typedef int (*func_ptr)(char **);

// input:
func_ptr input_finder(char **command);

// repo:
char *get_repo_path();
int create_repo(char **argv);

// alias
char *search_alias(char *command);
char **handle_alias(int *argc, char **argv);

// general:
char *get_parent_folder(char *path);
int count_word(char *str);
int is_file_empty(FILE *file);

// errors:
void Undefined_Behaviour();
void Invalid_Command();

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        // no command provided
        printf("Do you need help?\n");
        return 0;
    }

    argv = handle_alias(&argc, argv);
    debug(argc);
    for (int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;

    func_ptr command_func = input_finder(argv[1]);
    if (!command_func)
    {
        fprintf(stderr, "there is no command named: \"%s\".\nyou may need some help.\n", argv[1]);
        return 0;
    }

    int status = command_func(&argv[1]);
    return 0;
}

int change_config(char **argv)
{
    int is_it_global = 0, idx = 1;
    if (!strcmp(argv[idx], "-global"))
    {
        is_it_global = 1;
        idx++;
    }

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

    char str[1000], key[100], *val;
    // debug_s(command);
    // char *tmp = fgets(str, sizeof(str), alias);
    // printf("tmp:%s\n", tmp);
    while (fgets(str, sizeof(str), alias))
    {
        debug_s(key);
        sscanf(str, "%s", key);

        if (!strcmp(key, command))
        {

            // get val
            val = strstr(str, ":") + 2;

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
        char *val = search_alias(argv[i]);

        // there is no value for key
        if (!val)
            continue;

        if (val[strlen(val) - 1] == '\n')
            val[strlen(val) - 1] = '\0';

        if (val == NULL)
            continue;

        int count = count_word(val);

        // try to put the alias one among other word;
        argv = (char **)realloc(argv, sizeof(char *) * (*argc + count - 1));

        // shift the rest of the word to right:
        for (int j = *argc - 1; j > i; j--)
        {
            char *tmp;
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

    // for (int i = 0; i < *argc; i++)
    // {
    //     printf("%s ", argv[i]);
    // }
    // printf("\n");
    return argv;
}

func_ptr input_finder(char **argv)
{
    char *command = argv[0];
    // looking for command in valid commands and then return a sutable function
    if (command == NULL)
    {
        fprintf(stderr, "[ERR] Command is null.\n");
        return NULL;
    }

    if (!strcmp(command, "init"))
    {
        return &create_repo;
    }

    else
    {
        // looking in alias commands
        char *alias_key = search_alias(command);

        if (!alias_key)
        {
            free(alias_key);
            return NULL;
        }

        strcpy(command, alias_key);
        free(alias_key);
        return input_finder(command);
    }

    Undefined_Behaviour();
    return NULL;
}

int create_repo(char **argv)
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

    // FILE *global_config = fopen(global_conf_path, "r");
    // if (global_conf_path == NULL)
    // {
    //     printf("[ERR] couldn't find the global config path\n");
    //     return 0;
    // }

    // FILE *conf = fopen("conf.txt", "w+");
    // if (global_conf_path == NULL)
    // {
    //     printf("[ERR] can not build a config file\n");
    //     return 0;
    // }
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
