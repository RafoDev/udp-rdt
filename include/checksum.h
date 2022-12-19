#include <iostream>
#include <string>
using namespace std;

int checkSum(string &data)
{
  int acc = 0;
  for (auto &i : data)
  {
    acc = (acc + i) % 7;
  }
  return acc % 7;
}