#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include "File.h"
using namespace std;

void test(bool cleanup = true)
{
  bool failed;
  vector<string> filenames = getFilenames("client");

  for (auto file : filenames)
  {
    string outputFile = file + "_output.txt";
    string command = "diff client/" + file + " server/" + file + " > " + outputFile;
    system(command.data());
    File f;
    f.setReadFilename(file + "_output.txt");

    if (!f.endReached())
    {
      cout << "Error in file <" << file << "> \n";
    }
    if (cleanup)
    {
      command = "rm -f " + outputFile;
      system(command.data());
    }
  }
}