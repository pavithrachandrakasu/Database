#include "storage_mgr.h"
#include "buffer_mgr_stat.h"
#include "buffer_mgr.h"
#include "dberror.h"
#include "test_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* var to store the current test's name */
char *testName;

/* check whether two the content of a buffer pool is the same as an expected content */
/* (given in the format produced by sprintPoolContent) */
#define ASSERT_EQUALS_POOL(expected,bm,message)			        \
  do {									\
    char *real;								\
    char *_exp = (char *) (expected);                                   \
    real = sprintPoolContent(bm);					\
    if (strcmp((_exp),real) != 0)					\
      {									\
	printf("[%s-%s-L%i-%s] FAILED: expected <%s> but was <%s>: %s\n",TEST_INFO, _exp, real, message); \
	free(real);							\
	exit(1);							\
      }									\
    printf("[%s-%s-L%i-%s] OK: expected <%s> and was <%s>: %s\n",TEST_INFO, _exp, real, message); \
    free(real);								\
  } while(0)

/* test and helper methods */
static void testCreatingAndReadingDummyPages (void);
static void createDummyPages(BM_BufferPool *bm, int num);
static void checkDummyPages(BM_BufferPool *bm, int num);
static void testReadPage (void);
static void testCLOCK (void);

/* main method */
int 
main (void) 
{
  initStorageManager();
  testName = "";
  testCreatingAndReadingDummyPages();
  testReadPage();
  testCLOCK();
}

/* create n pages with content "Page X" and read them back to check whether the content is right */
void
testCreatingAndReadingDummyPages (void)
{
  BM_BufferPool *bm = MAKE_POOL();
  testName = "Creating and Reading Back Dummy Pages";

  CHECK(createPageFile("testbuffer.bin"));

  createDummyPages(bm, 22);
  checkDummyPages(bm, 20);

  createDummyPages(bm, 10000);
  checkDummyPages(bm, 10000);

  CHECK(destroyPageFile("testbuffer.bin"));

  free(bm);
  TEST_DONE();
}


void 
createDummyPages(BM_BufferPool *bm, int num)
{
  int i;
  BM_PageHandle *h = MAKE_PAGE_HANDLE();

  CHECK(initBufferPool(bm, "testbuffer.bin", 3, RS_CLOCK, NULL));
  
  for (i = 0; i < num; i++)
    {
      CHECK(pinPage(bm, h, i));
      sprintf(h->data, "%s-%i", "Page", h->pageNum);
      CHECK(markDirty(bm, h));
      CHECK(unpinPage(bm,h));
    }

  CHECK(shutdownBufferPool(bm));

  free(h);
}

void 
checkDummyPages(BM_BufferPool *bm, int num)
{
  int i;
  BM_PageHandle *h = MAKE_PAGE_HANDLE();
  char *expected = malloc(sizeof(char) * 512);

  CHECK(initBufferPool(bm, "testbuffer.bin", 3, RS_CLOCK, NULL));

  for (i = 0; i < num; i++)
    {
      CHECK(pinPage(bm, h, i));

      sprintf(expected, "%s-%i", "Page", h->pageNum);
      ASSERT_EQUALS_STRING(expected, h->data, "reading back dummy page content");

      CHECK(unpinPage(bm,h));
    }

  CHECK(shutdownBufferPool(bm));

  free(expected);
  free(h);
}

void
testReadPage ()
{
  BM_BufferPool *bm = MAKE_POOL();
  BM_PageHandle *h = MAKE_PAGE_HANDLE();
  testName = "Reading a page";

  CHECK(createPageFile("testbuffer.bin"));
  CHECK(initBufferPool(bm, "testbuffer.bin", 3, RS_CLOCK, NULL));
  
  CHECK(pinPage(bm, h, 0));
  CHECK(pinPage(bm, h, 0));

  CHECK(markDirty(bm, h));

  CHECK(unpinPage(bm,h));
  CHECK(unpinPage(bm,h));

  CHECK(forcePage(bm, h));

  CHECK(shutdownBufferPool(bm));
  CHECK(destroyPageFile("testbuffer.bin"));

  free(bm);
  free(h);

  TEST_DONE();
}

void testCLOCK (void)
{
    BM_BufferPool *bm = MAKE_POOL();
    BM_PageHandle *h = MAKE_PAGE_HANDLE();
    testName = "testClock";
    CHECK(createPageFile("testbuffer.bin"));
    createDummyPages(bm, 25);
    CHECK(initBufferPool(bm, "testbuffer.bin", 3, RS_CLOCK, NULL));
 
    const char *poolContents[] = {
        "[2 0],[-1 0],[-1 0]" ,
        "[2 0],[3x0],[-1 0]",
        "[2 0],[3x0],[-1 0]",
        "[2 0],[3x0],[1x0]",

    };
    const int requests[] = {2,3,2,1};
    int d[]={0,1,0,1};
    int u[]={1,1,1,1};
    int i, rlen=4;
    for(i=0; i<rlen; i++)
    {
        CHECK(pinPage(bm, h, requests[i]));
       if(d[i])
            CHECK(markDirty(bm, h));
        if(u[i])
            CHECK(unpinPage(bm, h)); 
        ASSERT_EQUALS_POOL(poolContents[i], bm, "check pool content");
    }

  /* check number of write IOs */
  ASSERT_EQUALS_INT(0, getNumWriteIO(bm), "check number of write I/Os");
  ASSERT_EQUALS_INT(3, getNumReadIO(bm), "check number of read I/Os");
   
    CHECK(shutdownBufferPool(bm));
    free(bm);free(h);
    CHECK(destroyPageFile("testbuffer.bin"));
    
    TEST_DONE();
}
