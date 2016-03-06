#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mythread.h"
#include "mystruct.h"

void pushToRQueue(UThread* threadnode)
{
	if (rdyhead == NULL)
	{
		rdyhead = threadnode;
		rdytail = threadnode;
	}
	else
	{
		rdytail->next = threadnode;
		rdytail = rdytail->next;
	}
}

void pushToBQueue(UThread* threadnode)
{
	if (blkhead == NULL)
	{
		blkhead = threadnode;
		blktail = threadnode;
	}
	else
	{
		blktail->next = threadnode;
		blktail = blktail->next;
	}
}

void pushToSemQueue(USemaphore* tempsem, UThread* threadnode)
{
    if (tempsem->semhead == NULL)
	{
		tempsem->semhead = threadnode;
		tempsem->semtail = threadnode;
	}
	else
	{
		tempsem->semtail->next = threadnode;
		tempsem->semtail = tempsem->semtail->next;
	}
    
}

void popQueue()
{
	if (rdyhead == NULL)
	{
		currThreadNode = NULL;
		return;
	}
	else
	{
		if(rdyhead == rdytail)
		{
			rdytail = NULL;
		}
		currThreadNode = rdyhead;
		rdyhead = rdyhead->next;
		currThreadNode->next = NULL;
		return;
	}

}

void delQueue(UThread* parent)
{
	UThread* tempnode = blkhead;
	UThread* result = NULL;
	if (tempnode == NULL)
	{
		return;
	}
	else
	{
		if(tempnode == parent)
		{
			if(blktail == blkhead)
			{
				blktail = NULL;
				blkhead = NULL;
			}
			else
			{
				blkhead = blkhead->next;
				tempnode->next = NULL;
			}
			result = tempnode;
		}
		else
		{
			result = tempnode->next;
			{
				int found = 0;
				while(result != NULL)
				{
					if(result == parent)
					{
						found = 1;
						tempnode->next = result->next;
						result->next = NULL;
						if(result == blktail)
						{
							blktail = tempnode;
						}
						break;
					}
					else
					{
						tempnode = result;
						result = result->next;
					}
				}
				if(found == 0)
				{
					return;
				}
			}
		}
	}
	if (rdyhead == NULL)
	{
		rdyhead = result;
		rdytail = result;
	}
	else
	{
		rdytail->next = result;
		rdytail = rdytail->next;
	}
	return;
}

void popSemQueue(USemaphore* tempsem)
{
    //printf("In popSemQueue\n");
    //UThread *shead = tempsem->semhead;
    //UThread *stail = tempsem->semtail;
    UThread* result = NULL;
    if (tempsem->semhead == NULL)
	{
		printf("shead is NULL so returning\n");
		return;
	}
	else
	{ 
		if(tempsem->semhead == tempsem->semtail)
		{
			tempsem->semtail = NULL;
		}
        	result = tempsem->semhead;
        	tempsem->semhead = tempsem->semhead->next;
        	result->next = NULL;
	}
    // inserting the thread poppoed from semqueue to readyqueue
        //printf("Inserting removed into the ready queue\n");
	if (rdyhead == NULL)
	{
		rdyhead = result;
		rdytail = result;
	}
	else
	{
		rdytail->next = result;
		rdytail = rdytail->next;
	}
	return;
}
void MyThreadInit(void(*start_funct)(void *), void *args)
{
	rdyhead = NULL;
	rdytail = NULL;
	
	blkhead = NULL;
	blktail = NULL;
	
	UThread* threadnode = (UThread*)malloc(sizeof(UThread));

	mainThreadctx = (ucontext_t*)malloc(sizeof(ucontext_t));
	threadnode->ctx = (ucontext_t*)malloc(sizeof(ucontext_t));
	if (getcontext(threadnode->ctx) == -1)
		perror("Error getting the context");
	
	threadnode->parent = NULL;
	threadnode->child = NULL;
	threadnode->sib = NULL;
	threadnode->JoinPresent = 0;
	threadnode->next = NULL;
	threadnode->ctx->uc_link = mainThreadctx;
	threadnode->ctx->uc_stack.ss_sp = malloc(8192);
	threadnode->ctx->uc_stack.ss_size = 8192;

	makecontext(threadnode->ctx, (void(*)(void))start_funct, 1, args);
	
	currThreadNode = threadnode;
	
	swapcontext(mainThreadctx, threadnode->ctx);
}

MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
	UThread* threadnode = (UThread*)malloc(sizeof(UThread));

	threadnode->ctx = (ucontext_t*)malloc(sizeof(ucontext_t));
	if (getcontext(threadnode->ctx) == -1)
		perror("Error getting the context");
	
	threadnode->parent = currThreadNode;
	threadnode->child = NULL;
	threadnode->sib = NULL;
	threadnode->JoinPresent = 0;
	threadnode->next = NULL;
        
	threadnode->ctx->uc_link = NULL;
	threadnode->ctx->uc_stack.ss_sp = malloc(8192);
	threadnode->ctx->uc_stack.ss_size = 8192;

	makecontext(threadnode->ctx, (void(*)(void))start_funct, 1, args);
 
	if (currThreadNode->child == NULL)
    {
		currThreadNode->child = threadnode;
    }
	else
	{
		UThread* temp = currThreadNode->child;
        while (temp->sib != NULL)
        {
            temp = temp->sib;
        }
        
		temp->sib = threadnode;
	}

	pushToRQueue(threadnode);

	return (MyThread)threadnode;
}

void MyThreadYield(void)
{
	if(rdyhead == NULL)
		return;
	UThread* tempnode = currThreadNode;
	pushToRQueue(currThreadNode);
	popQueue();
	if(currThreadNode != NULL)
		swapcontext(tempnode->ctx, currThreadNode->ctx);
	return;
}

int MyThreadJoin(MyThread thread)
{
	UThread* childthread = (UThread *)thread;

	bool flag = false;

	UThread*  temp = currThreadNode->child;

	while (temp != NULL)
	{
		if (temp == thread)
		{
			flag = true;
			temp->JoinPresent = 1;
			break;
		}
		else
			temp = temp->sib;
	}

	if (!flag)
		return -1;

	else
	{
		UThread*  tempnode = currThreadNode;
		pushToBQueue(currThreadNode);
		popQueue();	
		if(currThreadNode != NULL)
			swapcontext(tempnode->ctx, currThreadNode->ctx);
		return 0;
	}
}

void MyThreadJoinAll(void)
{
	UThread*  temp = currThreadNode->child;
	if(temp == NULL)
		return;
	while (temp != NULL)
	{
		temp->JoinPresent = 1;
		temp = temp->sib;
	}

	UThread* tempNode = currThreadNode;
	pushToBQueue(currThreadNode);
	popQueue();
	if (currThreadNode != NULL)
    {
        swapcontext(tempNode->ctx, currThreadNode->ctx);
    }
}

void MyThreadExit(void)
{
	bool flag = false;
	UThread* parentthread = currThreadNode->parent;
	UThread* currChild = currThreadNode->child;
	if (parentthread != NULL)
	{
		UThread* temp = parentthread->child;

		while (temp != NULL)
		{
			if (temp->JoinPresent == 1 && temp != currThreadNode)
			{
				flag = true;
				break;
			}
			temp = temp->sib;
		}

		if ((!flag) && (currThreadNode->JoinPresent == 1))
		{
			delQueue(parentthread);
		}

		if (parentthread->child == currThreadNode)
		{
			parentthread->child = currThreadNode->sib;
		}
		else
		{
			UThread*  temp2 = parentthread->child;
			while (temp2->sib != currThreadNode)
			{
				temp2 = temp2->sib;
			}
			temp2->sib = temp2->sib->sib;
		}
		currThreadNode->parent = NULL;
	}

	while (currChild != NULL)
	{
		currChild->parent = NULL;
		currChild = currChild->sib;
	}

	UThread* temp3 = currThreadNode;
	popQueue();
	
	if (currThreadNode != NULL)
	{
		free(temp3->ctx);
		free(temp3);
		setcontext(currThreadNode->ctx);
	}
}

MySemaphore MySemaphoreInit(int initialValue)
{
    USemaphore*  mysem = (USemaphore*)malloc(sizeof(USemaphore));
     mysem->val = initialValue;
     mysem->semhead = NULL;
     mysem->semtail = NULL;
     
     return (MySemaphore)mysem;
}
void MySemaphoreWait(MySemaphore sem)
{
    USemaphore *tempsem = (USemaphore*)sem;
    if(tempsem ->val > 0)
    {
        tempsem->val--;
    }
    else{
        pushToSemQueue(tempsem, currThreadNode);
        UThread* tempthread = currThreadNode;
        popQueue();
        if (currThreadNode != NULL)
	    {
            swapcontext(tempthread->ctx, currThreadNode->ctx);
	    }         
	}
}

void MySemaphoreSignal(MySemaphore sem) 
{
    USemaphore* tempsem1 = (USemaphore*)sem;
    tempsem1->val++;
    
    if(tempsem1->val > 0)
    {
        if(tempsem1->semhead != NULL)
        {
          popSemQueue(tempsem1);
          tempsem1->val--;
        }
    }
      
}

int MySemaphoreDestroy(MySemaphore sem)
{
    USemaphore* tempsem2 = (USemaphore*)sem;
    
    if(tempsem2 == NULL)
        return -1;
    if(tempsem2->semhead != NULL) // sem queue not empty
        return -1;
    else 
    {       
        free(tempsem2);
        return 0;
    }
    
}