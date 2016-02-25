CC=gcc

default: assign2_1

assign2_1: test_assign2_1.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o
		$(CC) -o assign2_1 test_assign2_1.o buffer_mgr.o buffer_mgr_stat.o  storage_mgr.o dberror.o -I.
				
		
dberror.o: dberror.c
	$(CC) -o dberror.o -c dberror.c

storage_mgr.o: storage_mgr.c
	$(CC) -o storage_mgr.o -c storage_mgr.c

test_assign2_1.o: test_assign2_1.c
	$(CC) -o test_assign2_1.o -c test_assign2_1.c	

buffer_mgr.o: buffer_mgr.c
	$(CC) -o buffer_mgr.o -c buffer_mgr.c

buffer_mgr_stat.o: buffer_mgr_stat.c
	$(CC) -o buffer_mgr_stat.o -c buffer_mgr_stat.c
   
clean:
	rm *.o assign2_1 op.txt
