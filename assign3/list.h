#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "record_mgr.h"
#include "tables.h"

/* ----------------- NEW DATA STRUCTURES DEFINED --------------------- */

typedef struct PageFrame {
                int fixCount; /* no of times page is pinned */
                bool dirty; /* tells if page is dirty */
                BM_PageHandle ph; /* PageHandle contains page no and data */
                int position; /* used for LRU algo (at what position this page inserted recently) for eg 2, 4, 1 ,3, 4 Position of 2 is 0, 4 is 4, 1 is 2, 3 is 3. */
                bool handle;  /* used for clock strategy */
                struct PageFrame *next;
}PageFrame;

typedef struct PoolInfo {
	int readPages; /* no of pages read from page file since pool initialisation */
	int writePages; /* no of pages written to page file since pool initialisation */
	PageFrame *pf; /* structure to store each information about the page */
	int position; /* global position no for pages used for LRU strategy */
	int nodeCount; /* used for FIFO */
	int lru_k; /* used for LRU_K */
	SM_FileHandle fh;

} PoolInfo;


// management data for RM_TableData
typedef struct TableInfo {
    int totalRecords; 
    BM_BufferPool *bm;
    int recordLength;
    int numSlots;
    char *name;
    int end;
}TableInfo;

typedef struct ScanInfo{
    int page; // current page
    int slot; // current slot
    Expr *condition;
}ScanInfo;

/* ----------------- ADDITIONAL FUNCTIONS USED --------------------- */

PageFrame* linked_list(int n);
bool searchLinkList(int pageNum, PageFrame* pf);
bool FIFO1(int pageNum, BM_BufferPool * const bm);
bool LRU(int pageNum, BM_BufferPool * const bm);
PageFrame *getPageFrame(int pageNum, PageFrame* pf);
PageFrame* getFIFONodeToReplace(PoolInfo* pi);
PageFrame* getCLOCKNodeToReplace(int pageNum , PoolInfo* pi,int totalPages);
int getEmptyFramesCnt(PageFrame *pf);
bool ifHit(int pageNum, PoolInfo* pi);
bool ifHit_LRU_K(int pageNum, PoolInfo* pi);
PageFrame* getVictimToReplace(PageFrame *pf);
void display_list(BM_BufferPool *const bm,int numPages);
bool isClockHit(int pageNum, PageFrame *pf);
bool Clock(int pageNum, int totalPages, BM_BufferPool* const bm);
bool LRU_K(int pageNum,int totalPages, BM_BufferPool* const bm);
PageFrame* getVictimToReplace_LRU_K(int pageNum,PageFrame *pf,BM_BufferPool* const bm);



