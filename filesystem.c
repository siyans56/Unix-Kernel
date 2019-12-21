#include "filesystem.h"
#include "types.h"
#include "lib.h"
#include "sys_call.h"

static uint32_t file_begin; //start of the filesystem
static uint32_t boot_begin; //start of the bootblock
static uint32_t inode_begin; //start of the inodes
static uint32_t data_begin; //start of the data blocks

// static uint32_t total_dirs;
// static uint32_t total_inodes;
// static uint32_t total_data;

fstats_t file_stats; // holds data about the file MOVEC TO HEADER
dentry_t * dentry_arr = NULL; // array of the data entries
inode_t * inode_arr; // array of inodes MOVED TO HEADER

uint8_t current_dir = 0;

/* read_dentry_by_name
* Inputs: - * fname : filename
          - * dentry : pointer to a dentry
* Outputs: return 0 for success ; return -1 for failure
* Side Effects: copies the appropriate data into the dentry struct based on if the file is found
*/
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){

    int i, copy_length;
    int32_t ret_val;
    int8_t * in_name; int8_t * curr_name;

    in_name = (int8_t *)fname; //filename passed in
    ret_val = FS_FAIL; //set to -1 if not found or error

    //go through dentry array
    for(i = 0; i < MAX_NUM_DENTRY; ++i) {

        curr_name = dentry_arr[i].filename; //the name of the current file in the dentry array to compare with passed in filename
        copy_length = (strlen(in_name) == MAX_ENTRY_LEN) ? MAX_ENTRY_LEN : strlen(in_name) + 1; //set the copy length for strncmp (+1 for null character at end of string)

        if(strncmp(curr_name, in_name, copy_length) != 0) {  //check if the filenames are the same
            continue; // go to next element in dentry array
        }

        dentry->filetype = dentry_arr[i].filetype; // fill in the dentry with the filetype
        dentry->inode_index = dentry_arr[i].inode_index; //fill in the dentry with the inode index
        strncpy(dentry->filename, (const int8_t *)curr_name, MAX_ENTRY_LEN); //copy the file name from the dentry array to the dentry

        ret_val = FS_SUCCESS; //return 0
        break; // the file was found so break the loop
    }
    return ret_val;
}

/* read_dentry_by_index
* Inputs: - index: index into dentry array
          - * dentry : pointer to a dentry
* Outputs: return 0 for success ; return -1 for failure
* Side Effects: copies the appropriate data into the dentry struct
*/
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    int32_t ret_val = FS_FAIL;

    // index is unsigned, so if it's less than 63, it's valid
    if(index < MAX_NUM_DENTRY) {
        //copy the appropriate data into the dentry based on the dentry arr element
        dentry->filetype = dentry_arr[index].filetype;
        dentry->inode_index = dentry_arr[index].inode_index;
        strncpy(dentry->filename, (const int8_t *)dentry_arr[index].filename, MAX_ENTRY_LEN);
        ret_val = FS_SUCCESS;
    }

    return ret_val;
}

/* read_data
* Inputs: - inode : index of inode
          - offset : loc in dir entry array
          - *buf : buffer to hold certain data string
          - length : length of block
* Outputs: return number of bytes read for success ; return -1 for failure
* Side Effects: writes the data to the buffer's location in memory
*/
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length) {

    uint32_t inode_block_loc_begin, inode_block_loc_end, num_reads, first_byte, last_byte, byte_range;
    int32_t fsize;
    int i;
    block_data_t * curr_block;

    // nothing read yet
    num_reads = 0;

    // start of the inode block
    inode_block_loc_begin = offset / SIZE_OF_BLOCKS;

    // check for invalid parameters - return FS_FAIL if invalid
    if( (inode >= file_stats.total_inodes) || check_invalid_block(inode_block_loc_begin, inode) ) {
        return FS_FAIL;
    }

    // use inode_arr to get size of file
    fsize = inode_arr[inode].length;

    if(offset > fsize - 1 || length == 0) return 0;

    // length can't be bigger than the size of the file
    if(offset + length > fsize) {
        length = fsize - offset;
    }

    // end of the inode block (given by length of block)
    inode_block_loc_end = (offset + length) / SIZE_OF_BLOCKS;

    // loop through entire set of inode block
    for(i = inode_block_loc_begin; i < inode_block_loc_end + 1; ++i) {
        // location
        curr_block = (block_data_t *) boot_begin + (file_stats.total_inodes + 1 + (inode_arr[inode].node_data[i]));

        // assume entire block [0.4095]
        first_byte = 0;
        last_byte = SIZE_OF_BLOCKS;

        // check whether entire block will be iterated over - find proper first_byte locatoin
        if(i * SIZE_OF_BLOCKS < offset) {
            first_byte = offset - (i * SIZE_OF_BLOCKS);
        }

        // check whether entire block will be iterated over - find proper last_byte locatoin
        if((++i) * SIZE_OF_BLOCKS > (offset + length)) {
            last_byte = (offset + length) - (--i * SIZE_OF_BLOCKS);
        } else {
            --i;
        }

        // number of bytes read
        byte_range = last_byte - first_byte;

        // write the data to the buffer's location in memory!
        memcpy( (char *)buf + num_reads, (char *)curr_block + first_byte, byte_range);

        // increment number of bytes read
        num_reads += byte_range;
    }

    // returning num_reads indicates successful read_data
    return num_reads;
}

/* check_invalid_block
* Inputs: - inode : the inode index to check
          - block_loc: the block to check
* Outputs: return 1 if the data in the inode is bigger than the total data; return 0 else
* Side Effects: none
*/
uint32_t check_invalid_block(uint32_t block_loc, uint32_t inode) {

    //if the data in the inode is bigger than the total data
    return (inode_arr[inode].node_data[block_loc] >= file_stats.total_data) ? 1 : 0;

}

/* init_files
* Inputs: - fstart: starting address of the file system
* Outputs: none
* Side Effects: initialize all globals in the filesystem to appropriate values
*/
void init_files(uint32_t fs_start){
    file_begin = fs_start; //the starting address of the file system
    boot_begin = file_begin; // boot block starting address
    inode_begin = SIZE_OF_BLOCKS + boot_begin; // inode starting address

    // memcpy(&total_dirs, (void*) boot_begin, FOUR_B_OFFSET); //get the number of directories
    // memcpy(&total_inodes, (void*) (boot_begin + FOUR_B_OFFSET), FOUR_B_OFFSET); //get the number of inodes
    // memcpy(&total_data, (void*) (boot_begin + (2 * FOUR_B_OFFSET)), FOUR_B_OFFSET); //get the the number of data blocks
    memcpy(&file_stats, (void*) boot_begin, STATS_SIZE); // initializing file_stats global variable

    data_begin = inode_begin + (SIZE_OF_BLOCKS * ((file_stats.total_inodes + 1))); // data_blocks starting address
    dentry_arr = (dentry_t *)(boot_begin + STATS_SIZE); // setting starting address of data entries array
    inode_arr = (inode_t *)(inode_begin); // setting starting address of inode data array
}

/* file_read
* Inputs: -fname  : name of the file
          -buf    : buffer that will hold file name
          -offset : location in dentry array
          -length : length of filename [0,32] ****(NBYTES IS LENGTH NOW)****
* Outputs: either failure, or number of bytes read
* Side Effects: writes the file name to given buffer (through read_data call)
*/
//int32_t file_read(const int8_t * fname, uint8_t * buf, , int32_t nbytes, uint32_t offset) {

int32_t file_read(int32_t fd, void *buf, int32_t nbytes) {

    uint32_t curr_offset;
    uint32_t curr_inode;
    pcb_t * current = curr_pcb();
    int32_t total_bytes_read;

    file_desc_t curr_fd_info;

    // casting name to be unsigned
    // uint8_t * unsigned_name = (uint8_t *)fname;

    // assume failure
    int32_t ret_val = FS_FAIL;

    // invalid name or buffer
    if((uint8_t *)buf == NULL) {
        return ret_val;
    }

    curr_fd_info = current->fd_arr[fd];

    curr_inode = curr_fd_info.file_inode;
    curr_offset = curr_fd_info.file_pos;

    //printf("File position %d", curr_offset);

    total_bytes_read =  read_data(curr_inode, curr_offset, (uint8_t *)buf, (uint32_t)nbytes);

    // ensuring that fname is in dir_entry array
    // read_dentry_result = read_dentry_by_name(unsigned_name, &dentry);

    // upon failure, return FS_FAIL, otherwise return number of bytes read
    current->fd_arr[fd].file_pos += total_bytes_read;

    return total_bytes_read;
}

/* file_write
* Inputs: none
* Outputs: return -1;
* Side Effects: Do nothing
*/
int32_t file_write(int32_t fd, const void *buf, int32_t nbytes) {
    return FS_FAIL;
}


/* file_close
* Inputs: none
* Outputs: return 0;
* Side Effects: Do nothing
*/
int32_t file_close(int32_t fd) {
    return FS_SUCCESS;
}

/* file_open
* Inputs: none
* Outputs: return 0
* Side Effects: Do nothing
*/
int32_t file_open(const uint8_t* filename) {
    return FS_SUCCESS;
}

/* dir_read
* Inputs: offset - file loc in dentry arr (*** nbytes is offset here ***)
          buf - buffer that will hold file string
* Outputs: return length of filename written to buffer
* Side Effects: writing file name to buffer's location in memory
*/
// int32_t dir_read(uint32_t offset, char * buf) {
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes) {

    dentry_t dentry;

    if (current_dir >= file_stats.total_dirs) {
      current_dir = 0;
      return 0;
    }

    if(read_dentry_by_index(current_dir, &dentry) == 0) {
        // current_dir = 0;
        // return current_dir;


    int32_t ret_string;

    int i;
    ret_string = strlen((int8_t *)dentry.filename);
    if(ret_string > MAX_ENTRY_LEN) {
      ret_string = MAX_ENTRY_LEN;
    }
    for(i = 0; i <= MAX_ENTRY_LEN; ++i) {
        ((int8_t *)buf)[i] = '\0';
    }
    strncpy((int8_t *)buf, (int8_t *)dentry.filename, ret_string);
    memcpy((int8_t *)buf, (int8_t *)dentry.filename, ret_string);
    current_dir++;
    return ret_string;
    }

    return 0;
}

/* get_file_size
* Inputs: -node_index: index into inode
* Outputs: return 0
* Side Effects:
*/
int32_t get_file_size(uint32_t node_index) {
    //get the length of the file from the inode
    return inode_arr[node_index].length;

}

/* dir_write
* Inputs: none
* Outputs: return -1
* Side Effects: Do nothing
*/
int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes) {
    return FS_FAIL;
}


/* dir_close
* Inputs: none
* Outputs: return 0
* Side Effects: Do nothing
*/
int32_t dir_close(int32_t fd) {
    return FS_SUCCESS;
}


/* dir_open
* Inputs: none
* Outputs: return 0
* Side Effects: Do nothing
*/
int32_t dir_open(const uint8_t* filename) {
    return FS_SUCCESS;
}

/* check_fs_init
* Inputs: none
* Outputs: return -1 if null; return 0 if successful
* Side Effects: none
*/
int32_t check_fs_init() {
    return (dentry_arr == NULL) ? FS_FAIL : FS_SUCCESS;
}
