// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
FileHeader::FileHeader()
{
   numSectors=0;
   numBytes=0;
   sector=0;
   for(int i=0;i<NumDirect;i++)
      dataSectors[i]=-1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////FILEHDR tiene otra variable extra por ello el tamaño de numSectors = 29 apuntadores a sector////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    int limit,numSectorsAux,i,sectorHdr, sectorHdr1, totalSec = 0, limitDoble, sectorHdrAux, niv;
    bool band;
    limit=NumDirect;
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);   
	//verifica que hay espacio en disco
	if(numSectors<=29)
	{
		totalSec = numSectors+1;
	}
	else if(numSectors<=2+28+32)
	{
		totalSec = numSectors+2;
	}
	else
	{
		totalSec = divRoundUp((numSectors-28-NumDirect), NumDirect)+3+numSectors;
	}
	printf("NumClear %d < %d\n", freeMap->NumClear(), totalSec);
    if (freeMap->NumClear() < totalSec)
    {
		printf("Espacio insuficiente en disco!! tamaño libre: %d.\n",freeMap->NumClear());
		return FALSE;		
    }
    //verifica el limite de apuntadores en la cabecera 
	if(numSectors<=29)	
	{
	    limit = numSectors;
	    band = true;
	}
	else if(numSectors<=28+32)
	{
		limit = NumDirect-1;
	    band = false;		
	}
	else
	{
		limit = NumDirect -2; 	   
	    band = false;		
	}
    //obtiene y asigna sectores libres para apuntadores directos
	printf("Apuntadores Directos: %d\n",limit);
	for (i = 0; i < limit;i++)
	{
		dataSectors[i] = freeMap->Find();
		printf("Apuntador directo %d: Sector %d\n", i, dataSectors[i]);
    }
	if(!band)//si necesita apuntadores indirectos 
    {
    
		if(numSectors<=28+32)
		{
			limit = numSectors-limit;
			band = true;		
			niv = 1;
		}
		else
		{
			limit = 32; 	   
			band = false;		
			niv = 2;
		}
		printf("Apuntadores indirectos sencillos: %d\n",limit);
       FileHeader32 *hdrSencillo=new FileHeader32();//creamos una nueva cabecera logica
       sectorHdr=freeMap->Find();//reservamos un sector para la nueva cabecera
       dataSectors[NumDirect-niv]=sectorHdr;//asignamos el apuntador sencillo indirecto
		//llenamos el nuevo archivo con apuntadores indirectos sencillos
       for (i = 0; i < limit; i++)
       {
          hdrSencillo->dataSectors[i] = freeMap->Find();
          printf("Apuntador indirecto sencillo %d: Sector %d\n", i,hdrSencillo->dataSectors[i]);
	   }
	   //hacemos la asignacion fisica
       hdrSencillo->WriteBack(sectorHdr);
       delete hdrSencillo;
       if(!band)//si necesita apuntadores dobles
       {
          numSectorsAux=numSectors-(NumDirect-2)-32;
          limitDoble = divRoundUp(numSectorsAux,32);//cantidad de apuntadores dobles que necesita
          FileHeader32 *hdrDobles=new FileHeader32();//creamos una cabecera logica para los apuntadores dobles
          sectorHdr1=freeMap->Find();//se hace el mismo porcedimiento 
          this->dataSectors[NumDirect-1]=sectorHdr1;//asigna apuntador indirecto doble
		  printf("Apuntadores indirectos dobles: %d\n",limitDoble);
          for(i=0;i<limitDoble;i++)//por cada apuntador doble crea uno archivo de apuntadores indirectos sencillos
          {
             FileHeader32 *hdrAux=new FileHeader32();
             sectorHdrAux=freeMap->Find();
             hdrDobles->dataSectors[i]=sectorHdrAux;
             band = numSectorsAux <= 32;
             limit = (band? numSectorsAux:32);
               printf("Apuntador indirecto Doble-Cabecera %d: Sector %d\n", i, hdrDobles->dataSectors[i]);
               printf("Cantidad:  %d\n",limit);
             for(int j=0;j<limit;j++)
             {
                hdrAux->dataSectors[j]=freeMap->Find();
                printf("\tApuntador indirecto Doble[%d] %d: Sector %d\n", i, j, hdrAux->dataSectors[j]);
             }
             hdrAux->WriteBack(sectorHdrAux);
             delete hdrAux;
             numSectorsAux=numSectorsAux-limit;
          }
          hdrDobles->WriteBack(sectorHdr1);
          delete hdrDobles;
       }
    }
    return TRUE;
}

/*bool							//ALLOCATE ORIGINAL
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space

    for (int i = 0; i < numSectors; i++)
	dataSectors[i] = freeMap->Find();
    return TRUE;
}*/

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
   int i,sectoreseliminar, limit;
   sectoreseliminar=numSectors;
   printf("Sectores Libres %d < %d\n", freeMap->NumClear(), sectoreseliminar);   
   	if(numSectors<=29)
	{
		limit = numSectors;
	}
	else if(numSectors<=29+32)
	{
		limit = NumDirect-1;
	}
	else
	{
		limit = NumDirect-2;
	}
   //liberamos los sectores de los apuntadores directos
    for (i = 0; i < limit && sectoreseliminar>0; i++) 
    {
			freeMap->Clear((int) dataSectors[i]);
			sectoreseliminar--;
	}	
	//indirectos sencillos
    if(sectoreseliminar > 0)
    {
		if(numSectors<=28+29)
		{
			limit = numSectors-limit;
		}
		else
		{
			limit = NumDirect;
		}
       FileHeader *hdrSencillo=new FileHeader();
       hdrSencillo->FetchFrom(dataSectors[NumDirect-2]);
       for (i = 0; i < limit && sectoreseliminar; i++)
       {
			printf("rm sector %d: %d\n",i,hdrSencillo->dataSectors[i]);
       		if(hdrSencillo->dataSectors[i]!=-1)
       		{
				freeMap->Clear((int) (hdrSencillo->dataSectors[i]));
				sectoreseliminar--;
    	    }
       }
    }
    //indirectos dobles
    if(sectoreseliminar > 0)
    {
       FileHeader *hdrDobles=new FileHeader();
	   hdrDobles->FetchFrom(this->dataSectors[NumDirect-1]);
       for(i=0;i<NumDirect && sectoreseliminar;i++)
       {
		 if(hdrDobles->dataSectors[i]!=-1)
		 {
			 FileHeader *hdrAux=new FileHeader();
			 hdrAux->FetchFrom(hdrDobles->dataSectors[i]);
             printf("sector: %d\n", hdrAux->sector);
			 for(int j=0;j<NumDirect && sectoreseliminar;j++)
			 {
			 		 if(hdrAux->dataSectors[i]!=-1)
					 {
					 	printf("\tsector: %d\n", hdrAux->dataSectors[j]);
						freeMap->Clear((int) (hdrAux->dataSectors[j]));
						sectoreseliminar--;
					 }
			 }
			 freeMap->Clear((int) (hdrAux->sector));
			 delete hdrAux;
		 }
       }
       printf("NumClear %d\n", freeMap->NumClear());
       freeMap->Clear((int) (hdrDobles->sector));
       printf("NumClear %d\n", freeMap->NumClear());
       delete hdrDobles;
    }
	printf("NumClear %d\n", freeMap->NumClear());   
}
void 
FileHeader::DeallocateDirRecursivo(BitMap *freeMap)
{
    for (int i = 0; i < numSectors; i++) {
	freeMap->Clear((int) dataSectors[i]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

/*int
FileHeader::ByteToSector(int offset)
{
    return(dataSectors[offset / SectorSize]);
}*/

int
FileHeader::ByteToSector(int offset)
{
   int sector=offset / SectorSize,res;
   if(sector<NumDirect-3)
   {
      res=(dataSectors[sector]);
      return res;
   }
   if(sector<2*NumDirect-4)
   {
      FileHeader *hdrSencillo=new FileHeader();
      hdrSencillo->FetchFrom(dataSectors[NumDirect-3]);
      sector-=(NumDirect-3);
      res=hdrSencillo->dataSectors[sector];
      delete hdrSencillo;
      return res;
   }
   sector-=(2*NumDirect-4);
   sector=divRoundDown(sector,NumDirect-1);
   FileHeader *hdrDobles=new FileHeader();
   hdrDobles->FetchFrom(dataSectors[NumDirect-2]);
   FileHeader *hdrAux=new FileHeader();
   hdrAux->FetchFrom(hdrDobles->dataSectors[sector]);
   sector=offset / SectorSize-(2*NumDirect-4);
   sector=sector%(NumDirect-1);
   res=hdrAux->dataSectors[sector];
   delete hdrAux;
   delete hdrDobles;
   return res;
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}























FileHeader32::FileHeader32()
{
   for(int i=0;i<32;i++)
      dataSectors[i]=-1;
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader32::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader32::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}


