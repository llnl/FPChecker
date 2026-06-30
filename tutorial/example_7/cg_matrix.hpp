#ifndef CG_MATRIX_HPP
#define CG_MATRIX_HPP

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

template <typename T>
struct MatrixData
{
    size_t n = 0;
    bool tridiagonal = false;
    T diag = static_cast<T>(0);
    T offdiag = static_cast<T>(0);
    std::vector<T> dense;
};

inline bool has_suffix(const std::string &value, const std::string &suffix)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::string trim_copy(const std::string &value)
{
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

template <typename T>
int load_tridiagonal_matrix(const char *matrix_path, MatrixData<T> &A, std::vector<T> &b)
{
    std::ifstream file(matrix_path);
    if (!file)
    {
        std::cerr << "Error opening matrix file" << std::endl;
        exit(-1);
    }

    bool have_n = false;
    bool have_diag = false;
    bool have_offdiag = false;
    std::string line;
    while (std::getline(file, line))
    {
        line = trim_copy(line);
        if (line.empty() || line[0] == '#')
            continue;

        const size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        const std::string key = trim_copy(line.substr(0, eq));
        const std::string value = trim_copy(line.substr(eq + 1));
        if (key == "n")
        {
            A.n = static_cast<size_t>(std::strtoull(value.c_str(), nullptr, 10));
            have_n = true;
        }
        else if (key == "diag")
        {
            A.diag = static_cast<T>(std::strtod(value.c_str(), nullptr));
            have_diag = true;
        }
        else if (key == "offdiag")
        {
            A.offdiag = static_cast<T>(std::strtod(value.c_str(), nullptr));
            have_offdiag = true;
        }
    }

    if (!have_n || !have_diag || !have_offdiag || A.n == 0)
    {
        std::cerr << "Invalid tridiagonal matrix file" << std::endl;
        exit(-1);
    }

    A.tridiagonal = true;
    A.dense.clear();
    b.assign(A.n, static_cast<T>(1.0));
    return static_cast<int>(A.n);
}

template <typename T>
int load_dense_csv_matrix(const char *matrix_path, MatrixData<T> &A, std::vector<T> &b)
{
    std::ifstream file(matrix_path);
    if (!file)
    {
        std::cerr << "Error opening matrix file" << std::endl;
        exit(-1);
    }

    std::string line;
    size_t cols = 0;
    size_t rows = 0;
    while (std::getline(file, line))
    {
        if (trim_copy(line).empty())
            continue;

        std::stringstream ss(line);
        std::string token;
        size_t row_cols = 0;
        while (std::getline(ss, token, ','))
        {
            A.dense.push_back(static_cast<T>(std::strtod(token.c_str(), nullptr)));
            row_cols++;
        }

        if (cols == 0)
            cols = row_cols;
        else if (row_cols != cols)
        {
            std::cerr << "Non-rectangular matrix detected" << std::endl;
            exit(-1);
        }
        rows++;
    }

    if (rows == 0 || cols == 0 || rows != cols)
    {
        std::cerr << "Matrix must be square" << std::endl;
        exit(-1);
    }

    A.n = cols;
    A.tridiagonal = false;
    b.assign(A.n, static_cast<T>(1.0));
    return static_cast<int>(A.n);
}

template <typename T>
int loadMatrix(const char *matrix_path, MatrixData<T> &A, std::vector<T> &b)
{
    if (matrix_path == nullptr)
    {
        std::cerr << "Matrix path not provided" << std::endl;
        exit(-1);
    }

    const std::string path(matrix_path);
    if (has_suffix(path, ".tri"))
        return load_tridiagonal_matrix(matrix_path, A, b);

    return load_dense_csv_matrix(matrix_path, A, b);
}

template <typename T, typename AccumT>
inline __attribute__((always_inline))
std::vector<T> mat_vec_mult(const MatrixData<T> &A, const std::vector<T> &v)
{
    if (v.size() != A.n)
    {
        std::cout << "Matrix size does not match vector size." << std::endl;
        exit(1);
    }

    std::vector<T> result(A.n, static_cast<T>(0));
    if (A.tridiagonal)
    {
        for (size_t i = 0; i < A.n; ++i)
        {
            AccumT acc = static_cast<AccumT>(A.diag) * static_cast<AccumT>(v[i]);
            if (i > 0)
                acc += static_cast<AccumT>(A.offdiag) * static_cast<AccumT>(v[i - 1]);
            if (i + 1 < A.n)
                acc += static_cast<AccumT>(A.offdiag) * static_cast<AccumT>(v[i + 1]);
            result[i] = static_cast<T>(acc);
        }
        return result;
    }

    if (A.dense.size() != A.n * A.n)
    {
        std::cout << "Matrix size does not match vector size." << std::endl;
        exit(1);
    }

    for (size_t i = 0; i < A.n; ++i)
    {
        AccumT acc = static_cast<AccumT>(0);
        for (size_t j = 0; j < A.n; ++j)
        {
            acc += static_cast<AccumT>(A.dense[i * A.n + j]) * static_cast<AccumT>(v[j]);
        }
        result[i] = static_cast<T>(acc);
    }
    return result;
}

#endif
