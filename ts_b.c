/*
 * Russell Stanley
 * CSE 3320
 * Lab 1
 *
 * This file uses example code from assignment handout
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <curses.h>
#include <limits.h>
#include <unistd.h>

#define MAX_FILES 1024

// I made this struct because I had a hard time working with dirents
typedef struct struct_entry
{
    char name[NAME_MAX];
    char type[10];
    struct stat info;
} entry;

int print_next_chunk(int chunk_size, entry *entries, int current, int *size)
{
    int end = (current + chunk_size < *size) ? current + chunk_size : *size;
    printw("%-5s %-6s%-30s%s\n", "#", "Type", "Name", "Last Modified");
    printw("---------------------------------------------------------\n");
    while (current < end)
    {
        printw("%-5d %-6s%-30s%s\n", current, entries[current].type, entries[current].name, ctime(&entries[current].info.st_mtime));
        current++;
    }

    return current;
}

/*
 * Comparison functions for qsort()
 */
int cmp_type_name(const void *pa, const void *pb) 
{
    entry a = *(entry *)pa;
    entry b = *(entry *)pb;
    return (strcmp(a.type, b.type)) ? strcmp(a.type, b.type) :
                                      strcmp(a.name, b.name);
}

int cmp_name(const void *pa, const void *pb)
{
    entry a = *(entry *)pa;
    entry b = *(entry *)pb;
    return strcmp(a.name, b.name);
}

int cmp_type(const void *pa, const void *pb)
{
    entry a = *(entry *)pa;
    entry b = *(entry *)pb;
    return strcmp(a.type, b.type);
}

int cmp_mtime(const void *pa, const void *pb)
{
    entry a = *(entry *)pa;
    entry b = *(entry *)pb;
    return difftime(b.info.st_mtim.tv_sec, a.info.st_mtim.tv_sec); 
}

/*
 * Returns a sorted array of directory entries
 */
entry* build_dir_list(char *dest_dir, int *size)
{
    DIR *d;
    int c = 0;
    struct dirent *de;

    d = opendir(dest_dir);

    // count entries, allocate array large enough to hold them
    int count = 0;
    while (readdir(d) && count < MAX_FILES) count++;
    rewinddir(d);

    entry *result = malloc(sizeof(entry) * count);

    while ((de = readdir(d)))
    {
        if ((de->d_type) & DT_DIR)
        {
            strcpy(result[c].type, "Dir");
        }
        else if ((de->d_type) & DT_REG)
        {
            strcpy(result[c].type, "File");
        }
        else
        {
            strcpy(result[c].type, "Unknown");
        }
    
        stat(de->d_name, &result[c].info);
        strcpy(result[c++].name, de->d_name);
        
        if (count > MAX_FILES) break;
    }

    qsort(result, count, sizeof(entry), cmp_type_name);

    closedir(d);
    
    *size = count;
    return result;

}

/*
 * Returns the index of a previous chunk
 * specify number of chunks to rewind in iterations
 */
int chunk_rewind(int start, int chunk, int iterations)
{
    if (start < 0 || chunk < 0 || iterations < 0) return 0;
    return (start - chunk * iterations >= 0) ? start - chunk * iterations : 0;
}

int main(int argc, char *argv[])
{
    pid_t child;
    int i, c, k;
    char s[256], cmd[256], dest_dir[256]; // consider modifying
    time_t t;
    const int chunk = 10;
    int current = 0;
    int *size = malloc(sizeof(int));

    strcpy(dest_dir, (argc > 1) ? argv[1] : ".");
    chdir(dest_dir);

    entry *entries = build_dir_list(dest_dir, size);
    
    // NCURSES init
    initscr();
    cbreak();
    keypad(stdscr, TRUE);

    while (1)
    {
        noecho();
        clear();
        t = time(NULL);
        printw("Time: %s\n", ctime(&t));

        getcwd(s, sizeof(s));
        printw("\nCurrent Directory: %s \n", s);


        current = print_next_chunk(chunk, entries, current, size);


        printw("---------------------------------------------------------\n");
        printw("E: edit\t\tR: run\t\tC: change dir\t\tS: sort\nP: previous %d entries\t\tN: next %d entries\nQ: quit\n", chunk, chunk);
        c = tolower(getch());
        refresh();
        switch (c)
        {
            case 'q': echo();
                      nocbreak();
                      endwin();
                      free(entries);
                      exit(0);

            case 'e': printw("Edit what?:");
                      echo();
                      getstr(s);
                      printw(s);
                      strcpy(cmd, "vim ");
                      strcat(cmd, entries[atoi(s)].name);
                      system(cmd);
                      current = chunk_rewind(current, chunk, 1);
                      break;

            case 'r': printw("Run what?:");
                      echo();
                      getstr(s);
                      def_prog_mode();
                      endwin();
                      strcpy(cmd, "./");
                      strcat(cmd, entries[atoi(s)].name);
                      printw("Arguments: ");
                      getstr(s);
                      strcat(cmd, " ");
                      strcat(cmd, s);
                      system(cmd);
                      reset_prog_mode();
                      current = chunk_rewind(current, chunk, 1);
                      break;

            case 'c': printw("Change To?: ");
                      echo();
                      getstr(s);
                      strcpy(dest_dir, entries[atoi(s)].name);
                      entries = build_dir_list(dest_dir, size);
                      chdir(dest_dir);
                      current = 0;
                      break;

            case 's': printw("Sort by (N)ame, (T)ype, or (L)ast modified?\n");
                      char t = tolower(getch());
                      switch(t)
                      {
                          case 'n': qsort(entries, *size, sizeof(entry), cmp_name);
                                    break;
                          case 't': qsort(entries, *size, sizeof(entry), cmp_type); 
                                    break;
                          case 'l': qsort(entries, *size, sizeof(entry), cmp_mtime);
                                    break;
                          default:  qsort(entries, *size, sizeof(entry), cmp_type_name);
                      }
                      current = chunk_rewind(current, chunk, 1);
                      break;

            case 'p': current = chunk_rewind(current, chunk, 2);
                      break;

            case 'n': break;

            default:  printw("Invalid selection. Press any key to continue.\n");
                      current = chunk_rewind(current, chunk, 1);
                      getch();
                      break;
        }
    }

    free(entries);
    echo();
    nocbreak();
    endwin();
    return 0;
}

