// Standard C libraries
#include<stdio.h>
#include<stdlib.h>

// Local libraries
#include "buffer_mgr.h"

int main(){
	
	createPageFile("testbuffer.bin");
	BM_BufferPool * const bm = MAKE_POOL();
	initBufferPool(bm, "testbuffer.bin", 3, RS_FIFO, NULL);
	return 0;
}

/***Replacement stratagies implementation****/
/**
 * FIFO
 * */
 
extern RC FIF0(BM_BufferPool *const bm, pageListT *pageT){
	
	if(bm == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
    }
    
	pageListT *node = (pageListT *)bm->mgmtData;
	SM_FileHandle fHandle;
  	while( node!= NULL)
  	{	
		if(node->fixCount == 0)
		{	
			/* If the page is modified by the client, then write the page to disk*/
			if(node->dirtyBit == 1)
			{
				openPageFile(bm->pageFile, &fHandle);
				writeBlock(node->pgNum, &fHandle, node->data);
				closePageFile(&fHandle);
			}
			/* Assigning the new page contents*/
			node->data = pageT -> data;
			node->pgNum = pageT -> pgNum;
			node->dirtyBit = 0;
			node->fixCount = 0;
			return RC_OK;
		}
		node = node->next;
	}
	return RC_OK;
}

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
    node->readCount=0;
    node->writeCount=0;
    // Linked list pointers.
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
 * pinPage
 * */
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum){
			
	if(bm == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
    }		
  			
  	pageListT *node = (pageListT *)bm->mgmtData;
  	SM_FileHandle fHandle;
  	RC readError;
  	// No pages in memory.
  	if(node->pgNum == NO_PAGE){
			
		openPageFile(bm->pageFile, &fHandle);
		node->data = (SM_PageHandle)malloc(sizeof(PAGE_SIZE));
		readError = readBlock(pageNum, &fHandle, node->data);
		if(readError == RC_READ_ERROR)
		  return RC_READ_ERROR;
		node->pgNum = page->pageNum;
		node->fixCount++;
		page->pageNum = pageNum;
		page->data = node->data;
		return RC_OK;
	}
	else{
		while(node != NULL || node->pgNum != NO_PAGE){
		  // Page already exist in memory	
		  if(node->pgNum == pageNum){
			  node->fixCount++;
			  page->pageNum = pageNum;
			  page->data = node->data;
			  return RC_OK;
		  }
		  node = node->next; 
		}
		// Page not in memory.
		if(node != NULL){
			openPageFile(bm->pageFile, &fHandle);
			node->data = (SM_PageHandle)malloc(sizeof(PAGE_SIZE));
			readError = readBlock(pageNum, &fHandle, node->data);
			if(readError == RC_READ_ERROR)
			  return RC_READ_ERROR;
			node->pgNum = page->pageNum;
			node->fixCount++;
			page->pageNum = pageNum;
			page->data = node->data;
			return RC_OK;
		}
		// Page not in memory and buffer full. Replace page
		else{
			pageListT *newNode = (pageListT *)malloc(sizeof(pageListT));
			openPageFile(bm->pageFile, &fHandle);
			node->data = (SM_PageHandle)malloc(sizeof(PAGE_SIZE));
			readError = readBlock(pageNum, &fHandle, newNode->data);
			newNode->pgNum = pageNum;
			if(readError == RC_READ_ERROR)
		      return RC_READ_ERROR;
		      
			if(bm->strategy == RS_FIFO)
			  FIF0(bm, newNode);
			else if(bm->strategy == RS_LRU);
			  //implement LRU
			page->pageNum = pageNum;
			page->data = newNode->data;  
			return RC_OK;
		}
	}		
}
