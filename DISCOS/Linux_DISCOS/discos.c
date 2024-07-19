#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

# define RAMDISK_SIZE (2 * 1024 * 1024) // 2MB or 2,097,152 Bytes

// A note, we pick char* as we want to manipulate the data byte-by-byte
char* ramdisk;

// Our inode type
typedef struct {
	char type[4];
	char size[4];
	char location[40];
	char reserved[16];
} inode_t;

// Our directory entry structure
typedef struct {
	char filename[14];
	uint16_t index_node_number; // Index node number within the inode array. 2 bytes
} dir_entry_t;

// ====== FILE UTILITY ======
// Reads Contigious memory for Debugging
void read_bytes(const inode_t *pointer, size_t num_bytes) {
  // Cast the inode_ptr to a byte pointer to read individual bytes
  const unsigned char *byte_ptr = (const unsigned char *)(pointer);

  size_t max_bytes = sizeof(inode_t);
  size_t num_bytes_to_read = (num_bytes < max_bytes) ? num_bytes : max_bytes;

  size_t i;
  for (i = 0; i < num_bytes_to_read; i++) {
    printf("Byte %zu: %02x\n", i, byte_ptr[i]);
  }

  return;
}

// Allocates the next avalible block of memeory it see's, returns the address if sucessful, if not, you recieve a NULL
char* allocate_block() {
	size_t i;
	for (i = 66816; i < RAMDISK_SIZE; i += 256) {
		if (ramdisk[i] == 00) {
		  return &ramdisk[i]; // Gonna assume its a 4 byte address on 32-bit system
		}
	}

	return NULL;
}

// ======= INIT FUNCS ======
int init_ramdisk() {
	ramdisk = (char *)malloc(RAMDISK_SIZE); // We use vmalloc() in kernel sp
	if (ramdisk == NULL) {
		return -1;
	}
	
	memset(ramdisk, 0, RAMDISK_SIZE);
	return 0; // Success of initalizing the RAMDISK
}

int init_superblock(char* ramdisk, int inode_blck_cnt) {
	// Here we have to set the number of inodes we want to init for the system
	// I didn't add any safety checks for max inodes because I am lazy
	int free_inodes = (inode_blck_cnt * 256) / 64; // Each inode is 64 bytes
	int free_blocks = (RAMDISK_SIZE - ((1 + inode_blck_cnt + 4) * 256)) / 256;

	*(int *)(ramdisk) = free_inodes; // Casts char* pointer to int* and deferences it to store our block info
	*(int *)(ramdisk + 4) = free_blocks; // Remember 4 bytes for an unsigned integer
	
	// We then read from ramdisk to make sure everything is good
	printf("Free inodes: %d\n", *(int *)(ramdisk));
	printf("Free blocks: %d\n\n", *(int *)(ramdisk + 4));

	printf("Creating rootdir '/'...\n"); // Note that we always place the rootdir at the first 64 bytes after 256 bytes or 1 block (superblock)
	// Create a dir or inode data struct and insert it right after 256 bytes of our superblock
	inode_t *root_inode = (inode_t *)malloc(sizeof(inode_t));

	strncpy(root_inode->type, "dir\0", sizeof(root_inode->type)); // Set the root directory as type root file
	memset(root_inode->size, 25, sizeof(root_inode->size)); // We set the size as 0 as there are no entries so far...
	
	// A function should scan and and see if the number of block space needed can be satisfied
	char* free_block = allocate_block(); // 'free_block' is the address

	if (!free_block) {
		printf("FATAL ERROR: Could not make root directory! Something is seriously wrong!");
		return -1; // This means we couldn't event find a block to create our root directory. Something is definitely wrong!
	}

	// We create a directory entry object
	dir_entry_t *root_entry = (dir_entry_t *)malloc(sizeof(dir_entry_t));
	
	// Here we create our directory entry!
	strncpy(root_entry->filename, "/\0", sizeof(root_entry->filename));
	memset(&root_entry->index_node_number, 0, sizeof(root_entry->index_node_number)); // Since this is 2 byte integer used to index into the i-node array
	
	// Then we record our first dir block's address into the location portion
	memcpy(root_inode->location, free_block, 4); // 4 byte address assuming that this is a 32 bit system
	
	// We need to check if the contigious memory made it in???
	read_bytes(root_inode, 30);

	// ---- Copying things into Memory! ----
	// Then we insert the dir_entry to be our first item in our dir block
	memcpy(free_block, &root_entry, sizeof(dir_entry_t));
	
	// Then we take the root_inode and then copy it at the beginning of the superblock
	memcpy(ramdisk + 256, root_inode, sizeof(inode_t));

	printf("Finished creating rootdir!\n\n");

	return 0;
}

// ====== FILE RELATED FUNCTIONS ======
// (1) Creating Directories
int rd_mkdir(char* pathname) { 
	return 0;
}

// (2) Creating Regular Files
int rd_create(char *pathname) {
	return 0;
}

// ====== FILE UTILITY FOR READING AND VERIFYING ======
// Some Utility functions to traverse and scan our File System (Ramdisk)
void bybyte_traverse_filesystem(char* ramdisk, int size) {
	size_t i;
	for (i = 0; i < size; i++) {
		unsigned char byte = ramdisk[i];
		printf("Byte %zu: %02x\n", i, byte);
	}
	printf("\n\n");
	return;
}

// We want to read a inode block assuming that it is 64 bit inode block
// Blocks are indexed at 0
void read_inode_block(int block_number) {
	size_t i;
	for (i = (256 + (256 * block_number)); i < (256 + (256 * block_number) + 64); i++) {
		unsigned char byte = ramdisk[i];
		printf("Byte %zu: %02x\n", i, byte);
	}
	printf("\n\n");
	return;
}

// ================ DISCOS FILE LAYOUT ===================
// *** Each block is 256 bytes ***
//
// ---SUPERBLOCK---
// The Superblock is 256 bytes or a block -> Responsible to keep information:
// > The number of FREE BLOCKS
// > The number of FREE INDEX NODES
//
//---INODE-BLOCKS---
// The 256 BLOCKS are i-node BLOCKS. Each i-node is 64 BYTES.
// We see that 256 * 256 = 65536 / 64 = 1024 possible i-nodes!
//
// An i-node:
// TYPE: (DIR or REG) -> 4 BYTES
// SIZE: (Current Size) -> 4 BYTES
// LOCATION: 8 Direct Block Pointers, 1 indirect single, 1 indirect double -> 40 BYTES
//
//---BLOCK-BIT-MAP---
// 4 blocks (1024 bits) will designate as a block bitmap. Each bit of the block will be either 0/1
// 1 -> Occupied
// 0 -> Free
//
//---STORAGE---
//The rest is for dir/reg files
// 1 block (superblock) + 256 (inodes) + 4 blocks (block bitmap) = 261 blocks
// 261 * 256 = 66816 bytes use for filesystem
// 2,097,152 - 66,816 = 2,030,336 bytes free
// This leaves 7931 blocks free for DIR or REG files

int main() {
	printf("Welcome to DISCOS!\n");
	printf("Initializing RAMDISK...\n");
	init_ramdisk();
	printf("Done!\n");
	printf("Base Address of Ramdisk is %p\n\n", (void *)ramdisk);
	
	// First we init our superblock and our root dir
	init_superblock(ramdisk, 256);

	// We read our root block
	//read_inode_block(0);

	printf("\n\nFinished program! Exiting...\n\n");
}
