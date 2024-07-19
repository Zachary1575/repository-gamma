
#ifndef PARAMETER_H_
#define PARAMETER_H_
 

#include <iostream>

#define PAGE_SIZE 4096

using namespace std;

class Simulation_Environment
{
private:
  Simulation_Environment(); 
  static Simulation_Environment *instance;

public:
  static Simulation_Environment* getInstance();

  int buffer_size_in_pages;   // b
  int disk_size_in_pages;     // n
  int entry_size;        // e

  int num_operations;    // x
  int perct_reads;       // r
  int perct_writes;      // w 

  float skewed_perct;      //s
  float skewed_data_perct; //d

  bool pin_mode;   

  int verbosity;         // v
  int algorithm;         // a

  bool simulation_on_disk;

};

#endif /*PARAMETER_H_*/

