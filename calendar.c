#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* 
gcc -Wvla -Wall -g -std=c99 -o calendar calendar.c 
valgrind --track-origins=yes --leak-check=full ./calendar
valgrind --tool=memcheck --track-origins=yes ./calendar
*/

typedef struct
{
    int month;
    int day;
    int hour;
} Date;

typedef struct
{
    char descr[80];
    int index;
    Date date;
} Entry;

typedef struct
{
    int size;
    int amount;
    Entry *entries;
} Calendar;

Calendar *initCalendar(int size)
{
    Calendar *calendar = malloc(sizeof(Calendar));
    calendar->size = size;
    calendar->amount = 0;
    calendar->entries = malloc(sizeof(Entry) * size);
    return calendar;
}

void freeCalendar(Calendar *calendar)
{
    free(calendar->entries);
    free(calendar);
}

int expandCalendar(Calendar *calendar)
{
    Entry *newentries = realloc(calendar->entries, sizeof(Entry) * (calendar->size * 2));

    if (newentries == NULL)
    {
        return 0;
    }
    calendar->entries = newentries;
    calendar->size *= 2;
    return 1;
}

Entry *entryExists(Calendar *calendar, Entry *entry)
{
    for (int i = 0; i < calendar->amount; i++)
    {
        if (calendar->entries[i].date.month == entry->date.month &&
            calendar->entries[i].date.day == entry->date.day &&
            calendar->entries[i].date.hour == entry->date.hour)
        {
            return calendar->entries + i;
        }
    }
    return NULL;
}

int addEntry(Calendar *calendar, Entry *new_entry)
{
    if (entryExists(calendar, new_entry))
    {
        printf("There is already an entry for this time and date");
        return 0;
    }
    if (calendar->amount == calendar->size)
    {
        if (!expandCalendar(calendar))
            return 0;
    }
    new_entry->index = calendar->amount;
    calendar->entries[calendar->amount] = *new_entry;
    calendar->amount++;
    return 1;
}

/*
Deleting an entry from the calendar occurs by copying
the fields (except for the index) of the last entry
to the location of the entry to be deleted. 
If the entry to be deleted is the last one,
the amount of entries is simply decremented by 1. 
*/
int deleteEntry(Calendar *calendar, Entry *entry)
{
    Entry *deletee = entryExists(calendar, entry);
    if (deletee != NULL)
    {
        if (deletee->index == (calendar->amount - 1))
        {
            calendar->amount = calendar->amount - 1;
        }
        else
        {
            calendar->entries[deletee->index].date = calendar->entries[calendar->amount - 1].date;
            strcpy(calendar->entries[deletee->index].descr, calendar->entries[calendar->amount - 1].descr);
            calendar->amount = calendar->amount - 1;
        }
        return 1;
    }
    return 0;
}

void printEntry(Entry *entry)
{
    printf("%s %02d.%02d. klo %02d\n",
           entry->descr, entry->date.day, entry->date.month, entry->date.hour);
}

/*
Copies the main calendar and prints entries in order
from the copy, deleting from the copy the current earliest 
entry after printing it.
 */
void listEntries(Calendar *calendar)
{
    if (calendar->amount > 0)
    {
        Calendar *list = initCalendar(calendar->size);
        for (int i = 0; i < calendar->amount; i++)
        {
            list->entries[i].index = calendar->entries[i].index;
            list->entries[i].date.month = calendar->entries[i].date.month;
            list->entries[i].date.day = calendar->entries[i].date.day;
            list->entries[i].date.hour = calendar->entries[i].date.hour;
            strcpy(list->entries[i].descr, calendar->entries[i].descr);
            list->amount += 1;
        }
        Entry *earliest;
        printf("\n");
        while (list->amount > 1)
        {
            earliest = list->entries;
            for (int j = 1; j < list->amount; j++)
            {
                if (earliest->date.month > list->entries[j].date.month)
                {
                    earliest = list->entries + j;
                }
                else if (earliest->date.month == list->entries[j].date.month)
                {
                    if (earliest->date.day > list->entries[j].date.day)
                    {
                        earliest = list->entries + j;
                    }
                    else if (earliest->date.day == list->entries[j].date.day)
                    {
                        if (earliest->date.hour > list->entries[j].date.hour)
                        {
                            earliest = list->entries + j;
                        }
                    }
                }
            }
            printEntry(earliest);
            deleteEntry(list, earliest);
        }
        if (list->amount == 1)
        {
            printEntry(list->entries);
        }
        freeCalendar(list);
    }
    else
    {
        printf("Calendar is empty");
    }
}

void freeCommandArr(char **command)
{
    if (command != NULL)
    {
        for (int i = 0; i <= 3; i++)
        {
            free(command[i]);
        }
        free(command);
    }
}

/*
Parses the command string into an array of four strings.
The first string is assumed to be not more than 80 characters 
long including the null character. The other strings are assumed 
to be max 3 characters long including the null character.
 */
char **parseCommand(char *command)
{
    command++;
    if (!*command || *command != ' ')
    {
        printf("Error: invalid command format");
        return NULL;
    }
    char **res = malloc(4 * sizeof(char *));
    res[0] = malloc(80 * sizeof(char));
    res[1] = malloc(3 * sizeof(char));
    res[2] = malloc(3 * sizeof(char));
    res[3] = malloc(3 * sizeof(char));
    res[0][0] = '\0';
    res[1][0] = '\0';
    res[2][0] = '\0';
    res[3][0] = '\0';
    for (int i = 0; i <= 3; i++)
    {
        command++;
        int j = 0;
        while (*command && (*command != ' ' && *command != '\n'))
        {
            j++;
            if ((i > 0 && j > 2) || j > 79)
            {
                printf("Error: invalid command format");
                freeCommandArr(res);
                return NULL;
            }
            strncat(res[i], command, 1);
            command++;
        }
        if (*command == '\n')
        {
            i = 3;
        }
    }
    return res;
}

/*
Parses a calendar entry from the output of parseCommand().
If descr is 0, no description string is assumed and 
the created Entry struct will be initialized
with a null character as its description.
 */
Entry *parseEntry(char *command, int descr)
{
    char **pieces = parseCommand(command);
    if (pieces != NULL)
    {
        if (descr && pieces[0][0] == '\0')
        {
            printf("Error: description cannot be empty");
            freeCommandArr(pieces);
            return NULL;
        }
        int month = atoi(pieces[0 + descr]);
        if (month > 12 || month < 1 || pieces[0 + descr][0] == '\0')
        {
            printf("Error: invalid or missing month");
            freeCommandArr(pieces);
            return NULL;
        }
        int day = atoi(pieces[1 + descr]);
        if (day > 31 || day < 1 || pieces[1 + descr][0] == '\0')
        {
            printf("Error: invalid or missing day");
            freeCommandArr(pieces);
            return NULL;
        }
        int hour = atoi(pieces[2 + descr]);
        if (hour == 24)
        {
            hour = 0;
        }
        else if (hour > 23 || hour < 0 || pieces[2 + descr][0] == '\0')
        {
            printf("Error: invalid or missing hour");
            freeCommandArr(pieces);
            return NULL;
        }
        Date date;
        date.month = month;
        date.day = day;
        date.hour = hour;
        Entry *entry = calloc(1, sizeof(Entry));
        entry->date = date;
        if (descr)
        {
            strcpy(entry->descr, pieces[0]);
        }
        else
        {
            strcpy(entry->descr, "\0");
        }
        freeCommandArr(pieces);
        return entry;
    }
    free(pieces);
    return NULL;
}

int saveCalendar(Calendar *calendar, char *command)
{
    char **pieces = parseCommand(command);
    char *filename = pieces[0];
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        freeCommandArr(pieces);
        printf("Error while saving to file");
        return 0;
    }
    fwrite(&calendar->size, sizeof(int), 1, fp);
    fwrite(&calendar->amount, sizeof(int), 1, fp);
    fwrite(calendar->entries, sizeof(Entry), calendar->amount, fp);
    fclose(fp);
    freeCommandArr(pieces);
    return 1;
}

Calendar *loadCalendar(char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("File does not exist");
        return NULL;
    }
    if (!fp)
    {
        printf("Error while opening file");
        return NULL;
    }
    int size;
    int amount;
    fread(&size, sizeof(int), 1, fp);
    fread(&amount, sizeof(int), 1, fp);
    Calendar *calendar = initCalendar(size);
    calendar->size = size;
    calendar->amount = amount;
    fread(calendar->entries, sizeof(Entry), calendar->amount, fp);
    fclose(fp);
    return calendar;
}

int main(void)
{
    printf("           _                _\n");
    printf("          | |              | |\n");
    printf("  ___ __ _| | ___ _ __   __| | __ _ _ __ \n");
    printf(" / __/ _` | |/ _ \\ '_ \\ / _` |/ _` | '__|\n");
    printf("| (_| (_| | |  __/ | | | (_| | (_| | |   \n");
    printf(" \\___\\__,_|_|\\___|_| |_|\\__,_|\\__,_|_|   \n\n");
    printf("(A)dd an entry          (W)rite to file\n");
    printf("(D)elete an entry       (O)pen from file\n");
    printf("(L)ist all entries      (Q)uit\n");

    Calendar *calendar = initCalendar(1);
    int exit = 0;
    char command[100];

    while (exit == 0)
    {
        printf("\n> ");
        fgets(command, sizeof(command), stdin);
        switch (command[0])
        {
        case 'A':;
            Entry *entry = parseEntry(command, 1);
            if (entry != NULL && addEntry(calendar, entry))
            {
                printf("Entry saved");
            }
            free(entry);
            break;
        case 'D':;
            Entry *deletee = parseEntry(command, 0);
            if (deletee != NULL)
            {
                if (deleteEntry(calendar, deletee))
                {
                    printf("Entry deleted");
                }
                else
                {
                    printf("Entry not found");
                }
            }
            free(deletee);
            break;
        case 'L':
            listEntries(calendar);
            break;
        case 'W':
            if (saveCalendar(calendar, command))
            {
                printf("Calendar saved");
            }
            else
            {
                printf("Save failed");
            }
            break;
        case 'O':;
            char **pieces = parseCommand(command);
            if (pieces != NULL)
            {
                if (pieces[0][0] == '\0')
                {
                    printf("No file name given");
                }
                else
                {
                    char *filename = pieces[0];
                    Calendar *new_calendar = loadCalendar(filename);
                    if (new_calendar != NULL)
                    {
                        freeCalendar(calendar);
                        calendar = new_calendar;
                        printf("Calendar loaded");
                    }
                }
            }
            freeCommandArr(pieces);
            break;
        case 'Q':
            exit = 1;
            printf("Bye!\n");
            break;
        default:
            printf("Invalid command");
        }
    }
    freeCalendar(calendar);
    return 0;
}