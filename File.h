#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
using namespace std;

struct File
{
  string filename;
  int size;
  ifstream file;
  int pivot;
  File(int _pivot = 10) { pivot = _pivot; }
  void setFilename(string _filename)
  {
    filename = _filename;
    if(!std::filesystem::exists(filename)) perror("ERROR, file doesn't exist.\n");
    file.open(filename, ios::binary | ios::ate);
    if (!file)
      size = 0;
    else
      size = file.tellg();
    file.seekg(0, ios::beg);
  }
  string read()
  {
    if (size == 0)
      return "";
    int buffSize = size >= 10 ? 10 : size;
    size -= buffSize;
    vector<char> buffer(buffSize);
    file.read(buffer.data(), buffSize);
    string data(buffer.begin(), buffer.end());
    return data;
  }
  bool endReached()
  {
    return !size;
  }
};

vector<string> getFilenames(string path)
{
  vector<string> ans;
  for (const auto &entry : filesystem::directory_iterator(path))
    ans.push_back(entry.path().filename());
  return ans;
}
int getAmountOfFiles(string path)
{
  int c = 0;
  for (const auto &entry : filesystem::directory_iterator(path))
    c++;
  return c;
}

// int main()
// {

//   File f("file.txt");
//   string buffData = f.read();
//   while(buffData.size()!=0)
//   {
//     cout<<buffData;
//     buffData = f.read();
//   }

//   // ifstream file("file.txt", ios::binary | ios::ate);
//   // if (!file)
//   // {
//   //   cout << "ERROR\n";
//   // }
//   // else
//   // {
//   //   streamsize size = file.tellg();
//   //   streamsize rest = size % 10;
//   //   file.seekg(0, ios::beg);
//   //   // cout << size;
//   //   vector<char> buffer(size);
//   //   vector<char> bufferRest(rest);

//   //   file.read(bufferRest.data(), rest);
//   //   cout << bufferRest.data();
//   //   while (file.read(buffer.data(), 10))
//   //   {
//   //     cout << buffer.data();
//   //   }
//   // }
//   // // vector<char> buffer(size);
//   // file.close();
//   // // if(file.read(buffer.data(), size))
// }