// directory.h 
//	Data structures to manage a UNIX-like directory of file names.
// 
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and 
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk.
//
//      We assume mutual exclusion is provided by the caller.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
//#include "list.h"
#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

#define FileNameMaxLen 		9	// for simplicity, we assume 
					// file names are <= 9 characters long


// The following class defines a "list element" -- which is
// used to keep track of one item on a list.  It is equivalent to a
// LISP cell, with a "car" ("next") pointing to the next element on the list,
// and a "cdr" ("item") pointing to the item on the list.
//
// Internal data structures kept public so that List operations can
// access them directly.

class ListElemento {
   public:
     ListElemento(void *itemPtr, int sortKey);	// initialize a list element

     ListElemento *next;		// next element on list, 
				// NULL if this is the last
     int key;		    	// priority, for a sorted list
     void *item; 	    	// pointer to item on the list
};

// The following class defines a "list" -- a singly linked list of
// list elements, each of which points to a single item on the list.
//
// By using the "Sorted" functions, the list can be kept in sorted
// in increasing order by "key" in ListElement.

class Lista {
  public:
    Lista();			// initialize the list
    ~Lista();			// de-allocate the list

    void Prepend(void *item); 	// Put item at the beginning of the list
    void Append(void *item); 	// Put item at the end of the list
    void *Remove(); 	 	// Take item off the front of the list

    void Mapcar(VoidFunctionPtr func);	// Apply "func" to every element 
					// on the list
    bool IsEmpty();		// is the list empty? 

    // Routines to put/get items on/off list in order (sorted by key)
    void SortedInsert(void *item, int sortKey);	// Put item into list
    void *SortedRemove(int *keyPtr); 	  	// Remove first item from list

    ListElemento *first;  	// Head of the list, NULL if list is empty
    ListElemento *last;		// Last element of list
};


// The following class defines a "directory entry", representing a file
// in the directory.  Each entry gives the name of the file, and where
// the file's header is to be found on disk.
//
// Internal data structures kept public so that Directory operations can
// access them directly.

class DirectoryEntry {
  public:
    bool inUse;				// Is this directory entry in use?
    int sector;				// Location on disk to find the 
					//   FileHeader for this file 
    char name[FileNameMaxLen + 1];	// Text name for file, with +1 for 
					// the trailing '\0'
    bool archivo;                       //true = archivo, false = directorio
};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk.
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file.
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk. 

class Directory {
  public:
    Directory(); 		// Initialize an empty directory
					// with space for "size" files
    ~Directory();			// De-allocate the directory

    void FetchFrom(OpenFile *file);  	// Init directory contents from disk
    void WriteBack(OpenFile *file);	// Write modifications to 
					// directory contents back to disk

    bool tipoArchivo(char *name);
    int Find(char *name);		// Find the sector number of the 
					// FileHeader for file: "name"

    bool Add(char *name, int newSector, bool archivo);  // Add a file name into the directory

    bool Remove(char *name);		// Remove a file from the directory

    void List();			// Print the names of all the files
					//  in the directory
    void Print();			// Verbose print of the contents
					//  of the directory -- all the file
					//  names and their contents.
    int dirAct();
  //private:
    Lista *table;		// Table of pairs: 
    int tableSize;			// Number of directory entries
					// <file name, file header location> 
    int hijo;
    int padre;
    int sector; 
    DirectoryEntry* FindIndex(char *name);		// Find the index into the directory 
    int FindDirectorio(char *name);
					//  table corresponding to "name"
};

#endif // DIRECTORY_H
