
#include <iostream>
#include <cmath>
#include <sys/time.h>
#include "parameter.h"

/*Set up the singleton object with the experiment wide options*/
Simulation_Environment* Simulation_Environment::instance = 0;

Simulation_Environment::Simulation_Environment() 
{
  int buffer_size_in_pages = 100;
  int disk_size_in_pages = 10000;
  int entry_size = 128;

  int num_operations = 10000;
  float perct_reads = 70;
  float perct_writes = 30;
  int algorithm = 1;

  float skewed_perct = 100;
  float skewed_data_perct = 100;

  int verbosity = 0;
  bool pin_mode = false;

  bool simulation_on_disk = false;
}

Simulation_Environment* Simulation_Environment::getInstance()
{
  if (instance == 0)
    instance = new Simulation_Environment();

  return instance;
}
