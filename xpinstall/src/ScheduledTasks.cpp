/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 *
 * Contributors:
 *     Daniel Veditz <dveditz@netscape.com>
 *     Douglas Turner <dougt@netscape.com>
 */

#include "nscore.h"
#include "NSReg.h"
#include "nsFileSpec.h"
#include "nsFileStream.h"

REGERR DeleteFileLater(nsFileSpec& filename)
{
    RKEY newkey;
    REGERR result = -1;
    HREG reg;
    if ( REGERR_OK == NR_RegOpen("", &reg) ) 
    {
        if (REGERR_OK == NR_RegAddKey( reg, ROOTKEY_PRIVATE, REG_DELETE_LIST_KEY, &newkey) )
        {
            nsPersistentFileDescriptor savethis(filename);
            char* buffer;
            nsOutputStringStream s(buffer);
            s << savethis;

            result = NR_RegSetEntry( reg, newkey, "", REGTYPE_ENTRY_BYTES, buffer, strlen(buffer));
        }

        NR_RegClose(reg);
    }

    return result;
}

REGERR ReplaceFileLater(nsFileSpec& tmpfile, nsFileSpec& target )
{
    RKEY newkey;
    REGERR result;
    HREG reg;

    if ( REGERR_OK == NR_RegOpen("", &reg) ) 
    {
        result = NR_RegAddKey( reg, ROOTKEY_PRIVATE, REG_REPLACE_LIST_KEY, &newkey);
        if ( result == REGERR_OK ) 
        {
            nsPersistentFileDescriptor tempDesc(tmpfile);
            nsPersistentFileDescriptor targDesc(target);
            
            char* tempBuffer;
            char* targBuffer;

            nsOutputStringStream tempStream(tempBuffer);
            nsOutputStringStream targStream(targBuffer);
            
            tempStream << tempDesc;
            targStream << targDesc;

            result = NR_RegSetEntry( reg, newkey, tempBuffer, REGTYPE_ENTRY_BYTES, targBuffer, strlen(targBuffer));
        }
        NR_RegClose(reg);
    }
    return result;
}

void DeleteScheduledFiles(void);
void ReplaceScheduledFiles(void);

extern "C" void PerformScheduledTasks(void *data)
{
    DeleteScheduledFiles();
    ReplaceScheduledFiles();
}


void DeleteScheduledFiles(void)
{
    HREG reg;

    if (REGERR_OK == NR_RegOpen("", &reg))
    {
        RKEY    key;
	    REGENUM state;

        /* perform scheduled file deletions and replacements (PC only) */
        if (REGERR_OK ==  NR_RegGetKey(reg, ROOTKEY_PRIVATE, REG_DELETE_LIST_KEY,&key))
        {
            char buf[MAXREGNAMELEN];  // what about the mac?  FIX

            while (REGERR_OK == NR_RegEnumEntries(reg, key, &state, buf, sizeof(buf), NULL ))
            {
                nsFileSpec doomedFile;
                nsPersistentFileDescriptor doomedDesc(doomedFile);
                nsInputStringStream tempStream(buf);
                tempStream >> doomedDesc;
                

                doomedFile.Delete(PR_FALSE);
                
                if (! doomedFile.Exists()) 
                {
                    NR_RegDeleteEntry( reg, key, buf );
                }
            }

            /* delete list node if empty */
			if (REGERR_NOMORE == NR_RegEnumEntries( reg, key, &state, buf, sizeof(buf), NULL ))
            {
                NR_RegDeleteKey(reg, ROOTKEY_PRIVATE, REG_DELETE_LIST_KEY);
            }
        }
        
        NR_RegClose(reg);
    }
}

void ReplaceScheduledFiles(void)
{
    HREG reg;
     
    if (REGERR_OK == NR_RegOpen("", &reg))
    {
        RKEY    key;
	    REGENUM state;

        /* replace files if any listed */
        if (REGERR_OK ==  NR_RegGetKey(reg, ROOTKEY_PRIVATE, REG_REPLACE_LIST_KEY, &key))
        {
            char tmpfile[MAXREGNAMELEN];
            char target[MAXREGNAMELEN];

            state = 0;
            while (REGERR_OK == NR_RegEnumEntries(reg, key, &state, tmpfile, sizeof(tmpfile), NULL ))
            {

                nsFileSpec replaceFile;
                nsPersistentFileDescriptor doomedDesc(replaceFile);
                nsInputStringStream tempStream(tmpfile);
                tempStream >> doomedDesc;

                if (! replaceFile.Exists() )
                {
                    NR_RegDeleteEntry( reg, key, tmpfile );
                }
                else if ( REGERR_OK != NR_RegGetEntryString( reg, key, tmpfile, target, sizeof(target) ) )
                {
                    /* can't read target filename, corruption? */
                    NR_RegDeleteEntry( reg, key, tmpfile );
                }
                else 
                {
                    nsFileSpec targetFile;
                    nsPersistentFileDescriptor targetDesc(targetFile);
                    nsInputStringStream anotherStream(target);
                    anotherStream >> targetDesc;

                    targetFile.Delete(PR_FALSE);
                
                    if (!targetFile.Exists())
                    {
                        nsFileSpec parentofTarget;
                        targetFile.GetParent(parentofTarget);                                           
                    
                        nsresult result = replaceFile.Move(parentofTarget);
                        if ( NS_SUCCEEDED(result) )
                        {
                            char* leafName = targetFile.GetLeafName();
                            replaceFile.Rename(leafName);
                            delete [] leafName;
                            
                            NR_RegDeleteEntry( reg, key, tmpfile );
                        }
                    }
                }
            }
            /* delete list node if empty */
            if (REGERR_NOMORE == NR_RegEnumEntries(reg, key, &state, tmpfile, sizeof(tmpfile), NULL )) 
            {
                NR_RegDeleteKey(reg, ROOTKEY_PRIVATE, REG_REPLACE_LIST_KEY);
            }
        }
        
        NR_RegClose(reg);
    }
}
