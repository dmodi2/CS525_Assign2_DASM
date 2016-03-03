												Buffer Manager
  												   README

Buffer Manager acts as a main memory based cache between database files (managed by Storage Manager) and its clients.
Buffer Manager can only hold specific number of file blocks in page frames it's allowed to have.

Highlights/Extras:
------------------

•	Very High Performance (Takes ~13 sec on fourier for 1M pages' Read + Dirty + Write ops with only 3 page frames).
   
•	If a client requests pages not already present as blocks in underlying pageFile, we have to add new block to disk before that page is swapped out from pool.
  
•	Thread Safe, with optimizations to reduce lock overheads.
   
•	We have implemented pthread_mutex_t lock on initBufferPool, pinPage and unPinPage methods. This approach reduces waiting time compared to that caused by lock approach.
   
•	Stress tested with 10M pages on fourier.

•	Simple design. Well organized code base.

I. Contents
-----------
All source files (.c, .h)
this README file
Makefile

II. Build and Run
------------------
A. Build
	$ make assign2_1

B. Execute
	$ ./assign2_1

C. Clean
	$ make clean


III. Design and Implementation
------------------------------
A. Data Structures

* BM_BufferPool
	Contains information about a buffer pool which typically specifies the page file associated with the buffer pool, maximum number of pages of the corresponding page file that the buffer pool can hold, the page replacement strategy used by buffer manager for this page file and bookkeeping data.

* BM_PageHandle
	Contains information about a page. Stores position of the page in the file (page num) and pointer to memory area holding contents of the page.


B. Methods

* FIFO - Dhruvit Modi (dmodi2@hawk.iit.edu)
       	Page replacement strategy to replace page frames using first in first out algorithm.

* LRU - Anirudh Deshpande(adeshp17@hawk.iit.edu)
	Page replacement strategy to replace page frames using least recently used algorithm.

* Mutex Locks - Anirudh Deshpande(adeshp17@hawk.iit.edu)
	Mutex locks allows only single thread to execute the critical  section of the program by blocking all the other processes try to execute concurrently
	
* initBufferPool - Dhruvit Modi(dmodi2@hawk.iit.edu)
	Calls initPageFrame. Initializes buffer pool. Creates and initializes the buffer pool. 
	Returns RC_OK when success

* initPageFrame - Dhruvit Modi  (dmodi2@hawk.iit.edu)
	This method will initialize the contents of BufferPool.
	On success returns RC_OK.

* shutdownBufferPool - Sahil Chalke(schalke@hawk.iit.edu)
	Forces writes any dirty pages to disk and if all pages have fix count of 0. AT the end of this operations, all the buffer manager 
	resources are released.	Unsuccessful attempt returns following error codes -
	RC_BUFFER_POOL_NOT_INIT;
	

* forceFlushPool - Sahil Chalke(schalke@hawk.iit.edu)
	Writes all dirty pages from buffer pool with fix count of 0 to disk. Once the dirty pages are flushed off to the disk, the dirty flags
	are reset. Returns following error codes -
	RC_BUFFER_POOL_NOT_INIT
	
* markDirty - Sahil Chalke(schalke@hawk.iit.edu)
	Marks the specified page in the buffer pool as modified or dirty and also increments the dirty page count. Returns RC_OK on sucess, else
	following error codes are returned -
	RC_BUFFER_POOL_NOT_INIT
	RC_PAGE_NOT_PINNED_IN_BUFFER_POOL

* unpinPage- - Anirudh Deshpande(adeshp17@hawk.iit.edu)
	Unpins and markes the page for swap out. As the buffer manager uses lazy page swap out strategy, unpinned pages are swapped out only
	when pool gets full. This approach significantly reduces the number of disk reads made. Returns RC_OK on success, else following error
	codes are returned
	RC_BUFFER_POOL_NOT_INIT
	

* forcePage - Anirudh Deshpande(adeshp17@hawk.iit.edu)
	Force writes the specified page to disk irrespective of whether its dirty or not. Returns RC_OK on success, else following error
        codes are returned
	RC_BUFFER_POOL_NOT_INIT
	RC_PAGE_NOT_PINNED_IN_BUFFER_POOL
	
* pinPage - Dhruvit Modi (dmodi2@hawk.iit.edu)
	Pins page to an empty page frame if it doesn't already exist in any of the frames, otherwise data from already pinned page is returned.
	Returns RC_OK on success, else following error codes are returned
	RC_BUFFER_POOL_NOT_INIT
	RC_READ_ERROR.
	
* getFrameContents - Monika Priyadarshini(mpriyad1@hawk.iit.edu)
	Returns an array of PageNumbers (of size numPages) where the ith element is the number of the page stored in the ith page frame.

* getDirtyFlags - Monika Priyadarshini(mpriyad1@hawk.iit.edu)
	Returns an array of bools (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty. Empty 
	page frames are considered as clean.

* getFixCounts - Monika Priyadarshini(mpriyad1@hawk.iit.edu)
	Returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame. Returns 0 
	for empty page frames.

* getNumReadIO - Sahil Chalke(schalke@hawk.iit.edu)
	Returns the number of pages that have been read from disk since a buffer pool has been initialized. 

* getNumWriteIO - Monika Priyadarshini(mpriyad1@hawk.iit.edu)
	Returns the number of pages that have been written to disk since a buffer pool has been initialized. 
	
________________________________________________________________________________________________________________________________________________________

* Readme: Monika Priyadarshini(mpriyad1@hawk.iit.edu)
* Makefile: Sahil Chalke(schalke@hawk.iit2.edu)

