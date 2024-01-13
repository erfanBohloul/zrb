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
func_ptr input_finder(char *command);

// repo:
char *get_repo_path();
int create_repo(char **argv);
char *search_alias(char *command);

// general:
char *get_parent_folder(char *path);

// errors:
void Undefined_Behaviour();

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        // no command provided
        printf("Do you need help?\n");
        return 0;
    }

    func_ptr command_func = input_finder(argv[1]);
    if (!command_func)
    {
        fprintf(stderr, "there is no command named: \"%s\".\nyou may need some help.\n", argv[1]);
        return 0;
    }

    int status = command_func(&argv[1]);
    return 0;
}

char *search_alias(char *command)
{
    FILE *alias = fopen(alias_path, "r");
    if (!alias)
    {
        fprintf(stderr, "[ERR] can not open alias file.\n");
        return NULL;
    }

    char str[300], key[100], val[100];
    while (fgets(str, sizeof(str), alias) != NULL)
    {
        sscanf(str, "%s : %s", key, val);
        if (!strcmp(key, command))
        {
            printf("found: %s -> %s\n", key, val);
            return strdup(val);
        }
    }

    return NULL;
}

func_ptr input_finder(char *command)
{
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
    fprintf(stderr, "[ERR] undefined behaviour");
}
