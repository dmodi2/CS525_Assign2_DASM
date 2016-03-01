// Standard C libraries
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
// Local libraries
#include "buffer_mgr.h"

static pthread_mutex_t mutex_init = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_unpinPage = PTHREAD_MUTEX_INITIALIZER; 
static pthread_mutex_t mutex_pinPage = PTHREAD_MUTEX_INITIALIZER;  

int readCount = 0, writeCount = 0, hit = 0;
pageListT *head = NULL;

/***Replacement stratagies implementation****/

/****************************************************************
 * Function Name: 
 * 
 * Description: 
 * 
 * Parameter: 
 * 
 * Return: 
 * 
 * Author:
 ****************************************************************/
extern RC FIF0(BM_BufferPool *const bm, pageListT *pageT){
   
    if(bm == NULL){
      return RC_BUFFER_POOL_NOT_INIT;
    }
    pageListT *node = (pageListT *)bm->mgmtData;

    while(node != NULL){
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
                closePageFile(&fHandle);
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


/****************************************************************
 * Function Name: 
 * 
 * Description: 
 * 
 * Parameter: 
 * 
 * Return: 
 * 
 * Author:
 ****************************************************************/
 extern RC LRU(BM_BufferPool *const bm,  pageListT *pageT)
{ 
    pageListT *node = (pageListT *)bm->mgmtData;
    int min = hit+1;
   
    while(node != NULL)
    {
        if(node->hitrate < min && node->fixCount == 0){
            min = node->hitrate;
        }
        node = node->next;
    }
    node = (pageListT *)bm->mgmtData;
    while(node != NULL){
        if(node->hitrate == min){
                if(node->dirtyBit==1){
                    SM_FileHandle fHandle;
                    openPageFile(bm->pageFile, &fHandle);
                       writeBlock(node->pgNum, &fHandle, node->data);
                       closePageFile(&fHandle);
                       writeCount++;
                       hit++;
                 }    
            node->data = pageT->data;
            node->pgNum = pageT->pgNum;
            node->dirtyBit = pageT->dirtyBit;
            node->fixCount = pageT->fixCount;
            node->hitrate = pageT->hitrate;
            return RC_OK;
        }
        node= node->next;
   
    }
    return RC_NO_UNPINNED_PAGES_IN_BUFFER_POOL;
}
 
/****************************************************************
 * Function Name: 
 * 
 * Description: 
 * 
 * Parameter: 
 * 
 * Return: 
 * 
 * Author:
 ****************************************************************/
RC initBufferPool(BM_BufferPool *const bm, char *pageFileName,
                    const int numPages, ReplacementStrategy strategy,
                    void *stratData){
						
  pthread_mutex_lock(&mutex_init); 				
						
  head = (pageListT *)malloc(sizeof(pageListT));                       
  bm->pageFile = pageFileName;
  bm->numPages = numPages;
  bm->strategy = strategy;
  writeCount = 0;
  readCount = 0;
  hit=0;
 
   // Initilize head of the page frame. 
    head->data = NULL;
    head->fixCount = 0;
    head->dirtyBit = 0;
    head->fifoBit = 1;
    head->pgNum = NO_PAGE;
    head->next = NULL;
    head->hitrate=0;
   
  int bufferSize = numPages;
 
  //pageListT *node = NULL;
  while(bufferSize > 1){
    initPageFrame(head);
    bufferSize--;
  }
  //bm->mgmtData = node;
  bm->mgmtData = head;
  pthread_mutex_unlock(&mutex_init); 
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
    current->next->hitrate=0;
    return RC_OK;
}

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
        closePageFile(&fHandle);
        node->dirtyBit = 0;
        writeCount++;
      }
      node = node->next;
  }
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
			
    pthread_mutex_lock(&mutex_pinPage);          
    if(bm == NULL){
      return RC_BUFFER_POOL_NOT_INIT;
    }       
         
             
      pageListT *node = (pageListT *)bm->mgmtData;
     
      //SM_FileHandle fHandle;
      RC readError;
      // No pages in memory.
      if(node->pgNum == NO_PAGE){
        SM_FileHandle fHandle;   
        openPageFile(bm->pageFile, &fHandle);
        node->data = (SM_PageHandle)malloc(PAGE_SIZE);
        readError = readBlock(pageNum, &fHandle, node->data);
        if(readError == RC_READ_ERROR){
          return readError;
        }
        readCount++;
        hit++;
        node->hitrate = hit;
        node->pgNum = pageNum;
        node->fixCount++;
        page->pageNum = pageNum;
        page->data = node->data;
        closePageFile(&fHandle);
        pthread_mutex_unlock(&mutex_pinPage);        
        return RC_OK;
    }else{
        while(node != NULL && node->pgNum != NO_PAGE){
          // Page already exist in memory   
          if(node->pgNum == pageNum){
              node->fixCount++;
              hit++;
              node->hitrate = hit;
              page->pageNum = pageNum;
              page->data = node->data;
              pthread_mutex_unlock(&mutex_pinPage);        
              return RC_OK;
          }
          node = node->next;
        }
        // Page not in memory and buffer has spce left.
        if(node != NULL){
            SM_FileHandle fHandle;
            openPageFile (bm->pageFile, &fHandle);
            node->data = (SM_PageHandle)malloc(PAGE_SIZE);
            readError = readBlock(pageNum, &fHandle, node->data);
            if(readError == RC_READ_ERROR){
              return readError;
            }
            closePageFile(&fHandle);
            readCount++;
            hit++;
            node->hitrate = hit;
            node->pgNum = pageNum;
            node->fixCount++;
            page->pageNum = pageNum;
            page->data = node->data;
            pthread_mutex_unlock(&mutex_pinPage);        
            return RC_OK;
        }
        // Page not in memory and buffer full. Replace page
        else{
            SM_FileHandle fHandle;
            pageListT *newNode = (pageListT *)malloc(sizeof(pageListT));
            openPageFile(bm->pageFile, &fHandle);
            newNode->data = (SM_PageHandle)malloc(PAGE_SIZE);
            readError = readBlock(pageNum, &fHandle, newNode->data);
            newNode->pgNum = pageNum;
            if(readError == RC_READ_ERROR){
              return RC_READ_ERROR;
            }
            closePageFile(&fHandle);
            newNode->fixCount = 1;
            newNode->dirtyBit = 0;
            newNode->hitrate = hit;
            readCount++;
            hit++;
            // Implement replacement startegy 
            if(bm->strategy == RS_FIFO)
              FIF0(bm, newNode);
            else if(bm->strategy == RS_LRU)
              LRU(bm, newNode);
            else; 
            page->pageNum = pageNum;
            page->data = newNode->data; 
            pthread_mutex_unlock(&mutex_pinPage);        
            return RC_OK;
        }
    }   
}

RC unpinPage(BM_BufferPool * const bm, BM_PageHandle * const page) {
   
    if(bm == NULL){
      return RC_BUFFER_POOL_NOT_INIT;
    }       
      pthread_mutex_lock(&mutex_unpinPage);    
             
      pageListT *node = (pageListT *)bm->mgmtData;
        while(node != NULL && node->pgNum != NO_PAGE){   
          if(node->pgNum == page->pageNum){
              node->fixCount--;
              break;
            }
            node = node->next;
        }      
        pthread_mutex_unlock(&mutex_unpinPage);           
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
