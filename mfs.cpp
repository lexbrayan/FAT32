// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include<iostream>
#include<vector>
#include <signal.h>
using namespace std;

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

 //

FILE *fp; // File Pointer
char BS_OEMName[8];
uint16_t BPB_BytesPerSec;
uint8_t BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t BPB_NumFATs;
int16_t BPB_RootEntCnt;
char BS_VolLab[11];
uint32_t BPB_FATSz32;
int32_t BPB_RootClus;

int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FirstSectorofCluster = 0;
int directoryAddress;
int clust_address=0; // Stores the the starting cluster of the root directory

char expanded_name[12]; //Would be used in the compare() Function 

struct __attribute__((__packed__)) DirectoryEntry
{
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};
// DIR_Name[11]='\0';// add a Null Terminator to the end of the string
struct DirectoryEntry dir[16];




int LBAToOffset(int32_t sector)
{
    return ((sector - 2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);
}

int16_t NextLB(uint32_t sector)
{
    uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector * 4);
    int16_t val;
    fseek(fp, FATAddress, SEEK_SET);
    fread(&val, 2, 1, fp);
    return val;
}
/*Compare functions takes in an input string and 
compares it to the name of the files in the FAT32 img*/
void compare(string letter)
{
  char input [letter.length()+1]; // create a char array with it size equal to the length of the parameter
  strcpy(input,letter.c_str()); // convert the string to a char array 
  char IMG_Name[]= "FOO     TXT";
  char IMG_Name2[]="BAR     TXT";
  char IMG_Name3[]="DEADBEEFTXT";
  char IMG_Name4[]="NUM     TXT";
  memset( expanded_name, ' ', 12 );

  char *token = strtok( input, "." );

  strncpy( expanded_name, token, strlen( token ) );

  token = strtok( NULL, "." );

  if( token )
  {
    strncpy( (char*)(expanded_name+8), token, strlen(token ) );
  }

  expanded_name[11] = '\0';

  int i;
  for( i = 0; i < 11; i++ )
  {
    expanded_name[i] = toupper( expanded_name[i] );
  }

  if( strncmp( expanded_name, IMG_Name, 11 ) == 0 || strncmp( expanded_name, IMG_Name2, 11 ) == 0|| strncmp( expanded_name, IMG_Name3, 11 ) == 0 ||strncmp( expanded_name, IMG_Name4, 11 ) == 0 )
  {
      
    printf("They matched\n");
  }

  //printf(" Test Case: %s ",expanded_name);

}
int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality

    // int token_index  = 0;
    // for( token_index = 0; token_index < token_count; token_index ++ ) 
    // {
    //   printf("token[%d] = %s\n", token_index, token[token_index] );  
    // }
    
                        /* Opening the File */

    if(strcmp(token[0],"open")==0 && token[1]!=NULL)
    {
        fp=fopen(token[1],"r");
        if(!fp)
        {
            cout<<"Error opening %s"<<token[1]<<endl;
        }
        else
        {
            printf("%s was successfully opened \n\n",token[1]);
            fseek(fp,11,SEEK_SET);
            fread(&BPB_BytesPerSec,2,1,fp);
            fseek(fp,13,SEEK_SET);
            fread(&BPB_SecPerClus,1,1,fp);
            fseek(fp,14,SEEK_SET);
            fread(&BPB_RsvdSecCnt,2,1,fp);
            fseek(fp,16,SEEK_SET);
            fread(&BPB_NumFATs,1,1,fp);
            fseek(fp,36,SEEK_SET);
            fread(&BPB_FATSz32,4,1,fp);
            fseek(fp,44,SEEK_SET);
            fread(&BPB_RootEntCnt,4,1,fp);

             /* Calculation of the Cluster start Address 
                this would point to the beginning of the Files & 
                Directories AKA Root Directory */
            clust_address = (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);



            /* storing the Directory attributes 
                NOTE: We are assuming that this FIle System supports 
                16 of fewer files per Directory
            */
             fseek(fp,clust_address,SEEK_SET);
             for(int i= 0; i<16;i++)
             {
                 //memset(&dir[i], '\0', 32);
                 fread(&dir[i],32,1,fp);
                //  dir[i].DIR_Name[11]='\0';// add a Null Terminator to the end of the string
                 
             }
        }
        // cout<<"%s opened successfully "<<token[1]<<endl;
        

    }
    if(strcmp(token[0],"open")==0 && token[1]==NULL)
    {
        cout<<"File name missing"<<endl;
    }
    if(strcmp(token[0],"info")==0)
    {
        printf("Infomation coming soon \n");
        printf("BPB_BytesPerSec is: %8d \n",BPB_BytesPerSec);
        printf("BPB_BytesPerSec is: %8x \n\n",BPB_BytesPerSec);

        printf("BPB_SecPerClus is: %8d \n",BPB_SecPerClus);
        printf("BPB_SecPerClus is: %8x \n\n",BPB_SecPerClus);

        printf("BPB_RsvdSecCnt is: %8d \n",BPB_RsvdSecCnt);
        printf("BPB_RsvdSecCnt is: %8x \n\n",BPB_RsvdSecCnt);

        printf("BPB_NumFATs is: %8d \n",BPB_NumFATs);
        printf("BPB_NumFATs is: %8x \n\n",BPB_NumFATs);

        printf("BPB_FATSz32 is: %8d \n",BPB_FATSz32);
        printf("BPB_FATSz32 is: %8x \n\n",BPB_FATSz32);
        
        // printf("This is the ROOT DIRECTORY INFO: %8d \n",clust_address);
    }
    if(strcmp(token[0],"close")==0)
    {
        fclose(fp); // close the file
        fp=NULL; //set fp to ensure it was deleted.
        if(fp==NULL)
        {
            cout<<"File was closed succesfully \n\n"<<endl; 
        }
    }

     
            /* If User just enters 'stat' it 
            will give the user the complete
            stat of all the files */
    if(strcmp(token[0],"stat")==0 && token[1]==NULL)
    {
        if(fp)
        {
             printf("Attributes \t Cluster Low \t Size\n");
            for(int i=0;i<16;i++)
             {
                 
                if(dir[i].DIR_Attr==16 || dir[i].DIR_Attr==32)
                {
                    printf("%d \t\t %d \t\t %d \n", dir[i].DIR_Attr, dir[i].DIR_FirstClusterLow , dir[i].DIR_FileSize);
                }

            }

        }
        else
        {
            printf("File Does not exist\n");
        }
        

    }
     if(strcmp(token[0],"stat")==0 && token[1]!=NULL)
    {
        compare(token[1]);
        for(int i=0;i<16;i++)
        {
            char name[12];
            memcpy(name,dir[i].DIR_Name,11);
            if(dir[i].DIR_Name==expanded_name)
            {
                printf("BINGO \n");
            }
        }
        printf(" Expanded Name: %s \n",expanded_name);
       
    }
    if(strcmp(token[0],"ls")==0)
    {
        if(fp)
        {
            
            for(int i=0;i<16;i++)
             {
                 char name[12];
                 memcpy(name,dir[i].DIR_Name,11);
                 name[11]='\0';
                if(dir[i].DIR_Attr==32)
                {   
                    printf("%s \n", name);
                }

            }

        }
    }
    fclose(fp);

    free( working_root );

  }
  return 0;
}
