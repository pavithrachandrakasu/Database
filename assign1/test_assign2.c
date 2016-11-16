#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

/*test name*/
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testCreateOpenClose(void);
static void testAppendandReadWrite(void);
/* main function running all tests */
int
main (void)
{
  testName = "";
  
  initStorageManager();
  testCreateOpenClose();
  testAppendandReadWrite();
    return 0;
}


/* check a return code. If it is not RC_OK then output a message, error description, and exit */
/* Try to create, open, and close a page file */
void
testCreateOpenClose(void)
{

  SM_FileHandle fh;
  testName = "test create open and close methods";
  /* creat and open page file */ 
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  ASSERT_TRUE(strcmp(fh.fileName, TESTPF) == 0, "filename correct");

  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");

  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));

  /*after destruction trying to open the file should cause an error*/
  ASSERT_TRUE((openPageFile(TESTPF, &fh) != RC_OK), "opening non-existing file should return an error.");
  ASSERT_TRUE((createPageFile('\0')!= RC_OK), "file not found");
  
  TEST_DONE();
}

/* Try to append new block, checking the functionality of ensurecapacity method, read and write the appended blocks*/
void
testAppendandReadWrite(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test Ensure Capacity,Append,read and write blocks";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  /*create a new page file*/
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));

  /* Total number of pages should be 1 just created using createPageFile */
  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");
  
  
   /*get current block position */ 
  int blockpos;
  blockpos = getBlockPos(&fh);
  printf ("%d",blockpos);
  readBlock (blockpos,&fh, ph);
 
  /*append a new empty block*/
  appendEmptyBlock(&fh);

  /*the page should be empty (zero bytes)*/
  for (i=0; i < PAGE_SIZE; i++)
  ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
  printf("Appended block was empty\n");

  ASSERT_TRUE((fh.totalNumPages == 2), "Total Number of pages = 2");
  ASSERT_TRUE((fh.curPagePos == 1), "freshly opened file's page position should be 1");
 
  /* calling ensure capacity - adds 3 more to the file */
  ensureCapacity(5,&fh);
  ASSERT_TRUE((fh.totalNumPages == 5), " Number of pages = 5 after calling ensureCapacity function ");
  readBlock (5,&fh, ph);
  ASSERT_TRUE((fh.curPagePos == 5), "Current page position: 5");

  /* Verify whether it is trying to read the page which is not present*/ 

  ASSERT_TRUE((readNextBlock(&fh,ph) == RC_READ_NON_EXISTING_PAGE), "opening non-existing page should return an error.");  

  /* Reading the current block which should be 5 */ 

  TEST_CHECK(readCurrentBlock(&fh,ph))
  blockpos = getBlockPos(&fh);
  printf("\n After ensure capacity has been called,  Current Block Position: %d \n ",fh.curPagePos);

  /* Reading the previous block*/

  TEST_CHECK(readPreviousBlock(&fh,ph))
  blockpos = getBlockPos(&fh);
  printf("\n Read previous block and Block Position: %d\n",fh.curPagePos);

  /* Reading the last block*/
  TEST_CHECK(readLastBlock(&fh,ph))
  ASSERT_TRUE((readLastBlock(&fh,ph) == RC_OK ), "Read the last Page");
  
  /* Writing the current block with the values. Here it is Block 4 */ 

  blockpos = getBlockPos(&fh);
  printf("Block Position before write: %d ",blockpos);
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  TEST_CHECK(writeCurrentBlock (&fh, ph));
  printf("writing current block\n");
  
  /* reading the currently written block*/

  TEST_CHECK(readCurrentBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading current block\n");
  
  /* destroy new page file */ 
  TEST_CHECK(destroyPageFile (TESTPF));  

  TEST_DONE();
}


