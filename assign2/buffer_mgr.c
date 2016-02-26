// Standard C libraries
#include<stdio.h>
#include<stdlib.h>

// Local libraries
#include "buffer_mgr.h"

/*int main(){
	
	createPageFile("testbuffer.bin");
	BM_BufferPool * const bm = MAKE_POOL();
	initBufferPool(bm, "testbuffer.bin", 3, RS_FIFO, NULL);
	return 0;
}*/

int readCount = 0, writeCount = 0;
pageListT *head = NULL;

/***Replacement stratagies implementation****/
/**
 * FIFO
 * */
 
extern RC FIF0(BM_BufferPool *const bm, pageListT *pageT){
	
	printf("Begin: FIFO\n");
	if(bm == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
    }
    pageListT *node = (pageListT *)bm->mgmtData;
  printf("FIFO: Pages in memory.\n");
  while(node != NULL){
		printf("%d\n",node->pgNum);
		node = node->next;
	}
	node = (pageListT *)bm->mgmtData;
    SM_FileHandle fHandle;
    while(node != NULL){
	  if(node->fifoBit == 1){
		  if(node->fixCount == 0){
		     /* If the page is modified by the client, then write the page to disk*/
			 if(node->dirtyBit == 1)
			 {
				openPageFile(bm->pageFile, &fHandle);
		   		writeBlock(node->pgNum, &fHandle, node->data);
		   		writeCount++;
			 }
			 node->data = pageT->data;
			 node->pgNum = pageT->pgNum;
			 node->dirtyBit = 0;
			 node->fifoBit = 0;
			 if(node->next == NULL){
			   head->fifoBit = 1;
		     }else{
			   node->next->fifoBit = 1;	
			 }
			 node->fixCount = 1;
			 return RC_OK;
		}
		node->next->fifoBit = 1;
	  }
      node = node->next;
    }	
	return RC_NO_UNPINNED_PAGES_IN_BUFFER_POOL;
}

/**
 * clock
 * */
extern RC clock(BM_BufferPool *const bm, pageListT *pageT){
	
	if(bm == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
    }
    
    pageListT *node = (pageListT *)bm->mgmtData;
	SM_FileHandle fHandle;
	
	while(true)
	{
		if(node->useCount == 1) 
		{
			if(node->dirtyBit == 1)
			{
				openPageFile(bm->pageFile, &fHandle);
				writeBlock(node->pgNum, &fHandle, node->data);
				writeCount++;
				closePageFile(&fHandle);
			}
			node->data = pageT -> data;
			node->pgNum = pageT -> pgNum;
			node->fixCount = 1;
			node->dirtyBit = 0;
			node->useCount = 0;
			node = node->next;
			return RC_OK;
		}
		else
		{
			node->useCount = 0;
			node = node->next;
		}
	}
}

/**
 * */
 /*extern void LRU(BM_BufferPool *const bm, pageListT *pageT, const int numPages)
{
    pageListT *node = (pageListT *)bm->mgmtData;
    pageListT *lruNode = node;
   
    while(node->next != NULL)
    {
        if(node->fixCount == 0)
        {
            if(node->usedTime.tv_usec < temp->usedTime.tv_usec)
            {
                lruNode = node;
            }
        }
        node = node->next;
    }
   
    if(lruNode->dirtyBit==1)
    {
        //TODO
            //SM_fileHandle fHandle;
            //openPageFile(bm->pageFile, &fHandle);
            //writeBlock(lruNode->pgNum,
    }
    s
    lruNode->data = null;
    lruNode->dirtyBit = 0;
    lruNode->fixedCount = 1;
    lruNode->useCount = 0;
   
    pageT = lruNode;
}*/

/**
 * initBufferPool
 * */
RC initBufferPool(BM_BufferPool *const bm, char *pageFileName, 
					const int numPages, ReplacementStrategy strategy, 
					void *stratData){
  head = (pageListT *)malloc(sizeof(pageListT));						
  bm->pageFile = pageFileName;
  bm->numPages = numPages;
  bm->strategy = strategy;
  writeCount = 0;
  readCount = 0;
  
   // Initilize head of the page frame.  
    head->data = NULL;
    head->fixCount = 0;
    head->dirtyBit = 0;
    head->fifoBit = 1;
    head->pgNum = NO_PAGE;
    head->next = NULL;
    
  int bufferSize = numPages;
  
  //pageListT *node = NULL;
  while(bufferSize > 1){
    initPageFrame(head);
    bufferSize--;
  }
  //bm->mgmtData = node;
  bm->mgmtData = head;
  return RC_OK; 
}

/**
 * initPageFrame
 * */
RC initPageFrame(pageListT* head_ref){
	
	//Represents a page in a frame.
    pageListT* current = head_ref;
    while(current->next != NULL){
		current = current->next;
	}
	current->next = (pageListT *)malloc(sizeof(pageListT));
	current->next->data = NULL;
	current->next->fixCount = 0;
	current->next->dirtyBit = 0;
	current->next->fifoBit = 0;
	current->next->pgNum = NO_PAGE;
	current->next->next = NULL;
}

/**
 * shutdownBufferPool
 * */
RC shutdownBufferPool(BM_BufferPool *const bm){
  
  printf("shutdownBufferPool\n");
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
  return RC_OK;
}

/**
 * forceFlushPool
 * */
RC forceFlushPool(BM_BufferPool *const bm){
  
  printf("forceFlushPool\n");
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
	    writeCount++;
	  }
	  node = node->next;
  }
  printf("forceFlushPool\n");
  return RC_OK;
}
/**
 * markDirty
 * */
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){
  
  printf("markDirty page: %d\n", page->pageNum);
  if(bm == NULL){
	return RC_BUFFER_POOL_NOT_INIT;
  }
  pageListT *node = (pageListT *)bm->mgmtData;
  while(node != NULL){
	  if(node->pgNum == page->pageNum){
		node->data = page->data;  
        node->dirtyBit = 1;
        return RC_OK;
	  }
	  node = node->next;
  }
	return RC_PAGE_NOT_PINNED_IN_BUFFER_POOL;
}
/**
 * forcePage
 * */
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){
  
  printf("forcePage\n");
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
  if(node->dirtyBit == 1){  
    openPageFile(bm->pageFile, &fHandle);
    writeBlock(node->pgNum, &fHandle, node->data);
    node->dirtyBit = 0;  
    writeCount++;
  }
  
  return RC_OK;
}

/**
 * pinPage
 * */
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum){
	
	printf("begin: pinPage\n");		
	if(bm == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
    }		
  			
  	pageListT *node = (pageListT *)bm->mgmtData;
  	
  	//SM_FileHandle fHandle;
  	RC readError;
  	// No pages in memory.
  	if(node->pgNum == NO_PAGE){
		printf("pinPage: No pages in memory\n");
		SM_FileHandle fHandle;	
		openPageFile(bm->pageFile, &fHandle);
		node->data = (SM_PageHandle)malloc(PAGE_SIZE);
		readError = readBlock(pageNum, &fHandle, node->data);
		if(readError == RC_READ_ERROR){
		  return readError;
	    }
	    readCount++;
		node->pgNum = pageNum;
		node->fixCount++;
		page->pageNum = pageNum;
		page->data = node->data;
		closePageFile(&fHandle);
		return RC_OK;
	}else{
		while(node != NULL && node->pgNum != NO_PAGE){
		  // Page already exist in memory	
		  if(node->pgNum == pageNum){
			  printf("pinPage: page already exists in memory\n");
			  node->fixCount++;
			  page->pageNum = pageNum;
			  page->data = node->data;
			  return RC_OK;
		  }
		  node = node->next; 
		}
		// Page not in memory and buffer has spce left.
		if(node != NULL){
			printf("pinPage: page not in memory\n");
			SM_FileHandle fh;
		    openPageFile (bm->pageFile, &fh);
			node->data = (SM_PageHandle)malloc(PAGE_SIZE);
			readError = readBlock(pageNum, &fh, node->data);
			if(readError == RC_READ_ERROR){
			  return readError;
		    }
	        readCount++;
			node->pgNum = pageNum;
			node->fixCount++;
			page->pageNum = pageNum;
			page->data = node->data;
			return RC_OK;
		}
		// Page not in memory and buffer full. Replace page
		else{
			printf("pinPage: buffer full\n");
			SM_FileHandle fHandle;
			pageListT *newNode = (pageListT *)malloc(sizeof(pageListT));
			openPageFile(bm->pageFile, &fHandle);
			newNode->data = (SM_PageHandle)malloc(PAGE_SIZE);
			readError = readBlock(pageNum, &fHandle, newNode->data);
			newNode->pgNum = pageNum;
			if(readError == RC_READ_ERROR){
		      return RC_READ_ERROR;
		    }
		    readCount++;
		    // Implement replacement startegy  
			if(bm->strategy == RS_FIFO)
			  FIF0(bm, newNode);
			else if(bm->strategy == RS_CLOCK)
			  clock(bm, newNode);
			else if(bm->strategy == RS_LRU);
			  //call to LRU
			page->pageNum = pageNum;
			page->data = newNode->data;  
			
			return RC_OK;
		}
	}		
}

RC unpinPage(BM_BufferPool * const bm, BM_PageHandle * const page) {
    
    printf("unpinPage %d\n", page->pageNum);
	if(bm == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
    }		
  			
  	pageListT *node = (pageListT *)bm->mgmtData;
		while(node != NULL && node->pgNum != NO_PAGE){	
		  if(node->pgNum == page->pageNum){
			  node->fixCount--;
			  break;
			}
			node = node->next;
		}			  
 return RC_OK;
}

int getNumReadIO (BM_BufferPool *const bm){

	   return readCount;

}

int getNumWriteIO (BM_BufferPool *const bm){

      	return writeCount;
}

bool *getDirtyFlags(BM_BufferPool *const bm) {
    
    bool *flags = (bool*)malloc(sizeof(bool) * bm->numPages);
    
    pageListT *node = (pageListT *)bm->mgmtData;
    
    int i;
    for (i = 0; i < bm->numPages; i++) {
        flags[i] = node->dirtyBit;
        node = node->next;
    }

    return flags;
}

int *getFixCounts(BM_BufferPool *const bm) {
    int *fixcount = malloc(sizeof(bool) * bm->numPages);
    
    	pageListT *node = (pageListT *)bm->mgmtData;
    
    int i;
    for (i = 0; i < bm->numPages; i++) {
        fixcount[i] = node->fixCount;
        node = node->next;
    }

    return fixcount;
}

PageNumber *getFrameContents(BM_BufferPool *const bm) {
    int *content = malloc(sizeof(int) * bm->numPages);
    pageListT *node = (pageListT *)bm->mgmtData;
    int i;
    for (i = 0; i < bm->numPages; i++) {
        if (node->pgNum != NO_PAGE) {
            content[i] = node->pgNum;
        } else {
            content[i] = NO_PAGE;
        }
        node = node->next;
    }
    return content;
}
