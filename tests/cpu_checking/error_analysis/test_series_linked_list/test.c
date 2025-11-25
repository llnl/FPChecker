
#include "../../../../src/FPC_FloatSeries_List.h"

int main()
{
    // 1. Initialize the manager
    FPC_SeriesManager *manager = FPC_create_manager();
    if (manager == NULL)
    {
        // Exit if the initial allocation failed
        return EXIT_FAILURE;
    }

    printf("SeriesManager initialized.\n\n");

    // 2. Append values for Key 10
    printf("--- Appending values for Key 10 ---\n");
    if (FPC_append_value(manager, 10, 1.1))
        printf("Append failed.\n");
    if (FPC_append_value(manager, 10, 2.2))
        printf("Append failed.\n");
    if (FPC_append_value(manager, 10, 3.3))
        printf("Append failed.\n");

    // 3. Append values for Key 25
    printf("\n--- Appending values for Key 25 ---\n");
    if (FPC_append_value(manager, 25, 100.01))
        printf("Append failed.\n");
    if (FPC_append_value(manager, 25, 200.02))
        printf("Append failed.\n");

    // 4. Append more for Key 10
    printf("\n--- Appending more values for Key 10 ---\n");
    if (FPC_append_value(manager, 10, 4.4))
        printf("Append failed.\n");

    // 5. Print the results
    printf("\n--- Printing Series ---\n");
    FPC_print_series(manager, 10);
    FPC_print_series(manager, 25);
    FPC_print_series(manager, 99); // Key that doesn't exist

    // 6. Clean up
    printf("\n--- Cleaning Up ---\n");
    FPC_destroy_manager(manager);

    return EXIT_SUCCESS;
}
