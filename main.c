#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_COMMAND_LEN 32
#define NO_DATE_SPECIFIED -1
#define EXPIRY_DATE 7
#define ITEMLIST "items.csv"
#define CALL_UPLOAD_SCRIPT "python uploadCSV.py"
#define CALL_DOWNLOAD_SCRIPT "python downloadCSV.py"

typedef struct item_t
{
    char* itemName;
    time_t dateOfOpening; //in UNIX-based time representation, defaults to NOW
    unsigned char daysToExpire; //default: 7
} item;

item* initItem(char *input, time_t opening)
{
    item* i = (item *)malloc(sizeof(item));
    if(!i)
    {
        return NULL;
    }
    if(opening == NO_DATE_SPECIFIED)
    {
        time_t now = time(NULL);
        i->dateOfOpening = now;
    } else
    {
        i->dateOfOpening = opening;
    }
    char* itemName = strtok(input, ",");
    i->itemName = (char *)malloc(strlen(itemName) + 1);
    if(!i->itemName)
    {
        return NULL;
    }
    i->itemName = strcpy(i->itemName, itemName);
    char* days = strtok(NULL, "\n");
    if(days != NULL)
    {
        i->daysToExpire = atoi(days);
    } else
    {
        i->daysToExpire = EXPIRY_DATE;
    }
    return i;
}

item* parseItem(char* userInput)
{
    if(userInput == NULL)
    {
        fprintf(stderr, "You have not provided any itemname required by the command.");
        return NULL;
    }
    item* it = initItem(userInput, NO_DATE_SPECIFIED);
    return it;
}

void deleteItem(FILE* f, item* it)
{
    char fline[1024] = {0};
    bool exists = false;
    FILE* tmpF = fopen(".tmp", "w");
    if(!f)
    {
        return;
    }
    while(fgets(fline, 1024, f) != NULL)
    {
        char* itemName = strtok(fline, ";");
        if(strcmp(it->itemName, itemName))
        {
            fprintf(tmpF, "%s;%s\n", itemName, strtok(NULL, "\n"));
        } else
        {
            exists = true;
        }
    }
    if(rename(".tmp", "items.csv") == -1)
    {
        fprintf(stderr, "An error has occured during the operation 'delete'. (Temp file renaming)");
    }
    fclose(tmpF);
    if(!exists)
    {
        fprintf(stdout, "'%s': no such item found.\n", it->itemName);
    }
}

void addItem(FILE* f, item* it)
{
    char fline[1024] = {0};
    while(fgets(fline, 1024, f) != NULL)
    {
        if(!strcmp(strtok(fline, ";"), it->itemName))
        {
            fprintf(stdout, "This item already exists! (added %s). \nAre you sure you"
                            " want to add it to the list? [y/n]\n", strtok(ctime(&it->dateOfOpening), "\n"));
            char answer[1];
            scanf("%s", answer);
            if(strcmp(answer, "y"))
            {
               return;
            }
        }
        if(*fline == '\n')
        {
            break;
        }
    }
    fprintf(f, "%s;%ld;%u\n", it->itemName, it->dateOfOpening, it->daysToExpire);
}

unsigned int countDaysOpened(time_t timeOfOpening)
{
    timeOfOpening -= timeOfOpening % (24*60*60); //the DAY of opening
    return (time(NULL) - timeOfOpening) / (24*60*60);
}

void searchItem(FILE* f, item* it)
{
    char fline[1024] = {0};
    while(fgets(fline, 1024, f) != NULL)
    {
        char* itemName = strtok(fline, ";");
        if(!strcmp(it->itemName, itemName))
        {
           fprintf(stdout, "MATCH: %s, opened %d days ago.\n", itemName, countDaysOpened(atoi(strtok(NULL, ";"))));
           return;
        }
    }
    fprintf(stdout, "'%s': no such item found.\n", it->itemName);
}

void display(FILE* f)
{
    char fline[1024] = {0};
    fseek(f, 0, SEEK_SET);
    while(fgets(fline, 1024, f) != NULL)
    {
        if(*fline == '\n')
        {
            continue;
        }
        char* itemName = strtok(fline, ";");
        if(itemName == NULL)
        {
            continue;
        }
        time_t timeAdded = atoi(strtok(NULL, "\n"));
        fprintf(stdout, "%s (added: %s, %d days ago).\n", itemName, strtok(ctime(&timeAdded), "\n"), countDaysOpened(timeAdded));
    }
    fprintf(stdout, "\n");
}
/**
 * @brief decodeCommand Decodes a command given by user. IMPORTANT NOTE: the userInput
 * is stripped away from the \n character, therefore following parsing doesn't have to
 * deal with it.
 * @param userInput
 * @param isRunning
 * @param comm
 * @return The false return statement is used when user entered nonexistent command.
 */
bool decodeCommand(char* userInput, char* comm)
{
    char* command = NULL; // {help, remove, add, search, display, quit}
    command = strtok(strtok(userInput, "\n"), " ");
    if(command == NULL || !strcmp(command, "quit")) //the input is '\n'
    {
        return true;
    }
    if(!strcmp(command, "help"))
    {
        static char helpInfo[] = "USER HELP FOR ANTIrot PROGRAM.\n"
                               "Basic commands you can use:\n"
                               "\t'help' - prints out this help.\n"
                               "\t'remove' <itemName> - removes an item from the list.\n"
                               "\t'add' <itemName> <expiry date>(optional)- adds an item from the list. If you want to specify specific expiry date, you must separate the itemname and the number with a comma!\n"
                               "\t'search' <itemName>\n"
                               "\t'display' prints out the whole list of items.\n"
                               "\t'quit' ends this program.\n"
                               "Please note that this program (for the time being)\
does not deal with duplicit items.\
                                \n\n";
        fprintf(stdout, "%s", helpInfo);

    } else if(!strcmp(command, "remove") || !strcmp(command, "add") \
              || !strcmp(command, "search") || !strcmp(command, "display"))
    {
        *comm = command[0];
        if(*comm != 'd')
        {
            char* remainder = NULL;
            remainder = strtok(NULL, "\n");
            if(remainder == NULL)
            {
                return false;
            }
            strcpy(userInput, remainder);
        }
    } else
    {
      return false;
    }
    return true;
}

void performRotCheck()
{
    FILE* f = fopen(ITEMLIST, "r");
    if(f == NULL)
    {
        perror(ITEMLIST);
        return;
    }
    char fline[1024] = {0};
    while(fgets(fline, 1024, f) != NULL)
    {

       char* name = strtok(strtok(fline, "\n"), ";");
       char* date = strtok(NULL, ";");
       unsigned int daysToExpire = atoi(strtok(NULL, ";"));
       if(date == NULL)
       {
           continue;
       }
       time_t timeOpened = atoi(date);
       static unsigned int daysOpened;
       if ((daysOpened = countDaysOpened(timeOpened)) > daysToExpire)
       {
            printf("WARNING: %s is more than %d days old! (%d days)\n", name, EXPIRY_DATE, daysOpened);
       }
    }
    fclose(f);
}

bool performCommand(char command, item* it)
{
    FILE* f = fopen(ITEMLIST, "a+");
    assert(f != NULL);
    switch(command)
    {
        case'r':
            {
                deleteItem(f, it);
                fclose(f);
            } //remove command
        case 'd': //this part of display is the same as for the remove part
                f = fopen(ITEMLIST, "r");
                display(f);
                break;
        case'a':
            {
                addItem(f, it);
                display(f);
                break;
            } //add command
        case's':
            {
                searchItem(f, it);
                break;
            } //search command
        default:
            {
                break;
            }
    }
    fclose(f);
    return true;
}

void releaseItem(item** it)
{
    item* it_aux = *it;
    if(*it == NULL)
    {
        return;
    }
    if(it_aux->itemName != NULL)
    {
        free(it_aux->itemName);
    }
    free(*it);
    *it = NULL;
}

int main(void)
{
    bool isRunning = true;
    char* pUserInput;
    char* userInput = NULL;
    item* it = NULL;
    userInput = malloc(sizeof(char)*1024);
    assert(userInput != NULL);
    pUserInput = userInput;
    performRotCheck();
    system(CALL_DOWNLOAD_SCRIPT);
    while(isRunning)
    {
        printf(">>");
        char command = 0;
        fgets(userInput, MAX_COMMAND_LEN-1, stdin);
        if(!decodeCommand(userInput, &command))
        {
            printf("\tCommand error.\n");
            continue;
        } else if(command != 0)
        {
            it = parseItem(userInput);
            if(it == NULL)
            {
                printf("The creation of item you provided was not successful.\n");
                continue;
            }
            if(!performCommand(command, it))
            {
                fprintf(stderr, "Error.\n");
            }
        } else {
            isRunning = false;
        }
        releaseItem(&it);
    }
    free(pUserInput);
}
