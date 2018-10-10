#include "alang.hpp"

#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
using namespace std;

template <typename T>
class matrix {
  int r, c;
  std::vector<T> v;
public:
  matrix(int rows, int cols) : r(rows), c(cols) { v.reserve(rows * cols); }
  T& operator()(int i, int j) { return v[i*r + j]; }
  const T& operator()(int i, int j) const { return v[i*r + j]; }
  int rows() const { return r; }
  int cols() const { return c; }
};

template <typename T>
matrix<T> operator+(const matrix<T>& a, const matrix<T>& b) {
  assert(a.rows() == b.rows() && a.cols() == b.cols()); // matrix shape mismatch

  matrix<T> sum(a.rows(), a.cols());
  for (int i=0; i<a.rows(); ++i) {
    for (int j=0; j<a.cols(); ++j) {
      sum(i, j) = a(i, j) + b(i, j);
    }
  }
  return sum;
}

template <typename T>
matrix<T> constant_matrix(int rows, int cols, T value) {
  matrix<double> m(rows, cols);
  for (int i=0; i<rows; ++i)
    for (int j=0; j<cols; ++j)
      m(i, j) = value;
  return m;
}

template <typename T>
matrix<T> row_add(const matrix<T>& a, const matrix<T>& b) {
  matrix<double> m(a.rows(), a.cols());

  {
    processes ps;

    for (int i=0; i<a.rows(); ++i) {
      ps += [&, i] {
        for (int j=0; j<b.rows(); ++j) {
          m(i, j) = a(i, j) + b(i, j);
        }
      };
    }
  }

  return m;
}

template <typename T>
matrix<T> col_add(const matrix<T>& a, const matrix<T>& b) {
  matrix<double> m(a.rows(), a.cols());

  {
    processes ps;

    for (int i=0; i<a.rows(); ++i) {
      ps += [&, i] {
          for (int j=0; j<b.rows(); ++j) {
            m(j, i) = a(j, i) + b(j, i);
          }
      };
    }
  }

  return m;
}

template <typename T>
matrix<T> elem_add(const matrix<T>& a, const matrix<T>& b) {
  matrix<double> m(a.rows(), a.cols());

  {
    processes ps;

    for (int i=0; i<a.rows(); ++i) {
      for (int j=0; j<b.rows(); ++j) {
        ps += [&, i, j] {
            m(i, j) = a(i, j) + b(i, j);
        };
      }
    }
  }

  return m;
}


int main() {

  auto m1 = constant_matrix(1000, 1000, 1.0);
  auto m2 = constant_matrix(1000, 1000, 3.14);

  auto m3 = matrix<double>(1000, 1000);

  // time taken to add two million-element matrices
  auto t = time_ms([&] {
    m3 = m1 + m2;
  });
  std::cout << "TIME: " << t << std::endl;

  auto t_rows = time_ms([&] {
      m3 = row_add(m1, m2);
  });
  std::cout << "TIME W/ ROW_ADD: " << t_rows << std::endl;

  auto t_cols = time_ms([&] {
      m3 = col_add(m1, m2);
  });
  std::cout << "TIME W/ COL_ADD: " << t_cols << std::endl;

  auto t_elem = time_ms([&] {
      m3 = elem_add(m1, m2);
  });
  std::cout << "TIME W/ ELEM_ADD: " << t_elem << std::endl;

  // a test that the addition does something sensible
  double epsilon = 0.000001;
  assert(std::abs(m3(140, 323) - 4.14) < epsilon);
}
