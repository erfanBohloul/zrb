#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#define debug(x) printf("%s:%d\n", #x, x)
#define debug_s(x) printf("%s:%s\n", #x, x)

char *get_parent_folder(char *path);
char *get_repo_path();

char *func()
{
    char arr[] = "salam";
    return NULL;
}

int main(int argc, char **argv)
{

    // char *buffer = getcwd(NULL, 0);

    // char str[100] = "/Users/erfan/Codes/crack/.zrb";

    // DIR *dObj;
    // dObj = opendir(str);

    // if (dObj != NULL)
    // {
    //     printf("Directory exists.\n");
    //     // Close the directory stream
    //     closedir(dObj);
    // }
    // else
    // {
    //     printf("Directory does not exist.\n");
    // }

    // DIR *folder = opendir(".");
    // struct dirent *entry;
    // while ((entry = readdir(folder)) != NULL)
    // {
    //     debug_s(entry->d_name);
    // }

    // DIR *folder = opendir(".");
    // struct dirent *entry;
    // for (int i = 0; i < 3; i++)
    // {
    //     while ((entry = readdir(folder)) != NULL)
    //     {
    //         debug_s(entry->d_name);
    //     }

    //     folder = opendir("..");
    //     printf("\n\n");
    // }

    // char *repo_path = get_repo_path();
    // if (repo_path == NULL)
    // {
    //     printf("Could not find zrb repository!\n");
    // }
    // else
    //     debug_s(repo_path);

    char *c = func();
    free(c);
    printf("%s\n", c);
}

void joint(char *str1, char *str2)
{
    int n = strlen(str1),
        m = strlen(str2);

    // str1 = (char *)realloc(str1, (n + m) * sizeof(char));

    for (int i = 0; i < m; i++)
    {
        str1[n + i] = str2[i];
    }

    str1[n + m] = '\0';
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
                joint(path, "/.zrb");
                return path;
            }
        }

        path = get_parent_folder(path);
    }

    return NULL;
}