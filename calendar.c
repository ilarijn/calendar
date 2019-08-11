#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* 
gcc -Wvla -Wall -g -std=c99 -o projekti projekti.c 

valgrind --track-origins=yes --leak-check=full ./projekti
valgrind --tool=memcheck --track-origins=yes ./projekti
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
from the copy, deleting the current earliest entry 
after printing it.
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

void freeParseArray(char **arr, int rows, int descr)
{
    for (int i = 0; i < rows; i++)
    {
        free(arr[i]);
    }
    if (descr)
        free(arr[3]);
    free(arr);
}

/*
Parses the command string into an array of three or four separate strings.
If value of parameter "descr" is 1, a description string is assumed to exist
and placed in the fourth string.
The first three strings in the array are assumed to be 1 or 2 characters long.
All four strings are assumed to be separated by whitespaces in char* str with
a newline at the end.
 */
char **parseSpaces(char *str, int descr)
{
    if (*(++str) != ' ')
    {
        printf("Error: invalid command format");
        return NULL;
    }
    char **res = malloc(sizeof(char *) * (3 + descr));
    if (descr)
    {
        res[3] = malloc(sizeof(char) * 80);
        int i = 0;
        while (*(str + 1) != ' ' && *(str + 1) != '\n')
        {
            str++;
            res[3][i] = *str;
            i++;
        }
        str++;
        res[3][i] = '\0';
    }
    for (int i = 0; i <= 2; i++)
    {
        if (*str != ' ')
        {
            printf("Error: invalid command format");
            freeParseArray(res, i, descr);
            return NULL;
        }
        str++;
        if (i == 2 && *(str + 2) != '\n' && *(str + 1) != '\n')
        {
            printf("Error: invalid command format");
            freeParseArray(res, i, descr);
            return NULL;
        }
        res[i] = malloc(sizeof(char) * 3);
        //Add null char for strncat()
        res[i][0] = '\0';
        if (*(str + 1) == ' ')
        {
            strncat(res[i], str, 1);
            str++;
        }
        else
        {
            strncat(res[i], str, 2);
            str += 2;
        }
    }
    return res;
}

/*
If descr is 0, no description string is assumed
and the created Entry struct will be initialized
with a null character as its description.
 */
Entry *parseEntry(char *str, int descr)
{
    char **pieces = parseSpaces(str, descr);
    if (pieces != NULL)
    {
        int month = atoi(pieces[0]);
        if (month > 12 || month < 1)
        {
            printf("Error: invalid month");
            freeParseArray(pieces, 3, descr);
            return NULL;
        }
        int day = atoi(pieces[1]);
        if (day > 31 || day < 1)
        {
            printf("Error: invalid day");
            freeParseArray(pieces, 3, descr);
            return NULL;
        }
        int hour = atoi(pieces[2]);
        if (hour == 24)
        {
            hour = 0;
        }
        else if (hour > 23 || hour < 0)
        {
            printf("Error: invalid hour");
            freeParseArray(pieces, 3, descr);
            return NULL;
        }
        Date date;
        date.month = month;
        date.day = day;
        date.hour = hour;
        Entry *entry = malloc(sizeof(Entry));
        entry->date = date;
        if (descr)
        {
            strcpy(entry->descr, pieces[3]);
        }
        else
        {
            strcpy(entry->descr, "\0");
        }
        freeParseArray(pieces, 3, descr);
        return entry;
    }
    free(pieces);
    return NULL;
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
            break;
        case 'O':
            break;
        case 'Q':
            exit = 1;
            break;
        default:
            printf("Invalid command");
        }
    }
    freeCalendar(calendar);
    return 0;
}