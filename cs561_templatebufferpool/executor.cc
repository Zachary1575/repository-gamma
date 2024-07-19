#include <iostream>
#include <cmath>
#include <sys/time.h>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "parameter.h"
#include "executor.h"

using namespace std;
using namespace bufmanager;

// ---------------- Buffer ---------------- 

Buffer *Buffer::buffer_instance;
long Buffer::max_buffer_size = 0;
int Buffer::buffer_hit = 0;
int Buffer::buffer_miss = 0;
int Buffer::read_io = 0;
int Buffer::write_io = 0;

Buffer::Buffer(Simulation_Environment *_env)
: bufferpool_LRU(determineBufferSize(_env))
{
  algorithm = _env->algorithm;
  simulation_disk = _env->simulation_on_disk;
  entry_size = _env->entry_size;

  // I keep forgetting what each is each
  cout << "****************************************************************" << endl;
  cout << "Creating Buffer Pool Instance!" << endl;
  cout << "****************************************************************" << endl;
  cout << "Buffer Size in Pages: " << _env->buffer_size_in_pages << endl;
  cout << "Disk Size in Pages: " << _env->disk_size_in_pages << endl;
  cout << "Entry Size: " << _env->entry_size << endl;

  cout << "Number of Operations: " << _env->num_operations << endl;
  cout << "Percentage Reads: " << _env->perct_reads << '%' << endl;
  cout << "Percentage Writes: " << _env->perct_writes << '%' << endl;

  cout << "Pin Mode: " << ( _env->pin_mode ? "Enabled" : "Disabled") << endl;
  cout << "Verbosity Level: " << _env->verbosity << endl;
  cout << "Algorithm: " << _env->algorithm << endl;

  cout << "Simulation on Disk: " << (_env->simulation_on_disk ? "Yes" : "No") << endl;
}

// Change Made - buffer size shoould correctly initialize for all algorithms used - RGVA
int Buffer::determineBufferSize(Simulation_Environment* _env) 
{
  return _env->buffer_size_in_pages;
}


Buffer *Buffer::getBufferInstance(Simulation_Environment *_env)
{
  if (buffer_instance == 0)
    printf("%s\n", "No Buffer Pool Detected. Creating a New Buffer Pool.");
    buffer_instance = new Buffer(_env);
    cout << "****************************************************************" << endl;
    std::cout << "The address of Buffer is: " << buffer_instance << std::endl;
    cout << "****************************************************************" << endl;
  return buffer_instance;
}

// Statistics with Simulation Environments
int Buffer::printStats(int elapsed_time)
{
  Simulation_Environment* _env = Simulation_Environment::getInstance();
  cout << "\n\n********************** Overall Statistics **********************" << endl;
  cout << "Printing Stats..." << endl;
  cout << "Number of operations: " << _env->num_operations << endl;
  cout << "Buffer Hit: " << buffer_hit << endl;
  cout << "Buffer Miss: " << buffer_miss << endl;
  cout << "DISK SIMULATION: " << (_env->simulation_on_disk ? "Yes" : "No (Disk & Write IO should be '0')") << endl;
  cout << "Read IO: " << read_io << endl;
  cout << "Write IO: " << write_io << endl; 
  cout << "Global Clock: " << elapsed_time << "ms" << endl;
  cout << "****************************************************************" << endl;
  return 0;
}

// Print specific
int Buffer::printBufferStats(Buffer* buffer_instance)
{
  auto& bufferpool = buffer_instance->bufferpool_LRU;
  cout << "\n\n********************** Buffer Statistics ***********************" << endl;
  cout << "Printing Buffer Stats..." << endl;
  cout << "Eviction Call Count: " << bufferpool.algorithm_eviction_count << endl; // (How many times the cache evicted data ex. Calling LRU)
  cout << "****************************************************************" << endl;
  return 0;
}

// ---------------- Workload Executer ---------------- 
int WorkloadExecutor::read(Buffer* buffer_instance, int pageId, int offset, int algorithm)
{
  printf("\nRead Instruction Issued: (pageID~%d, offset~%d, algorithm~%d)\n", pageId, offset, algorithm);
  if (algorithm == 1) { // LRU
    auto& bufferpool = buffer_instance->bufferpool_LRU;
    auto& disk_sim = buffer_instance->simulation_disk;

    bufferpool.instructions_seen++;
    printf("Total Istructions Seen is: %d!\n", bufferpool.instructions_seen);
    
    // Search in Hashmap to see if that pageID is made...
    Page* cache_page = bufferpool.lookupInBuffer(pageId);
    if (cache_page == nullptr) // If we yield no such page in the Bufferpool
    {
      Buffer::buffer_miss++; // Buffer MISS
      if (disk_sim) // DISK SIMULATION
      {
        string datFilePath = "./rawdata_database.dat";
        std::fstream file(datFilePath, std::ios::in | std::ios::out | std::ios::binary);
        if (!file) 
        {
          std::cerr << "Unable to open file." << std::endl;
          return 1;
        }
        int byteStart = pageId * 4096; // 4KB page
        file.seekg(byteStart, std::ios::beg);

        if (!file) 
        {
            std::cerr << "Seek failed. Check if the position is beyond the file size." << std::endl;
            file.close();
            return 1;
        }
        Page bufferPage = Page(pageId);

        file.read(bufferPage.getPageContent(), 4096); // Read the database entries into a buffer page, 4096 because of page size
        // bufferPage.printAllPageEntries(); // For Debug

        if (!file) {
          std::cerr << "Read failed. Check if there are enough bytes left in the file." << std::endl;
          file.close();
          return 1;
        }
        file.close();
        bufferpool.prepend(bufferPage, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
        Buffer::read_io++;
        return 1;
      } 
        else // NOT DISK SIMULATION
      {
        Page x = Page(pageId); // Create a dummy page | Here we simulate us going into disk and getting the associated page.
        bufferpool.prepend(x, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
        //printf("[DEBUG] "); // Debug
        //bufferpool.display(); // Will slowdown and effect Runtime
      }
      return 1;
    } 
      else 
    {
      Buffer::buffer_hit++; // Buffer HIT
      if (disk_sim) // DISK SIMULATION
      {
        Page hitBufferPage = bufferpool.getPage(pageId)->data;
        char* buffer = hitBufferPage.getPageContent();
        auto& entry_size = buffer_instance->entry_size;
        string entry = "";
        for (int i = offset; i < offset + entry_size; i++) // Runtime is O(k) where k is the size of the entry.
        {
          entry = entry + buffer[i];
        }
        printf("[READ CACHE HIT | DISK SIMULATION] Page & Offset Query: %d, %d. Returned Entry: %s\n", pageId, offset, entry.c_str());
        return 1;
      } 
        else // NOT DISK SIMULATION
      { 
        printf("[READ CACHE HIT | NOT DISK SIMULATION] Getting Entry and Pretending to Return Data to Satisfy Query: %d, %d.\n", pageId, offset);
        //printf("[DEBUG] "); // Debug
        //bufferpool.display(); // Will slowdown and effect Runtime
      }
    }
  }

  if (algorithm == 2) { // CFLRU
    auto& bufferpool = buffer_instance->bufferpool_LRU;
    auto& disk_sim = buffer_instance->simulation_disk;

    bufferpool.instructions_seen++;
    printf("Total Istructions Seen is: %d!\n", bufferpool.instructions_seen);
    
    Page* cache_page = bufferpool.lookupInBuffer(pageId);
    if (cache_page == nullptr) {
      Buffer::buffer_miss++;

      if (bufferpool.current_page_cnt >= bufferpool.page_capacity) {
        int evictionCount = bufferpool.cflruEvictPage(disk_sim, Buffer::buffer_miss); // Evict pages using CFLRU logic
        bufferpool.current_page_cnt = bufferpool.current_page_cnt - evictionCount;
      }
      if (disk_sim) {
        Page newPage = bufferpool.simulateDiskRead(pageId);
        bufferpool.prepend(newPage, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm);
        Buffer::read_io++;
      } else {
        Page x = Page(pageId); // Create a dummy page | Here we simulate us going into disk and getting the associated page.
        bufferpool.prepend(x, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
      }      
    } else {
      Buffer::buffer_hit++;
      if (disk_sim) {
        Page hitBufferPage = bufferpool.getPage(pageId)->data;
        char* buffer = hitBufferPage.getPageContent();
        auto& entry_size = buffer_instance->entry_size;
        string entry = "";
        for (int i = offset; i < offset + entry_size; i++) // Runtime is O(k) where k is the size of the entry.
        {
          entry = entry + buffer[i];
        }
        printf("[READ CACHE HIT | CFLRU | DISK SIMULATION] Page & Offset Query: %d, %d. Returned Entry: ...\n", pageId, offset);
      } else {
        printf("[READ CACHE HIT | CFLRU | NOT DISK SIMULATION] Getting Entry and Pretending to Return Data to Satisfy Query: %d, %d.\n", pageId, offset);
      }
    }
  }

  if (algorithm == 3) { // LRU-WSR
    auto& bufferpool = buffer_instance->bufferpool_LRU;
    auto& disk_sim = buffer_instance->simulation_disk;

    bufferpool.instructions_seen++;
    printf("Total Istructions Seen is: %d!\n", bufferpool.instructions_seen);
    
    // Search in Hashmap to see if that pageID is made...
    Page* cache_page = bufferpool.lookupInBuffer(pageId);
    if (cache_page == nullptr) // If we yield no such page in the Bufferpool
    {
      Buffer::buffer_miss++; // Buffer MISS
      if (disk_sim) // DISK SIMULATION
      {
        string datFilePath = "./rawdata_database.dat";
        std::fstream file(datFilePath, std::ios::in | std::ios::out | std::ios::binary);
        if (!file) 
        {
          std::cerr << "Unable to open file." << std::endl;
          return 1;
        }
        int byteStart = pageId * 4096; // 4KB page
        file.seekg(byteStart, std::ios::beg);

        if (!file) 
        {
            std::cerr << "Seek failed. Check if the position is beyond the file size." << std::endl;
            file.close();
            return 1;
        }
        Page bufferPage = Page(pageId);

        file.read(bufferPage.getPageContent(), 4096); // Read the database entries into a buffer page, 4096 because of page size
        // bufferPage.printAllPageEntries(); // For Debug

        if (!file) {
          std::cerr << "Read failed. Check if there are enough bytes left in the file." << std::endl;
          file.close();
          return 1;
        }
        file.close();
        bufferpool.prepend(bufferPage, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
        Buffer::read_io++;
        return 1;
      } 
        else // NOT DISK SIMULATION
      {
        Page x = Page(pageId); // Create a dummy page | Here we simulate us going into disk and getting the associated page.
        bufferpool.prepend(x, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
        //printf("[DEBUG] "); // Debug
        //bufferpool.display(); // Will slowdown and effect Runtime
      }
      return 1;
    } 
      else 
    {
      Buffer::buffer_hit++; // Buffer HIT
      if (disk_sim) // DISK SIMULATION
      {
        Page hitBufferPage = bufferpool.getPage(pageId)->data;
        char* buffer = hitBufferPage.getPageContent();
        auto& entry_size = buffer_instance->entry_size;
        string entry = "";
        for (int i = offset; i < offset + entry_size; i++) // Runtime is O(k) where k is the size of the entry.
        {
          entry = entry + buffer[i];
        }
        printf("[READ CACHE HIT | DISK SIMULATION] Page & Offset Query: %d, %d. Returned Entry: %s\n", pageId, offset, entry.c_str());
        return 1;
      } 
        else // NOT DISK SIMULATION
      { 
        printf("[READ CACHE HIT | NOT DISK SIMULATION] Getting Entry and Pretending to Return Data to Satisfy Query: %d, %d.\n", pageId, offset);
        //printf("[DEBUG] "); // Debug
        //bufferpool.display(); // Will slowdown and effect Runtime
      }
    }
  }

  return 1;
}

int WorkloadExecutor::write(Buffer* buffer_instance, int pageId, int offset, const string new_entry, int algorithm)
{
  printf("\nWrite Instruction Issued: (pageID~%d, offset~%d, entry~%s, algorithm~%d)\n", pageId, offset, new_entry.c_str(), algorithm);
    if (algorithm == 1) { // LRU
      auto& bufferpool = buffer_instance->bufferpool_LRU;
      auto& disk_sim = buffer_instance->simulation_disk;

      bufferpool.instructions_seen++;
      printf("Total Istructions Seen is: %d!\n", bufferpool.instructions_seen);
      
      // Search in Hashmap to see if that pageID is made...
      Page* cache_page = bufferpool.lookupInBuffer(pageId);
      if (cache_page == nullptr) // If we yield no such page in the Bufferpool
      {
        Buffer::buffer_miss++; // Buffer MISS
        if (disk_sim) // DISK SIMULATION
        {
          string datFilePath = "./rawdata_database.dat";
          std::fstream file(datFilePath, std::ios::in | std::ios::out | std::ios::binary);
          if (!file) 
          {
            std::cerr << "Unable to open file." << std::endl;
            return 1;
          }
          int byteStart = pageId * 4096; // 4KB page
          file.seekg(byteStart, std::ios::beg);

          if (!file) 
          {
              std::cerr << "Seek failed. Check if the position is beyond the file size." << std::endl;
              file.close();
              return 1;
          }
          Page bufferPage = Page(pageId);

          file.read(bufferPage.getPageContent(), 4096); // Read the database entries into a buffer page, 4096 because of page size
          // bufferPage.printAllPageEntries(); // For Debug

          if (!file) {
            std::cerr << "Read failed. Check if there are enough bytes left in the file." << std::endl;
            file.close();
            return 1;
          }
          file.close();

          // We then write the requested entry into the page
          char* buffer = bufferPage.getPageContent();
          auto& entry_size = buffer_instance->entry_size;
          int entry_cnter = 0;
          for (int i = offset; i < offset + entry_size; i++) // Runtime is O(k) where k is the size of the entry.
          {
            buffer[i] = new_entry[entry_cnter]; // Here we write to our buffer page
            entry_cnter++;
          }
          bufferPage.setDirtyPage(true); // Set the page is dirty as it doesnt reflect the actual database data
          bufferpool.prepend(bufferPage, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
          Buffer::write_io++;
          return 1;
        } 
          else // NOT DISK SIMULATION
        {
          Page x = Page(pageId); // Create a dummy page | Here we simulate us going into disk and getting the associated page.
          // We then write in it... (pretend it does here)
          x.setDirtyPage(true); // Set the page to be dirty
          bufferpool.prepend(x, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
          printf("[DEBUG] "); // Debug
          bufferpool.display(); // Will slowdown and effect Runtime
        }
        return 1;
      } 
        else 
      {
        Buffer::buffer_hit++; // Buffer HIT
        if (disk_sim) // DISK SIMULATION
        {
          Page hitBufferPage = bufferpool.getPage(pageId)->data;
          char* buffer = hitBufferPage.getPageContent();
          auto& entry_size = buffer_instance->entry_size;
          int entry_cnter = 0;
          string entry = "";
          for (int i = offset; i < offset + entry_size; i++) // Runtime is O(k) where k is the size of the entry.
          {
            buffer[i] = new_entry[entry_cnter]; // Here we write to our buffer page
            entry = entry +  buffer[i]; // Just as a sanity check
            entry_cnter++;
          }
          printf("[WRITE CACHE HIT | DISK SIMULATION] Page & Offset Query: %d, %d. WRITTEN & Returned Entry: %s\n", pageId, offset, entry.c_str());
          return 1;
        } 
          else // NOT DISK SIMULATION
        { 
          printf("[WRITE CACHE HIT | NOT DISK SIMULATION] Getting Entry and Pretending to WRITE & Return Data to Satisfy Query: %d, %d.\n", pageId, offset);
          // printf("[DEBUG] "); // Debug
          // bufferpool.display(); // Will slowdown and effect Runtime
        }
      }
    }

    if (algorithm == 2) { // CFLRU
      auto& bufferpool = buffer_instance->bufferpool_LRU;
      auto& disk_sim = buffer_instance->simulation_disk;

      bufferpool.instructions_seen++;
      printf("Total Istructions Seen is: %d!\n", bufferpool.instructions_seen);
      
      Page* cache_page = bufferpool.lookupInBuffer(pageId);
      if (cache_page == nullptr) {
        Buffer::buffer_miss++;
        if (bufferpool.current_page_cnt >= bufferpool.page_capacity) {
          int evictionCount = bufferpool.cflruEvictPage(disk_sim, Buffer::buffer_miss); // Evict pages using CFLRU logic
          bufferpool.current_page_cnt = bufferpool.current_page_cnt - evictionCount;
        }

        if (disk_sim) {
          Page newPage = bufferpool.simulateDiskRead(pageId);
          newPage.setDirtyPage(true);
          bufferpool.prepend(newPage, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm);
          Buffer::write_io++;
        } else {
          Page x = Page(pageId); // Create a dummy page | Here we simulate us going into disk and getting the associated page.
          // We then write in it... (pretend it does here)
          x.setDirtyPage(true); // Set the page to be dirty
          bufferpool.prepend(x, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
        }
      } else {
        Buffer::buffer_hit++;

        if (disk_sim) {
          cache_page->insertEntryIntoPage(offset, new_entry);
          cache_page->setDirtyPage(true);
          printf("[WRITE CACHE HIT | CFLRU | DISK SIMULATION] Page & Offset Query: %d, %d.\n", pageId, offset);
        } else {
          printf("[WRITE CACHE HIT | CFLRU | NOT DISK SIMULATION] Page & Offset Query: %d, %d. WRITTEN & Returned Entry: %s\n", pageId, offset, new_entry.c_str());
        }
      }
    }

    if (algorithm == 3) { // LRU-WSR
      auto& bufferpool = buffer_instance->bufferpool_LRU;
      auto& disk_sim = buffer_instance->simulation_disk;

      bufferpool.instructions_seen++;
      printf("Total Istructions Seen is: %d!\n", bufferpool.instructions_seen);
      
      // Search in Hashmap to see if that pageID is made...
      Page* cache_page = bufferpool.lookupInBuffer(pageId);
      if (cache_page == nullptr) // If we yield no such page in the Bufferpool
      {
        Buffer::buffer_miss++; // Buffer MISS
        if (disk_sim) // DISK SIMULATION
        {
          string datFilePath = "./rawdata_database.dat";
          std::fstream file(datFilePath, std::ios::in | std::ios::out | std::ios::binary);
          if (!file) 
          {
            std::cerr << "Unable to open file." << std::endl;
            return 1;
          }
          int byteStart = pageId * 4096; // 4KB page
          file.seekg(byteStart, std::ios::beg);

          if (!file) 
          {
              std::cerr << "Seek failed. Check if the position is beyond the file size." << std::endl;
              file.close();
              return 1;
          }
          Page bufferPage = Page(pageId);

          file.read(bufferPage.getPageContent(), 4096); // Read the database entries into a buffer page, 4096 because of page size
          // bufferPage.printAllPageEntries(); // For Debug

          if (!file) {
            std::cerr << "Read failed. Check if there are enough bytes left in the file." << std::endl;
            file.close();
            return 1;
          }
          file.close();

          // We then write the requested entry into the page
          char* buffer = bufferPage.getPageContent();
          auto& entry_size = buffer_instance->entry_size;
          int entry_cnter = 0;
          for (int i = offset; i < offset + entry_size; i++) // Runtime is O(k) where k is the size of the entry.
          {
            buffer[i] = new_entry[entry_cnter]; // Here we write to our buffer page
            entry_cnter++;
          }
          bufferPage.setDirtyPage(true); // Set the page is dirty as it doesnt reflect the actual database data
          bufferpool.prepend(bufferPage, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
          Buffer::write_io++;
          return 1;
        } 
          else // NOT DISK SIMULATION
        {
          Page x = Page(pageId); // Create a dummy page | Here we simulate us going into disk and getting the associated page.
          // We then write in it... (pretend it does here)
          x.setDirtyPage(true); // Set the page to be dirty
          bufferpool.prepend(x, pageId, disk_sim, Buffer::buffer_miss, buffer_instance->algorithm); // Insert the page into the Buffer pool
          printf("[DEBUG] "); // Debug
          bufferpool.display(); // Will slowdown and effect Runtime
        }
        return 1;
      } 
        else 
      {
        Buffer::buffer_hit++; // Buffer HIT
        if (disk_sim) // DISK SIMULATION
        {
          Page hitBufferPage = bufferpool.getPage(pageId)->data;
          char* buffer = hitBufferPage.getPageContent();
          auto& entry_size = buffer_instance->entry_size;
          int entry_cnter = 0;
          string entry = "";
          for (int i = offset; i < offset + entry_size; i++) // Runtime is O(k) where k is the size of the entry.
          {
            buffer[i] = new_entry[entry_cnter]; // Here we write to our buffer page
            entry = entry +  buffer[i]; // Just as a sanity check
            entry_cnter++;
          }
          printf("[WRITE CACHE HIT | DISK SIMULATION] Page & Offset Query: %d, %d. WRITTEN & Returned Entry: %s\n", pageId, offset, entry.c_str());
          return 1;
        } 
          else // NOT DISK SIMULATION
        { 
          printf("[WRITE CACHE HIT | NOT DISK SIMULATION] Getting Entry and Pretending to WRITE & Return Data to Satisfy Query: %d, %d.\n", pageId, offset);
          // printf("[DEBUG] "); // Debug
          // bufferpool.display(); // Will slowdown and effect Runtime
        }
      }
    }
  return 1;
}

int WorkloadExecutor::unpin(Buffer* buffer_instance, int pageId)
{
  return -1;   // This is optional I think?
}


// ---------------- Page (4KB) ---------------- 
// I asked Papon about this: 
// We can assume that the entry sizes are within 4KB (it won't spill over a page)
// ---- Page Constructor ----
Page::Page(int pageId)
: pageId(pageId), dirty(false), cold(false)
{
  std::fill_n(pageContent, 4096, '0'); // Fill it up with 0. We know all entries are alphebetical data.
}

// ---- Page Instance Methods ----
char* Page::getPageContent() {
    return pageContent;
}

void Page::printAllPageEntries() 
{
  std::string overall_string = "";
  for (char c: pageContent) {
    if (c != '0') {
      overall_string += c;
    }
  }
  printf("\n\nPAGE %d: %s \n", pageId, overall_string.c_str());
}

void Page::setDirtyPage(bool dirty)
{
  Page::dirty = dirty;
}

bool Page::isDirtyPage() const 
{
  return dirty;
}

int Page::getPageID() const 
{
    return pageId;
}

void Page::insertEntryIntoPage(int offset, string entry) 
{
    if(offset + entry.size() > sizeof(pageContent)) {
        cerr << "Error: Entry exceeds page size limits." << endl;
        return;  // Or handle the error as necessary
    }
    memcpy(pageContent + offset, entry.c_str(), entry.size());
}

void Page::setColdFlag(bool flag) {//LRU-WSR Integration
        this->cold = flag;
}

bool Page:: isCold() const { //LRU-WSR Integration
        return this->cold;
}