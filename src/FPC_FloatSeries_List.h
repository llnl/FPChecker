#ifndef FPC_FLOATSERIES_LIST_H
#define FPC_FLOATSERIES_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#define HASH_TABLE_SIZE 128

// --- Structure Definitions ---

typedef struct FPC_SeriesNode
{
    double value;
    struct FPC_SeriesNode *next;
} FPC_SeriesNode;

typedef struct FPC_KeySeries
{
    int key;
    FPC_SeriesNode *head;
} FPC_KeySeries;

typedef struct FPC_SeriesManager
{
    FPC_KeySeries table[HASH_TABLE_SIZE];
    int size; // Current number of occupied slots
} FPC_SeriesManager;

// --- Interface Functions ---

/**
 * @brief Simple hash function for the integer key.
 * @param key The integer key.
 * @return The hash index (0 to HASH_TABLE_SIZE - 1).
 */
static int hash_function(int key)
{
    // Simple modulo hash (handles positive and negative keys by absolute value)
    return abs(key) % HASH_TABLE_SIZE;
}

/**
 * @brief Initializes a new SeriesManager structure.
 * @return A pointer to the newly allocated SeriesManager, or NULL on failure.
 */
FPC_SeriesManager *FPC_create_manager()
{
    // Use calloc to allocate and initialize memory to zero
    FPC_SeriesManager *manager = (FPC_SeriesManager *)calloc(1, sizeof(FPC_SeriesManager));
    if (manager == NULL)
    {
        fprintf(stderr, "FPCHECKER: ERROR: Memory allocation failed for SeriesManager.\n");
        // Don't exit here, just return NULL so the caller can handle it
        return NULL;
    }

    // Initialize keys to an impossible value (or use a flag)
    // calloc already set all to 0, which is acceptable if 0 is not a valid key.
    // Assuming positive keys for simplicity in this simple hash table.

    return manager;
}

/**
 * @brief Appends a double value to the series associated with the given key.
 *
 * This function performs the core task:
 * 1. Finds the series for the key (simple linear probe on collision).
 * 2. Creates a new SeriesNode for the value.
 * 3. Appends the new node to the end of the series' linked list.
 * 4. Checks memory availability at all steps.
 *
 * @param manager The SeriesManager structure.
 * @param key The integer key.
 * @param value The double precision value to append.
 * @return 0 on success, -1 on failure (memory or key lookup error).
 */
int FPC_append_value(FPC_SeriesManager *manager, int key, double value)
{
    if (manager == NULL)
        return -1;

    // 1. Find the KeySeries slot using simple linear probing
    int index = hash_function(key);
    int start_index = index;
    FPC_KeySeries *series = NULL;

    // Simple open addressing/linear probing for collision resolution and lookup
    do
    {
        if (manager->table[index].key == key || manager->table[index].head == NULL)
        {
            // Found existing key or an empty slot
            series = &manager->table[index];
            break;
        }
        // Move to the next slot (linear probe)
        index = (index + 1) % HASH_TABLE_SIZE;
    } while (index != start_index); // Stop after checking the whole table

    if (series == NULL)
    {
        fprintf(stderr, "FPCHECKER: ERROR: Hash table is full or key lookup failed for key %d.\n", key);
        return -1;
    }

    // 2. Allocate and initialize the new SeriesNode
    FPC_SeriesNode *newNode = (FPC_SeriesNode *)malloc(sizeof(FPC_SeriesNode));
    if (newNode == NULL)
    {
        // Memory check failure
        fprintf(stderr, "FPCHECKER: ERROR: Failed to allocate memory for new node (value %f).\n", value);
        return -1;
    }

    newNode->value = value;
    newNode->next = NULL;

    // 3. Update the SeriesManager and append the node
    if (series->head == NULL)
    {
        // First value for this key (and possibly the first time this slot is used)
        if (series->key != key)
        {
            // Initialize the slot (only if it was an empty slot)
            series->key = key;
        }
        series->head = newNode;
    }
    else
    {
        // Append to the end of the existing list
        FPC_SeriesNode *current = series->head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = newNode;
    }

    return 0; // Success
}

/**
 * @brief Frees all memory associated with the SeriesManager.
 * @param manager The SeriesManager structure to destroy.
 */
void FPC_destroy_manager(FPC_SeriesManager *manager)
{
    if (manager == NULL)
        return;

    // Iterate through the hash table slots
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        FPC_SeriesNode *current = manager->table[i].head;
        FPC_SeriesNode *next;

        // Traverse and free the linked list for this key
        while (current != NULL)
        {
            next = current->next;
            free(current);
            current = next;
        }
    }

    // Free the manager itself
    free(manager);
    // printf("SeriesManager and all associated data freed successfully.\n");
}

/**
 * @brief Prints the series of values for a given key. (Utility function)
 * @param manager The SeriesManager structure.
 * @param key The integer key.
 */
void FPC_print_series(FPC_SeriesManager *manager, int key)
{
    if (manager == NULL)
        return;

    int index = hash_function(key);
    int start_index = index;
    FPC_KeySeries *series = NULL;

    // Find the KeySeries slot
    do
    {
        if (manager->table[index].key == key)
        {
            series = &manager->table[index];
            break;
        }
        if (manager->table[index].head == NULL)
        {
            // Empty slot, key doesn't exist
            break;
        }
        index = (index + 1) % HASH_TABLE_SIZE;
    } while (index != start_index);

    if (series == NULL || series->head == NULL)
    {
        printf("Key %d: Series not found or empty.\n", key);
        return;
    }

    printf("Key %d Series: [ ", key);
    FPC_SeriesNode *current = series->head;
    while (current != NULL)
    {
        printf("%.17e", current->value);
        if (current->next != NULL)
        {
            printf(", ");
        }
        current = current->next;
    }
    printf(" ]\n");
}

void FPC_series_to_json(FPC_SeriesManager *manager)
{
    // Create directory
    struct stat st;

    char dir_name[] = ".fpc_logs";
    if (stat(dir_name, &st) == -1)
    { // dir doesn't exists
        mkdir(dir_name, 0775);
    }

    // Set filename
    // On Linux: The maximum length for a file name is 255 bytes.
    // The maximum combined length of both the file name and path name is 4096 bytes.
    char executionId[5000];
    char fileName[5000];
    char errorFileName[5000];
    errorFileName[0] = '\0';
    strcpy(errorFileName, ".fpc_logs/errors_per_line_");

    // ----------- Get execution ID -----------
    // size_t len=256;
    //  According to Linux manual:
    //  Each element of the hostname must be from 1 to 63 characters long
    //  and the entire hostname, including the dots, can be at most 253
    //  characters long.
    executionId[0] = '\0';
    if (gethostname(executionId, 256) != 0)
        strcpy(executionId, "node-unknown");

    // Maximum size for PID: we assume 2,000,000,000
    int pid = (int)getpid();
    char pidStr[11];
    // pidStr[0] = '\0';
    // sprintf(pidStr, "%d", pid);
    snprintf(pidStr, sizeof(pidStr), "%d", pid);
    strcat(executionId, "_");
    strcat(executionId, pidStr);
    strcat(executionId, ".json");

    strcat(fileName, executionId);
    strcat(errorFileName, executionId);

    printf("#FPCHECKER: Writing errors per line to: %s\n", errorFileName);

    FILE *fp = fopen(errorFileName, "w");
    if (!fp)
    {
        perror("fopen");
        return;
    }

    // Iterate on the hash table and print all series
    fprintf(fp, "[\n");
    int first_series = 1;
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        FPC_KeySeries *series = &manager->table[i];
        if (series->head != NULL)
        {
            if (!first_series)
                fprintf(fp, ",\n");
            first_series = 0;
            fprintf(fp, "  {\n");
            fprintf(fp, "    \"line\": %d,\n", series->key);
            fprintf(fp, "    \"values\": [ ");
            FPC_SeriesNode *current = series->head;
            int first_value = 1;
            while (current != NULL)
            {
                if (!first_value)
                    fprintf(fp, ", ");
                first_value = 0;
                fprintf(fp, "%.17e", current->value);
                current = current->next;
            }
            fprintf(fp, " ]\n");
            fprintf(fp, "  }");
        }
    }
    fprintf(fp, "\n]\n");
    fclose(fp);
}

#endif // FPC_FLOATSERIES_LIST_H