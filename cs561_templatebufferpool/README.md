# CS 561: Data Systems Architecture - TemplateBufferpool


## About

TemplateBufferpool is a simple template for the students of CS 561. It can be used for the systems projects where the student need
to implement a simple buffermanage of a DBMS. Note that this is a simple template, it is
not extensive, rather it is meant to help guide you on what we expect when
you implement the bufferpool. You can use this as base code or start from scratch.


## Requirements

You will need the following on your system (or alternatively develop on the
CSA machines). Note that, we are considering a linux-based system. But the code should work in other 
environments as well provided that your system has all the dependencies supported.

    1. Make
    2. C++ Compiler


## Usage

You can clean using


```bash
make clean
```

Afterwards, build using


```bash
make
```

The executable should be generated in the directory. Currently this template generates a workload and writes the workload
in a file named 'workload.txt'. It then reads this file and issues read/write requests based on the workload. An example command can be: 

```bash
./buffermanager -b 150 -n 1500 -x 7500 -r 60 -a 1 -s 90 -d 10
```

This generates a workload of 7500 operations with 60% reads where the bufferpool size is 150 pages and disk size is 1500 pages. 
The workload is skewed (90% operations are on 10% data). The parameter 'a' shall determine the eviction policy (LRU/LRU-WSR).
Currently, in the program all the parameters default value is set. So, running only ./buffermanager will generate a default workload.
You can view the default values by running the following command

```bash
./buffermanager -h
```
In this template, we have the following parameters. However, you can/should add parameters as you dig deeper into the implementation.
```bash
int buffer_size_in_pages;	// b
int disk_size_in_pages;   	// n
int num_operations;    		// x
int entry_size;                 // e
int perct_reads;       		// r
float skewed_perct;      	// s
float skewed_data_perct; 	// d
int algorithm;         		// a
int verbosity;         		// v
bool pin_mode;   		// enable pin_mode by adding "--pin_mode"
bool simulation_on_disk;        // enable simulation on disk by adding "--simulation_on_dsik"
```
With simulation on disk, we will generate a raw database file named by "rawdata_database.dat", fulfilled by specified number of pages.
Every read operation should read the specific page, go to the corresponding offset according to the workload, find out the whole entry
as required. Similarly, every write option should update the corresponding entry with the new entry.
Currently this implementation has no bufferpool and no way to simulate read/writes. 
Your job is to implement them and implement three eviction policies.


## Contact

If you have any questions please feel free to see Papon in office hours, or
email him at papon@bu.edu.
