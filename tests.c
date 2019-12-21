#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "paging.h"
#include "rtc.h"
#include "kb.h"
#include "filesystem.h"
#include "sys_call.h"

#define PASS 0
#define FAIL -1

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result == 0) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
 	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* Dvide By Zero Test
 *
 * Asserts that dividing by zero should raise divide by zero exception
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Divide by zero
 * Files: None
 */
int divide_test(){

	TEST_HEADER;
	int result = FAIL;
	int zero, five, check;
	zero = 0;
	five = 5;

	// exception should be raised here
	check = five/zero;

	// if exception not raised, the test fails
	return result;
}

/* Page Fault Test
 *
 * Asserts that derefering NULL or an illegal (negative) address raises a page fault exception
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Page Fault exception
 * Files: paging.c
 */
int deref_test(){
	TEST_HEADER;

	int result = FAIL;

	// attempting to dereference nullptr. exception should be raised.
	int * pointer1 = NULL;
	*pointer1 = DEREF_TEST;

	// attempting to dereference illegal address. exception should be raised.
	int * pointer2 = (int *)BAD_PTR;
	*pointer2 = DEREF_TEST;

	// if no exception raised, then test fails
    return result;
}


/* Paging Test
 *
 * Asserts that there is the correct amount of present and not present pages in the first 4MB paged
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage:
 * Files: paging.h
 */
int paging_test() {
	TEST_HEADER;
	int i, not_present = 0, present = 0;
	int result = FAIL;
	for (i = 0; i < ONEKB; i++) {
		if ((page_table[i] & 1) == 0) {
			not_present++;
		}
		if ((page_table[i] & 1) == 1) {
			present++;
		}
	}
	printf("Present: %d\nNot Present: %d", present, not_present );
	if (present == 1 && not_present == ONEKB-1) {
		result = PASS;
	}
	return result;
}


/* Paging Present Test
 *
 * Asserts that the kernel PDE and video memory PTE are present in the paged 8MB
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage:
 * Files: paging.h
 */
 int paging_present_test() {
	TEST_HEADER;
	int result = FAIL;

	if ((page_dir[1] & 1) == 1) {
		printf("kernel is present in page\n" );
		if ((page_table[V_MEM] & 1) == 1) {
			printf("VMEM is present in page\n" );
			result = PASS;
		} else {
			printf("VMEM is not present in page\n");
		}
	} else {
		printf("Kernel is not present in page\n");
	}
	return result;
}

/* Paging Out of Bounds Test
 *
 * Asserts that dereferencing a pointer outside the pageed 8MB will generate a page fault
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Page Fault exception if working properly
 * Coverage:
 * Files: kb.c
 */
int paging_out_of_bounds_test() {
	TEST_HEADER;

	// attempting to dereference pointer outside the 8MB paged. exception should be raised.
	uint32_t* ptr = (uint32_t*)LARGE_PTR;
	printf("DEREF: %d", *ptr);
	return FAIL;
}

/* Paging In Bounds Test
 *
 * Asserts that dereferencing a pointer inside video memory the pageed 8MB will generate a page fault
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: none
 * Coverage:
 * Files: kb.c
 */
int paging_in_bounds_test() {
	TEST_HEADER;

	// attempting to dereference pointer of video memory
	int* ptr = (int*)PAGE_IN_BOUND;
	printf("DEREF: %d", *ptr);
	return PASS; // if reached no exception was made
}

/* Checkpoint 2 tests */

/* Keyboard Write System Call Test
 *
 * Asserts that the user can print to screen any string that is written
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: String is displayed on screen
 * Coverage:
 * Files: kb.c
 */
 int kb_write_syscall_test() {
	 TEST_HEADER;
	 int resp = kb_write_syscall(0, (uint8_t*)"If this is printed write is good", KB_WRITE_SIZE);
	 if (resp == KB_WRITE_SIZE) return PASS;
	 return FAIL;
 }

 /* Keyboard Long Write System Call Test
  *
  * Asserts that the user can print to screen any string that is written, EVEN if
	* it is longer than the keyboard buffer
  * Inputs: None
  * Outputs: PASS/FAIL
  * Side Effects: String is displayed on screen
  * Coverage:
  * Files: kb.c
  */
int kb_long_write_syscall_test() {
	TEST_HEADER;
	int resp = kb_write_syscall(0, (uint8_t*)"Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang Gucci Gang ", KB_LONG_WRITE_SIZE);
	if (resp == KB_LONG_WRITE_SIZE) return PASS;
	return FAIL;
}

/* RTC write test
 *
 * Shows the frequency change on the screen
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Frequency display
 * Coverage:
 * Files: rtc.c
 */
int rtc_write_test()	{
	TEST_HEADER;
	//use write to set freq
	//use read to decide when to print char to screen
	//rtc open more or less does what write does ?
	//how to prove the open?
	long long count = 0;
	int32_t buffer = 2;			// need to check buffer 2 - 1024
	while(1){
		int val = rtc_write(0,&buffer,4);
		if(val == -1){
			return FAIL;
		}
		if(count == 45){
				buffer = (buffer == 1024) ? 1024 : buffer*2;
				count = 0;
				printf("FREQ CHANGE");
		}
		rtc_read(0,0,0);
		printf("1");
					// we need to print a 1 for every interupts
								// added printf("1") to interrupt call, we should get
		count++;
	}
	return PASS;
}

/* Fileystem init test
 *
 * Simple test to ensure filesystem was initialized by making sure dentry array is not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage:
 * Files: filesystem.c
 */
int init_fs_test() {
	TEST_HEADER;

	// call to function in filesystem.c
	return (check_fs_init() == FAIL) ? FAIL : PASS;
}

/* File size test
 *
 * Simple test to ensure that the proper file size is found given a filename
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage:
 * Files: filesystem.c
 */
int file_size_test() {
	TEST_HEADER;

	dentry_t dentry;

	// file that we will be testing - is valid
	char * text_file = "frame0.txt";

	// frame0.txt should be found
	if(read_dentry_by_name( (uint8_t *)text_file, &dentry) != PASS) {
		return FAIL;
	}

	// frame0.txt size is 187
	if(get_file_size(dentry.inode_index) != FRAME0_SIZE) {
		return FAIL;
	}

	// file that we will be testing - is valid
	text_file = "verylargetextwithverylongname.tx";

	// verylargetextwithverylongname.tx should be found
	if(read_dentry_by_name( (uint8_t *)text_file, &dentry) != PASS) {
		return FAIL;
	}

	// verylargetextwithverylongname.tx size is 5277
	if(get_file_size(dentry.inode_index) != VERY_LARGE_SIZE) {
		return FAIL;
	}

	return PASS;
}

/* File read test - text
 *
 * Test to read a text file and display contents
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: prints file name and contents to the console
 * Coverage:
 * Files: filesystem.c
 */
//  int read_text_file_test() {

//  	// clear screen first
// 	clear_screen();

// 	TEST_HEADER;

// 	int i;
// 	dentry_t dentry;
// 	char read_buf[FRAME0_SIZE];

// 	// displaying frame0.txt
// 	char * text_file = "frame0.txt";

// 	// existent text file
// 	if(read_dentry_by_name( (uint8_t *)text_file, &dentry) != PASS) {
// 		return FAIL;
// 	}

// 	// gets contents of frame0.txt into read_buf
// 	read_data(dentry.inode_index, 0, read_buf, FRAME0_SIZE);

// 	// displays contents of the file
// 	for(i = 0; i < FRAME0_SIZE; ++i) {
// 		putc(read_buf[i]);
// 	}

// 	printf("\n\nFile name: ");

// 	// prints each character of the file
// 	for(i = 0; i < strlen(text_file); ++i) {

// 		if(text_file[i] == 0) {
// 			break;
// 		}

// 		putc(text_file[i]);

// 	}

// 	printf("\n\n");

// 	// non-existent file
// 	text_file = "frame3000.txt";

// 	// non-existent text file - should NOT be found (fail if it is found)
// 	if(read_dentry_by_name( (uint8_t *)text_file, &dentry) == PASS) {
// 		return FAIL;
// 	}

// 	return PASS;
// }

/* File read test - non-text
 *
 * Test to read a non-text file and display partial contents
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: writes end characters of the file to a read_buffer's location in memory
 * Coverage:
 * Files: filesystem.c
 */
// int read_non_text_file_test() {

// 	TEST_HEADER;

// 	dentry_t dentry;
// 	char read_buf[MAX_ENTRY_LEN + 1];
// 	read_buf[MAX_ENTRY_LEN] = '\0';

// 	// look at file: shell
// 	char * text_file = "shell";
// 	// these are last 32 characters in the file
// 	char * test_str1 = "456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// 	// existent text file
// 	if(read_dentry_by_name( (uint8_t *)text_file, &dentry) != PASS) {
// 		return FAIL;
// 	}

// 	// these are two tests - the first ensures that 32 bytes are read (test_str1 is 32 long), and the second ensures that the 32 bytes read are copied properly into the read_buf
// 	if( (read_data(dentry.inode_index, SHELL_OFFSET, read_buf, MAX_ENTRY_LEN) != MAX_ENTRY_LEN) || (strncmp(read_buf, test_str1, MAX_ENTRY_LEN) != 0) ) {
// 		return FAIL;
// 	}

// 	return PASS;

// }

/* File read test - too large of name
 *
 * Test to read a file with a name that's too long
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: NONE
 * Coverage:
 * Files: filesystem.c
 */
//  int read_text_file_too_long() {
// 	TEST_HEADER;

// 	dentry_t dentry;
// 	int ret_val = FAIL;

// 	// this name is 33 characters. INVALID
// 	char * text_file = "verylargetxtwithverylongname.txt";

// 	// this should FAIL (because the name of the file is too long. If read_dentry_by_name doesn't fail then this test fails)
// 	if(read_dentry_by_name( (uint8_t *)text_file, &dentry) != PASS) {
// 		ret_val = PASS;
// 	}

// 	return ret_val;
// }

/* File read test - large file
 *
 * Test to read a file with a lot of characters
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: NONE
 * Coverage:
 * Files: filesystem.c
 */
// int read_large_file_test() {
// 	TEST_HEADER;

// 	dentry_t dentry;

// 	// lots of characters in this file
// 	char * text_file = "verylargetextwithverylongname.tx";

// 	// text_file name is only 32 characters, so this is valid (unlike last test)
// 	if(read_dentry_by_name( (uint8_t *)text_file, &dentry) != PASS) {
// 		return FAIL;
// 	}

// 	char read_buf[MAX_ENTRY_LEN + 1];

// 	// two different strings within the file at given points
// 	// test_str1 starts at 21 (VERY_LARGE_SMALL_OFFSET) - it is 26 characters long (VERY_LARGE_NUM_COPIED)
// 	// test_str2 starts at 5229 (VERY_LARGE_OFFSET) - it is 32 characters long (MAX_ENTRY_LEN)
// 	char * test_str1 = "with a very long name\n1234";
// 	char * test_str2 = "./<>?~!@#$%^&*()_+`1234567890-=[";
// 	read_buf[MAX_ENTRY_LEN] = '\0';

// 	// this conditional checks to ensure that 26 characters are found starting the 21st character in the file. then it ensures that the 26 characters written to read_buf are the same as the ones in the file
// 	if( (read_data(dentry.inode_index, VERY_LARGE_SMALL_OFFSET, read_buf, VERY_LARGE_NUM_COPIED) != VERY_LARGE_NUM_COPIED) || (strncmp(read_buf, test_str1, VERY_LARGE_NUM_COPIED) != 0) ) {
// 		return FAIL;
// 	}

// 	// this conditional checks to ensure that 26 characters are found starting the 21st character in the file. then it ensures that the 26 characters written to read_buf are the same as the ones in the file
// 	if( (read_data(dentry.inode_index, VERY_LARGE_OFFSET, read_buf, MAX_ENTRY_LEN) != MAX_ENTRY_LEN) || (strncmp(read_buf, test_str2, MAX_ENTRY_LEN) != 0) ) {
// 		return FAIL;
// 	}

// 	return PASS;

// }

/* File read index test
 *
 * Test to take the index of a file and use thta to display the contents
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: prints the contents of the file to the console
 * Coverage:
 * Files: filesystem.c
 */
//  int read_index_test() {
// 	TEST_HEADER;

// 	int i;
// 	dentry_t dentry;

// 	// index of FRAME0 is 10 -> this was calculated just for testing
// 	uint32_t index = FRAME0_INDEX;

// 	// will be using frame0.txt again
// 	char * text_file = "frame0.txt";
// 	char read_buf[FRAME0_SIZE];

// 	// valid index
// 	if(read_dentry_by_index(index, &dentry) != PASS) {
// 		return FAIL;
// 	}

// 	// ensure that file at index 10 is of type FILE (2)
// 	if(dentry.filetype != FILE_TYPE) {
// 		return FAIL;
// 	}

// 	// writes fill contents to read_buf and then prints to the screen
// 	read_data(dentry.inode_index, 0, read_buf, FRAME0_SIZE);

// 	for(i = 0; i < FRAME0_SIZE; ++i) {
// 		putc(read_buf[i]);
// 	}

// 	// the rest just prints the name of the file
// 	printf("\n\nFile name: ");

// 	for(i = 0; i < strlen(text_file); ++i) {

// 		if(text_file[i] == 0) {
// 			break;
// 		}

// 		putc(text_file[i]);

// 	}

// 	printf("\n\n");

// 	// 64 is an invalid index (anything over 63 is invalid)
// 	index = MAX_NUM_DENTRY + 1;

// 	// invalid index
// 	if(read_dentry_by_index(index, &dentry) == PASS) {
// 		return FAIL;
// 	}

// 	return PASS;
// }

/* List of every file in directory test
 *
 * Test to display all files within directory
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: uses dir_read to display all files in the directory
 * Coverage:
 * Files: filesystem.c
 */
// int dir_list_test() {

// 	// clear screen first
// 	clear_screen();

// 	TEST_HEADER;

// 	int i;
// 	int32_t num_dir;
// 	char read_buf[MAX_NUM_DENTRY + 1];
// 	dentry_t dentry;

// 	// go through the entire dir entry (can be 63 big)
// 	for(i = 0; i < MAX_NUM_DENTRY; ++i) {
// 		// read the file at a given directory index
// 		// dir read writes that file to read_buf's memory location
// 		num_dir = dir_read(0,read_buf,i);


// 		// if directory number is 0, you know you're done
// 		if(!num_dir) {
// 			break;
// 		}
// 		// make sure valid directory
// 		if(num_dir < 0 || num_dir > MAX_NUM_DENTRY) {
// 			return FAIL;
// 		}

// 		read_buf[num_dir] = '\0';

// 		// to get dentry information - filetype and inode index
// 		read_dentry_by_name((const uint8_t *)read_buf, &dentry);

// 		// displaying name of file, filetype, and inode index to screen for each file in the directory
// 		printf("FILE_NAME: %s    FILE_TYPE: %u    FILE_SIZE: %u", read_buf,  dentry.filetype, get_file_size(dentry.inode_index));
// 		printf("\n");

// 	}

// 	return PASS;
// }

/* Checkpoint 3 tests */

/* Bad Inputs Test
 *
 * Test to ensure we check for bad inputs across syscalls
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: NONE
 * Coverage:
 * Files: sys_call.c
 */
int bad_inputs_test() {
	TEST_HEADER;
	// const uint8_t * command_1 = (uint8_t*)"test";
	const uint8_t * command_2 = (uint8_t*)"This doesnt exist";
	char buf[3] = {'d','o','p'};

	// bad buffer tests
	if (read(1, NULL, 12) != -1) return FAIL;
	if (write(2, NULL, 12) != -1) return FAIL;

	printf("Bad buffer tests passed \n");

	// bad general args
	if (read(1, &buf, -32) != -1) return FAIL;
	if (write(2, &buf, -1) != -1) return FAIL;
	if (open(command_2) != -1) return FAIL;

	printf("Bad general arg tests passed \n");

	// attempt to close stdin
	if (close(0) != -1) return FAIL;

	return PASS;

}

/* Run Shell Test
 *
 * Test to run the shell program
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: NONE
 * Coverage:
 * Files: sys_call.c, ece391shell.c
 */
int run_shell_test(){
	TEST_HEADER;
	const uint8_t * command_1 = (uint8_t*)"shell";


	if (execute(command_1) < 0 || execute(command_1) > 255) return FAIL;		// valid return

	return PASS;
}

/* Run Testprint Test
 *
 * Test to run the testprint program
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: NONE
 * Coverage:
 * Files: sys_call.c, ece391testprint.c
 */
int run_testprint_test(){
	TEST_HEADER;
	const uint8_t * command_1 = (uint8_t*)"testprint";


	// int result = FAIL;
	if (execute(command_1) < 0 || execute(command_1) > 255) return FAIL;		// valid return

	return PASS;

}

/* Execute Test
 *
 * Test to run the execute sycall
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: NONE
 * Coverage:
 * Files: sys_call.c, ece391testprint.c
 */
int execute_test(){
	TEST_HEADER;
	const uint8_t * command_1 = (uint8_t*)"testprint";

	execute(command_1);
	execute(command_1);
	execute(command_1);
	return PASS;
}

/* File Descriptor Flags Test
 *
 * Test to check that fd flags are set
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: NONE
 * Coverage:
 * Files: sys_call.c
 */
int file_desc_flags_test() {
	TEST_HEADER;
	int i, orig = 0, num_fd = 0;
	pcb_t* curr = curr_pcb();
	const uint8_t * command_1 = (uint8_t*)"hello";

	// get number of fd in use
	for (i = 0; i < MAX_FILE_OPS; i++) {
		if (curr->fd_arr[i].file_flags > 0) orig++;
	}
	printf("Original: %d\n", orig);

	open(command_1); // open a new file to change fd array

	// if (curr->fd_arr[num_fd].file_flags != IN_USE) return FAIL; // check that new fd allocated

	// get number of fd in use after open
	for (i = 0; i < MAX_FILE_OPS; i++) {
		if (curr->fd_arr[i].file_flags > 0) num_fd++;
	}

	printf("After open: %d\n", num_fd);

	if (num_fd != orig+1) return FAIL;

	close(2); // close the new file to change fd array

	num_fd = 0;

	// get number of fd in use after open
	for (i = 0; i < MAX_FILE_OPS; i++) {
		if (curr->fd_arr[i].file_flags > 0) num_fd++;
	}

	printf("After close: %d\n", num_fd);

	if (num_fd != orig) return FAIL;

	// if (curr->fd_arr[num_fd].file_flags != FREE) return FAIL; // check that new fd cleared

	return PASS;
}

/* Shell Test
 *
 * Test to execute the shell program
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: NONE
 * Coverage:
 * Files: sys_call.c, ece391shell.c
 */
int test_shell(){
	TEST_HEADER;
	clear_screen();
	const uint8_t * command_1 = (uint8_t*)"shell";
	execute(command_1);
	return 0;
}

/* LS Test
 *
 * Test to execute the ls program
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: NONE
 * Coverage:
 * Files: sys_call.c, ece391ls.c
 */
int test_ls() {
	TEST_HEADER;
	const uint8_t * command_1 = (uint8_t*)"ls";
	execute(command_1);
	return 0;
}


/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	// CP 1
	// TEST_OUTPUT("idt_test", idt_test());
	//TEST_OUTPUT("divide_test", divide_test());
	// TEST_OUTPUT("paging_test", paging_test());
	// TEST_OUTPUT("paging_present_test", paging_present_test());
	// TEST_OUTPUT("deref_test", deref_test());
	// TEST_OUTPUT("scancode_conversion_test", scancode_conversion_test());
	// TEST_OUTPUT("paging_out_of_bounds_test", paging_out_of_bounds_test());
	//TEST_OUTPUT("paging_in_bounds_test", paging_in_bounds_test());
	// TEST_OUTPUT("paging_out_of_bounds_test", paging_out_of_bounds_test());
	//TEST_OUTPUT("paging_in_bounds_test", paging_in_bounds_test());

	// CP 2
	//TEST_OUTPUT("init_fs_test", init_fs_test()); // PASS
	//TEST_OUTPUT("file_size_test", file_size_test()); // PASS
	//TEST_OUTPUT("read_text_file_test", read_text_file_test()); // PASS
	//TEST_OUTPUT("read_text_file_too_long", read_text_file_too_long()); // PASS
	//TEST_OUTPUT("read_index_test", read_index_test()); // PASS
	//TEST_OUTPUT("read_non_text_file_test", read_non_text_file_test()); // PASS
	//TEST_OUTPUT("dir_list_test", dir_list_test()); // PASS
	//TEST_OUTPUT("read_large_file_test", read_large_file_test()); // PASS
	//TEST_OUTPUT("rtc_write_test", rtc_write_test());
	// TEST_OUTPUT("kb_write_syscall_test", kb_write_syscall_test());
	// TEST_OUTPUT("kb_long_write_syscall_test", kb_long_write_syscall_test());

	// CP 3
	// TEST_OUTPUT("bad_inputs_test", bad_inputs_test());
	 // TEST_OUTPUT("file_desc_flags_test", file_desc_flags_test());
	// TEST_OUTPUT("run_shell_test", run_shell_test());
	// TEST_OUTPUT("run_testprint_test", run_testprint_test());
	// TEST_OUTPUT("execute_test", execute_test());
	 TEST_OUTPUT("test_shell", test_shell());
	// TEST_OUTPUT("test_ls", test_ls());
	// CP 4
}
