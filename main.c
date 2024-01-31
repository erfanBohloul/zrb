#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <time.h>

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

// add
int add(int argc, char **argv);

// general:
char *get_parent_folder(char *path);
int count_word(char *str);
int is_file_empty(FILE *file);
char *get_local_config_path();
void copy_file(FILE *src, FILE *dest);
char *string_format(const char *format, ...);
char *sha_hash(char *path_file);
void cd_proj();
char *get_proj_path();
char *get_author_name();
char *get_author_email();
FILE *get_helper_file();
char *get_helper_file_path();
char *get_stage_path();
char *get_file_repo_path();

// errors:
void Undefined_Behaviour();
void Invalid_Command();
void Not_repo();

// config
int change_config(int argc, char **argv);

// commit
int commit(int argc, char **argv);

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

    func_ptr command_func = input_finder(argc - 1, &argv[1]);
    if (!command_func)
    {
        fprintf(stderr, "there is no command named: \"%s\".\nyou may need some help.\n", argv[1]);
        return 0;
    }

    int status = command_func(argc - 1, &argv[1]);
    return 0;
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

    if (!strcmp(command, "add"))
    {
        return &add;
    }

    if (!strcmp(command, "commit"))
    {
        if (!strcmp(argv[1], "-m"))
            return &commit;
        else if (!strcmp(argv[1], "-s"))
            return NULL;

        Invalid_Command();
        return NULL;
    }

    // else:
    Invalid_Command();

    return NULL;
}

FILE *get_helper_file()
{
    char *proj_path = get_proj_path();
    char *command = string_format("%s/.zrb/helper.txt", proj_path);
    FILE *helper = fopen(command, "r+");
    if (helper == NULL)
    {
        perror("opening helper file");
        return NULL;
    }

    free(proj_path);
    free(command);
    return helper;
}

char *get_helper_file_path()
{
    char *proj_path = get_proj_path();
    char *command = string_format("%s/.zrb/helper.txt", proj_path);
    free(proj_path);
    return command;
}

char *sha_hash(char *path_file)
{
    char *helper_path = get_helper_file_path();
    char *command = string_format("shasum -a 256 %s > %s", path_file, helper_path);
    debug_s(command);
    system(command);
    free(command);

    FILE *helper = get_helper_file();
    if (helper == NULL)
    {
        return NULL;
    }

    char sha[1000];
    fgets(sha, sizeof sha, helper);
    char *hash = strtok(sha, " ");
    if (hash[strlen(hash) - 1] == '\n')
        hash[strlen(hash) - 1] = '\0';

    fclose(helper);
    return strdup(hash);
}

int add_file_to_repo_files(FILE *file, char *file_name, char *repo_path, FILE *commit_file)
{

    // hash file with sha algo
    char *sha = sha_hash(repo_path);
    if (sha == NULL)
    {
        return -1;
    }

    char *file_repo_path = get_file_repo_path();
    char *sha_file_path = string_format("%s/%s.txt", file_repo_path, sha);

    // rename the file to its hash name:
    char *command = string_format("mv %s %s", repo_path, sha_file_path);
    debug_s(command);
    system(command);
    free(command);

    // add this hash to parent-commit folder as an hash
    command = string_format("%s.txt %s\n", sha, file_name);
    printf("saved in commit: %s\n", command);
    fputs(command, commit_file);
    free(command);

    free(sha);
    free(sha_file_path);
    free(file_repo_path);
    return 0;
}

char *commit_in_depth(DIR *folder, char *stage_path, char *repo_path)
{
    if (folder == NULL)
    {
        perror("[ERROR] can not going depth with null folder");
        return NULL;
    }

    // create a subdir commit-file
    char *repo_folder_path = string_format("%s.txt", repo_path);
    // create a file with a temprary name
    FILE *sub_commit_file = fopen(repo_folder_path, "w+");
    if (sub_commit_file == NULL)
    {
        perror("[ERROR] building sub commit folder");
        return NULL;
    }

    // add folder key word to start of "sub_commit_file":
    fputs("folder\n", sub_commit_file);

    struct dirent *entry;
    while ((entry = readdir(folder)) != NULL)
    {

        // skip . and .. file
        if (
            !strcmp(entry->d_name, ".") ||
            !strcmp(entry->d_name, ".."))
            continue;

        debug_s(entry->d_name);

        char *sub_stage_path = string_format("%s/%s", stage_path, entry->d_name);
        char *sub_repo_path = string_format("%s-%s", repo_path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            char *sub_hash = commit_in_depth(opendir(sub_stage_path), sub_stage_path, sub_repo_path);
            if (sub_hash == NULL)
            {
                return NULL;
            }

            char *commandu = string_format("%s %s\n", sub_hash, entry->d_name);
            debug_s(commandu);
            fputs(commandu, sub_commit_file);

            free(commandu);
            free(sub_hash);
        }

        else
        {
            // its a file
            FILE *file = fopen(sub_repo_path, "w+");
            if (file == NULL)
            {
                perror("[ERROR] can not create a duplicate in file-repo");
                continue;
            }

            FILE *origin_file = fopen(sub_stage_path, "r+");
            if (origin_file == NULL)
            {
                perror("[ERROR] can not open the origin file from stage area");
                continue;
            }

            // add "file" key word to start of the duplicate file in file-repo folder
            fputs("file\n", file);

            // copy content of origin file in stage area into duplicate file in file-repo folder
            copy_file(origin_file, file);

            // this function create a hash of dup file and then rename it with hash name
            int res = add_file_to_repo_files(file, entry->d_name, sub_repo_path, sub_commit_file);
            if (res == -1)
            {
                perror("[ERROR] can not commit file");
                continue;
            }

            free(sub_repo_path);
            free(sub_stage_path);
        }
    }

    fclose(sub_commit_file);
    sub_commit_file = fopen(repo_folder_path, "r+");

    char line[1000];
    debug_s(repo_folder_path);
    printf("content:\n");
    while (fgets(line, sizeof line, sub_commit_file))
    {
        printf("%s", line);
    }
    printf("end\n");

    // calculate the hash
    char *sha = sha_hash(repo_folder_path),
         *repo_file_path = get_file_repo_path();

    char *sha_file = string_format("%s.txt", sha);
    char *sha_file_path = string_format("%s/%s", repo_file_path, sha_file);

    // rename the sub name to it's hash
    char *command = string_format("mv %s %s", repo_folder_path, sha_file_path);
    debug_s(command);
    system(command);

    free(command);
    free(repo_folder_path);
    free(sha);
    free(repo_file_path);
    free(sha_file_path);
    fclose(sub_commit_file);

    return sha_file;
}

char *get_author_name()
{
    char *proj_path = get_proj_path();
    char *command = string_format("%s/.zrb/author.txt", proj_path);
    FILE *conf = fopen(command, "r+");
    if (conf == NULL)
    {
        perror("[ERROR] open conf file");
    }

    char author_name[1000];
    fscanf(conf, "%*s = %s", author_name);
    return strdup(author_name);
}

char *get_author_email()
{
    char *proj_path = get_proj_path();
    char *command = string_format("%s/.zrb/author.txt", proj_path);
    FILE *conf = fopen(command, "r+");
    if (conf == NULL)
    {
        perror("[ERROR] open conf file");
    }

    // skip first line
    fgets(NULL, 0, conf);

    char author_email[1000];
    fscanf(conf, "%*s = %s", author_email);
    return strdup(author_email);
}

int set_conf_of_commit(FILE *commit)
{
    /*
        1. commit
        2. user.name
        3. user.email
        4. time
        5. hash of last commit
    */

    // author info
    char *username = get_author_name(),
         *user_email = get_author_email();

    // time
    struct tm *timeinfo;
    time_t curr;
    time(&curr);
    timeinfo = localtime(&curr);

    // write to the file
    /*1*/ fputs("commit\n", commit);
    /*2*/ fputs(username, commit);
    /*3*/ fputs(user_email, commit);
    /*4*/ fputs(asctime(timeinfo), commit);
    return 0;
}

int commit(int argc, char **argv)
{
    /*
     file:
        file
        content
    */

    /*
    folder:
        folder
        content
    */

    /*
    commit:
        commit
        message
        content
    */

    // is it a valid commit command?
    if (argc != 3)
    {
        Invalid_Command();
        return -1;
    }

    if (strlen(argv[2]) > 72)
    {
        printf("[ERROR] message length is out of bound!\n");
        return -1;
    }

    // erfan bohloul erfan bohloul erfan bohloul erfan bohloul
    // it is valid
    char *proj_path = get_proj_path();
    char *stage_path = get_stage_path();

    DIR *folder = opendir(stage_path);

    if (folder == NULL)
    {
        perror("[ERROR] opendir the .zrb/stage folder");
        return -1;
    }

    char *files_repo_path = string_format("%s/.zrb/repo/.", proj_path);

    char *hash_commit = commit_in_depth(folder, stage_path, files_repo_path);
    // hash_commit -> hash.txt
    if (hash_commit == NULL)
    {
        free(hash_commit);
        free(files_repo_path);
        return -1;
    }

    char *commit_path = string_format("%s/%s", files_repo_path, hash_commit);
    debug_s(commit_path);
    FILE *commit_file = fopen(commit_path, "r+");
    if (commit_file == NULL)
    {
        perror("[ERROR] can not find the commit file in commiting process.");
        return -1;
    }

    // enter "commit" keyword instead of "folder"
    // using a very weird way
    char *tmp_path = string_format("%s/tmp.txt", files_repo_path);
    FILE *tmp = fopen(tmp_path, "w+");
    fputs("commit\n", tmp);

    // get message of commit
    char *message = string_format("%s\n", argv[2]);
    fputs(message, tmp);
    free(message);

    // skip the first line
    char line[10000];
    fgets(line, sizeof line, commit_file);
    copy_file(commit_file, tmp);

    fclose(tmp);

    // delete commit-file
    char *command = string_format("rm %s", commit_path);
    debug_s(command);
    system(command);
    free(command);

    // get hash of tmp file
    char *hash = sha_hash(tmp_path);
    char *hash_file_path = string_format("%s/%s.txt", files_repo_path, hash);

    // rename tmp file to hash name
    command = string_format("mv %s %s", tmp_path, commit_path);
    debug_s(command);
    system(command);

    free(command);
    free(tmp_path);
    fclose(commit_file);
    free(stage_path);
    free(files_repo_path);
    free(commit_path);
    free(hash_commit);
    free(proj_path);
    return 0;
}

char *string_format(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    int size = vsnprintf(NULL, 0, format, args) + 1;
    char *out = malloc(size * sizeof(char));
    vsnprintf(out, size, format, args);

    va_end(args);
    return out;
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

int add_file_to_stage(char *absolute_path)
{
    char *proj_path = get_proj_path(),
         *repo_path = get_repo_path();
    char *path = absolute_path + strlen(proj_path) + 1;
    char command[1000];

    if (strnstr(path, ".zrb", 5))
        return 0;

    char word[1000];
    int i = 0, idx = 0;
    for (; 1; i++, idx++)
    {
        if (path[i] == '/')
        {
            word[idx] = '\0';
            sprintf(command, "%s/stage/%s", repo_path, word);
            debug_s(command);
            mkdir(command, 0777);
            word[idx] = '/';
            continue;
        }

        word[idx] = path[i];

        if (path[i] == '\0')
            break;
    }

    sprintf(command, "cp %s %s/stage/%s", absolute_path, repo_path, path);
    debug_s(command);
    int status = system(command);
    if (status == -1)
    {
        perror("[ERROR] Failed to execute the command.");
        return -1;
    }

    return 0;
}

int from_helper_to_stage()
{
    char *repo_path = get_repo_path();
    char path[1000];

    sprintf(path, "%s/helper.txt", repo_path);
    FILE *helper = fopen(path, "r");

    sprintf(path, "%s/log.txt", repo_path);
    FILE *log = fopen(path, "r");

    if (helper == NULL)
    {
        perror("[ERROR] can not open helper file");
        return -1;
    }

    if (log == NULL)
    {
        perror("[ERROR] can not open log file");
        return -1;
    }

    int all = 0, succ = 0;
    char str[1000], command[1000], *tmp;
    while (fgets(str, sizeof str, helper))
    {
        all++;
        if (str[strlen(str) - 1] == '\n')
            str[strlen(str) - 1] = '\0';

        add_file_to_stage(str);

        succ++;
    }

    fclose(helper);
    fclose(log);
    printf("%d item successfully added to stage from %d item\n", succ, all);
    return 0;
}

int show_staged_files(int depth, int top)
{
    // depth start form 1 and increase by one every time this func call itself
    // until depth reach the top

    if (depth > top)
        return 0;

    DIR *dir;
    struct dirent *entry;

    // open the current dir:
    dir = opendir(".");
    if (dir == NULL)
    {
        perror("unable to open cur dir");
        return -1;
    }

    // loop throw all entries in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // skip . and .. and .zrb file
        if (
            !strcmp(entry->d_name, ".") ||
            !strcmp(entry->d_name, "..") ||
            !strcmp(entry->d_name, ".zrb"))
            continue;

        // for (int i = 1; i < depth; i++)
        //     printf("\t");

        // check if the entry is a dir
        if (entry->d_type == DT_DIR)
        {
            // printf("")
            // print sub-dir:
            printf("%s/\n", entry->d_name);
        }
    }
    return 0;
}

int add(int argc, char **argv)
{
    // zrb add -n depth
    if (!strcmp(argv[1], "-n"))
    {
    }

    // zrb add < -f >
    int idx = 1;
    if (!strcmp(argv[1], "-f"))
    {
        idx++;
    }

    char *repo_path = get_repo_path();
    char command[1000];
    for (int i = idx; i < argc; i++)
    {
        sprintf(command, "find %s -type f -exec realpath {} + > %s/helper.txt", argv[i], repo_path);
        int status = system(command);

        if (status == -1)
        {
            perror("[ERROR] system find add");
        }

        from_helper_to_stage();
    }

    return 0;
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
    fputs("commit\nroot\n", root_commit);

    fclose(root_commit);
    printf("[SUCC] folder of file has created\n");

    // create stage folder:
    if (mkdir("./.zrb/stage", 0777) == -1)
    {
        perror("[ERROR] mkdir ./.zrb/stage");
        return -1;
    }

    printf("[SUCC] the stage has created\n");

    // create a helper txt file:
    FILE *helper = fopen("./.zrb/helper.txt", "w");
    if (!helper)
    {
        perror("[ERORR] fopen helper.txt");
        return -1;
    }

    fclose(helper);
    printf("[SUCC] helper.txt has created\n");

    // create a log txt file:
    FILE *log = fopen("./.zrb/log.txt", "w");
    if (!log)
    {
        perror("[ERORR] fopen log.txt");
        return -1;
    }

    fclose(log);
    printf("[SUCC] log.txt has created\n");

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
    char *proj = get_proj_path();
    strcat(proj, "/.zrb");
    return proj;
}

char *get_proj_path()
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
                return path;
        }

        path = get_parent_folder(path);
    }

    return NULL;
}

char *get_stage_path()
{
    char *repo_path = get_repo_path();
    char *res = string_format("%s/stage", repo_path);
    free(repo_path);
    return res;
}

char *get_file_repo_path()
{
    char *proj_path = get_proj_path();
    char *res = string_format("%s/.zrb/repo", proj_path);
    free(proj_path);
    return res;
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
