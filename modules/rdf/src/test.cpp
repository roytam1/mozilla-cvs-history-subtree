/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/* 
 * test.cpp
 *
 * This file is provided to ensure that the RDF engine will
 * compile and run standalone (outside of mozilla). It may also
 * be useful as a demonstration of how to initialize and
 * use the engine, and possibly for performance testing.
 * Finally, it ensures that the header files are probably written
 * for C++.
 *
 * Currently this program simply reads in an rdf site-map file
 * and spits it back out to the display.  Feel free to
 * modify/enhance as desired.
 *
 * See Dan Libby (danda@netscape.com) for more info.
 * 
 */

/* $Id$ 
 */

#include "stdlib.h"
#include "rdf-int.h"
#include "rdf.h"
#include "rdfparse.h"
#include "mcf.h"

const char    *dataSources[] = { 
   "rdf:remoteStore", NULL 
}; 

/* prototypes */
void printDB(RDF rdf);
void printRDFFile(RDFFile file);
void printRDFNotif(RDF_Notification notif);

/* fns */

void printDB(RDF rdf)
{
    int i;
    assert(rdf);

    assert(rdf->translators);
    for (i = 0; i < rdf->numTranslators; i++) {
        RDFT trans = rdf->translators[i];
        assert(trans);        
        RDFL list = trans->rdf;
        assert(list);
    
        printf("Translator #%d: %s\n", i, trans->url);

        if (list->rdf != rdf) { /* the list DB should be the same as the current DB */
            printf("Error: list->rdf = %p, rdf = %p, should be equal!\n", list->rdf, rdf);
        }

    }    

    RDFFile file = rdf->files;    
    if (file == NULL) {
        printf("RDFDB->files is NULL\n");
    }
    else {
        printRDFFile(file);
    }

    RDF_Notification notif = rdf->notifs;

    if (notif == NULL) {
        printf("RDFDB->notif is NULL\n");
    }
    else {
        printRDFNotif(notif);
    }
}

void printRDFFile(RDFFile file) 
{
    int i;

    assert(file);

    printf("1. List of All Assertions.\n");

    assert(file->assertionList);
    for (i = 0; i < file->assertionCount; i++) {
        Assertion a = file->assertionList[i];
        printf("Assertion Chain #%d:\n", i);
        assert(a);

        while (a != null) {
            printf("Assertion: [u: %s, s: %s, ", a->u->url, a->s->url);

            if (a->s == gCoreVocab->RDF_parent) {
                printf("parent name: %s]\n", ((RDF_Resource)(a->value))->url);
            }
            else {
                printf("value: %s]\n", a->value);
            }
            a = a->next;
        }
    }

    printf("\n2. List of All Resources.\n");

    printf("Resource Top: [u: %s]\n", file->top->url);
    printf("Resource RTop: [u: %s]\n", file->rtop->url);

    for (i = 0; i < file->resourceCount; i++) {
        RDF_Resource r = file->resourceList[i];
        printf("Resource #%d: [u: %s]\n", i, r->url);
    }
}

void printRDFNotif(RDF_Notification notif)
{
    while (notif != NULL) {
        printf("\tNotification: pdata = %s, event = ", notif->pdata);

        RDF_EventType eventType = notif->theEvent->eventType;
        switch(eventType) {
        case RDF_ASSERT_NOTIFY:
            printf("Assert\n"); break;
        case RDF_DELETE_NOTIFY:
            printf("Delete\n"); break;
        case RDF_KILL_NOTIFY:
            printf("Kill\n"); break;
        case RDF_CREATE_NOTIFY:
            printf("Create\n"); break;
        case RDF_RESOURCE_GC_NOTIFY:
            printf("ResourceGC\n"); break;
        case RDF_INSERT_NOTIFY:
            printf("Insert\n"); break;
        default:
            printf("Unknown\n"); break;
        }

        notif = notif->next;
    }
}

void printChildren(RDF rdf, RDF_Resource root, int depth)
{
      RDF_Cursor c;
      RDF_Resource u;
      int i;
      char spaces[40];
    
      for (i = 0; i < depth; i++) {
         spaces[i] = ' ';
      }
      spaces[i] = 0;

      i = 0;

      c = RDF_GetSources(rdf, root, gCoreVocab->RDF_parent, RDF_RESOURCE_TYPE, true);
      if (c)
      {
         printf("Listing children of resource %s:\n", root->url);

         u = (RDF_Resource)RDF_NextValue(c);
         while (u)
         {
            printf("%s%i: %s\n", spaces, ++i, u->url);
            printChildren(rdf, u, depth+1);
            u = (RDF_Resource)RDF_NextValue(c);
         }

         RDF_DisposeCursor(c);
      }
}

void 
main(int argc, char** argv) 
{ 
   RDF                     rdf; 
   RDF_Error               err; 
   RDF_InitParamsStruct    initParams = {0}; 
   RDF_Resource  u, s, root;
   void   *v; 
   RDFFile file;

   char *fileURL = argv[1];
   char *rootURL = (char*)getMem(200);

   /* If no URL specified, print out usage info. */

   if (argc < 2) {
     printf("Mozilla RDF Standalone Test Utility\n");
     printf("Usage: %s <URL>\n", argv[0]);
     exit(1);
   }

   /* first init the RDF engine */

   err = RDF_Init(&initParams); 
   if (err)
   {
      perror("RDF_Init: "); 
      exit(1); 
   }
   else
   {
      printf("RDF_Init: success\n"); 
   }

  /* 
   * Try and get a reference to the remote store DB 
   */ 
   rdf = RDF_GetDB(dataSources); 
   if (rdf == NULL)
   {
      perror("RDF_GetDB"); 
      exit(1); 
   }
   else
   {
      printf("RDF_GetDB: success\n"); 
   }

   sprintf(rootURL, "%s#root", fileURL);
   root = (RDF_Resource)RDF_GetResource(NULL, rootURL, PR_TRUE);
   setResourceType(root, RDF_RT);

   /* Create a test resource */
   u = RDF_GetResource(rdf, "http://people.netscape.com/danda/", TRUE); 
   s = gCoreVocab->RDF_name; 
   v = "Dan Libby";  

   printf("\nTEST: Adding assertion to RDF DB.\n");

   /* make an assertion into RDF's graph */
   RDF_Assert(rdf, u, s, v, RDF_STRING_TYPE);

   /* check to see if assertion exists */
   if (!RDF_HasAssertion(rdf, u, s, v, RDF_STRING_TYPE, true))
   {
      printf("Assertion does not exist!\n\n"); 
   }
   else
   {
      printf("Assertion [%s,%s,%s] exists.\n\n", u, s, v); 
   } 

   printDB(rdf);

   /* Import an RDF file */
   printf("Reading \"%s\"\n", fileURL);
   fflush(stdout);

   file = readRDFFile (fileURL, root, PR_TRUE, gRemoteStore);
   if (file && file->assertionCount > 0)
   {
      printf("\"%s\" read in successfully. (%i assertions)\n", fileURL, file->assertionCount);
      fflush(stdout);

      printf("\nTEST: Listing all Assertions and Resources in File Structure.\n");

      printRDFFile(file);

      PRFileDesc *fd = PR_GetSpecialFD(PR_StandardOutput);
 
      /* in order to get the top of the tree, I use this "0-th element" hack.  Isn't there
         a more elegant way to find the root node of an RDF file?  */

      printf("\nTEST: Print out file from DB.\n");

      outputRDFTree (rdf, fd, file->resourceList[0]); 

      printf("\nTEST: Print Tree (homemade version).\n");

      printChildren(rdf, file->resourceList[0], 0);
   }
   else
   {
      printf("error reading %s\n", fileURL);
   }

   RDF_Shutdown(); 
}


/* This function has to be here when building standalone RDF or you
 * will get a link error.
 */
extern "C"
void notifySlotValueAdded(RDF_Resource u, RDF_Resource s, void* v, RDF_ValueType type)
{
   if(type == RDF_STRING_TYPE)
   {
#if 0
      printf("String Value added: %s\n", (char*)v);
#endif
   }
   else if(type == RDF_RESOURCE_TYPE)
   {
      if(type == RDF_RESOURCE_TYPE)
      {
         /* Right here you can find out when 
          * resources are added, and what their
          * ids are, for querying later.  This is
          * useful when the ID of the resource is
          * not known at compile time.
          */
#if 0
          printf("Resource added, ID: %s\n", resourceID((RDF_Resource)v));
#endif
      }
   }
}


