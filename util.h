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
int stringToInt(string s)
{
  stringstream ss;
  ss << s;
  int i;
  ss >> i;
  return i;
}

string normalize(string s, int n)
{
  int size = s.size();
  for (int i = size; i < n; i++)
    s = "0" + s;
  return s;
}
string normalizeMessage(string s, int n)
{
  int size = s.size();
  for (int i = size; i < n; i++)
    s = s + '0';
  return s;
}

string normalize(int i, int n)
{
  return normalize(intToString(i), n);
}