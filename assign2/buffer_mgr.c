// Standard C libraries
#include<stdio.h>
#include<stdlib.h>

// Local libraries
#include "buffer_mgr.h"
#include "storage_mgr.h"

int main(){
	
	createPageFile("testbuffer.bin");
	BM_BufferPool * const bm = MAKE_POOL();
	initBufferPool(bm, "testbuffer.bin", 3, RS_FIFO, NULL);
	return 0;
}

int front, last;

/**
 * initBufferPool
 * */
RC initBufferPool(BM_BufferPool *const bm, char *pageFileName, 
					const int numPages, ReplacementStrategy strategy, 
					void *stratData){
  
  bm->pageFile = pageFileName;
  bm->numPages = numPages;
  bm->strategy = strategy;
  
  int bufferSize = numPages;
  
  pageListT *node = NULL;
  while(bufferSize > 0){
    initPageFrame(&node);
    bufferSize--;
  }
  bm->mgmtData = node;
  //printlist(node);
  return RC_OK; 
}

/**
 * initPageFrame
 * */
RC initPageFrame(pageListT** head_ref){
	
    pageListT* node = (pageListT *)malloc(sizeof(pageListT));
    
    /*SM_PageHandle ph;
    int i;
    ph = (SM_PageHandle) malloc(10);
    for (i=0; i < 10; i++)
      ph[i] = '0';*/
      
    // Initilize empty page frame.  
    node->data = NULL;
    node->fixCount = 0;
    node->dirtyBit = 0;
    node->pgNum = NO_PAGE;
    node->useCount = 0;
    node->next = (*head_ref);
    (*head_ref)->prev = node;
    (*head_ref) = node;
    
    return RC_OK;
}

/*void printlist(pageListT* node){
    while(node != NULL){
      printf("%sDONE\n", node->data);
      node = node->next;
    }
}*/

/**
 * shutdownBufferPool
 * */
RC shutdownBufferPool(BM_BufferPool *const bm){

  if(bm == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
  }
  
  pageListT *node = (pageListT *)bm->mgmtData;
  
  /*Check each node to see if its fix count is 0*/  
  while(node != NULL){
	  if(node->fixCount != 0)
	    return RC_BUFFER_POOL_CONTAINS_PINNED_PAGES;
	  node = node->next;  
  }
  forceFlushPool(bm);
  free(node);
  bm->mgmtData = NULL;
  free(bm);
  return RC_OK;
}

/**
 * forceFlushPool
 * */
RC forceFlushPool(BM_BufferPool *const bm){
  
  if(bm == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
  }
  
  pageListT *node = (pageListT *)bm->mgmtData;
  SM_FileHandle fHandle;
  
  /*Check each node to see if its dirty bit is set and fix count is 0*/  
  while(node != NULL){
	  if(node->dirtyBit == 1 && node->fixCount == 0){
	    openPageFile(bm->pageFile, &fHandle);
	    writeBlock(node->pgNum, &fHandle, node->data);
	    node->dirtyBit = 0;
	  }
	  node = node->next;
  }
  closePageFile(&fHandle);
  return RC_OK;
}
/**
 * markDirty
 * */
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){
	
  if(bm == NULL){
	return RC_BUFFER_POOL_NOT_INIT;
  }

  pageListT *node = (pageListT *)bm->mgmtData;
  
  while(node->pgNum != page->pageNum){
	  node = node->next;
  }
  if(node == NULL)
    return RC_PAGE_NOT_PINNED_IN_BUFFER_POOL;
    
  node->dirtyBit = 1;
  return RC_OK;
}
/**
 * forcePage
 * */
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){

  if(bm == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
  }
  
  pageListT *node = (pageListT *)bm->mgmtData;
  SM_FileHandle fHandle;
  
  while(node->pgNum != page->pageNum){
	  node = node->next;
  }
  if(node == NULL)
    return RC_PAGE_NOT_PINNED_IN_BUFFER_POOL;
    
  openPageFile(bm->pageFile, &fHandle);
  writeBlock(node->pgNum, &fHandle, node->data);
  closePageFile(&fHandle);
  node->dirtyBit = 0;  
  
  return RC_OK;
}

/**
 * FIFO
 * */
 
extern void FIF0(BM_BufferPool *const bm, pageListT *pageT, const int numPages){
	
	
	pageListT *node = (pageListT *)bm->mgmtData;
	
	int bufferSize = numPages;
	
	
	front=last%bufferSize;
	
	for( int i=0; i<bufferSize; i++)
	{
		if(node[front].fixCount == 0)
		{
			if(node[front].dirtyBit == 1)
			{
				SM_FileHandle fHandle;
				openPageFile(bm->pageFile, &fHandle);
				writeBlock(node[front].pageNum, &fHandle, node[front].data);
				mgmtData->writeCount++;
			}
			
			node[front].data = pageT->data;
			node[front].pageNum = pageT->pageNum;
			node[front].dirtyBit = pageT->dirtyBit;
			node[front].fixCount = pageT->fixCount;
			break;
		}
		else
		{
			front++;
			front = (front % bufferSize == 0) ? 0 : front; 
		}
  	}
 }
 
 
extern int getNumReadIO (BM_BufferPool *const bm){
	
	if(bm->mgmtData == NULL)dd
	{
		return false;
	}
	else
	{
		pageListT *node = (pageListT *)bm->mgmtData;
		return node->readCount;	
	}

extern int getNumWriteIO (BM_BufferPool *const bm){
	
	if(bm->mgmtData == NULL)
	{
		return false;
	}
	else
	{
		pageListT *node = (pageListT *)bm->mgmtData;
		return node->writeCount;	
	}

}
	
 
 



