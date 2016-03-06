#include <ucontext.h>

#define MAXSIZE 16384

typedef struct Thread
{
	ucontext_t *ctx;
	struct Thread *parent;
	struct Thread *child;
	struct Thread *sib;
	int JoinPresent;
	struct Thread *next;
}UThread;

typedef char uc_stk;

UThread* currThreadNode;

UThread* rdyhead;
UThread* rdytail;

UThread* blkhead;
UThread* blktail;

ucontext_t* mainThreadctx;

typedef struct semaphore
{
    int val;
    UThread* semhead;
    UThread* semtail;   
}USemaphore;