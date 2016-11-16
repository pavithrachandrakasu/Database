#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "list.h"
#include "dt.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>


/* ---------------- Buffer Manager Interface Pool Handling -------------- */ 

/*Creates a buffer pool with the given number of page frames and initialise the values for the various data structures */

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy,  void *stratData)
{

  bm->pageFile = (char*)pageFileName;
  bm->numPages = numPages;
  bm->strategy = strategy;
  PoolInfo *pi = (PoolInfo *)malloc(sizeof(PoolInfo));
  pi->readPages = 0;
  pi->writePages = 0;
  pi->nodeCount=0;
  pi->lru_k = 0;
  PageFrame *start = linked_list(numPages);
  pi->pf = start;
  bm->mgmtData = pi;
  return RC_OK;
}

/* Deallocates memory allocated by the initBufferPool() */

RC shutdownBufferPool(BM_BufferPool *const bm)
{
  SM_FileHandle fHandle;
  PageFrame *tmp;
  PoolInfo *pi;
  pi =(PoolInfo*)bm->mgmtData;
  tmp = pi->pf;
  CHECK(openPageFile(bm->pageFile, &fHandle)); /* opening and closing outside the loop to avoid opening and closing multiple times in loop */
  do 
  {
	if (tmp->fixCount != 0) 
	{
	  return RC_SHUTDOWN_POOL_ERROR; 
	}	

	/*if dirty writing to disk */
	if (tmp->dirty == TRUE ) 
	{
	  CHECK(writeBlock(tmp->ph.pageNum, &fHandle, tmp->ph.data)); /* Write back the data from Page Handler to File Handler. */

	  pi->writePages++; /* incrementing write pages count */
	  tmp->dirty = FALSE; /* reset the dirty to false after writing to the disk */
	} 
	tmp=tmp->next;
  } while(tmp != NULL && tmp != pi->pf);
  CHECK(closePageFile(&fHandle));

  /* free the memory alloted to the PoolInfo and PageFrame */

  pi->readPages = 0;
  pi->writePages = 0;
  pi->nodeCount=0;
  pi->position=0;
  tmp = pi->pf;
  free(tmp);
  tmp=NULL;
  free(pi);
  pi=NULL;
  return RC_OK;
	
}

/* writes all dirty pages with fix count 0 in the pool */

RC forceFlushPool(BM_BufferPool * const bm) 
{
  SM_FileHandle fHandle;   
  PoolInfo *pi;
  pi = (PoolInfo*) bm->mgmtData;
  PageFrame *tmp;
  tmp = pi->pf;
  CHECK(openPageFile(bm->pageFile, &fHandle)); /* opening and closing outside the loop to avoid opening and closing multiple times in loop */
  do
  {
	if (tmp->dirty == TRUE && tmp->fixCount == 0) 
	{
	  CHECK(writeBlock(tmp->ph.pageNum, &fHandle, tmp->ph.data));/* Write back the data from Page Handler to File Handler */
	  pi->writePages++; /* incrementing write pages count */
	  tmp->dirty = FALSE; /* reset the dirty to false after writing to the disk */
	} 
	tmp=tmp->next; 
  } while(tmp != NULL && tmp != pi->pf);
  CHECK(closePageFile(&fHandle));
  return RC_OK;
}

/* -------------------------- Buffer Manager Interface Access Pages -------------------- */


/* Sets the currently used page to dirty so that it can be written back to memory before being flushed */ 

RC markDirty(BM_BufferPool * const bm, BM_PageHandle * const page)
{
  PoolInfo *pi = (PoolInfo*) bm->mgmtData;
  PageFrame *tmp = pi->pf;
  if(tmp == NULL)
  {
    return RC_DIRTY_FAILED;
  }
  do 
  {
	if (tmp->ph.pageNum == page->pageNum) 
	{
	  tmp->dirty = TRUE;
	  break;
	} 
	else 
	{
	  tmp=tmp->next;
	}
  } while(tmp != NULL && tmp != pi->pf);
  return RC_OK;
}


/* Unpins the currently pinned page and decrements the fixCount */ 

RC unpinPage(BM_BufferPool * const bm, BM_PageHandle * const page) 
{
  SM_FileHandle fHandle;      
  PoolInfo *pi;
  pi = (PoolInfo*) bm->mgmtData;
  PageFrame *tmp;
  tmp = pi->pf;
  int numPages= bm->numPages;
  if (bm == NULL) /* if buffer pool is empty throws an error code */
  {
     return RC_EMPTY_FRAME; 
  } 
  else 
  {
     do 
     {
	if (tmp->ph.pageNum == page->pageNum) 
	{
	   if (tmp->fixCount > 0) /* checks the fix count of the given page */
	   {
	      tmp->fixCount--; /* decrements the fix count */
	   }
	   else 
	   {
	      return RC_UNPIN_FAILED;
	    }
	   break;
	} 
	else 
	{
	   tmp = tmp->next;
	}
     } while (tmp != NULL && tmp != pi->pf);
     return RC_OK;
  }
}			

/*writes the requested page back to disk */ 

RC forcePage(BM_BufferPool * const bm, BM_PageHandle * const page) 
{
  SM_FileHandle fHandle;
  fHandle.fileName = bm->pageFile;
  PoolInfo *pi = (PoolInfo*) bm->mgmtData;
  openPageFile(bm->pageFile, &fHandle);            /*write the page back to file */
  ensureCapacity(page->pageNum, &fHandle);
  writeBlock(page->pageNum, &fHandle, page->data);
  pi->writePages++; /*increments the write pages count */
  closePageFile(&fHandle);
  return RC_OK;
}

/* pins the current page with the pageNum based on the page replacement strategy used */

RC pinPage(BM_BufferPool* const bm, BM_PageHandle *const page, const PageNumber pageNum) 
{

  SM_FileHandle fHandle;
  PoolInfo *pi;
  PageFrame *pf;
  pi = (PoolInfo*) bm->mgmtData;
  pf = pi->pf;
  int numPages= bm->numPages;
  bool  pageFound = searchLinkList(pageNum, pf); /* traverse through the buffer pool and returns true if the page is found in the buffer pool*/
  bool pageReplaced = 0;
  if (!pageFound) 
	pi->readPages++; /* increment the read count */

  switch (bm->strategy)
  {
	case  RS_FIFO : /* enters FIFO strategy */ 
		if (!pageFound)
		  pageReplaced = FIFO1(pageNum,bm);		
		break;

	case RS_LRU :	/* enters LRU strategy */ 
		pageReplaced = LRU(pageNum,bm);
		break;

	case RS_CLOCK : /* enters CLOCK strategy */ 
		if (!pageFound)
		  pageReplaced = Clock(pageNum, bm->numPages, bm);
		break;

	case RS_LRU_K : /* enters LRU_K strategy */ 
		if (!pageFound)
		  pageReplaced = LRU_K(pageNum,bm->numPages, bm);
		break;
	default:
		return RC_NOT_VALID_REPLACEMENT_STRATEGY;
		break;
  }

  PageFrame *tmp = getPageFrame(pageNum, pf);
  if (tmp == NULL)
  {
	return RC_EMPTY_FRAME; /* throws an error if the replaced page frame is empty ideally this case not possible */
  } 

  CHECK(openPageFile(bm->pageFile,&fHandle)); 
  CHECK(ensureCapacity(pageNum + 1, &fHandle)); /*to ensure all pages are there n file - assuming my page number has correct page number has total number of pages. */
  CHECK(readBlock(pageNum, &fHandle, tmp->ph.data)); /* read data for files to pageframe’s page handle data */
  CHECK(closePageFile(&fHandle));
  page->data = tmp->ph.data; /*setting data to pagehandle */
  page->pageNum = pageNum; /* setting page Num */
  tmp->fixCount++; /* increment the fix count of the pinned page */
  return RC_OK;
}

/* --------------------------------- Statistics Interface -------------------------- */


/*returns the array of page numbers*/

PageNumber *getFrameContents (BM_BufferPool *const bm)
{
  PoolInfo *pi;
  pi = (PoolInfo*) bm->mgmtData;

  PageFrame *tmp;
  tmp=pi->pf;
  int numPages= bm->numPages;
  int i=0;
  PageNumber *frameContent;
  frameContent = (PageNumber *)malloc(sizeof(PageNumber)*numPages);
  do
  {	
    if(i<numPages)
    {
	frameContent[i] = tmp->ph.pageNum; /* assigns the page number to the allocated frame content */
	i=i+1;
	tmp=tmp->next;
    }
  }while(tmp!=NULL && tmp!=pi->pf);
  return frameContent;
}

/* returns an array of dirty flags */

bool *getDirtyFlags (BM_BufferPool *const bm)
{
  PoolInfo *pi;
  pi = (PoolInfo*) bm->mgmtData;
  PageFrame *tmp;
  tmp=pi->pf;
  int numPages= bm->numPages;
  int i=0;
  bool *dirtyCount;
  dirtyCount = (bool *)malloc(sizeof(bool)*bm->numPages);
  do
  {
    if(i<numPages)
    {
	dirtyCount[i] = tmp->dirty; /* assigns the dirty bit value to the created array */
	i=i+1;
	tmp=tmp->next;
    }
  }while(tmp!=NULL && tmp!=pi->pf);
  return dirtyCount;
}

/* returns an array of fix counts */

int *getFixCounts (BM_BufferPool *const bm)
{
  PoolInfo *pi;
  pi = (PoolInfo*) bm->mgmtData;
  PageFrame *tmp;
  tmp=pi->pf;
  int numPages= bm->numPages;
  int i=0;
  int *fix_Count;
  fix_Count = (int *)malloc(sizeof(int)*bm->numPages);
  do
  {	
    if(i<numPages)
    {
	fix_Count[i] = tmp->fixCount; /* assigns the fix count value to the created array */
	i=i+1;
	tmp=tmp->next;
    }
  }while(tmp!=NULL && tmp!=pi->pf);
  return fix_Count;
}

/* returns the number of reads */

int getNumReadIO (BM_BufferPool *const bm)
{
  PoolInfo *pi;
  pi = (PoolInfo*) bm->mgmtData;
  return pi->readPages;
}

/* returns the number of writes */
int getNumWriteIO (BM_BufferPool *const bm)
{
  PoolInfo *pi;
  pi = (PoolInfo*) bm->mgmtData;
  return pi->writePages;
}


/* Page Replacement for FIFO */ 

bool FIFO1(PageNumber pageNum,BM_BufferPool * const bm)
{
	
  SM_FileHandle fHandle;
  PoolInfo *pi= (PoolInfo*) bm->mgmtData;
  PageFrame *pf= pi->pf;
  PageFrame *tmp = pi->pf;
  int emptyFrameCnt = getEmptyFramesCnt(pf);
  bool  nodeFound = 0;

  do
  { 
	if (tmp->ph.pageNum == -1) /* First empty page frame case */
	{
	  tmp->ph.pageNum = pageNum;
	  break; 
	}	
	else if (emptyFrameCnt > 0) /* finds the next empty page frame */
	{
	  tmp = tmp->next;
	}
	else if (emptyFrameCnt == 0) /* replaces the existing page if no empty frames exists */
	{	
	  tmp = getFIFONodeToReplace(pi);
	  if (tmp->dirty == TRUE) 
	  {
	    CHECK(openPageFile(bm->pageFile, &fHandle));
	    CHECK(writeBlock(tmp->ph.pageNum, &fHandle, tmp->ph.data));/* Write data from Handler to file */
     	    pi->writePages++; /*incrementing write pages count */
	    tmp->dirty = FALSE;
	    CHECK(closePageFile(&fHandle));
	  }
	  if (tmp != NULL)
	  {
	    tmp->ph.pageNum = pageNum;
	    break;
	  }
	  else
	    return 0; /* no change */
	}
	
  }while (tmp != pf);

  return 1; /* if it comes here page is successfully inserted/replaced */
}

/* Function to find the victim node for replacement logic for fifo */

PageFrame* getFIFONodeToReplace(PoolInfo* pi)
{
  PageFrame *tmp = pi->pf;
  PageFrame *victim = NULL;
  int i;
  for(i=0;i< pi->nodeCount;i++) 
  {
    tmp=tmp->next;
  }
  int j = 0;
  do 
  {
     j=j+1;
     if (tmp->fixCount ==0) /* checks for the unaccessed page and returns that page frame as victim */
     {
	victim = tmp;
	break;
     } 
     else 
     {
	tmp = tmp->next;
     }
  } while (tmp != NULL && tmp != pi->pf);

  if(victim!=NULL)
  {
    pi->nodeCount= pi->nodeCount+1; /* increase node count by 1 */
  }
  return victim;
}

/* Page Replacement for LRU */ 

bool LRU(int pageNum, BM_BufferPool * const bm)
{

  SM_FileHandle fHandle;
  PoolInfo *pi = (PoolInfo*) bm->mgmtData;
  PageFrame* tmp = pi->pf;
  int position = pi->position;
  bool hit = ifHit(pageNum, pi);  /*  iterates the list and check if page num is present in frame */
  int emptyframes = getEmptyFramesCnt(pi->pf);  /* get the total number of empty frames in d list. (if nodes data is -1 then frame s empty) */
  if (!hit)
  {
    bool nodeFound = 0;
    do{ 
	if (tmp->ph.pageNum == -1) /* First empty page frame case */
	{
	  nodeFound = 1;
	}
	else if (emptyframes > 0) /* checks for next empty page frame */
	{
	  tmp = tmp->next;
	}
	else if (emptyframes == 0) /* replaces the existing page if no empty frames exists */
	{
	  tmp = getVictimToReplace(pi->pf);
	  if (tmp->dirty == TRUE)  /* writes the page to the disk if it is dirty before replacing */
	  {
	    CHECK(openPageFile(bm->pageFile, &fHandle));
	    CHECK(writeBlock(tmp->ph.pageNum, &fHandle, tmp->ph.data)); /* Write data from Handler to file */
     	    pi->writePages++; /*incrementing write pages count */
	    tmp->dirty = FALSE;
	    CHECK(closePageFile(&fHandle));
	  }
          if (tmp != NULL) 
	  {
	    nodeFound = 1;
	  } 
	  else 
	  {
	    return 0;
	  }		
	}
	if (nodeFound) 
	{
	  tmp->ph.pageNum = pageNum;	
	  tmp->position = position;
	  pi->position++;
	  break;
	} /* incrementing so that next page inserted has next position */
	
     }while (tmp != pi->pf);
  }
  return 1; /* it comes here if page successfully inserted/replaced */
}


/* Page Replacement for LRU_K */ 

bool LRU_K(int pageNum,int totalPages,BM_BufferPool * const bm) 
{

  SM_FileHandle fHandle;
  PoolInfo *pi = (PoolInfo*) bm->mgmtData;
  PageFrame* tmp = pi->pf;
  int position = pi->lru_k;
  bool hit = ifHit_LRU_K(pageNum, pi);  /*  iterates the list n check if page num is present in frame */
  int emptyframes = getEmptyFramesCnt(pi->pf);  /* get the total no of empty frames in d list. (if nodes data is -1 then frame s empty) */
  if(!hit) 
  {
    bool nodeFound = 0;
    do { 
	  if (tmp->ph.pageNum == -1) /* First page frame case */
	  {	
	    nodeFound = 1;
	  }
	  else if (emptyframes > 0) /* checks for the empty frame */
	  {
	    tmp = tmp->next;
	  }
	  else if (emptyframes == 0) /* replacement case */
	  {
	    tmp = getVictimToReplace_LRU_K(pageNum,pi->pf,bm);
	    if (tmp->dirty == TRUE) 
	    {
	      CHECK(openPageFile(bm->pageFile, &fHandle));
	      CHECK(writeBlock(tmp->ph.pageNum, &fHandle, tmp->ph.data));/* Write data from Handler to file */
 	      pi->writePages++; /* incrementing write pages count */
	      tmp->dirty = FALSE;
	      CHECK(closePageFile(&fHandle));
	    }
	    if (tmp != NULL) 
	    {
	      nodeFound = 1;
	    } 
	    else 
	    {
	      return 0;
	    }		
	   }
           if (nodeFound) 
           {
	     tmp->ph.pageNum = pageNum;	
	     pi->lru_k = position;
	    pi->lru_k++;
	    break;
           } /* incrementing so that next page inserted has next position */
    }while (tmp != pi->pf);
  }
  int j = 0;
  for(j=0;j< totalPages;j++)
  {
    pi->lru_k--;
  }
  return 1; /* it comes here if page successfully inserted/replaced */
}

/* Checks if the page is present in the buffer for fifo */

bool ifHit(int pageNum, PoolInfo* pi)
{
  PageFrame* pf = pi->pf;
  PageFrame* tmp =pf;
  int position = pi->position;
  bool hit = 0;
  do 
  {
    if (tmp->ph.pageNum == pageNum)  /*if page already found, just changing the position of the page */
    { 
	hit = 1;
	tmp->position = position;
	pi->position++; /* increment the page position */
	break;
    }
    else
    {
	tmp = tmp->next;
    }
  } while (tmp != NULL && tmp != pf);
  return hit;
}


/* Checks if the page is present in the buffer for lru_k */

bool ifHit_LRU_K(int pageNum, PoolInfo* pi)
{
  PageFrame* pf = pi->pf;
  PageFrame* tmp =pf;
  int position = pi->lru_k;
  bool hit = 0;
  do 
  {
    if (tmp->ph.pageNum == pageNum)  /*if page already found, just changing the position of the page */
    {
	hit = 1;
	pi->lru_k = position;
	pi->lru_k++;
	break;
    }
    else
    {
	tmp = tmp->next;
    }
  } while (tmp != NULL && tmp != pf);
  return hit;
}

/* returns the empty frames in the buffer */

int getEmptyFramesCnt(PageFrame *pf) /*iterates entire list and find the no of empty frames */
{
  struct PageFrame *tmp =pf;
  int count = 0;
  int i;
  do
  {
    if (tmp->ph.pageNum == -1)
    {
	count++; /* Increments the count of empty page frame */
    }
    tmp= tmp->next;	
  } while (tmp != NULL && tmp != pf);

  return count;
}


/* returns the victim node for LRU */

PageFrame* getVictimToReplace(PageFrame *pf)
{
  PageFrame *tmp = pf;
  int min = 65535; /* assigning largest number */
  PageFrame* victimNode;
  do 
  {
    if (tmp->position < min && tmp->fixCount == 0) /* checking if page is not currently used also */
    { 
	min = tmp->position;
	victimNode = tmp;
    } 
    tmp = tmp->next;
  } while (tmp != NULL && tmp != pf);
  return victimNode;
}


/* returns the victim node for LRU_K */

PageFrame* getVictimToReplace_LRU_K(int pageNum,PageFrame *pf,BM_BufferPool * const bm)
{
  PageFrame *tmp = pf;
  PoolInfo *pi = (PoolInfo*) bm->mgmtData;
  int min = 65535; /* assigning largest number */
  PageFrame* victimNode;
  do 
  {
    if (pi->lru_k < min && tmp->fixCount == 0) /*checking if page is not currently used also */
    { 
	min = pi->lru_k;
	victimNode = tmp;
    } 
    tmp = tmp->next;
  }while (tmp != NULL && tmp != pf);
  return victimNode;
}

/* returns the victim node for CLOCK */

PageFrame* getCLOCKNodeToReplace(int pageNum, PoolInfo* pi,int totalPages)
{
  PageFrame *tmp = pi->pf;
  PageFrame *victim = NULL;
  int i;
  bool pagePresent = isClockHit(pageNum, pi->pf);
  for(i=0;i< pi->nodeCount;i++)
  {
    tmp=tmp->next;
  }
  if(pagePresent)
  {
    tmp->position = 1;
    tmp->handle = 1;
    victim = tmp;
    return victim;
  }
  else
  {
    int j = 0;
    do 
    {
	j=j+1;
	if (tmp->fixCount == 0 && tmp->position == 0 && tmp->handle == 0) 
	{
	  victim = tmp;
	  break;
	} 
	else if(tmp->position == 1 &&  tmp->handle == 1)
	{
	  tmp->position = 0;	
	}
	tmp = tmp->next;
    } while (tmp != NULL && tmp != pi->pf);

    /*increase node count by 1 */

    if(victim!=NULL)
    {
      pi->nodeCount= pi->nodeCount+1;
    }
    return victim;
  }
}

/* Page Replacement for CLOCK */ 

bool Clock(int pageNum, int totalPages, BM_BufferPool* const bm)
{
	
  SM_FileHandle fHandle;
  PoolInfo *pi= (PoolInfo*) bm->mgmtData;
  PageFrame *pf= pi->pf;
  PageFrame *tmp = pi->pf;
  int emptyFrameCnt = getEmptyFramesCnt(pf);
  bool hit = isClockHit(pageNum, pf);
  bool  nodeFound = 0;
  do
  { 
    if (tmp->ph.pageNum == -1)
    {
	tmp->ph.pageNum = pageNum;
	break; 
    }	
    else if (emptyFrameCnt > 0) 
    {
	tmp = tmp->next;
    }
    else if (emptyFrameCnt == 0)
    {	
	tmp = getCLOCKNodeToReplace(pageNum,pi,totalPages);
	if (tmp->dirty == TRUE) 
	{
	  CHECK(openPageFile(bm->pageFile, &fHandle));
	  CHECK(writeBlock(tmp->ph.pageNum, &fHandle, tmp->ph.data));/* Write data from handler to file */
	  pi->writePages++; /*incrementing write pages count */
	  tmp->dirty = FALSE;
	  CHECK(closePageFile(&fHandle));
	}
	if (tmp != NULL)
	{
	  tmp->ph.pageNum = pageNum;
	  break;
	}
	else
	  return 0; /*no change */	
    }
  }while (tmp != pf);

  return 1; /* if it comes here page s successfully inserted/replaced */
}


/* Checks if the page is present in the buffer for clock */

bool isClockHit(int pageNum, PageFrame *pf)
{
  PageFrame* tmp = pf;
  bool hit =0;
  while (tmp != NULL && tmp != pf)
  {
    if (tmp->ph.pageNum == pageNum) /*if page already found, just set its ref bit */
    { 
	hit = 1;
	tmp->position = 1;
	break;
    }
    else
    {
	tmp = tmp->next;
    }
  }
  return hit;
}


/* searches if a page is present in the buffer pool */

bool searchLinkList(int pageNum, PageFrame* pf)
{
  PageFrame *tmp = pf;
  bool found = 0;
  int i = 0;
  do 
  {
    if (tmp->ph.pageNum == pageNum) /* checks whether the given page number exists in the buffer pool or not */
    {
	found =1;
	break;
    } 
    else
	tmp = tmp->next;
  } while (tmp != NULL && tmp != pf);

  return found;		
}


/* returns the recently accessed page frame */ 

PageFrame *getPageFrame(int pageNum, PageFrame* pf)
{
  PageFrame *tmp = pf;
  PageFrame *pageFrame;
  do
  {
    if (tmp->ph.pageNum == pageNum)
    {
	tmp->ph.data = malloc(4096);
	pageFrame = tmp;
	break;
    }
    else
	tmp = tmp->next;
  }while(tmp != NULL && tmp != pf);
  return pageFrame;
}


