#include "FPC_Hashtable_Error.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <number_of_items>\n", argv[0]);
        return 1;
    }
    int num_items = argv[1] ? atoi(argv[1]) : 10;

    /*---------------------------- Create Table ---------------------------*/
    int64_t size = 1024;
    _FPC_ADDRESS_HTABLE_T *address_table = _FPC_ADDRESS_HT_CREATE_(size);
    _FPC_REGISTER_HTABLE_T *register_table = _FPC_REGISTER_HT_CREATE_(size);

    for (int i = 0; i < num_items; i++)
    {
        /*------------------------- Register i ---------------------------*/
        char reg_name[20];
        snprintf(reg_name, sizeof(reg_name), "register_%d", i);
        double error = 0.01 * i;
        double rel_error = 0.001 * i;
        _FPC_REGISTER_HT_UPDATE_(register_table, reg_name, error, rel_error);

        /*------------------------- Address i (STORE) --------------------*/
        uintptr_t addr = 0x1000 + (i * 0x10);
        double error_tmp = -0.0;
        double rel_error_tmp = -0.0;
        _FPC_FIND_ERRORS_BY_REGISTER(register_table, reg_name, &error_tmp, &rel_error_tmp);
        _FPC_ADDRESS_HT_UPDATE_(address_table, addr, error_tmp, rel_error_tmp);
    }

    /*------------------------- Print Tables ---------------------------*/
    printf("Registers inserted: %llu\n", register_table->n);
    printf("Addresses inserted: %llu\n", address_table->n);
    _FPC_HT_PRINT_TABLES_(address_table, register_table);

    return 0;
}