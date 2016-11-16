#include "storage_mgr.h"
#include <stdio.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <stdlib.h>
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <fcntl.h> 


/* manipulating page files */

/*Intialising Storage Manager */
void initStorageManager (void)
{ 


}

/*createPageFile method - creates a new page file and fills the page with '\0' bytes.*/
RC createPageFile (char *fileName)
{ 
char create_char[PAGE_SIZE];
FILE *fp;
fp = fopen(fileName,"wb+");  
if (fp==NULL)
{
	return RC_FILE_NOT_FOUND;
}
else
{	  
	memset(create_char,'\0',PAGE_SIZE);
	fwrite(create_char ,1 , PAGE_SIZE,fp); 
	fclose(fp);  
	return RC_OK;
}	
}

/*openPageFile - opens an existing page file if found and intialises the Handler with the file information. If not found, returns RC_FILE_FOUND. */

RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{ 
	FILE *fp;
	fp = fopen(fileName , "rwb+"); 
	
	if(fp != NULL)
	{
		fHandle->mgmtInfo = fp;    
		fHandle->fileName = fileName;
		fHandle->curPagePos = 0;
		fseek(fp, 0, SEEK_END);
		fHandle->totalNumPages = (ftell(fp) / PAGE_SIZE );
		return RC_OK;
	}
	else{
		return RC_FILE_NOT_FOUND;
	}

}

/*closePageFile - closes the opened page file.*/

RC closePageFile (SM_FileHandle *fHandle)
{
	FILE *fp = (FILE*) fHandle->mgmtInfo;
	if (fp == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	fHandle->fileName = NULL;                
	fHandle->totalNumPages = 0;              
	fHandle->curPagePos = 0;                 
	fHandle->mgmtInfo = NULL;                
	fp=NULL;
	fHandle =NULL;
        return RC_OK;
}

/*destroyPageFile - deletes the opened page file.*/
RC destroyPageFile(char *fileName)
{	
	int destroyPage = remove(fileName); 
	if(destroyPage == 0){
	return RC_OK;
   	}
   	else {
	return RC_FILE_NOT_FOUND; 
   	}
}


/* reading blocks from disc */

/*readBlock - reads the page from 0 to PAGE_SIZE as per pageNum information provided.*/

RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{

FILE *fp = (FILE*) fHandle->mgmtInfo;
 
if (fp==NULL)
{
	return RC_FILE_NOT_FOUND;
} 
else 
{

	if ( pageNum > fHandle->totalNumPages )
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	else
	{
		int status=fseek(fp,pageNum*PAGE_SIZE,SEEK_SET);
		if(status==0)
		{
			fread(memPage,PAGE_SIZE,1,fp);
			fHandle->curPagePos = pageNum;
			return RC_OK;
		}
		else
		{
			return RC_READ_NON_EXISTING_PAGE;
		}

	}
}

}

/*readFirstBlock - reads the very first block till the end i.e., PAGE_SIZE: 4096*/

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	readBlock(0,fHandle,memPage);
}


/*readPreviousBlock - reads the previous block by using the current position of the block (getBlockPos() -1)*/

RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	int previousBlock = getBlockPos(fHandle) - 1;
        readBlock(previousBlock,fHandle,memPage);
}


/*readCurrentBlock - reads the current block by using current position of the block (getBlockPos())*/

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	int currentBlock = getBlockPos(fHandle);
	readBlock(currentBlock,fHandle,memPage);
}


/*readNextBlock - reads the next block by using current positon of the block (getBlockPos() -1)*/

RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	int nextBlock = getBlockPos(fHandle) + 1;
	readBlock(nextBlock,fHandle,memPage);
}


/*readLastBlock - reads the last block by using the total number of pages present. */

RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	int lastBlock= fHandle->totalNumPages -1;
	readBlock(lastBlock,fHandle,memPage);
}


/*getBlockPos - reads the current position of the block inside the file. */

int getBlockPos(SM_FileHandle *fHandle)
{
	return fHandle->curPagePos;
}


/* writing blocks to a page file */

/*writeBlock - writes the contents page handler to the file. */

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
int startPosition, status;
FILE *fp = (FILE*) fHandle->mgmtInfo;

if (fp==NULL)
{
	return RC_FILE_NOT_FOUND;
}	
if (pageNum < 0 || pageNum > fHandle->totalNumPages)
{	
		
	return RC_READ_NON_EXISTING_PAGE;
}
else
{
	startPosition = pageNum * PAGE_SIZE;
	status = fseek(fp,startPosition, SEEK_SET);

	if(status == 0)
	{				
		fwrite(memPage,1,PAGE_SIZE,fp);
        	return RC_OK;
        }  
        else
	{
		return RC_WRITE_FAILED;
        }
}
}

/*writeCurrentBlock - writes the contents of the page handler to the current block position. */

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{ 
 	int currentPage = getBlockPos(fHandle);
	writeBlock(currentPage,fHandle,memPage); 
}

/*appendEmptyBlock - appends the file with new empty block intializing it to '\0' bytes.*/

RC appendEmptyBlock (SM_FileHandle *fHandle)
{ 
	char create_char[PAGE_SIZE];
	FILE *fp = (FILE*) fHandle->mgmtInfo;
	if (fp == NULL) 
		return RC_FILE_NOT_FOUND;
	memset(create_char,'\0',PAGE_SIZE);
	fseek(fp, 0 , SEEK_END);
	fwrite(create_char ,1 , PAGE_SIZE,fp); 
	fHandle->totalNumPages = fHandle->totalNumPages + 1;
	fHandle->curPagePos = fHandle->totalNumPages - 1;
	return RC_OK;
}


/*ensureCapacity -  if the file has less than numberOfPages pages then increases the size to numberOfPages */

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{
	FILE *fp = (FILE*) fHandle->mgmtInfo;

	if (fp == NULL) 
		return RC_FILE_NOT_FOUND;	
	
	if (numberOfPages < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	if (numberOfPages > fHandle->totalNumPages)
	{
		do
		{
			appendEmptyBlock(fHandle);
		} while (numberOfPages > fHandle->totalNumPages );	
	}
	return RC_OK;
	
}	

