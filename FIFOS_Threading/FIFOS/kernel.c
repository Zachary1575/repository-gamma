#include<multiboot.h>

// ========================== Print functions =================================
void putc(char c, char attr) {
  char* vga_memory = (char*)0xB8000; // VGA Memory
  static int offset = 0;

  if (c == '\n') {
    int line_size = 80 * 2;
    offset += line_size - (offset % line_size); // We literally move the cursor across the terminal
  } else {
    vga_memory[offset] = c; // A byte for a character
    vga_memory[offset + 1] = attr; // A byte for the dispkay attributed
    offset += 2; // 2 bytes offset...
  }

}

// Fills the screen with spaces and resets cursor to (0,0)
void clear_screen() {
  char* vga_memory = (char*)0xB8000;

  // Assume a screen size of 80 by 25
  int i;
  for (i = 0; i < (80 * 25); i++) {
    vga_memory[i * 2] = ' ';
    vga_memory[i * 2 + 1] = 0x07; // Light gray on Black
    
  }

}

void print(const char* str, char attr) {
  // Iterate over the char* str, attr is just the VGA display attributes
  while(*str) { 
    putc(*str++, attr);
  }
}

void print_hex_64(unsigned long long value, char attr) {
  char hex_symbols[] = "0123456789ABCDEF";
  putc('0', attr);
  putc('x', attr);

  int printed = 0; // Again, Flag to determine whether we've started printing non-zero values
  
  int i;
  for (i = sizeof(value) * 8 - 4; i >= 0; i -= 4) {
    unsigned char nibble = (value >> i) & 0x0F;

    if (nibble || printed || i ==0) {
      putc(hex_symbols[nibble], attr);
      printed = 1;
    }

  }
}

void print_hex(unsigned long value, char attr) {
  putc('0', attr);
  putc('x', attr);

  int printed = 0; // Keep track whether we've printed any non-zero digits yet

  int i;
  for (i = sizeof(value) * 8 - 4; i >= 0; i -= 4) { // Subtract by 4 as we are trying to get nibbles
    unsigned char nibble = (value >> i) & 0x0F; // We shift by i values and AND with 0x0F;
    
    if (nibble != 0 || printed) {
      printed = 1;

      // A digit between 0-9 
      if (nibble < 10) {
	putc('0' + nibble, attr);
      } else { // A ASCII letter between A-F
	putc('A' + nibble - 10, attr);
      }
    }
      
  }

  if (!printed) {
    // If we just get a value of 0, print a single 0
    putc('0', attr);
  }

}

// =============  64bit to 32bit CONVERSION SHENANIGANS ====================
char* uint_to_string(unsigned int value, char* buffer) {
  int index = 0;

  if (value == 0) {
    buffer[index++] = '0';
  }

  while(value) {
    buffer[index++] = '0' + (value % 10);
    value /= 10;
  }
  buffer[index] = '\0';
  
  // Reverse the string to get the correct order
  int i, j;
  for (i = 0, j = index - 1; i < j; ++i, --j) {
    char tmp = buffer[i];
    buffer[i] = buffer[j];
    buffer[j] = tmp;
  }

  return buffer;

}

// String functions
void strcpy(char *dest, const char* src) {
  while((*dest++ = *src++));
}

void strcat(char* dest, const char* src) {
  while(*dest) {
    dest++;
  }

  while((*dest++ = *src++));
}


// Function to convert numbers to string for printing
char* ullong_to_string(unsigned long long value, char* buffer) {  
  // Edge case when the value is just 0, dont proc loop
  if (value == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
    
    return buffer;
  }

  unsigned int upper = (unsigned int)(value >> 32);
  unsigned int lower = (unsigned int)(value & 0xFFFFFFFF);

  char upper_buffer[11] = {0};
  char lower_buffer[11] = {0};

  if (upper > 0) {
    uint_to_string(upper, upper_buffer);
    uint_to_string(lower, lower_buffer);

    strcpy(buffer, upper_buffer);
    strcat(buffer, lower_buffer);
  } else {
    uint_to_string(lower, buffer);
  }

  return buffer;
}

// =============================================================================

// We have the multiboot info structure as parameter
// We have the magic number as the parameter
void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
  
  // Clear the screen
  clear_screen();

  if (magic != 0x2BADB002) {
    char* error1 = "Kernel Panick: Magic Number Not Detected!";
    print(error1, 0x0F);

    return;	
  }
	
  /* 
   * We need to check mbd->flags to verify that but 0 is set, then we can safely refer mbd->mem_lower for convetional memory
   * and mbd->mem_upper for high memory. We need to determine the contiguous memory size.
   */
  if (!(mbd->flags >> 6 & 0x1)) {
    char* error2 = "Kernel Panick: Invalid Memory Map given by GRUB Bootloader!";
    print(error2, 0x0F);

    return;
  }	

  // If we pass all the memory checks, we print a nice message
  print("Probing Memory Map...\n\n", 0x0F);
  
  print("Note: All printed addresses are the same length, anything after is just 0's.\n", 0x0F);
  unsigned long long total_avail_memory = 0;
  char buffer[21] = {0}; // Create a buffer for unsigned long long

  /* Loop through the memory map and check on the values */
  int i;
  for(i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
    // mmmt is defined as the mmap_addr + i, we are incrementing by page of memory blocks
    multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*) (mbd->mmap_addr + i);
    
    // Get the base address
    unsigned long long addr = mmmt-> addr;
    unsigned long type = mmmt-> type;
    unsigned long long length = mmmt->len;

    unsigned long long end_addr = addr + length;

    // We print our the address ranges and the status
    print("Address range[", 0x0F);
    print_hex_64(addr, 0x0F);
    print(" : ", 0x0F);
    print_hex_64(end_addr, 0x0F);
    print("] status:", 0x0F);
    print_hex(type, 0x0F);
    print("\n", 0x0F);
    
    // OSdev warns that available memory includes what the kernel may do some things on it
    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
      total_avail_memory += mmmt->len;
    }
  }
  
  print("\n", 0x0F);
  print("MemOS: Welcome *** System Memory is: ", 0x0F);
  print(ullong_to_string(total_avail_memory, buffer), 0x0F);
  print(" Bytes\n", 0x0F);
	
  return;
}
