// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"
#include "list.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define DirectoryFileSize       MaxFileSize

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        printf("Formateando el Disco...\n");
        BitMap *freeMap = new BitMap(NumSectors);
        Directory *directory = new Directory();
        FileHeader *mapHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

    // First, allocate space for FileHeaders for the directory and bitmap
    // (make sure no one else grabs these!)
	freeMap->Mark(FreeMapSector);	    
	freeMap->Mark(DirectorySector);

    // Second, allocate space for the data blocks containing the contents
    // of the directory and bitmap files.  There better be enough space!

	ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
	ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

    // Flush the bitmap and directory FileHeaders back to disk
    // We need to do this before we can "Open" the file, since open
    // reads the file header off of disk (and currently the disk has garbage
    // on it!).

        DEBUG('f', "Writing headers back to disk.\n");
	mapHdr->WriteBack(FreeMapSector);    
	dirHdr->WriteBack(DirectorySector);

    // OK to open the bitmap and directory files now
    // The file system operations assume these two files are left open
    // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
        directorioActual = new OpenFile(DirectorySector);
       
    // Once we have the files "open", we can write the initial version
    // of each file back to disk.  The directory at this point is completely
    // empty; but the bitmap has been changed to reflect the fact that
    // sectors on the disk have been allocated for the file headers and
    // to hold the file data for the directory and bitmap.

        directory->sector = 1;
        DEBUG('f', "Writing bitmap and directory back to disk.\n");
	freeMap->WriteBack(freeMapFile);	 // flush changes to disk
	directory->WriteBack(directoryFile);
        

	if (DebugIsEnabled('f')) {
	    freeMap->Print();
	    directory->Print();

        delete freeMap; 
	delete directory; 
	delete mapHdr; 
	delete dirHdr;
	}
    } else {
    // if we are not formatting the disk, just open the files representing
    // the bitmap and directory; these are left open while Nachos is running
        Directory *directory = new Directory();
        Directory *d = new Directory();
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
        directory->FetchFrom(directoryFile);
        directorioActual = new OpenFile(directory->dirAct());
        d->FetchFrom(directorioActual);
        delete directory;
        delete d;
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(char *name, int initialSize, bool archivo)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;
    OpenFile *of;
    int padre;

    DEBUG('f', "Creating file %s, size %d\n", name, initialSize);

    directory = new Directory();
    directory->FetchFrom(directorioActual);
    padre = directory->sector;

    if (directory->Find(name) != -1){
      success = FALSE;			// file is already in directory
      if(archivo){
                printf("El nombre del archivo ya existe.\n");
      }else{ printf("El nombre del directorio ya existe.\n"); }
          
    }
    else {	
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();	// find a sector to hold the file header
    	if (sector == -1) 		
            success = FALSE;		// no free block for file header 
        else if (!directory->Add(name, sector, archivo))
            success = FALSE;	// no space in directory
	else {
    	    hdr = new FileHeader;
	    if (!hdr->Allocate(freeMap, initialSize))
            	success = FALSE;	// no space on disk for data
	    else {	
	    	success = TRUE;
		// everthing worked, flush all changes back to disk
    	    	hdr->WriteBack(sector); 		
    	    	directory->WriteBack(directorioActual);
    	    	freeMap->WriteBack(freeMapFile);
                if(!archivo)
                {
                    of= new OpenFile(sector);
                    directory->FetchFrom(of);
                    directory->sector = sector;
                    directory->padre = padre;
                    directory->tableSize = 0;
                    directory->WriteBack(of);
                    delete of;
                }
	    }
            delete hdr;
	}
        delete freeMap;
    }
    delete directory;
    return success;
}

bool 
FileSystem::cambiaDirectorioActual(char *name)
{
    Directory *directory,*daux;
    OpenFile *of;
    int sector;
    if(!strncmp("..", name, FileNameMaxLen))
    {
        return cambiaDirectorioPadre();
    }
    else
    {
        directory = new Directory();
        daux = new Directory();
        directory->FetchFrom(directorioActual);
        sector = directory->FindDirectorio(name);
        if (sector == -1) {
           printf("No se ha encontrado el directorio especificado.\n");
           return FALSE;
        }
        if(directory->sector == 1)//si es el directorio RAIZ
        {
            directory->padre = -1;
        }
        directory->hijo = sector;
        directory->WriteBack(directorioActual);
        of = new OpenFile(sector);
        daux->FetchFrom(of);
        daux->padre = directory->sector;
        daux->hijo = -1;
        daux->WriteBack(of);
        printf("Directorio actual : %s.\n",name);
        delete directory;
        delete daux;
        delete of;
        return TRUE;
    }
}

bool 
FileSystem::cambiaDirectorioPadre()
{
    Directory *directory;
    OpenFile *of;
    
    directory = new Directory();
    directory->FetchFrom(directorioActual);
    if(directory->padre != -1)
    {
        Directory *padre;
        of = new OpenFile(directory->padre);
        padre = new Directory();
        padre->FetchFrom(of);
        padre->hijo = -1;
        if(padre->padre != -1)
        {
            Directory *abuelo;
            OpenFile *ofa;
            ofa = new OpenFile(padre->padre);
            abuelo = new Directory();
            abuelo->FetchFrom(ofa);
            padre->padre = abuelo->sector;
            delete abuelo;
            delete ofa;
        }
        directory->hijo = -1;
        directory->padre = -1;
        directory->WriteBack(directorioActual);
        padre->WriteBack(of);
        delete padre;
        delete of;
        delete directory;
        return TRUE;
    }
    else
    {
        printf("Se encuentra en el directorio RAIZ no se puede retroceder mas.\n");
        delete directory;
        return FALSE;
    }
}

bool
FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG('f', "Creating file %s, size %d\n", name, initialSize);

    directory = new Directory();
    directory->FetchFrom(directorioActual);

    if (directory->Find(name) != -1){
        success = FALSE;			// file is already in directory
        printf("El nombre del archivo ya existe.\n");
    }else {	
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();	// find a sector to hold the file header
    	if (sector == -1) 		
            success = FALSE;		// no free block for file header 
        else if (!directory->Add(name, sector, true))
            success = FALSE;	// no space in directory
	else {
    	    hdr = new FileHeader;
	    if (!hdr->Allocate(freeMap, initialSize))
	    {
            	success = FALSE;	// no space on disk for data
    	    	printf("false hdr->sector %d\n", hdr->dataSectors[29]);
		}
	    else {	
	    		success = TRUE;
	   			// everthing worked, flush all changes back to disk
	   			hdr->sector = sector;
    	    	hdr->WriteBack(sector); 		
    	    	printf("true hdr->sector %d\n", hdr->dataSectors[29]);
				directory->WriteBack(directorioActual);
    	    	freeMap->WriteBack(freeMapFile);
	    }
            delete hdr;
	}
        delete freeMap;
    }
    delete directory;
    return success;
}
//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{ 
    Directory *directory = new Directory();
    OpenFile *openFile = NULL;
    int sector;
    DEBUG('f', "Opening file %s\n", name);
    directory->FetchFrom(directorioActual);
    sector = directory->Find(name); 
    if (sector >= 0) 
    {
	openFile = new OpenFile(sector);	// name was found in directory 
    }
    delete directory;
    return openFile;				// return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------


bool
FileSystem::Remove(char *name)
{ 
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;
    
    directory = new Directory();
    directory->FetchFrom(directorioActual);
    sector = directory->Find(name);
    if (sector == -1) {
       delete directory;
       return FALSE;			 // file not found 
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap);  		// remove data blocks
    freeMap->Clear(sector);			// remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);		// flush to disk
    directory->WriteBack(directorioActual);        // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
} 

bool
FileSystem::Remove(char *name, bool archivo)
{ 
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;
    
    directory = new Directory();
    directory->FetchFrom(directorioActual);
    sector = directory->Find(name);
    if (sector == -1) {
        if(archivo){
            printf("No se ha encontrado el archivo especificado.\n");
        }        
       delete directory;
       return FALSE;
    }// file not found 
    
    if(directory->tipoArchivo(name)!=archivo) {
        printf("El nombre especificado no corresponde a un archivo.\n");
        delete directory;
        return FALSE;
    }
    fileHdr = new FileHeader();
    fileHdr->FetchFrom(sector);

    printf("SECTOR REMOVE: %d\n", fileHdr->dataSectors[29]);
    
    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap);  		// remove data blocks
    freeMap->Clear(sector);			// remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);		// flush to disk
    directory->WriteBack(directorioActual);        // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
} 

bool
FileSystem::RemoveRec(char *name, OpenFile *directorio)
{ 
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;
    
    directory = new Directory();
    directory->FetchFrom(directorio);
    sector = directory->Find(name);
    if (sector == -1) {       
       delete directory;
       return FALSE;
    }// file not found 
    
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap);  		// remove data blocks
    freeMap->Clear(sector);			// remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);		// flush to disk
    directory->WriteBack(directorio);        // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
}

bool
FileSystem::RemoveDirectory(char *name,OpenFile* directorio,int sec)
{ 
    Directory *directory,*daux;
    BitMap *freeMap;
    FileHeader *fileHdr;
    Lista *archs,*dirs;
    int sector_daux;
    ListElemento *ptr;
    DirectoryEntry *aux;
    char *saux;
    OpenFile *ofd;
    
    directory = new Directory();
    daux = new Directory();
    directory->FetchFrom(directorio);
    sector_daux = directory->Find(name);
    //printf("sector en sec_daux.. : %d\n",sector_daux);
    //printf("sector en removedirec.. : %d\n",sec);
    if(directory->tipoArchivo(name)==TRUE) {
        printf("El nombre especificado no corresponde a un directorio.\n");
        delete directory;
        return FALSE;
    }
    
    ofd = new OpenFile(sector_daux);
    daux->FetchFrom(ofd);
    archs = new Lista();
    dirs = new Lista();
    
    for (ptr  = daux->table->first; ptr != NULL;ptr = ptr->next) 
    {
        aux = (DirectoryEntry*)(ptr->item);
        if(aux->archivo == TRUE)
            archs->Append((void*)aux->name);
        else
            dirs->Append((void*)aux->name);     
    }
    
    for (ptr  = archs->first; ptr != NULL;ptr = ptr->next) 
    {
        saux = (char*)(ptr->item);
        //printf("sauxarchs : %s\n",saux);
        RemoveRec(saux,directorio);
    }
    
    for (ptr  = dirs->first; ptr != NULL;ptr = ptr->next) 
    {
        saux = (char*)(ptr->item);
        //printf("sauxdirs : %s\n",saux);
        RemoveDirectory(saux,ofd,daux->sector);
    }
    
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sec);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->DeallocateDirRecursivo(freeMap);  		// remove data blocks
    freeMap->Clear(sec);			// remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);		// flush to disk
    directory->WriteBack(directorio);        // flush to diskdelete archs;
    delete dirs;
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
}

void
FileSystem::muestraAyuda()
{
    printf("\n Ayuda:\n -cd nom_dir  Acessa a un directorio especificado.\n -cd ..  Acessa al directorio padre.\n");
    printf(" -cp nom_arch ruta_destino  Copia un archivo a un directorio especificado.\n -f  Formatea el disco.\n");
    printf(" -help  Muestra la ayuda.\n -ls  Muestra el contenido del directorio actual.\n");
    printf(" -mkdir nom_dir_nvo  Crea un directorio nuevo.\n -rd nom_dir  Borra un directorio recursivamente.\n");
    printf(" -rm nom_arch  Borra un archivo especificado.\n -rn nom_arch_actual nom_arch_nvo  Renombra un archivo especificado.\n");
    printf(" -touch nom_arch_nvo Crea un archivo nuevo.\n\n");
}

bool
FileSystem::renombrarArchivo(char* name,char* name_new)
{
    Directory *directory;
    DirectoryEntry *aux;
    int sector;
    
    directory = new Directory();
    directory->FetchFrom(directorioActual);
    sector = directory->Find(name);
    if (sector == -1) {
       printf("No se ha encontrado el archivo especificado.\n");   
       delete directory;
       return FALSE;
    }// file not found 
    
    if(directory->tipoArchivo(name)!=TRUE) {
        printf("El nombre especificado no corresponde a un archivo.\n");
        delete directory;
        return FALSE;
    }
    
    aux = directory->FindIndex(name);
    strncpy(aux->name, name_new, FileNameMaxLen);
    directory->WriteBack(directorioActual);
    printf("Se ha cambiado el nombre del archivo %s por %s.\n",name,aux->name);
    return TRUE;
}

bool 
FileSystem::eliminaDirectorio(char* name)
{
    Directory *directory;
    int sector;
    
    directory = new Directory();
    directory->FetchFrom(directorioActual);
    sector = directory->FindDirectorio(name);
    if (sector == -1) {
       printf("No se ha encontrado el directorio especificado.\n");
       return FALSE;
    }
    //printf("Llamada a RemoveDirectory.\n");
    this->RemoveDirectory(name,directorioActual,directory->sector);
    delete directory;
    return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List()
{
    Directory *directory = new Directory();

    directory->FetchFrom(directorioActual);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    BitMap *freeMap = new BitMap(NumSectors);
    Directory *directory = new Directory();

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    directory->FetchFrom(directorioActual);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}
