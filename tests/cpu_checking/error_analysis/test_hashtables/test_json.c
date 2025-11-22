
#include "FPC_Hashtable_Error.h"

/*
Example program:

3 double tmp1 = x + b;
4 double tmp2 = (tmp1 * c) / d;
// comment
5 array[i] = tmp2;
*/

int main()
{
    /*---------------------------- Create Table ---------------------------*/
    int64_t size = 1024;
    _FPC_ADDRESS_HTABLE_T *address_table = _FPC_ADDRESS_HT_CREATE_(size);
    _FPC_REGISTER_HTABLE_T *register_table = _FPC_REGISTER_HT_CREATE_(size);
    char *file_name = __FILE__;
    int line = __LINE__;

    /*------------------------- Register 1 (ADD) ---------------------------*/
    char reg_name1[] = "register_1";
    double error1 = 0.01;
    double rel_error1 = 0.001;
    line = 3;
    _FPC_REGISTER_HT_UPDATE_(register_table, reg_name1, error1, rel_error1, file_name, line);

    /*------------------------- Register 2  (MUL) ---------------------------*/
    char reg_name2[] = "register_2";
    double error2 = 0.02;
    double rel_error2 = 0.002;
    line = 4;
    _FPC_REGISTER_HT_UPDATE_(register_table, reg_name2, error2, rel_error2, file_name, line);

    /*------------------------- Register 2  (DIV) ---------------------------*/
    char reg_name3[] = "register_3";
    double error3 = 0.034;
    double rel_error3 = 0.0024;
    line = 4;
    _FPC_REGISTER_HT_UPDATE_(register_table, reg_name3, error3, rel_error3, file_name, line);

    /*------------------------- Address 1 (STORE) --------------------*/
    uintptr_t addr1 = 0x1000;
    double error_tmp = -0.0;
    double rel_error_tmp = -0.0;
    line = 5;
    _FPC_FIND_ERRORS_BY_REGISTER(register_table, reg_name3, &error_tmp, &rel_error_tmp);
    _FPC_ADDRESS_HT_UPDATE_(address_table, addr1, error_tmp, rel_error_tmp, file_name, line);

    /*------------------------- Print Tables ---------------------------*/
    _FPC_HT_PRINT_TABLES_(address_table, register_table);

    /*------------------------- Write JSON ---------------------------*/
    _FPC_WRITE_AND_PRINT_TO_JSON_(address_table, register_table);

    return 0;
}