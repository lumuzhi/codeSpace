CC = 		gcc
CFLAGS = 	-Wall -g

queue:
	$(CC) $(CFLAGS) queue_main.c c/queue/myqueue.c -o a.out
tree:
	$(CC) $(CFLAGS) tree_main.c c/tree/mytree.c -o a.out
dlink:
	$(CC) $(CFLAGS) doublelinkMain.c c/doublelink/mydoublelink.c -o a.out
clean:
	rm -f *.o *.out