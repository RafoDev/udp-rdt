#include <iostream>
#include <string>
using namespace std;

int checkSum(string &data)
{
  int acc = 0;
  for (auto &i : data)
    acc += i;
  return acc % 7;
}