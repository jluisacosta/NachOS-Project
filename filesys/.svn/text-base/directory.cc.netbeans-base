// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"
#include "system.h"

//----------------------------------------------------------------------
// ListElement::ListElement
// 	Initialize a list element, so it can be added somewhere on a list.
//
//	"itemPtr" is the item to be put on the list.  It can be a pointer
//		to anything.
//	"sortKey" is the priority of the item, if any.
//----------------------------------------------------------------------

ListElemento::ListElemento(void *itemPtr, int sortKey)
{
     item = itemPtr;
     key = sortKey;
     next = NULL;	// assume we'll put it at the end of the list 
}

//----------------------------------------------------------------------
// List::List
//	Initialize a list, empty to start with.
//	Elements can now be added to the list.
//----------------------------------------------------------------------

Lista::Lista()
{ 
    first = last = NULL; 
}

//----------------------------------------------------------------------
// List::~List
//	Prepare a list for deallocation.  If the list still contains any 
//	ListElements, de-allocate them.  However, note that we do *not*
//	de-allocate the "items" on the list -- this module allocates
//	and de-allocates the ListElements to keep track of each item,
//	but a given item may be on multiple lists, so we can't
//	de-allocate them here.
//----------------------------------------------------------------------

Lista::~Lista()
{ 
    while (Remove() != NULL)
	;	 // delete all the list elements
}

//----------------------------------------------------------------------
// List::Append
//      Append an "item" to the end of the list.
//      
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, put it at the end.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//----------------------------------------------------------------------

void
Lista::Append(void *item)
{
    ListElemento *element = new ListElemento(item, 0);

    if (IsEmpty()) {		// list is empty
	first = element;
	last = element;
    } else {			// else put it after last
	last->next = element;
	last = element;
    }
}

//----------------------------------------------------------------------
// List::Prepend
//      Put an "item" on the front of the list.
//      
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, put it at the beginning.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//----------------------------------------------------------------------

void
Lista::Prepend(void *item)
{
    ListElemento *element = new ListElemento(item, 0);

    if (IsEmpty()) {		// list is empty
	first = element;
	last = element;
    } else {			// else put it before first
	element->next = first;
	first = element;
    }
}

//----------------------------------------------------------------------
// List::Remove
//      Remove the first "item" from the front of the list.
// 
// Returns:
//	Pointer to removed item, NULL if nothing on the list.
//----------------------------------------------------------------------

void *
Lista::Remove()
{
    return SortedRemove(NULL);  // Same as SortedRemove, but ignore the key
}

//----------------------------------------------------------------------
// List::Mapcar
//	Apply a function to each item on the list, by walking through  
//	the list, one element at a time.
//
//	Unlike LISP, this mapcar does not return anything!
//
//	"func" is the procedure to apply to each element of the list.
//----------------------------------------------------------------------

void
Lista::Mapcar(VoidFunctionPtr func)
{
    for (ListElemento *ptr = first; ptr != NULL; ptr = ptr->next) {
       DEBUG('l', "In mapcar, about to invoke %x(%x)\n", func, ptr->item);
       (*func)((int)ptr->item);
    }
}

//----------------------------------------------------------------------
// List::IsEmpty
//      Returns TRUE if the list is empty (has no items).
//----------------------------------------------------------------------

bool
Lista::IsEmpty() 
{ 
    if (first == NULL)
        return TRUE;
    else
	return FALSE; 
}

//----------------------------------------------------------------------
// List::SortedInsert
//      Insert an "item" into a list, so that the list elements are
//	sorted in increasing order by "sortKey".
//      
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, walk through the list, one element at a time,
//	to find where the new item should be placed.
//
//	"item" is the thing to put on the list, it can be a pointer to 
//		anything.
//	"sortKey" is the priority of the item.
//----------------------------------------------------------------------

void
Lista::SortedInsert(void *item, int sortKey)
{
    ListElemento *element = new ListElemento(item, sortKey);
    ListElemento *ptr;		// keep track

    if (IsEmpty()) {	// if list is empty, put
        first = element;
        last = element;
    } else if (sortKey < first->key) {	
		// item goes on front of list
	element->next = first;
	first = element;
    } else {		// look for first elt in list bigger than item
        for (ptr = first; ptr->next != NULL; ptr = ptr->next) {
            if (sortKey < ptr->next->key) {
		element->next = ptr->next;
	        ptr->next = element;
		return;
	    }
	}
	last->next = element;		// item goes at end of list
	last = element;
    }
}

//----------------------------------------------------------------------
// List::SortedRemove
//      Remove the first "item" from the front of a sorted list.
// 
// Returns:
//	Pointer to removed item, NULL if nothing on the list.
//	Sets *keyPtr to the priority value of the removed item
//	(this is needed by interrupt.cc, for instance).
//
//	"keyPtr" is a pointer to the location in which to store the 
//		priority of the removed item.
//----------------------------------------------------------------------

void *
Lista::SortedRemove(int *keyPtr)
{
    ListElemento *element = first;
    void *thing;

    if (IsEmpty()) 
	return NULL;

    thing = first->item;
    if (first == last) {	// list had one item, now has none 
        first = NULL;
	last = NULL;
    } else {
        first = element->next;
    }
    if (keyPtr != NULL)
        *keyPtr = element->key;
    delete element;
    return thing;
}


//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory()
{
        table = new Lista();
        tableSize = 0;
        hijo = -1;
        padre = -1;
}
//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
        DirectoryEntry *aux;
        int i;
        
        file->ReadAt((char*)(&tableSize), sizeof(int),0);                 
        file->ReadAt((char*)(&hijo), sizeof(int),sizeof(int));                 
        file->ReadAt((char*)(&padre), sizeof(int),sizeof(int)*2);                 
        file->ReadAt((char*)(&sector), sizeof(int),sizeof(int)*3);   
        //printf("tableSize FetchFrom . %d\n",tableSize);
	for ( i = 0; i < tableSize; i++)
        {
            aux = new DirectoryEntry();
            (void) file->ReadAt((char*)aux, sizeof(DirectoryEntry), sizeof(DirectoryEntry)*i+sizeof(int)*4);        
            table->Append((void*)aux);
        }    
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
        DirectoryEntry *aux;
        int i=0;

        file->WriteAt((char*)(&tableSize), sizeof(int),0);                
        file->WriteAt((char*)(&hijo), sizeof(int),sizeof(int));                
        file->WriteAt((char*)(&padre), sizeof(int),sizeof(int)*2);                
        file->WriteAt((char*)(&sector), sizeof(int),sizeof(int)*3);                
        for (ListElemento *ptr = table->first; ptr != NULL; ptr = ptr->next)
        {
            aux = (DirectoryEntry*)(ptr->item);
            (void) file->WriteAt((char*)(ptr->item), sizeof(DirectoryEntry), sizeof(DirectoryEntry)*i+sizeof(int)*4);        
            i++;
        }    
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

DirectoryEntry*
Directory::FindIndex(char *name)
{
    DirectoryEntry *aux;
	for (ListElemento *ptr = table->first; ptr != NULL; ptr = ptr->next)
        {
            aux = (DirectoryEntry*)(ptr->item);
                if (!strncmp(aux->name, name, FileNameMaxLen))
                    return (DirectoryEntry*)(ptr->item);
        }
    return NULL;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
bool 
Directory :: tipoArchivo(char*name)
{
    DirectoryEntry *i = FindIndex(name);

    if (i!=NULL)
	return i->archivo;
    return false;
}

int
Directory::Find(char *name)
{
    DirectoryEntry *i = FindIndex(name);

    if (i!=NULL)
	return i->sector;
    return -1;
}

int
Directory::FindDirectorio(char *name)
{
    DirectoryEntry *i = FindIndex(name);

    if (i!=NULL && !(i->archivo))
    {
	return i->sector;
    }
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector, bool archivo)
{ 
    DirectoryEntry *nva = new DirectoryEntry;
    if (FindIndex(name)!=NULL)
	return FALSE;

    //printf("se añadio : %s\n",name);
    strncpy(nva->name, name, FileNameMaxLen); 
    nva->sector = newSector;
    nva->archivo = archivo;
    table->Append((void*)nva);
    tableSize++;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
	ListElemento *ptr, *ant;
        DirectoryEntry *aux;
	
	for (ant = ptr  = table->first; ptr != NULL;ant = ptr, ptr = ptr->next) 
        {
            aux = (DirectoryEntry*)(ptr->item);
                if (!strncmp(aux->name, name, FileNameMaxLen))
                    break;
        }
    if(ant == ptr)
    	table->Remove();
    else if(ptr)
    	ant->next = ptr->next;
	else
		return FALSE;
    tableSize--;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
    //printf("listdir : s %d, h %d, p %d.TABLESIZE : %d\n",sector,hijo,padre,tableSize);
    DirectoryEntry *aux;
	for (ListElemento *ptr = table->first; ptr != NULL; ptr = ptr->next)
        {
            aux = (DirectoryEntry*)(ptr->item);
            if(aux->archivo)
                printf("*");
            else
                printf("->");
            printf("%s\n", aux->name);
        }
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;
    DirectoryEntry *aux;
    
    printf("Directory contents:\n");

    for (ListElemento *ptr = table->first; ptr != NULL; ptr = ptr->next)
        {
            aux = (DirectoryEntry*)(ptr->item);
    	    hdr->FetchFrom(aux->sector);
    	    hdr->Print();
    	}
    printf("\n");
    delete hdr;
}


int
Directory::dirAct()
{
    Directory *aux;
    OpenFile *of;
    
    if(hijo!=-1)
    {
        of = new OpenFile(hijo);
        aux = new Directory();
        aux->FetchFrom(of);
        return aux->dirAct();
    }
    return sector;
}

