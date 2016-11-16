#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include "list.h"
#include "buffer_mgr.h"
#include "dberror.h"


/* creation of linked list and intialising the required values */

PageFrame* linked_list(int numPages)
{
  PageFrame *new_node, *current, *start;
  int i;
  int count =0;
  while ( count < numPages)
  {
    new_node = (PageFrame *) malloc(sizeof(PageFrame)*numPages); /* allocating the size for the linked list */
    new_node->dirty = FALSE;
    new_node->fixCount =0;
    new_node->handle = FALSE;
    new_node->ph.pageNum = -1;
    new_node->ph.data = NULL;
    new_node->position = 0;
    if (count==0)
    {
	start =new_node;
	current = new_node;
    }
    else
    {
        current->next = new_node;
	current = new_node;
    }
    current->next = start;
    count ++;
}
return start;

}

/* displays the contents of the linked list */

void display_list(BM_BufferPool *const bm,int numPages)
{
  PoolInfo *pi;
  PageFrame *pf;
  pi = (PoolInfo*) bm->mgmtData;
  pf = pi->pf;
  int i = 0;
  do 
  {
    if (i < numPages) 
    {
	printf("\nfix count = [%d] and FRAME [%d] and dirty [%d] and position [%d] and Handle[%d] and Pageno[%d]\n",pf->fixCount, i, pf->dirty,pf->position,pf->handle,pf->ph.pageNum );
	i = i + 1;
	pf = pf->next;
    } 
    else
	break;
  } while (pf != NULL);
}




