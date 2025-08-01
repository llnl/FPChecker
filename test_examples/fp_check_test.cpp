#include<iostream>
using namespace std;

// double math_opp(double a, double b) {
//     double c = a + b;
//     double d = a - b;
//     double e = a * b;
//     double f = a / b;

//     cout << "Sum :" << c << endl;
//     cout << "Subtraction :" << d << endl;
//     cout << "Product :  " << e << endl;
//     cout << "Division : " << f << endl;

//     return 0;
// }

// int main(int argc, char **argv) {
// // int main() {
//     double x = atof(argv[1]);
//     double y = atof(argv[2]);
//     // double x, y;
//     // cin >> x >> y;

//     math_opp(x, y);
//     return 0;
// }


double comp_interest(double principal, double rate, int years){
    double percentage = rate/100.0;
    double amount = principal;
    double prev_amount = principal;

    for (int i = 1; i<=years; ++i){
        double interest = amount * percentage;
        amount = amount + interest;
        double change = amount - prev_amount ;
        cout <<"Year "<< i << ": " <<amount << " | Change: " << change <<endl;
        prev_amount = amount;
    }

    return amount;

}

int main(int argc, char **argv){
    double principal = atof(argv[1]);
    double rate = atof(argv[2]);
    int year = atoi(argv[3]);

    comp_interest(principal, rate, year);
    return 0;

}