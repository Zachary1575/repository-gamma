#include <sstream>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <cstring>

#include "workload_generator.h"
#include "parameter.h"
using namespace std;


const char value_alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

// I think this causes Buffer overruns
// std::string get_random_entry(const int entry_size) {
//     char *s = new char[(int)entry_size];
//     for (int i = 0; i < entry_size; ++i) {
//         s[i] = value_alphanum[rand() % (sizeof(value_alphanum) - 1)];
//     }
//     s[entry_size] = '\0';
//     return s;
// }

std::string get_random_entry(const int entry_size) {
    std::string s(entry_size, '\0');
    for (int i = 0; i < entry_size; ++i) {
        s[i] = value_alphanum[rand() % (sizeof(value_alphanum) - 1)];
    }
    return s;  // s is automatically null-terminated by std::string
}


void fill_raw_data(long num_pages, int entry_size) {
  ofstream raw_data_file("rawdata_database.dat", std::ofstream::binary);
  int entries_per_page = PAGE_SIZE/entry_size;
  char buffer[PAGE_SIZE];
  string tmp_entry;
  for(long i = 0; i < num_pages; i++) {
    memset(buffer, 0, PAGE_SIZE);
    for(int j = 0; j < entries_per_page; j++) {
    	tmp_entry = get_random_entry(entry_size);
      strncpy(buffer + j * entry_size, tmp_entry.c_str(), entry_size);
    	// strcpy(buffer+j*entry_size, tmp_entry.c_str());
    	tmp_entry.clear();
    }
    raw_data_file.write((char*)&buffer[0], PAGE_SIZE);
  }
  raw_data_file.flush();
  raw_data_file.close();
}

int WorkloadGenerator::generateWorkload() {
  
  // This method generates the workload
  // However, this considers pinning is disabled.
  // If you want to enforce pin/unpin, you might need to modify this file

  ofstream workload_file;
  workload_file.open("workload.txt");
  Simulation_Environment* _env = Simulation_Environment::getInstance();

  if (_env->simulation_on_disk) {
    // With this mode on, fill the database file "rawdata_database.dat"
    // with specified number of pages using fixed-length entries
    fill_raw_data(_env->disk_size_in_pages, _env->entry_size);
  }

  int pageId;
  //int endPageId = _env->disk_size_in_pages*(_env->skewed_data_perct/100); // This is broken somehow
  int endPageId = _env->disk_size_in_pages*(100/100);
  int entries_per_page = PAGE_SIZE/_env->entry_size;
  int offset; // the ordinal number of a entry, also known as the offset
  string tmp_new_entry;
  //cout << "Printing Workload..." << endl;
  for (long i = 0; i < _env->num_operations; i++) {
    //srand(time(0));

    int typeDecider = rand() % 100;  
    int skewed = rand() % 100;
 
    // if (skewed < _env->skewed_perct) // Causes PageID Bugs, Dunno why...
    if (skewed < 100)
    {
      pageId = rand() % endPageId;
    }
    else
    {
      pageId = rand() % (_env->disk_size_in_pages - endPageId) + endPageId;
    }

    offset = rand() % entries_per_page;


    if(typeDecider < _env->perct_reads)
    {
      workload_file << "R " << pageId << " " << offset << std::endl;
    }
    else 
    {
      tmp_new_entry = get_random_entry(_env->entry_size);
      workload_file << "W " << pageId << " " << offset << " " <<
	      tmp_new_entry << std::endl;
      tmp_new_entry.clear();
    }
  }
  
  workload_file.close();

  return 1;
}
