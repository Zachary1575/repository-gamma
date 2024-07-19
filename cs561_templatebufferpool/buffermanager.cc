#include <sstream>
#include <iostream>
#include <cstdio>
#include <sys/time.h>
#include <cmath>
#include <unistd.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <chrono>

#include "args.hxx"
#include "parameter.h"
#include "executor.h"
#include "workload_generator.h"

using namespace std;
using namespace bufmanager;

int parse_arguments(int argc, char *argvx[], Simulation_Environment* _env);
void printParameters(Simulation_Environment* _env);
int runWorkload(Simulation_Environment* _env);

std::chrono::steady_clock::time_point globalStart; // Global start point
int elapsed_time;

int main(int argc, char *argvx[])
{
  Simulation_Environment* _env = Simulation_Environment::getInstance();

  // Take input parameters
  if (parse_arguments(argc, argvx, _env)){
    exit(1);
  }
  
  // Print parameters
  printParameters(_env);
  if (_env->num_operations > 0) 
  {
    std::cerr << "Generating the workload... " << std::endl << std::flush;  
    // Generate Workload
    WorkloadGenerator workload_generator;
    workload_generator.generateWorkload();

    std::cerr << "Issuing operations ... " << std::endl << std::flush;  
    // Execute Workload
    int s = runWorkload(_env); 
  }

  // Print Different Statistics
  Buffer::printStats(elapsed_time);
  return 0;
}

// Runs the Workload & Simulation Environment
int runWorkload(Simulation_Environment* _env) {

  // This method opens the workload file and according to the workload calls executor's read() or write() method
  Buffer* buffer_instance = Buffer::getBufferInstance(_env);
  bufmanager::WorkloadExecutor workload_executer;
  ifstream workload_file;
  workload_file.open("workload.txt");

  // auto& bufferpool = buffer_instance->bufferpool_LRU; // Incase for debug

  assert(workload_file);

  char instruction; // Instruction Buffer
  int pageId; // Page ID Var
  int offset; // Offset Var
  string tmp_new_entry;

  globalStart = std::chrono::steady_clock::now(); // Timer Start
  while(!workload_file.eof()) {
    workload_file >> instruction >> pageId;
    switch (instruction)
    {
        case 'R': // Read Instruction
          workload_file >> offset;
          workload_executer.read(buffer_instance, pageId, offset, _env->algorithm);
          break;

        case 'W': // Write Instruction
          workload_file >> offset >> tmp_new_entry;
          workload_executer.write(buffer_instance, pageId, offset, tmp_new_entry, _env->algorithm);
          tmp_new_entry.clear();
          break;
    }
    instruction='\0';
  }

  // Stop the Clock
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - globalStart).count();
  std::cout << "Elapsed time: " << elapsed << " ms" << std::endl;
  elapsed_time = elapsed;

  Buffer::printBufferStats(buffer_instance);

  return 1;
}


int parse_arguments(int argc,char *argvx[], Simulation_Environment* _env) {

  // This method is solely for parsing inputs. If you need more inputs you can add them here accordingly

  args::ArgumentParser parser("Buffer Manager Emulator", "");

  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::Group group1(parser, "Optional parameters:", args::Group::Validators::DontCare);

  args::ValueFlag<int> buffer_size_in_pages_cmd(group1, "b", "Size of Buffer in terms of pages [def: 100]", {'b', "buffer_size_in_pages"});
  args::ValueFlag<int> disk_size_in_pages_cmd(group1, "n", "Size of Disk in terms of pages [def: 10000]", {'n', "disk_size_in_pages"});
  args::ValueFlag<int> entry_size_in_bytes_cmd(group1, "e", "Entry size in bytes [def: 128] (for simplicity, restrict it to be lower than 4096)", {'e', "entry_size_in_bytes"});
  args::ValueFlag<int> num_operations_cmd(group1, "x", "Total number of operations to be performed [def: 10000]", {'x', "num_operations"});
  args::ValueFlag<int> perct_reads_cmd(group1, "r", "Percentage of read in workload [def: 70.0%]", {'r', "perct_reads"});
  args::ValueFlag<int> verbosity_cmd(group1, "v", "The verbosity level of execution [0,1,2; def:0]", {'v', "verbosity"});
  args::ValueFlag<int> algorithm_cmd(group1, "a", "Algorithm of page eviction [1: LRU, 2: CFLRU, 3: LRU-WSR, default: 0 (None Specified)]", {'a', "algorithm"}); //Added description of algorithms implemented
  args::ValueFlag<int> skewed_perct_cmd(group1, "s", "Skewed distribution of operation on data [s% r/w on d% data, def: 100]", {'s', "skewed_perct"});
  args::ValueFlag<int> skewed_data_perct_cmd(group1, "d", "Skewed distribution of operation on data [s% r/w on d% data, def: 100]", {'d', "skewed_data_perct"});
  args::Flag pin_mode_cmd(group1, "p", "Enabled the pin mode", {'p', "pin_mode"});
  args::Flag simulation_on_disk_cmd(group1, "simulation_on_disk", "Enable simulation on disk", {"simulation_on_disk"});

  try
  {
      parser.ParseCLI(argc, argvx);
  }
  catch (args::Help&)
  {
      std::cout << parser;
      exit(0);
      // return 0;
  }
  catch (args::ParseError& e)
  {
      std::cerr << e.what() << std::endl;
      std::cerr << parser;
      return 1;
  }
  catch (args::ValidationError& e)
  {
      std::cerr << e.what() << std::endl;
      std::cerr << parser;
      return 1;
  }

  _env->buffer_size_in_pages = buffer_size_in_pages_cmd ? args::get(buffer_size_in_pages_cmd) : 100;
  _env->disk_size_in_pages = disk_size_in_pages_cmd ? args::get(disk_size_in_pages_cmd) : 10000;
  _env->entry_size = disk_size_in_pages_cmd ? args::get(entry_size_in_bytes_cmd) : 128;
  _env->num_operations = num_operations_cmd ? args::get(num_operations_cmd) : 10000;
  _env->perct_reads = perct_reads_cmd ? args::get(perct_reads_cmd) : 70.0;
  _env->perct_writes = 100.0 - _env->perct_reads;
  _env->verbosity = verbosity_cmd ? args::get(verbosity_cmd) : 0;
  _env->algorithm = algorithm_cmd ? args::get(algorithm_cmd) : 0; 
  _env->skewed_perct = skewed_perct_cmd ? args::get(skewed_perct_cmd) : 100;
  _env->skewed_data_perct = skewed_data_perct_cmd ? args::get(skewed_data_perct_cmd) : 100;
  _env->pin_mode = pin_mode_cmd ? args::get(pin_mode_cmd) : false;
  _env->simulation_on_disk = simulation_on_disk_cmd ? args::get(simulation_on_disk_cmd) : false;
  
  return 0;
}

void printParameters(Simulation_Environment* _env) {
  //if (_env->verbosity >= 1) 
  std::cout << "b\t n\t e\t x\t r\t r'\t a\t s\t d\t pin\t simulation_on_disk  \n";
  std::cout << _env->buffer_size_in_pages << "\t ";
  std::cout << _env->disk_size_in_pages << "\t ";  
  std::cout << _env->entry_size << "\t ";  
  std::cout << _env->num_operations << "\t ";
  std::cout << _env->perct_reads << "\t ";
  std::cout << _env->perct_writes << "\t ";
  std::cout << _env->algorithm << "\t ";
  std::cout << _env->skewed_perct << "\t ";
  std::cout << _env->skewed_data_perct << "\t ";
  std::cout << (_env->pin_mode ? "true" : "false") << "\t ";
  std::cout << (_env->simulation_on_disk ? "true" : "false") << "\t ";

  std::cout << std::endl;
}