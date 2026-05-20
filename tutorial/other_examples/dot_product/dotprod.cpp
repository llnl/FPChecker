#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <iomanip>
#include <chrono>

using namespace std;

int loadVectors(vector<float>&v1, vector<float>&v2)
{
  const char *s = getenv("DP_VECS");
  if (s == NULL)
  {
    cerr << "DP_VECS var not found" << endl;
    exit(-1);
  }

  FILE *file = fopen(s, "r");
  if (file == NULL)
  { 
    cerr << "Error opening vectors file" << endl;
    exit(-1);
  }

  char line_buf[2000000];

  // load v1
  fgets(line_buf, sizeof(line_buf), file);
  char *token = strtok(line_buf, ",");
  while (token) {
    v1.push_back(atof(token));
    token = strtok(NULL, ",");
  }

  // load v2
  fgets(line_buf, sizeof(line_buf), file);
  token = strtok(line_buf, ",");
  while (token) {
    v2.push_back(atof(token));
    token = strtok(NULL, ",");
  }

  fclose(file);

  if (v1.size() != v2.size()) {
    cerr << "Vectors have different sizes: v1.size() = " << v1.size() << ", v2.size() = " << v2.size() << endl;
    exit(-1);
  }

  return v1.size();

}

float dot_product(const float* v1, const float* v2, size_t n, float* sk)
{
  float result = 0.0f;

  for (size_t i = 0; i < n; ++i) {
    float acc = v1[i] * v2[i];
    result += acc;
    sk[i] = result;
  }
  return result;
}

int main(int argc, char **argv)
{
  vector<float> v1, v2;
  int vec_size = loadVectors(v1, v2);
  cout << "Vector size: " << vec_size << endl;

  vector<float> sk(vec_size, 0.0f);
  float result = dot_product(v1.data(), v2.data(), v1.size(), sk.data());
  cout << fixed << setprecision(10);
  cout << "Dot Product Result: " << result << endl;

  {
    FILE *out = fopen("sk.out", "w");
    if (out == NULL) {
      cerr << "Error opening sk.out for writing" << endl;
      exit(-1);
    }
    for (size_t i = 0; i < sk.size(); ++i) {
      fprintf(out, "%.10f\n", sk[i]);
    }
    fclose(out);
    cout << "Wrote " << sk.size() << " partial results to sk.out" << endl;
  }

  return 0;


}
