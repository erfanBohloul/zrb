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
char *get_branches_path();
void clear_stage();
int is_stage_empty();

// errors:
void Undefined_Behaviour();
void Invalid_Command();
void Not_repo();

// config
int change_config(int argc, char **argv);

// commit
int pre_commit(int argc, char **argv);
int commit(char *message);
char *get_last_hash_commit_from_this_commit(FILE *commit);
char *get_author_name_from_this_commit(FILE *commit);
char *get_author_email_from_this_commit(FILE *commit);
char *get_time_from_this_commit(FILE *commit);
char *get_message_from_this_commit(FILE *commit);
char *get_latest_commit();
char *get_hash_of_file_from_hash_file(FILE *file, char *name);
int get_type_of_hash_file(FILE *hash_file);
void reach_the_content_of_hash_file(FILE *hash_file);

// set
int set(int argc, char **argv);
char *get_commit_shortcut_val(char *shortcut);
int shortcut_commit(int argc, char **argv);
int replace_commit_shortcut(int argc, char **argv);
int remove_commit_shortcut(int argc, char **argv);

// branch
int branch(int argc, char **argv);
int create_branch(char *new_branch_name);
char *get_head_commit_this_branch(char *branch_name);
char *get_curr_branch();
FILE *get_branches_file(char *mode);
int change_head_of_curr_branch(char *new_head_hash);
int change_head_of_branch(char *branch_name, char *new_head_hash);
int change_curr_branch(char *new_curr_branch_name);
void flush_branches();

// checkout
int checkout(int argc, char **argv);
int checkout_in_depth(char *dir_path, char *commit_hash);

// HEAD
char *get_HEAD_hash();
char *get_HEAD_path();
FILE *get_HEAD_file(char *mod);

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

    if (command_func != &create_repo)
    {
        char *repo = get_repo_path();
        if (get_repo_path() == NULL)
        {
            printf("[ERROR] not a zrb repo.\n");
            return -1;
        }
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
            return &pre_commit;
        else if (!strcmp(argv[1], "-s"))
            return &shortcut_commit;

        Invalid_Command();
        return NULL;
    }

    if (!strcmp(command, "set"))
    {
        return &set;
    }

    if (!strcmp(command, "replace"))
    {
        return &replace_commit_shortcut;
    }

    if (!strcmp(command, "remove"))
    {
        return &remove_commit_shortcut;
    }

    if (!strcmp(command, "branch"))
    {
        return &branch;
    }

    if (!strcmp(command, "checkout"))
    {
        return &checkout;
    }

    // else:
    Invalid_Command();

    return NULL;
}

char *get_HEAD_path()
{
    char *proj_path = get_proj_path();
    char *res = string_format("%s/.zrb/HEAD.txt", proj_path);

    free(proj_path);
    return res;
}

FILE *get_HEAD_file(char *mod)
{
    char *HEAD_path = get_HEAD_path();
    FILE *f = fopen(HEAD_path, mod);
    if (f == NULL)
    {
        perror("[ERROR] can not find the HEAD file");
    }

    free(HEAD_path);
    return f;
}

char *get_HEAD_hash()
{
    FILE *HEAD = get_HEAD_file("r");
    if (HEAD == NULL)
        return NULL;
    char line[1000];
    fgets(line, sizeof line, HEAD);
    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';

    return strdup(line);
}

int checkout_in_depth(char *dir_path, char *commit_hash)
{
    char *stage_path = get_stage_path(),
         *file_repo_path = get_file_repo_path(),
         *command = NULL;
    // get the file of commit
    char *commit_path = string_format("%s/%s.txt", file_repo_path, commit_hash);
    debug_s(commit_path);
    FILE *commit = fopen(commit_path, "r");
    if (commit == NULL)
    {
        perror("[ERROR] can not find the commited file in checkout_into_stage");
        return -1;
    }

    // checkouting
    reach_the_content_of_hash_file(commit);

    char line[100000], sub_hash[10000], sub_name[1000];
    struct dirent *d;
    DIR *dir;
    while (fgets(line, sizeof line, commit))
    {
        sscanf(line, "%s %s", sub_hash, sub_name);
        // debug_s(sub_name);
        if (sub_name[strlen(sub_name) - 1] == '\n')
            sub_name[strlen(sub_name) - 1] = '\0';

        // search the directory
        dir = opendir(dir_path);
        if (dir == NULL)
        {
            perror("[ERROR] open the directory failed in checkout_into_stage.");
            return -1;
        }

        int flag = 0;
        while ((d = readdir(dir)) != NULL)
        {
            if (!strcmp(d->d_name, sub_name))
            {
                flag = 1;
                break;
            }
        }

        // get the file of sub_file
        command = string_format("%s/%s.txt", file_repo_path, sub_hash);
        printf("command-str: %s\n", command);
        FILE *sub_file = fopen(command, "r");
        if (sub_file == NULL)
        {
            perror("[ERROR] can not find the commited sub_file in checkout_into_stage");
            return -1;
        }
        int type = get_type_of_hash_file(sub_file);
        // debug(type);
        // debug(flag);

        if (type == 1)
        {
            // directory
            char *sub_dir_path = string_format("%s/%s", dir_path, sub_name);

            if (!flag)
            {
                mkdir(sub_dir_path, 0777);
            }
            // got deep
            checkout_in_depth(sub_dir_path, sub_hash);

            free(sub_dir_path);
        }

        else if (type == 2)
        {
            // file

            // open the file
            command = string_format("%s/%s", dir_path, sub_name);
            debug_s(command);
            FILE *tmp_file = fopen(command, "w");
            reach_the_content_of_hash_file(sub_file);
            char t[1000];
            fgets(t, sizeof t, sub_file);
            debug_s(t);
            reach_the_content_of_hash_file(sub_file);
            copy_file(sub_file, tmp_file);

            fclose(tmp_file);
            free(command);
        }

        else if (type == 0 || type == -1)
        {
            Undefined_Behaviour();
            return -1;
        }

        fclose(sub_file);
        // free(command);
    }

    fclose(commit);
    free(commit_path);
    free(stage_path);
    free(file_repo_path);
    // free(command);
    return 0;
}

char *get_pre_commit(char *hash, int num_back)
{
    if (num_back <= 0)
        return hash;

    if (!strcmp(hash, "root"))
        return hash;

    char *file_repo_path = get_file_repo_path(),
         *commit_path = string_format("%s/%s.txt", file_repo_path, hash);

    FILE *commit = fopen(commit_path, "r");
    if (commit == NULL)
    {
        perror("[ERROR] can not open commit hash file in get_pre_commit func");
        return NULL;
    }

    char *pre_hash = get_last_hash_commit_from_this_commit(commit);
    free(commit_path);
    free(file_repo_path);
    fclose(commit);

    num_back--;
    char *tmp = get_pre_commit(pre_hash, num_back);

    return tmp;
}

int checkout(int argc, char **argv)
{
    if (argc != 2)
    {
        Invalid_Command();
        return -1;
    }

    char *commit_id,
        *file_repo_path = get_file_repo_path();

    if (!strcmp(argv[1], "HEAD"))
    {
        char *curr_branch = get_curr_branch();
        commit_id = get_head_commit_this_branch(curr_branch);

        free(curr_branch);
    }

    else if (strstr(argv[1], "HEAD"))
    {
        int back_num = 0;
        sscanf(argv[1], "HEAD-%d", &back_num);

        char *curr_branch = get_curr_branch(),
             *curr_head_branch = get_head_commit_this_branch(curr_branch);
        char *pre_hash = get_pre_commit(curr_head_branch, back_num);
        commit_id = pre_hash;

        free(curr_branch);
        free(curr_head_branch);
    }

    else
    {
        // assume argv[1] is a branch name:
        commit_id = get_head_commit_this_branch(argv[1]);

        if (NULL == commit_id)
        {
            char *command = string_format("%s/%s.txt", file_repo_path, argv[1]);
            FILE *f = fopen(command, "r");
            if (f == NULL)
            {
                printf("[ERROR] %s does not exist.\n", command);
                return -1;
            }
            fclose(f);
            free(command);

            commit_id = argv[1];
        }

        else
        {
            change_curr_branch(argv[1]);
        }
    }

    char *proj_path = get_proj_path();
    int res = checkout_in_depth(proj_path, commit_id);

    FILE *HEAD = get_HEAD_file("w");
    fprintf(HEAD, "%s", commit_id);
    fclose(HEAD);

    free(proj_path);
    free(file_repo_path);
    return res;
}

int change_curr_branch(char *new_curr_branch_name)
{
    char *old_curr_branch_name = get_curr_branch();
    if (!strcmp(old_curr_branch_name, new_curr_branch_name))
    {
        printf("Already on '%s'\n", old_curr_branch_name);
        return 0;
    }

    FILE *branches = get_branches_file("r");
    char line[10000], name[1000], head[10000], state[1000], tmp[100000] = "";
    while (fgets(line, sizeof line, branches))
    {
        sscanf(line, "%s %s %s", name, head, state);
        if (state[strlen(state) - 1] == '\n')
            state[strlen(state) - 1] = '\0';

        if (!strcmp(name, new_curr_branch_name))
        {
            // found the branch we want to switch to
            char *command = string_format("%s %s curr\n", name, head);
            strcat(tmp, command);
            free(command);
        }

        else if (!strcmp(state, "curr"))
        {
            char *command = string_format("%s %s\n", name, head);
            strcat(tmp, command);
            free(command);
        }

        else
        {
            strcat(tmp, line);
        }
    }

    fclose(branches);
    branches = get_branches_file("w");
    fprintf(branches, "%s", tmp);

    printf("[SUCC] the curr branch has changed\n");
    return 0;
}

void flush_branches()
{
    char *branches_path = get_branches_path();
    FILE *branches = fopen(branches_path, "r");

    char line[10000];
    while (fgets(line, sizeof line, branches))
    {
        printf("%s", line);
    }

    fclose(branches);
    free(branches_path);
}

int branch(int argc, char **argv)
{
    if (argc == 2)
    {
        char *branch_name = argv[1];
        return create_branch(branch_name);
    }

    else if (argc == 1)
    {
        flush_branches();
        return 0;
    }

    Invalid_Command();
    return -1;
}

int create_branch(char *new_branch_name)
{
    // is it a valid name?
    if (new_branch_name == NULL || strlen(new_branch_name) == 0)
    {
        printf("[ERROR] Please provide a valid name for the new branch.\n");
        return -1;
    }

    if (strstr(new_branch_name, " "))
    {
        printf("[ERROR]  Branch names cannot contain spaces. Try using hyphens or underscores instead.\n");
        return -1;
    }

    // check if this name already exists
    char *head_commit = get_head_commit_this_branch(new_branch_name);
    if (head_commit != NULL)
    {
        free(head_commit);
        printf("[ERROR] The branch %s already exists", new_branch_name);
        return -1;
    }

    char *latest_commit = get_latest_commit();

    // make new branch with that point to latest commit
    FILE *branches = get_branches_file("a");
    if (branches == NULL)
        return -1;

    char line[10000];
    sprintf(line, "%s %s", new_branch_name, latest_commit);
    fputs(line, branches);

    free(head_commit);
    free(latest_commit);
    fclose(branches);

    printf("[SUCC] the new branch has added\n");
    return 0;
}

char *get_latest_commit()
{
    char *curr_branch = get_curr_branch();
    char *lastest = get_head_commit_this_branch(curr_branch);

    free(curr_branch);
    return lastest;
}

char *get_curr_branch()
{
    char *branch_path = get_branches_path();
    FILE *branches = fopen(branch_path, "r");
    if (branches == NULL)
    {
        perror("[ERROR] can not open branches file");
        return NULL;
    }
    free(branch_path);

    char line[10000], name[100], commit_hash[1000], state[100];
    while (fgets(line, sizeof line, branches))
    {
        sscanf(line, "%s %s %s", name, commit_hash, state);
        if (!strcmp(state, "curr"))
        {
            fclose(branches);
            return strdup(name);
        }
    }

    fclose(branches);
    printf("[ERROR] can not find the curr branch... why?\n");
    return NULL;
}

char *get_head_commit_this_branch(char *branch_name)
{
    char *branch_path = get_branches_path();
    FILE *branches = fopen(branch_path, "r");
    if (branches == NULL)
    {
        perror("[ERROR] can not open branches file");
        return NULL;
    }
    free(branch_path);

    char line[10000], name[100], commit_hash[1000], state[100];
    while (fgets(line, sizeof line, branches))
    {
        sscanf(line, "%s %s %s", name, commit_hash, state);
        if (!strcmp(name, branch_name))
        {
            fclose(branches);
            return strdup(commit_hash);
        }
    }

    fclose(branches);
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

FILE *get_branches_file(char *mode)
{
    char *branches_path = get_branches_path();
    FILE *branches = fopen(branches_path, mode);
    if (branches == NULL)
    {
        perror("opening branches file");
    }

    free(branches_path);
    return branches;
}

int remove_commit_shortcut(int argc, char **argv)
{
    if (argc != 3)
    {
        Invalid_Command();
        return -1;
    }

    if (strcmp(argv[1], "-s"))
    {
        printf("[ERROR] there is no option named  \"%s\"\n", argv[1]);
        return -1;
    }

    char *key = argv[2],
         *val = get_commit_shortcut_val(key);

    if (val == NULL)
    {
        printf("[ERROR] No such shortcut exists.\n");
        return -1;
    }
    free(val);

    FILE *alias = fopen(alias_path, "r");
    char line[10000], buff[10000] = "", key_[100];
    while (fgets(line, sizeof line, alias))
    {
        sscanf(line, "%s", key_);
        if (!strcmp(key_, "m"))
        {

            sscanf(line + 2, "%s", key_);
            if (!strcmp(key_, key))
                continue;
        }

        strcat(buff, line);
    }

    fclose(alias);
    alias = fopen(alias_path, "w");
    fprintf(alias, "%s", buff);
    fclose(alias);

    printf("[SUCC] The commit shortcut has been removed successfully.\n");
    return 0;
}

int replace_commit_shortcut(int argc, char **argv)
{
    if (argc != 5)
    {
        Invalid_Command();
        return -1;
    }

    char *val = argv[2],
         *key = argv[4];

    // is message valid?
    if (strlen(val) == 0)
    {
        printf("[ERROR] invalid value for alias. It should not be empty.\n");
        return -1;
    }

    if (strlen(val) > 72)
    {
        printf("[ERROR] the length of your commit message should not exceed 72 characters\n");
        return -1;
    }

    char *tmp = get_commit_shortcut_val(key);
    if (tmp == NULL)
    {
        printf("[ERROR] such key doesn't exists.\n");
        return -1;
    }
    free(tmp);

    // go throw alias and then find the line that key exists and then change the value:
    char line[10000], key_[100], j[10000] = "";
    FILE *alias = fopen(alias_path, "r");
    if (alias == NULL)
    {
        perror("[ERROR] can not open alias file");
        return -1;
    }

    for (; fgets(line, sizeof line, alias); strcat(j, line))
    {
        sscanf(line, "%s", key_);
        if (strcmp(key_, "m"))
            continue;

        sscanf(line + 2, "%s", key_);
        if (strcmp(key_, key))
            continue;

        sprintf(line, "m %s %s\n", key, val);
    }

    fclose(alias);
    alias = fopen(alias_path, "w");
    fprintf(alias, "%s", j);

    fclose(alias);

    printf("[SUCC] the replacement was successful.\n");
    return 0;
}

int shortcut_commit(int argc, char **argv)
{
    if (argc != 3)
    {
        Invalid_Command();
        return -1;
    }

    char *val = get_commit_shortcut_val(argv[2]);
    if (val == NULL)
    {
        printf("[ERROR] such key doesn't exists!\n");
        return -1;
    }

    int res = commit(val);

    free(val);
    return res;
}

char *get_commit_shortcut_val(char *shortcut)
{
    char line[10000];
    FILE *alias = fopen(alias_path, "r+");

    char key[100], *val;
    while (fgets(line, sizeof line, alias))
    {
        sscanf(line, "%s", key);

        if (!strcmp(key, "g"))
            continue;

        // get key and val from the line
        sscanf(line + 2, "%s", key);
        val = line + 2 + strlen(key) + 1;

        // check if key is the shortcut
        if (!strcmp(key, shortcut))
        {
            printf("key:%s, val:%s\n", key, val);
            return strdup(val);
        }
    }

    return NULL;
}

int set(int argc, char **argv)
{
    // check if command is valid:
    if (argc != 5)
    {
        Invalid_Command();
        return -1;
    }

    if (strcmp(argv[1], "-m") || strcmp(argv[3], "-s"))
    {
        Invalid_Command();
        return -1;
    }

    char *val = get_commit_shortcut_val(argv[4]);
    if (val)
    {
        printf("[ERROR] this shortcut already exists");
        return -1;
    }
    free(val);

    // create a new shortcut:
    FILE *alias = fopen(alias_path, "a");
    val = argv[2];
    char *key = argv[4];

    // is message valid?
    if (strlen(val) == 0)
    {
        printf("[ERROR] invalid value for alias. It should not be empty.\n");
        return -1;
    }

    if (strlen(val) > 72)
    {
        printf("[ERROR] the length of your commit message should not exceed 72 characters\n");
        return -1;
    }

    char line[10000];
    sprintf(line, "m %s %s", key, val);
    fputs(line, alias);
    fclose(alias);

    // succ message:
    printf("[SUCC] the new commit shortcut has added\n");
    return 0;
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

int add_file_to_repo_files(char *file_name, char *repo_path, FILE *commit_file)
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
    system(command);
    free(command);

    // add this hash to parent-commit folder as an hash
    command = string_format("%s %s\n", sha, file_name);
    // printf("saved in commit: %s\n", command);
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
    debug_s(repo_folder_path);
    // create a file with a temprary name
    FILE *sub_commit_file = fopen(repo_folder_path, "w");
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
        debug_s(sub_repo_path);
        debug_s(sub_stage_path);

        if (entry->d_type == DT_DIR)
        {
            char *sub_hash = commit_in_depth(opendir(sub_stage_path), sub_stage_path, sub_repo_path);
            if (sub_hash == NULL)
            {
                return NULL;
            }

            char *commandu = string_format("%s %s\n", sub_hash, entry->d_name);
            fputs(commandu, sub_commit_file);

            free(commandu);
            free(sub_hash);
        }

        else
        {
            // its a file
            FILE *file = fopen(sub_repo_path, "w");
            if (file == NULL)
            {
                perror("[ERROR] can not create a duplicate in file-repo");
                continue;
            }

            FILE *origin_file = fopen(sub_stage_path, "r");
            if (origin_file == NULL)
            {
                perror("[ERROR] can not open the origin file from stage area");
                continue;
            }

            FILE *gh = fopen(sub_stage_path, "r");
            char content[10000];
            while (fgets(content, sizeof content, gh))
            {
                debug_s(content);
            }

            // add "file" key word to start of the duplicate file in file-repo folder
            // fputs("file\n", file);
            fprintf(file, "file\n");

            // copy content of origin file in stage area into duplicate file in file-repo folder
            copy_file(origin_file, file);
            fclose(file);
            fclose(origin_file);
            // this function create a hash of dup file and then rename it with hash name
            int res = add_file_to_repo_files(entry->d_name, sub_repo_path, sub_commit_file);
            if (res == -1)
            {
                perror("[ERROR] can not commit file");
                continue;
            }

            free(sub_repo_path);
            free(sub_stage_path);
        }
    }

    // reopen sub_commit_file
    fclose(sub_commit_file);
    sub_commit_file = fopen(repo_folder_path, "r+");

    // calculate the hash
    char *sha = sha_hash(repo_folder_path),
         *repo_file_path = get_file_repo_path();

    char *sha_file = string_format("%s.txt", sha);
    char *sha_file_path = string_format("%s/%s", repo_file_path, sha_file);

    // rename the sub name to it's hash
    char *command = string_format("mv %s %s", repo_folder_path, sha_file_path);
    system(command);

    free(command);
    free(repo_folder_path);
    free(sha_file);
    free(repo_file_path);
    free(sha_file_path);
    fclose(sub_commit_file);

    return sha;
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
    char line[10000];
    fgets(line, 10000, conf);

    char author_email[1000];
    fscanf(conf, "%*s = %s", author_email);
    return strdup(author_email);
}

int set_conf_of_commit(FILE *commit, char *message)
{
    /*
        1. commit
        2. hash of last commit
        3. user.name
        4. user.email
        5. time
        6. message
        7. branch
    */

    // author info
    char *username = get_author_name(),
         *user_email = get_author_email();

    strcat(username, "\n");
    strcat(user_email, "\n");
    char *mess = string_format("%s\n", message);

    // time
    struct tm *timeinfo;
    time_t curr;
    time(&curr);
    timeinfo = localtime(&curr);

    char *tmp;
    // branch
    char *curr_branch = get_curr_branch();

    // last commit
    tmp = get_head_commit_this_branch(curr_branch);
    char *head = string_format("%s\n", tmp);

    free(tmp);
    tmp = curr_branch;
    curr_branch = string_format("%s\n", tmp);
    free(tmp);

    // write to the file
    /*1*/ fputs("commit\n", commit);
    /*2*/ fputs(head, commit);
    /*3*/ fputs(username, commit);
    /*4*/ fputs(user_email, commit);
    /*5*/ fputs(asctime(timeinfo), commit);
    /*6*/ fputs(mess, commit);
    /*7*/ fputs(curr_branch, commit);

    free(username);
    free(user_email);
    free(mess);
    free(curr_branch);
    free(head);
    return 0;
}

char *get_last_hash_commit_from_this_commit(FILE *commit)
{
    rewind(commit);

    char line[10000];
    // travel to line 2
    for (int i = 1; i <= 2; i++)
    {
        fgets(line, sizeof line, commit);
    }

    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';

    return strdup(line);
}

char *get_author_name_from_this_commit(FILE *commit)
{
    rewind(commit);

    char line[10000];

    // travel to line 3
    for (int i = 1; i <= 3; i++)
    {
        fgets(line, sizeof line, commit);
    }

    return strdup(line);
}

char *get_author_email_from_this_commit(FILE *commit)
{
    rewind(commit);

    char line[10000];

    // travel to line 4
    for (int i = 1; i <= 4; i++)
    {
        fgets(line, sizeof line, commit);
    }

    return strdup(line);
}

char *get_time_from_this_commit(FILE *commit)
{
    rewind(commit);

    char line[10000];

    // travel to line 5
    for (int i = 1; i <= 5; i++)
    {
        fgets(line, sizeof line, commit);
    }

    return strdup(line);
}

char *get_message_from_this_commit(FILE *commit)
{
    rewind(commit);

    char line[10000];

    // travel to line 6
    for (int i = 1; i <= 6; i++)
    {
        fgets(line, sizeof line, commit);
    }

    return strdup(line);
}

char *get_branch_from_this_commit(FILE *commit)
{
    rewind(commit);

    char line[10000];

    // travel to line 7
    for (int i = 1; i <= 7; i++)
    {
        fgets(line, sizeof line, commit);
    }

    return strdup(line);
}

void flush_commit_info(char *commit_path, char *hash)
{
    FILE *file = fopen(commit_path, "r+");
    if (!file)
    {
        perror("[ERROR] can not open commit file to flush the info");
        return;
    }

    char *message = get_message_from_this_commit(file),
         *author_name = get_author_name_from_this_commit(file),
         *author_email = get_author_email_from_this_commit(file),
         *time = get_time_from_this_commit(file),
         *id = string_format("%s\n", hash),
         *last_commit = get_last_hash_commit_from_this_commit(file);
    printf("id-> %stime-> %slast_commit-> %sauthor: %sauthor-email: %smessage: %s", id, time, last_commit, author_name, author_email, message);

    free(message);
    free(author_email);
    free(author_name);
    free(time);
    free(id);
    return;
}

int pre_commit(int argc, char **argv)
{
    if (argc != 3)
    {
        Invalid_Command();
        return -1;
    }

    return commit(argv[2]);
}

void clear_stage()
{
    char *stage_path = get_stage_path();
    char *command = string_format("rm -r %s/*", stage_path);
    system(command);
    free(command);
    free(stage_path);
}

int is_stage_empty()
{
    char *stage_path = get_stage_path();
    struct dirent *entry;
    DIR *dir = opendir(stage_path);
    int flag = 1;
    while ((entry = readdir(dir)) != NULL)
    {
        /* skip . and .. */
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        flag = 0;
    }
    closedir(dir);
    free(stage_path);
    return flag;
}

char *get_hash_of_file_from_hash_file(FILE *file, char *name)
{
    rewind(file);
    char line[10000], hash[10000], name_[10000];
    while (fgets(line, sizeof line, file))
    {
        sscanf(line, "%s %s\n", hash, name_);
        if (!strcmp(name_, name))
            return strdup(hash);
    }

    return NULL;
}

int get_type_of_hash_file(FILE *hash_file)
{
    rewind(hash_file);
    char line[10000];
    fgets(line, sizeof line, hash_file);

    /*
        0 -> commit
        1 -> folder
        2 -> file
        -1 -> undef
    */
    int res = -1;
    if (!strcmp(line, "commit\n"))
        res = 0;
    else if (!strcmp(line, "folder\n"))
        res = 1;
    else if (!strcmp(line, "file\n"))
        res = 2;
    else
    {
        printf("[ERROR] can not detect the type of file\n");
    }

    rewind(hash_file);
    return res;
}

void reach_the_content_of_hash_file(FILE *hash_file)
{
    rewind(hash_file);
    int type = get_type_of_hash_file(hash_file);

    int i = 0;
    if (type == 0)
        i = 7;
    else if (type == 1)
        i = 1;
    else if (type == 2)
        i = 1;

    char line[10000];
    for (int j = 0; j < i; j++)
        fgets(line, sizeof line, hash_file);
}

int checkout_into_stage(char *dir_path, char *commit_hash)
{
    char *stage_path = get_stage_path(),
         *file_repo_path = get_file_repo_path(),
         *command = NULL;
    // get the file of commit
    char *commit_path = string_format("%s/%s.txt", file_repo_path, commit_hash);
    debug_s(commit_path);
    FILE *commit = fopen(commit_path, "r");
    if (commit == NULL)
    {
        perror("[ERROR] can not find the commited file in checkout_into_stage");
        return -1;
    }

    // checkouting
    reach_the_content_of_hash_file(commit);

    char line[100000], sub_hash[10000], sub_name[1000];
    struct dirent *d;
    DIR *dir;
    while (fgets(line, sizeof line, commit))
    {
        sscanf(line, "%s %s", sub_hash, sub_name);
        debug_s(sub_name);
        if (sub_name[strlen(sub_name) - 1] == '\n')
            sub_name[strlen(sub_name) - 1] = '\0';

        // search the directory
        dir = opendir(dir_path);
        if (dir == NULL)
        {
            perror("[ERROR] open the directory failed in checkout_into_stage.");
            return -1;
        }

        int flag = 0;
        while ((d = readdir(dir)) != NULL)
        {
            if (!strcmp(d->d_name, sub_name))
            {
                flag = 1;
                break;
            }
        }

        // get the file of sub_file
        command = string_format("%s/%s.txt", file_repo_path, sub_hash);
        FILE *sub_file = fopen(command, "r");
        if (sub_file == NULL)
        {
            perror("[ERROR] can not find the commited sub_file in checkout_into_stage");
            return -1;
        }
        int type = get_type_of_hash_file(sub_file);
        // debug(type);
        // debug(flag);

        if (type == 1)
        {
            // directory
            char *sub_dir_path = string_format("%s/%s", dir_path, sub_name);

            if (!flag)
            {
                mkdir(sub_dir_path, 0777);
            }
            // got deep
            checkout_into_stage(sub_dir_path, sub_hash);

            free(sub_dir_path);
        }

        else if (type == 2 && !flag)
        {
            // file

            // create the file
            command = string_format("%s/%s", dir_path, sub_name);
            FILE *tmp_file = fopen(command, "w");
            reach_the_content_of_hash_file(sub_file);
            copy_file(sub_file, tmp_file);

            fclose(tmp_file);
            free(command);
        }

        else if (type == 0 || type == -1)
        {
            Undefined_Behaviour();
            return -1;
        }

        fclose(sub_file);
        // free(command);
    }

    fclose(commit);
    free(commit_path);
    free(stage_path);
    free(file_repo_path);
    // free(command);
    printf("[SUCC] completing stage\n");
    return 0;
}

int commit(char *message)
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
        hash of last commit
        user.name
        user.email
        time
        message
        branch
        content
    */

    char *HEAD_hash = get_HEAD_hash(),
         *curr_branch = get_curr_branch(),
         *curr_branch_head = get_head_commit_this_branch(curr_branch);
    if (strcmp(HEAD_hash, curr_branch_head))
    {
        printf("[ERROR] can not commit not on elsewhere except on HEAD.\n");
        return -1;
    }

    // is message valid?
    if (strlen(message) > 72)
    {
        printf("[ERROR] message length is out of bound!\n");
        return -1;
    }

    if (strlen(message) == 0)
    {
        printf("[ERROR] you can't commit without message\n");
        return -1;
    }

    char *proj_path = get_proj_path();
    char *stage_path = get_stage_path();

    // completing stage area
    char *latest_commit_hash = get_latest_commit();
    char *path_to_commit = string_format("%s/.zrb/repo/%s.txt", proj_path, latest_commit_hash);
    FILE *latest_commit = fopen(path_to_commit, "r");

    if (is_stage_empty())
    {
        printf("[ERROR] there is noting in stage area.\n");
        return -2;
    }

    checkout_into_stage(stage_path, latest_commit_hash);

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

    char *commit_path = string_format("%s/%s.txt", files_repo_path, hash_commit);
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

    set_conf_of_commit(tmp, message);

    // skip the first line
    char line[10000];
    fgets(line, sizeof line, commit_file);
    copy_file(commit_file, tmp);

    fclose(tmp);

    // completing_stage(commit_file);

    // delete commit-file
    char *command = string_format("rm %s", commit_path);
    system(command);
    free(command);

    // get hash of tmp file
    char *hash = sha_hash(tmp_path);
    char *hash_file_path = string_format("%s/%s.txt", files_repo_path, hash);

    // rename tmp file to hash name
    command = string_format("mv %s %s", tmp_path, hash_file_path);
    system(command);

    // removing content of stage area:
    clear_stage();

    // change the head of the curr branch to this one
    change_head_of_curr_branch(hash);

    // update HEAD
    FILE *HEAD = get_HEAD_file("w");
    fprintf(HEAD, "%s", hash);
    fclose(HEAD);

    // print the message of succ
    printf("[SUCC] commited!\n");

    // print the info of this commit out
    flush_commit_info(hash_file_path, hash);

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

int change_head_of_branch(char *branch_name, char *new_head_hash)
{
    char *branches_path = get_branches_path();
    FILE *branches = fopen(branches_path, "r");
    if (branches == NULL)
    {
        perror("[ERROR] can not find the branches file.");
        return -1;
    }

    char line[10000], name[100], head[1000], state[100], tmp[10000] = "";
    while (fgets(line, sizeof line, branches))
    {
        sscanf(line, "%s %s %s", name, head, state);

        if (!strcmp(name, branch_name))
        {
            sprintf(line, "%s %s %s\n", name, new_head_hash, state);
        }

        strcat(tmp, line);
    }

    // reopen branches with write mode
    fclose(branches);
    branches = fopen(branches_path, "w");

    fprintf(branches, "%s", tmp);

    fclose(branches);
    free(branches_path);
    return 0;
}

int change_head_of_curr_branch(char *new_head_hash)
{
    char *branches_path = get_branches_path();
    FILE *branches = fopen(branches_path, "r");
    if (branches == NULL)
    {
        perror("[ERROR] can not find the branches file.");
        return -1;
    }
    free(branches_path);

    char line[10000], name[100], head[1000], state[100];
    while (fgets(line, sizeof line, branches))
    {
        sscanf(line, "%s %s %s", name, head, state);

        if (!strcmp(state, "curr"))
        {
            fclose(branches);
            return change_head_of_branch(name, new_head_hash);
        }
    }

    fclose(branches);
    printf("[ERROR] can not find the curr branch.");
    return -1;
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
    char ch;
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
    if (mkdir("./.zrb/repo", 0777) == -1)
    {
        perror("[ERROR] mkdir .zrb/repo");
        return -1;
    }

    // create a root commit:
    FILE *root_commit = fopen("./.zrb/repo/root.txt", "w");
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

    // create branches file
    FILE *branches = fopen("./.zrb/branches.txt", "w");
    if (!branches)
    {
        perror("[ERROR] fopen branches.txt");
        return -1;
    }

    // adding master (default) branch to branches
    fprintf(branches, "master root curr\n");

    fclose(branches);
    printf("[SUCC] branches.txt has created\n");

    FILE *HEAD = fopen("./.zrb/HEAD.txt", "w");
    if (!HEAD)
    {
        perror("[ERROR] fopen HEAD.txt");
        return -1;
    }
    fprintf(HEAD, "root");
    printf("[SUCC] HEAD.txt has created\n");

    fclose(HEAD);
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

    if (proj == NULL)
        return NULL;

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

char *get_branches_path()
{
    char *repo_path = get_repo_path();
    char *branch_path = string_format("%s/branches.txt", repo_path);

    free(repo_path);
    return branch_path;
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
