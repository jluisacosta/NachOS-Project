// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"


int fifo(TranslationEntry *pt, int vpn)
{
    int marcoVictima, aux;

    pt[vpn].physicalPage = stats->numPageFaults % NumPhysPages;//indicamos en numero de marco a la pagina
    aux = stats->numPageFaults % NumPhysPages;
    marcoVictima = marcosVictima[aux];//elegimos el marco victima        
    marcosVictima[aux] = vpn;

    return marcoVictima;
}

int reloj(TranslationEntry *pt, int vpn)
{
	int *marcoVictima = (int*)(lista->Remove());
	
	while(pt[*marcoVictima].use == TRUE)
	{
		pt[*marcoVictima].use = FALSE;
		lista->Append(marcoVictima);
		marcoVictima = (int*) (lista->Remove());
	}
	
	
	pt[vpn].physicalPage = pt[*marcoVictima].physicalPage;
	return *marcoVictima;
}

int LRU(TranslationEntry *pt, int vpn)
{
    int* victima, *Paux;
    *victima = 0;
    
    victima = (int *)(lista->Remove());	
	
    pt[*victima].valid = FALSE;
    pt[vpn].physicalPage = pt[*victima].physicalPage;
    return *victima;					
}



//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int marcoVictima,*vpn = new int;

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } 
    else if((which == PageFaultException)==TRUE)
    {
	if(stats->numPageFaults<NumPhysPages)//si existen marcos libres
        {
            //indica el numero de marco a la pagina
            machine->pageTable[machine->vpn].physicalPage = 	stats->
						numPageFaults;            
            marcoVictima = stats->numPageFaults;
            marcosVictima[stats->numPageFaults] = machine->vpn;
        }   
        else
        {
	    switch(numAlgoritmo)
            {                
                case 1://FIFO
                    marcoVictima = fifo(machine->pageTable,machine->vpn);
                    break;
                case 2:
                    marcoVictima = reloj(machine->pageTable,machine->vpn);
                break;
		case 3:
		    marcoVictima = LRU(machine->pageTable,machine->vpn);
		break;
            }
        }
	*vpn = machine->vpn;
        stats->numPageFaults++;//incrementamos el numero de fallos pagina
        machine->pageTable[marcoVictima].valid = false;//indicamos que no esta en memoria el marco victima
        if(machine->pageTable[marcoVictima].dirty)//si se modifico el marco guardar en el archivo swp
        {
	    stats->numDiskWrites++;
            archivo->WriteAt(&(machine->mainMemory[machine->pageTable[marcoVictima].physicalPage*PageSize]),PageSize,
                        machine->pageTable[marcoVictima].virtualPage*PageSize);
            machine->pageTable[marcoVictima].dirty = false;//indicar que ya se guardo y no se ha modificado
        }
        lista->Append(vpn);
        bzero(&(machine->mainMemory[machine->pageTable[machine->vpn].physicalPage*PageSize]),PageSize);
  
       archivo->ReadAt(&(machine->mainMemory[machine->pageTable[machine->vpn].physicalPage*PageSize]),PageSize,
               machine->pageTable[machine->vpn].virtualPage*PageSize);
        stats->numDiskReads++;
        machine->pageTable[machine->vpn].valid = true;
	//printf("vpn:%d reads:%d\n",*vpn,stats->numDiskReads++);
    }
    else
    {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
    //lista->imprimeLista();
}
