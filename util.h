#include <sstream>
#include <string>
using namespace std;

string intToString(int i)
{
  stringstream ss;
  ss << i;
  string s;
  ss >> s;
  return s;
}

string normalize(string s, int n)
{
  int size = s.size();
  for (int i = size; i < n; i++)
    s = "0" + s;
  return s;
}

string normInt(int i, int n)
{
  return normalize(intToString(i), n);
}