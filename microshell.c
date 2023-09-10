#include <stdio.h>
#include <string.h>
#include <stdlib.h>     
#include <fcntl.h>
#include <sys/types.h>  // exec functionality
#include <sys/wait.h>   //
#include <unistd.h>     // cd, exec, type prompt
#include <sys/stat.h>   // lsPermissions functionality
#include <pwd.h>        //
#include <grp.h>        //
#include <dirent.h>     // ls and find functionality
#include <errno.h>      // error printing
#include <regex.h>      // find regex functonality
#define COLOR_RESET "\x1b[0m"
#define INVALID_ARGS_STR "\x1b[31m" "Invalid arguments!\n" COLOR_RESET
#define BUFFER_SIZE 255
#define MAX_COMMAND_ARGS 10
void typePrompt()
{
    char cwd[255];
    char host[255];
    getcwd(cwd, sizeof(cwd));
    gethostname(host, sizeof(host));
    printf("[");
    printf("\x1b[36m" "%s" COLOR_RESET, getenv("USER"));
    printf("@");
    printf("\x1b[91m" "%s" COLOR_RESET, host);
    printf("]");
    printf(":~");
    printf("%s", cwd);
    printf(" $ ");
    return;
}

int takeCommand(char** args)
{
    char command[BUFFER_SIZE];
    int n = 0;

    fgets(command, sizeof(command), stdin);
    int size = sizeof(command);
    command[size-1] = '\0';

    char* ptr = strtok(command, " \n");

    while(ptr != NULL)
    {
        args[n] = malloc(sizeof(ptr));
        strcpy(args[n], ptr);
        n++;
        ptr = strtok(NULL, " \n");
    }
    args[n] = NULL;
    return n;
}

void find(char *dir, char *regexpression)
{
    struct dirent *item;
    DIR *dirp = opendir(dir);

    regex_t regex;
    int errorCode;
    int regexecCode;

    if (regexpression != NULL)
    {
        errorCode = regcomp(&regex, regexpression, REG_ICASE | REG_EXTENDED | REG_NOSUB);

        if (errorCode)
        {
            printf("\x1b[31m" "Error compiling regular expression\n" COLOR_RESET);
            return;
        }
    }

    if (!dirp)
    {
        perror("\x1b[31m" "Could not list found items" COLOR_RESET);
        return;
    }
    while((item = readdir(dirp)) != NULL)
    {
        if (item->d_name[0] == '.')
        {
            continue;
        }
        if (item->d_type == DT_DIR)
        {
            if (regexpression == NULL)
            {
                printf("\x1b[94m" "%s/%s\n" COLOR_RESET, dir ,item->d_name);
            }
            char new_dir[255];
            strcpy(new_dir, dir);
            strcat(new_dir, "/");
            strcat(new_dir, item->d_name);
            find(new_dir, regexpression);
        }
        else
        {
            if (regexpression != NULL)
            {
                regexecCode = regexec(&regex, item->d_name, 0, NULL, 0);
                if (!regexecCode)
                {
                    printf("%s/%s\n", dir ,item->d_name);
                }
            }
            else
            {
                printf("%s/%s\n", dir ,item->d_name);
            }
        }
    }
    closedir(dirp);
    return;
}

void lsPermissions(char *file, char* permsString){
    struct stat st;
    int permTypesInts[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    char permTypesChars[] = {'r', 'w', 'x'};
    if(stat(file, &st) == 0){
        mode_t perm = st.st_mode;
        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);

        for(int i = 0; i < 9; i++)
        {
            if (perm & permTypesInts[i])
            {
                permsString[i] = permTypesChars[i%3];
            }
            else
            {
                permsString[i] = '-';
            }
            
        }
        permsString[9] = ' ';
        permsString[10] = '\0';
        if (pw && gr)
        {
            strcat(permsString, pw->pw_name);
            strcat(permsString, " ");
            strcat(permsString, gr->gr_name);
        }
    }
    else{
        perror("\x1b[31m" "Could not access file permissions" COLOR_RESET);
    }   
}

void ls(char *dir, int arg)
{
    struct dirent *item;
    DIR *dirp = opendir(dir);

    if (!dirp)
    {
        perror("\x1b[31m" "Could not list items" COLOR_RESET);
        return;
    }

    while((item = readdir(dirp)) != NULL)
    {
        if (item->d_name[0] == '.' && arg < 1)
            {
                continue;
            }
        if (arg <= 1)
        {
            if (item->d_type == DT_DIR)
            {
                printf("\x1b[94m" "%s   " COLOR_RESET, item->d_name);
            }
            else
            {
                printf("\x1b[92m" "%s   " COLOR_RESET, item->d_name);
            }
        }
        else
        {
            char *perms = malloc(sizeof(char)*BUFFER_SIZE);
            lsPermissions(item->d_name, perms);
            if (item->d_type == DT_DIR)
            {
                printf("\x1b[94m" "%s\t%s\n" COLOR_RESET,perms, item->d_name);
            }
            else
            {
                printf("\x1b[92m" "%s\t%s\n" COLOR_RESET,perms, item->d_name);
            }
        }
    }
    if (arg <= 1)
    {
        printf("\n");
    }
    return;
}

void clear()
{
    printf("\033[2J\033[1;1H");
    return;
}

void cd(char *dir)
{
    if (chdir(dir) != 0)
    {
        perror("\x1b[31m" "Could not change directory" COLOR_RESET);
    }
}

void cp(char *file1, char *file2)
{
    char c;
    FILE *f1, *f2;

    f1 = fopen(file1, "r");
    if (f1 == NULL)
    {
        perror("\x1b[31m" "Could not open source file" COLOR_RESET);
        return;
    }

    f2 = fopen(file2, "w");
    if (f2 == NULL)
    {
        perror("\x1b[31m" "Could not write to output file" COLOR_RESET);
        return;
    }

    c = fgetc(f1);
    while (c != EOF)
    {
        fputc(c, f2);
        c = fgetc(f1);
    }
    fclose(f1);
    fclose(f2);
    return;
}

void rm(char *file)
{
    if (remove(file) == 0)
    {
        return;
    }
    else
    {
        perror("\x1b[31m" "Could not delete file" COLOR_RESET);
        return;
    }
}

void help()
{
    printf("Own implementations: \n");
    printf("    -exit\n");
    printf("    -help\n");
    printf("    -cp [file] [destination]\n");
    printf("    -ls\n");
    printf("        *ls -h (show hidden)\n");
    printf("        *ls -l (show file permissions) [perms|owner|group|name]\n");
    printf("    -cd [dir]\n");
    printf("    -rm [file]\n");
    printf("    -clear\n");
    printf("    -find\n");   
    printf("        *find [regex] [directory]\n");  
    printf("    *: optional arguments\n");   
    printf("Default shell commands are also supported\n");
    return;
}

void releaseMemory(char **command, int *l)
{
    for (int i = 0; i < *l; i++)
    {
        free(command[i]);
    }
    *l = 0;
}

int handleInternalFunctions(char **command)
{
    int commandHandled = 1;
    if (strcmp(command[0],"cd") == 0)
    {
        if (command[2] == NULL || command[1] == NULL)
        {
            if (command[1] != NULL)
            {
                cd(command[1]);
            }
            else
            {   
                cd(getenv("HOME"));
            }
        }
        else
        {
            printf(INVALID_ARGS_STR);
        }
    }
    else if (strcmp(command[0],"help") == 0)
    {
        if (command[1] == NULL)
        {
            help();
        }
        else
        {
            printf(INVALID_ARGS_STR);
        }  
    }
    else if (strcmp(command[0],"ls") == 0)
    {
        if (command[1] == NULL)
        {
            ls(".", 0);
        }
        else if (strcmp(command[1], "-h") == 0 && command[2] == NULL)
        {
            ls(".", 1);
        }
        else if (strcmp(command[1], "-l") == 0 && command[2] == NULL)
        {
            ls(".", 2);
        }
        else
        {
            printf(INVALID_ARGS_STR);
        } 
    }
    else if (strcmp(command[0],"find") == 0)
    {
        if (command[1] == NULL)
        {
            find(".", NULL);
        }
        else if (command[1] != NULL && command[2] == NULL )
        {
            find(".", command[1]);
        } 
        else if (command[1] != NULL && command[2] != NULL && command[3] == NULL)
        {
            find(command[2], command[1]);
        } 
        else
        {
            printf(INVALID_ARGS_STR);
        }
    }
    else if (strcmp(command[0],"clear") == 0)
    {
        if (command[1] == NULL)
        {
            clear();
        }
        else
        {
            printf(INVALID_ARGS_STR);
        }
            
    }
    else if (strcmp(command[0],"rm") == 0)
    {
        if (command[1] != NULL)
        {
            rm(command[1]);
        }
        else
        {
            printf(INVALID_ARGS_STR);
        }
            
    }
    else if (strcmp(command[0],"cp") == 0)
    {
        if (command[1] != NULL && command[2] != NULL)
        {
            cp(command[1], command[2]);
        }
        else
        {
            printf(INVALID_ARGS_STR);
        }
    }
    else
    {
        commandHandled = 0;
    }
    return commandHandled;
}

int main()
{
    char **command = malloc(MAX_COMMAND_ARGS * sizeof(char*));
    int commandLen = 0;
    int running = 1;

    while(running)
    {
        while(commandLen == 0)
        {
            typePrompt();
            commandLen = takeCommand(command);
        }

        if(strcmp(command[0],"exit") == 0)
        {
            running = 0;
        }

        if (!handleInternalFunctions(command) && running)
        {
            if (fork() != 0)
            {
                waitpid(-1, NULL, 0);
            }
            else
            {
                if (execvp(command[0], command) == -1)
                {
                    perror("\x1b[31m" "Exec() failed" COLOR_RESET);
                    return 1;
                }
            }
        }
        releaseMemory(command, &commandLen);
    }
    return 0;
}