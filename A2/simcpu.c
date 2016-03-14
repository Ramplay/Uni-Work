/*********
simcpu.c -- Program to simulate scheduling of multiple processes with multiple threads in FCFS or RR

Created: 1 Mar. 2016
Author: Sean Lunt #0885840
Contact: slunt@mail.uoguelph.ca

*********/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

//Type for status of threads
typedef enum { NEW=0,
	READY,
	RUN,
	IO,
	TERM,
} Stat;

//Type to know whether RR or FCFS
typedef enum { FCFS=0,
	RR,
} RunType;

typedef struct Thread Thread;
typedef struct Proc Proc;
//struct to store vital info about threads
typedef struct Thread{
	int thrdNum;
	int CPUBursts;
	int curBurst;
	int arrvlTime;
	int totalBurst;
	int totalIO;
	int burst[100];
	int IO[100];
	int endT;
	Stat status;
	Proc * parent;
	Thread * next;
	Thread * qNext;
} Thread;
//struct to store vital info about processes and store lists of threads for each
typedef struct Proc{
	int procNum;
	int thrdCnt;
	Thread * first;
	Proc * next;
} Proc;
//struct for having a ready queue in simulation
typedef struct Queue{
	Thread * head;
} Queue;

//Scheduler Declarations of functions
void FirstCome(Proc * pList, int pCntxt, int tCntxt, int procCnt, bool d, bool v);
void Round(Proc * pList, int pCntxt, int tCntxt, int procCnt, bool d, bool v, int quantum);
bool moreProc(Proc * pList);
void enq(Thread * thrd, Queue * que);
Thread* deq(Queue * que);

int main(int argc, char const *argv[])
{
	RunType run;
	//prcs/thrd variables
	Proc * pList = NULL, * tempP = NULL, * pCursor = NULL;
	Thread * tList = NULL, * tempT = NULL, * tCursor = NULL;
	//variables for file info
	int prcsCnt, thrdCntxt, prcsCntxt;
	int quan;
	int thrdCnt, prcsNum;
	int thrdNum, arvlT, numCPU;
	int burstNum, burst, IO; 
	
	//Error check args first
	bool d = false, v = false;
	if (argc == 1){
		run = FCFS;
	} else {
		for (int i = 1; i < argc; i++){
			if (!strcmp(argv[i], "-d")){
				d = true;
			} else if (!strcmp(argv[i], "-v")){
				v = true;
			} else if (!strcmp(argv[i], "-r")){
				run = RR;
				quan = atoi(argv[i+1]);
				break;
			}
			run = FCFS;
		}

	}

	//Get first arg for data input/sim information
	fscanf(stdin, "%d %d %d", &prcsCnt, &thrdCntxt, &prcsCntxt);
	for (int i = 0; i < prcsCnt; i++){
		//get process info
		fscanf(stdin, "%d %d", &prcsNum, &thrdCnt);
		if (i == 0){
			pList = malloc(sizeof(Proc));
			pList->procNum = prcsNum;
			pList->thrdCnt = thrdCnt;
			pList->first = NULL;
			pList->next = NULL;
			pCursor = pList;
		} else {
			tempP = malloc(sizeof(Proc));
			tempP->procNum = prcsNum;
			tempP->thrdCnt = thrdCnt;
			tempP->first = NULL;
			tempP->next = NULL;
			pCursor = pList;
			while (pCursor->next != NULL){
				pCursor = pCursor->next;
			}
			pCursor->next = tempP;
			pCursor = tempP;
			tempP = NULL;
		}
		for (int j = 0; j < thrdCnt; j++){
			//get prcs thread info
			fscanf(stdin, "%d %d %d", &thrdNum, &arvlT, &numCPU);
			if (j == 0){
				tList = malloc(sizeof(Thread));
				tList->thrdNum = thrdNum;
				tList->CPUBursts = numCPU;
				tList->totalBurst = 0;
				tList->totalIO = 0;
				tList->curBurst = 0;
				tList->arrvlTime = arvlT;
				tList->status = NEW;
				tList->endT = 0;
				tList->next = NULL;
				tList->qNext = NULL;
				tList->parent = pCursor;
				pCursor = pList;
				while (pCursor->next != NULL){
					pCursor = pCursor->next;
				}
				pCursor->first = tList;
				tempT = tList;
			} else {
				tempT = malloc(sizeof(Thread));
				tempT->thrdNum = thrdNum;
				tempT->CPUBursts = numCPU;
				tempT->totalBurst = 0;
				tempT->totalIO = 0;
				tempT->curBurst = 0;
				tempT->arrvlTime = arvlT;
				tempT->status = NEW;
				tempT->endT = 0;
				tempT->next = NULL;
				tempT->qNext = NULL;
				tempT->parent = pCursor;
				tCursor = tList;
				while (tCursor->next != NULL){
					tCursor = tCursor->next;
				}
				tCursor->next = tempT;
			}
			for (int k = 0; k < numCPU; k++){
				//gets cpu burst info
				if (k != (numCPU-1)){
					fscanf(stdin, "%d %d %d", &burstNum, &burst, &IO);
					tempT->burst[burstNum-1] = burst;
					tempT->totalBurst += burst;
					tempT->IO[burstNum-1] = IO;
					tempT->totalIO += IO;
				} else {
					fscanf(stdin, "%d %d", &burstNum, &burst);
					tempT->burst[burstNum-1] = burst;
					tempT->totalBurst += burst;
					tempT->IO[burstNum-1] = -1;
				}
			}
			tempT = NULL;
		}
		tList = NULL;
	}
	//choose proper sim to run
	if (run == FCFS){
		FirstCome(pList, prcsCntxt, thrdCntxt, prcsCnt, d, v);
	} else if (run == RR){
		Round(pList, prcsCntxt, thrdCntxt, prcsCnt, d, v, quan);
	}
	
	return 0;
}
//The FCFS scheduling function
void FirstCome(Proc * pList, int pCntxt, int tCntxt, int procCnt, bool d, bool v){
	int OverHead = 0;
	int T = 0;
	int IOcnt = 0;
	float totalThrd;
	float util;
	float turnaround;
	Proc * pCur;
	Thread * CPU = NULL;
	int CPUpid, CPUtid;
	Thread * tCur;
	Thread ** IOlist = malloc(sizeof(Thread*));
	Queue * ready = malloc(sizeof(Queue));

	ready->head = NULL;

	do {
		//Check for new -> ready threads
		pCur = pList;
		while(pCur != NULL){
			tCur = pCur->first;
			while(tCur != NULL){
				if (tCur->arrvlTime <= T && tCur->status == NEW){
					enq(tCur, ready);
					tCur->status = READY;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from NEW to READY\n", 
								T, tCur->thrdNum, pCur->procNum);
					}
				}
				tCur = tCur->next;
			}
			pCur = pCur->next;
		}

		//Deal with any threads currently in IO
		if (IOcnt > 0){
			for (int i = 0; i < IOcnt; i++){
				IOlist[i]->IO[IOlist[i]->curBurst] -= 1;
				//check if IO done on cur thread and add to ready queue if true
				if (IOlist[i]->IO[IOlist[i]->curBurst] == 0){
					enq(IOlist[i], ready);
					IOlist[i]->curBurst += 1;
					IOlist[i]->status = READY;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from IO to READY\n", 
								T, IOlist[i]->thrdNum, IOlist[i]->parent->procNum);
					}
					for (int j = i; j < IOcnt; j++){
						IOlist[j] = IOlist[j+1];
					}
					IOcnt -= 1;
				}
			}
		}

		//put first thread on cpu or progress time on cur thread and\or context switch
		if (CPU != NULL && moreProc(pList)){
			CPUpid = CPU->parent->procNum;
			CPUtid = CPU->thrdNum;
			CPU->burst[CPU->curBurst] -= 1;
			//code to deal with final cpu burst thread switching
			if (CPU->burst[CPU->curBurst] == 0 && CPU->IO[CPU->curBurst] == -1){
				CPU->status = TERM;
				CPU->endT = T;
				if (v){
					printf("At time %d: Thread %d of Process %d moves from RUNNING to TERMINATED\n", 
							T, CPUtid, CPUpid);
				}
				CPU = deq(ready);
				if (CPU == NULL){
					T++;
					OverHead++;
					continue;
				}
				CPU->status = RUN;
				if (CPUpid != CPU->parent->procNum){
					CPU->burst[CPU->curBurst] += pCntxt;
					OverHead += pCntxt;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
								(T+pCntxt), CPU->thrdNum, CPU->parent->procNum);
					}
				} else if (CPUtid != CPU->thrdNum){
					CPU->burst[CPU->curBurst] += tCntxt;
					OverHead += tCntxt;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
								(T+tCntxt), CPU->thrdNum, CPU->parent->procNum);
					}
				}
				CPUpid = CPU->parent->procNum;
				CPUtid = CPU->thrdNum;
			//code to deal with mid thread cpuburst finish -> IOtime
			} else if (CPU->burst[CPU->curBurst] == 0){
				IOcnt += 1;
				CPU->status = IO;
				IOlist = realloc(IOlist, (sizeof(Thread*)*(IOcnt+1)));
				IOlist[IOcnt-1] = CPU;
				if (v){
					printf("At time %d: Thread %d of Process %d moves from RUNNING to IO\n", 
							T, CPUtid, CPUpid);
				}
				CPU = deq(ready);
				if (CPU == NULL){
					T++;
					OverHead++;
					continue;
				}
				CPU->status = RUN;
				if (CPUpid != CPU->parent->procNum){
					CPU->burst[CPU->curBurst] += pCntxt;
					OverHead += pCntxt;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
								(T+pCntxt), CPU->thrdNum, CPU->parent->procNum);
					}
				} else if (CPUtid != CPU->thrdNum){
					CPU->burst[CPU->curBurst] += tCntxt;
					OverHead += tCntxt;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
								(T+tCntxt), CPU->thrdNum, CPU->parent->procNum);
					}
				}
				CPUpid = CPU->parent->procNum;
				CPUtid = CPU->thrdNum;
			}
		//code if nothing is currently on the CPU
		} else if (moreProc(pList)) {
			CPU = deq(ready);
			if (CPU == NULL){
				T++;
				OverHead++;
				continue;
			}
			CPUpid = CPU->parent->procNum;
			CPUtid = CPU->thrdNum;
			if (v){
				printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
						T, CPUtid, CPUpid);
			}
		}

		T++;
	} while(moreProc(pList));
	T--;
	OverHead--;
	//calculate turnaround and cpu usage%
	pCur = pList;
	while(pCur != NULL){
		totalThrd += (float)(pCur->thrdCnt);
		tCur = pCur->first;
		while(tCur != NULL){
			turnaround += (float)(tCur->endT - tCur->arrvlTime);
			tCur = tCur->next;
		}
		pCur = pCur->next;
	}
	turnaround = turnaround/totalThrd;
	util = (((float)(T-OverHead))/((float)T))*100.00;
	//Print out Metrics
	printf("FCFS Scheduling\n");
	printf("Total Time required is %d time units\n", T);
	printf("Average Turnaround Time is %.1f time units\n", turnaround);
	printf("CPU Utilization is %.1f%%\n", util);
	//prints extra info for -d flag
	if (d){
		pCur = pList;
		while(pCur != NULL){
			tCur = pCur->first;
			while(tCur != NULL){
				printf("Thread %d of Process %d:\n", tCur->thrdNum, pCur->procNum);
				printf("arrival time: %d\n", tCur->arrvlTime);
				printf("service time: %d units, I/O time: %d units, turnaround time: %d, finish time: %d\n",
						tCur->totalBurst, tCur->totalIO, (tCur->endT - tCur->arrvlTime), tCur->endT);
				tCur = tCur->next;
			}
			pCur = pCur->next;
		}
	}
}
//Round Robin simulator function
void Round(Proc * pList, int pCntxt, int tCntxt, int procCnt, bool d, bool v, int quantum){
	int OverHead = 0;
	int timer = 0;
	int T = 0;
	int IOcnt = 0;
	float totalThrd;
	float util;
	float turnaround;
	Proc * pCur;
	Thread * CPU = NULL;
	int CPUpid, CPUtid;
	Thread * tCur;
	Thread ** IOlist = malloc(sizeof(Thread*));
	Queue * ready = malloc(sizeof(Queue));

	ready->head = NULL;

	do {
		//Check for new -> ready threads
		pCur = pList;
		while(pCur != NULL){
			tCur = pCur->first;
			while(tCur != NULL){
				if (tCur->arrvlTime <= T && tCur->status == NEW){
					enq(tCur, ready);
					tCur->status = READY;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from NEW to READY\n", 
								T, tCur->thrdNum, pCur->procNum);
					}
				}
				tCur = tCur->next;
			}
			pCur = pCur->next;
		}

		//Deal with any threads currently in IO
		if (IOcnt > 0){
			for (int i = 0; i < IOcnt; i++){
				IOlist[i]->IO[IOlist[i]->curBurst] -= 1;
				//check if IO done on cur thread and add to ready queue if true
				if (IOlist[i]->IO[IOlist[i]->curBurst] == 0){
					enq(IOlist[i], ready);
					IOlist[i]->curBurst += 1;
					IOlist[i]->status = READY;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from IO to READY\n", 
								T, IOlist[i]->thrdNum, IOlist[i]->parent->procNum);
					}
					for (int j = i; j < IOcnt; j++){
						IOlist[j] = IOlist[j+1];
					}
					IOcnt -= 1;
				}
			}
		}

		//put first thread on cpu or progress time on cur thread and\or context switch
		if (CPU != NULL && moreProc(pList) && timer != quantum){
			CPUpid = CPU->parent->procNum;
			CPUtid = CPU->thrdNum;
			CPU->burst[CPU->curBurst] -= 1;
			//code to deal with final cpu burst thread switching
			if (CPU->burst[CPU->curBurst] == 0 && CPU->IO[CPU->curBurst] == -1){
				CPU->status = TERM;
				CPU->endT = T;
				if (v){
					printf("At time %d: Thread %d of Process %d moves from RUNNING to TERMINATED\n", 
							T, CPUtid, CPUpid);
				}
				CPU = deq(ready);
				timer = 0;
				if (CPU == NULL){
					T++;
					OverHead++;
					continue;
				}
				CPU->status = RUN;
				if (CPUpid != CPU->parent->procNum){
					CPU->burst[CPU->curBurst] += pCntxt;
					OverHead += pCntxt;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
								(T+pCntxt), CPU->thrdNum, CPU->parent->procNum);
					}
					timer -= pCntxt;
				} else if (CPUtid != CPU->thrdNum){
					CPU->burst[CPU->curBurst] += tCntxt;
					OverHead += tCntxt;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
								(T+tCntxt), CPU->thrdNum, CPU->parent->procNum);
					}
					timer -= tCntxt;
				}
				CPUpid = CPU->parent->procNum;
				CPUtid = CPU->thrdNum;
			//code to deal with mid thread cpuburst finish -> IOtime
			} else if (CPU->burst[CPU->curBurst] == 0){
				IOcnt += 1;
				CPU->status = IO;
				IOlist = realloc(IOlist, (sizeof(Thread*)*(IOcnt+1)));
				IOlist[IOcnt-1] = CPU;
				if (v){
					printf("At time %d: Thread %d of Process %d moves from RUNNING to IO\n", 
							T, CPUtid, CPUpid);
				}
				CPU = deq(ready);
				timer = 0;
				if (CPU == NULL){
					T++;
					OverHead++;
					continue;
				}
				CPU->status = RUN;
				if (CPUpid != CPU->parent->procNum){
					CPU->burst[CPU->curBurst] += pCntxt;
					OverHead += pCntxt;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
								(T+pCntxt), CPU->thrdNum, CPU->parent->procNum);
					}
					timer -= pCntxt;
				} else if (CPUtid != CPU->thrdNum){
					CPU->burst[CPU->curBurst] += tCntxt;
					OverHead += tCntxt;
					if (v){
						printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
								(T+tCntxt), CPU->thrdNum, CPU->parent->procNum);
					}
					timer -= tCntxt;
				}
				CPUpid = CPU->parent->procNum;
				CPUtid = CPU->thrdNum;
			}
		//code if nothing is currently on the CPU
		} else if (moreProc(pList) && timer != quantum) {
			CPU = deq(ready);
			if (CPU == NULL){
				T++;
				OverHead++;
				continue;
			}
			CPUpid = CPU->parent->procNum;
			CPUtid = CPU->thrdNum;
			timer = 0;
			if (v){
				printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
						T, CPUtid, CPUpid);
			}
		//code to deal with time quantum being maxed
		} else if (timer == quantum){
			if ((CPU->burst[CPU->curBurst] - 1) != 0){
				CPU->status = READY;
				CPU->burst[CPU->curBurst] -= 1;
				enq(CPU, ready);
				if (v){
					printf("At time %d: Thread %d of Process %d moves from RUNNING to READY (Preempted by time quantum)\n", 
							T, CPUtid, CPUpid);
				}
			} else if (CPU->IO[CPU->curBurst] == -1){
				CPU->status = TERM;
				if (v){
					printf("At time %d: Thread %d of Process %d moves from RUNNING to TERMINATED\n", 
							T, CPUtid, CPUpid);
				}
				CPU->endT = T;
			} else {
				CPU->status = IO;
				CPU->burst[CPU->curBurst] -= 1;
				if (v){
					printf("At time %d: Thread %d of Process %d moves from RUNNING to IO\n", 
							T, CPUtid, CPUpid);
				}
				IOcnt += 1;
				IOlist = realloc(IOlist, (sizeof(Thread*)*(IOcnt+1)));
				IOlist[IOcnt-1] = CPU;
			}
			CPU = deq(ready);
			timer = 0;
			if (CPU == NULL){
				T++;
				OverHead++;
				continue;
			}
			CPU->status = RUN;
			if (CPUpid != CPU->parent->procNum){
				CPU->burst[CPU->curBurst] += pCntxt;
				OverHead += pCntxt;
				if (v){
					printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
							(T+pCntxt), CPU->thrdNum, CPU->parent->procNum);
				}
				timer -= pCntxt;
			} else if (CPUtid != CPU->thrdNum){
				CPU->burst[CPU->curBurst] += tCntxt;
				OverHead += tCntxt;
				if (v){
					printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
							(T+tCntxt), CPU->thrdNum, CPU->parent->procNum);
				}
				timer -= tCntxt;
			} else {
				if (v){
					printf("At time %d: Thread %d of Process %d moves from READY to RUNNING\n", 
							T, CPU->thrdNum, CPU->parent->procNum);
				}
			}
			CPUpid = CPU->parent->procNum;
			CPUtid = CPU->thrdNum;
		}
		timer++;
		T++;
	} while(moreProc(pList));
	T--;
	OverHead--;
	//calculate turnaround and cpu usage%
	pCur = pList;
	while(pCur != NULL){
		totalThrd += (float)(pCur->thrdCnt);
		tCur = pCur->first;
		while(tCur != NULL){
			turnaround += (float)(tCur->endT - tCur->arrvlTime);
			tCur = tCur->next;
		}
		pCur = pCur->next;
	}
	turnaround = turnaround/totalThrd;
	util = (((float)(T-OverHead))/((float)T))*100.00;
	//Print out Metrics
	printf("RR Scheduling\n");
	printf("Total Time required is %d time units\n", T);
	printf("Average Turnaround Time is %.1f time units\n", turnaround);
	printf("CPU Utilization is %.1f%%\n", util);
	//prints extra info for -d flag
	if (d){
		pCur = pList;
		while(pCur != NULL){
			tCur = pCur->first;
			while(tCur != NULL){
				printf("Thread %d of Process %d:\n", tCur->thrdNum, pCur->procNum);
				printf("arrival time: %d\n", tCur->arrvlTime);
				printf("service time: %d units, I/O time: %d units, turnaround time: %d, finish time: %d\n",
						tCur->totalBurst, tCur->totalIO, (tCur->endT - tCur->arrvlTime), tCur->endT);
				tCur = tCur->next;
			}
			pCur = pCur->next;
		}
	}
}
//function to add items to the queue
void enq(Thread * thrd, Queue * que){
	Thread * cursor;
	if (que->head == NULL){
		que->head = thrd;
		return;
	}
	cursor = que->head;
	while (cursor->qNext != NULL){
		cursor = cursor->qNext;
	}
	cursor->qNext = thrd;
}
//function to retrieve next item form queue
Thread* deq(Queue * que){
	Thread * value;

	value = que->head;
	if (value == NULL){
		return value;
	}
	que->head = que->head->qNext;
	value->qNext = NULL;
	return value;
}
//function to check if all processes have terminated or more computation needed
bool moreProc(Proc * pList){
	Proc * pCur;
	Thread * tCur;

	pCur = pList;

	while(pCur != NULL){
		tCur = pCur->first;
		while(tCur != NULL){
			if (tCur->status != TERM){
				return true;
			}
			tCur = tCur->next;
		}
		pCur = pCur->next;
	}
	return false;
}