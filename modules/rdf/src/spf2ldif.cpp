/* 
 * spf2ldif.cpp
 *
 * This file contains the core functionality to convert an RDF-SP encoded file
 * into an LDIF file suitable for import into a Manhattan Project LDAP database.
 * This code relies on the mozilla (client) RDF engine and the Netscape
 * Portable Runtime (NSPR).  As such, it must be built in the client tree.
 *
 * See Dan Libby (danda@netscape.com) for more info.
 * 
 */

#include "stdlib.h"
#include "time.h"

#include "rdf-int.h"
#include "rdf.h"
#include "rdfparse.h"
#include "glue.h"
#include "hashtable.h"
#include "spf2ldif.h"

/* Function 'borrowed' from rdf engine to */
/* create a new ID based on time.         */
#define ID_BUF_SIZE 8
extern PLHashTable* resourceHash;
char *
makeNewID ()
{
   static HashTable hash;

	PRTime		tm = PR_Now();
	char		*id;

#ifndef HAVE_LONG_LONG
	double		doubletm;
#endif /* !HAVE_LONG_LONG */


	id = new char[ID_BUF_SIZE];

#ifdef HAVE_LONG_LONG
	PR_snprintf(id, ID_BUF_SIZE, "%1.0f", (double)tm);
#else
	LL_L2D(doubletm, tm);
	PR_snprintf(id, ID_BUF_SIZE, "%1.0f", doubletm);
#endif /* HAVE_LONG_LONG */

	/* prevent collisions by checking hash. */
	while (hash.get(id) != NULL)
	{
#ifdef HAVE_LONG_LONG
		tm = tm + (PR_MSEC_PER_SEC * 60); /* fix me - not sure what the increment should be */
		PR_snprintf(id, ID_BUF_SIZE, "%1.0f", (double)tm);
#else
		int64 incr;
		LL_I2L(incr, (PR_MSEC_PER_SEC * 60));
		LL_ADD(tm, tm, incr);
		LL_L2D(doubletm, tm);
		PR_snprintf(id, ID_BUF_SIZE, "%1.0f", doubletm);
#endif /* HAVE_LONG_LONG */
	}

   hash.put(id, (void*)1);
	return (id);
}

                                                          
/* Replace a string within a string.  Returns a new string. */
char* strreplacei(char* source, char* find, char* replace)
{
   int find_len = strlen(find);

   char* found = XP_STRCASESTR(source, find);
   if(!found) return found;

   int length = strlen(source) + (strlen(replace) - find_len);
   char* res = (char*)malloc(length + 1);

   length = found - source;
   strncpy(res, source, length);
   strcpy(res + length, replace);
   strcat(res + length, found + find_len);

   return res;
}


#define LDAP_DN_PROHIBITED_CHARS ",+\"<>;\\"

char *backslashEscape(char *sourceString, const char *specialChars, int destroy)
{
   if( sourceString == NULL ) return NULL;
   char *destString = new char[2 * strlen(sourceString) + 1];
   char *sourcePtr = sourceString;
   char *destPtr = destString;
   while( *sourcePtr )
   {
      for( const char *specialCharPtr = specialChars; 
         *specialCharPtr && (*sourcePtr != *specialCharPtr); specialCharPtr++ );
      if( *specialCharPtr )
      { // It's a special character, prepend a backslash
         *destPtr = '\\'; destPtr++;
      }
      *destPtr = *sourcePtr; destPtr++;
      sourcePtr++;
   }
   *destPtr = NULL;
   if( destroy ) delete[] sourceString;
   return destString;
}

char *ldapDnEscape(char *sourceString, int destroy=0); 
char *ldapDnEscape(char *sourceString, int destroy) 
{
   return(backslashEscape(sourceString, LDAP_DN_PROHIBITED_CHARS, destroy));
}



/********************\
* Vocabulary Classes *
\********************/

VocabElement::VocabElement(char* _xmlnamespace, char* _elementName, RDF_Wrapper& _rdf, RDF_Resource _res) :
rdf(_rdf)
{
   elementName = new char[strlen(_xmlnamespace) + strlen(_elementName) + 1];
   sprintf(elementName, "%s%s", _xmlnamespace, _elementName);

   res = (_res ? _res : rdf.getResource(elementName));
   id = (char*)(res ? res->url : NULL);
}

VocabElement::~VocabElement()
{
   delete[] elementName;
}

void VocabElement::RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{}

ArcToString::ArcToString(char* _xmlnamespace, char* _elementName, RDF_Wrapper& _rdf, RDF_Resource _res) :
VocabElement(_xmlnamespace, _elementName, _rdf, _res)
{}

ArcToString::~ArcToString()
{}

void ArcToString::RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{
   RDF_Wrapper rdf2(rdf);
   if(rdf2.getTargets(Parent, *this))
   {
      char* next = rdf2.getNext();
      while(next)
      {
         ldif.pushAttribute(getTagSuffix(), next);   
         next = rdf2.getNext();
      }
   }
/*
   char* next = getSlotValue(Parent);
   if(next)
      ldif.pushAttribute(getTagSuffix(), next);
 */      
}

ArcToResource::ArcToResource(char* _xmlnamespace, char* _elementName, RDF_Wrapper& _rdf, RDF_Resource _res) :
VocabElement(_xmlnamespace, _elementName, _rdf, _res)
{}

ArcToResource::~ArcToResource()
{}

void ArcToResource::RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{
}

Resource::Resource(char* _xmlnamespace, char* _elementName, RDF_Wrapper& _rdf, RDF_Resource _res) :
VocabElement(_xmlnamespace, _elementName, _rdf, _res),
ldiftitle(NULL)
{}

Resource::~Resource()
{
   if(ldiftitle)
      delete[] ldiftitle;
}

void Resource::RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{}

char* Resource::constructLDIFTitle(char* title)
{
   if(title) {
      if(ldiftitle)
         delete[] ldiftitle;

      ldiftitle = new char[strlen(title) + 1];
      char* t = ldiftitle, *s = title;

      /* Lower case the string.  Replace
       * ' ' with '-'.  Also, ignores commas.
       */
      while(*s != (char)NULL){
         *t = tolower(*s);
//         if(*t == ' ')
//            *t = '_';
         if(*t != ',' && *t != ' ')
            t++;
         s++;
      }
      *t = (char)NULL;
   }
   ldiftitle = ldapDnEscape(ldiftitle, TRUE);
   return ldiftitle;
}


DublinCore::DublinCore(char* xmlnamespace) :
_namespace(strdup(xmlnamespace))
{}

DublinCore::~DublinCore()
{
   free(_namespace);
}

SitePreview::SitePreview(char* xmlnamespace) :
_namespace(strdup(xmlnamespace))
{}

SitePreview::~SitePreview()
{
   free(_namespace);
}


DC_creator::DC_creator(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
DublinCore(),
ArcToString(_namespace, elementName, rdf, r)
{
}

DC_date::DC_date(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
DublinCore(),
ArcToString(_namespace, elementName, rdf, r)
{
}

void DC_date::RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{
   ArcToString::RDF_to_LDIF(Parent, vocab, ldif, fetch_fh);

   RDF_Wrapper rdf2(rdf);
   if(rdf2.getTargets(Parent, *this))
   {
      char* nextdate = rdf2.getNext();
      while(nextdate)
      {
         time_t time = makeTime(nextdate);
         char buf[20];
         sprintf(buf, "%i", time);
         ldif.pushAttribute("timestamp", buf);
         nextdate = rdf2.getNext();
      }
   }
}

time_t DC_date::makeTime(char* DateString)
{
   struct tm newtime = {0};
#ifndef XP_WIN
   extern time_t timezone; // difference, in seconds, between Coordinated Universal Time and the local standard time
   extern time_t daylight;
#endif
   // newtime.tm_sec   =  Seconds after minute (0 – 59)
   // newtime.tm_min   =  Minutes after hour (0 – 59)
   // newtime.tm_hour  =  Hours since midnight (0 – 23)
   // newtime.tm_mday  =  Day of month (1 – 31)
   // newtime.tm_mon   =  Month (0 – 11; January = 0)
   // newtime.tm_year  =  Year (current year minus 1900)
   // newtime.tm_wday  =  Day of week (0 – 6; Sunday = 0)
   // newtime.tm_yday  =  Day of year (0 – 365; January 1 = 0)
   // newtime.tm_isdst =  Always 0 for gmtime


   //YYYY-MM-DDTHH-MM-SSZ
   char* date = strdup(DateString);
   char* time = strchr(date, 'T');

   /* Parse the time */
   if(time){
     *time = 0;
     time++;

     char* temp = strchr(time, ':');
     if(temp){
        *temp = 0;

        char* minute = temp + 1;
        temp = strchr(time, ':');
        if(temp){
           *temp = 0;

           newtime.tm_sec = atoi(temp +1);
        }
        newtime.tm_min = atoi(minute);
     }
     newtime.tm_hour = atoi(time);
   }

   /* Parse the date */
   char* temp = strchr(date, '-');
   if(temp) {
      *temp = 0;

      char* month = temp + 1;
      temp = strchr(month, '-');
      if(temp){
         *temp = 0;

         newtime.tm_mday = atoi(temp + 1);
      }
      newtime.tm_mon = atoi(month) - 1;
   }
   newtime.tm_year = atoi(date) - 1900;

   free(date);
   return mktime(&newtime) - timezone;
}

DC_description::DC_description(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
DublinCore(),
ArcToString(_namespace, elementName, rdf, r)
{
}

DC_language::DC_language(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
DublinCore(),
ArcToString(_namespace, elementName, rdf, r)
{
}

DC_source::DC_source(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
DublinCore(),
ArcToString(_namespace, elementName, rdf, r)
{
}

DC_publisher::DC_publisher(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
DublinCore(),
ArcToResource(_namespace, elementName, rdf, r)
{
}

DC_subject::DC_subject(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
DublinCore(),
ArcToString(_namespace, elementName, rdf, r)
{
}

DC_title::DC_title(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
DublinCore(),
ArcToString(_namespace, elementName, rdf, r)
{
}

SP_aboutEvent::SP_aboutEvent(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
ArcToString(_namespace, elementName, rdf, r)
{
}

SP_aboutOrganization::SP_aboutOrganization(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
ArcToString(_namespace, elementName, rdf, r)
{
}

SP_aboutPlace::SP_aboutPlace(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
ArcToString(_namespace, elementName, rdf, r)
{
}

SP_aboutPerson::SP_aboutPerson(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
ArcToString(_namespace, elementName, rdf, r)
{
}

SP_Channel::SP_Channel(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
Resource(_namespace, elementName, rdf, r)
{
}

void SP_Channel::RDF_to_LDIF(Vocabulary& vocab, PRFileDesc* ldif_fh, PRFileDesc* fetch_fh)
{
   LDIF_Entry ldif(ldif_fh);

   if(ldif_header(vocab, ldif, fetch_fh))
     ldif_body(vocab, ldif, fetch_fh);
}

XP_Bool SP_Channel::ldif_header(Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{
   /* Get the Organization */
   char* org_title = NULL;
   RDF_Resource r_org = vocab.resource_arcs.publisher.getSlotValue(*this);
   if(!r_org)
      return FALSE;

   SP_Organization org(rdf, r_org);
   org_title = org.ldifTitle(org.getSlotValue(vocab.string_arcs.title));
   if(!org_title)
      return FALSE;

   char* l = ldifTitle(vocab.string_arcs.title.getSlotValue(*this));
   if(!l) return FALSE;

   ldif.pushDN("o", ROOT_ORGANIZATION, FALSE);
   ldif.pushDN("ou", l, FALSE);
   ldif.pushAttribute("changetype", "add");
   ldif.pushAttribute("objectClass", "top");
   ldif.pushAttribute("objectClass", "extensibleObject");
   ldif.pushAttribute("ou", l);
   ldif.pushAttribute("organization", org_title);
   ldif.write();

   return TRUE;
}

XP_Bool SP_Channel::ldif_body(Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{
   LDIF_Entry header(ldif);

   ldif.clearAttributes();
   ldif.pushAttribute("changetype", "modify");
   ldif.pushAttribute("replace", "*");

   /* Get Channel image */
   vocab.resource_arcs.image.RDF_to_LDIF(*this, vocab, ldif, fetch_fh);

   /* Get all other string values */
   VocabElement *tag = vocab.string_arcs.getHead();
   while(tag)
   {
      tag->RDF_to_LDIF(*this, vocab, ldif, fetch_fh);
      tag = vocab.string_arcs.getNext();
   }

   if(rdf.getResourceTargets(*this, vocab.resource_arcs.section))
   {
      /* Avoid duplicating items with same title.  
         This is a really kludgy way of doing it.
         However, I tried using a hashtable and ran
         into really odd problems.
       */
      List<SP_Item> items;

      RDF_Resource r = rdf.getNextResource();
      while(r)
      {
         SP_Item *Item = new SP_Item(rdf, r);
         char *item_title = Item->getSlotValue(vocab.string_arcs.title);
         item_title = item_title ? Item->ldifTitle(item_title) : *Item;

         SP_Item* next = items.getHead();
         XP_Bool found = FALSE;
         while(next){
            if(!strcmp(next->ldifTitle(), item_title))
               found = TRUE;
            next = items.getNext();
         }
         if(found == FALSE)
         {
            items.add(Item);
            ldif.pushAttribute(Item->getTagSuffix(), item_title, TRUE);
         }
         else
         {
            fprintf(stderr, "Ignoring duplicate item: %s\n", item_title);
         }

         r = rdf.getNextResource();
      }
      SP_Item* next = items.getTail();
      while(next){
         items.remove(TRUE);
         next = items.getPrev();
      }
   }
   ldif.write();

   if(rdf.getResourceTargets(*this, vocab.resource_arcs.section))
   {
      RDF_Resource r = rdf.getNextResource();
      while(r)
      {
         LDIF_Entry ldif_item(header);
         SP_Item Item(rdf, r);
         Item.RDF_to_LDIF(vocab, ldif_item, fetch_fh);
         ldif_item.write();
         r = rdf.getNextResource();
      }
   }

   return TRUE;
}



SP_Channel* SP_Channel::newChannel(Vocabulary& vocab, RDF_Wrapper& rdf, RDF_Resource r)
{
   /* Get the Organization */
   SP_Channel* chan = new SP_Channel(rdf, r);
   char* title = chan->ldifTitle(chan->getSlotValue(vocab.string_arcs.title));
   if(title)
   {
     char* style = (char*)(*SPF_APP::pApp).ChannelTable.get(title);
     if(style)
     {
        if(!strcmp(style, "news"))
        {
           delete chan;
           return new SP_NewsChannel(rdf, r);
        }
     }
   }

   /* default case */
   return chan;
}

SP_NewsChannel::SP_NewsChannel(RDF_Wrapper& rdf, RDF_Resource r) :
SP_Channel(rdf, r)
{
}

void SP_NewsChannel::RDF_to_LDIF(Vocabulary& vocab, PRFileDesc* ldif_fh, PRFileDesc* fetch_fh)
{
   LDIF_Entry ldif(ldif_fh);

   if(!ldif_header(vocab, ldif, fetch_fh))
      return;

   /* Get Channel image */
   vocab.resource_arcs.image.RDF_to_LDIF(*this, vocab, ldif, fetch_fh);

   /* Get all other string values */
   VocabElement *tag = vocab.string_arcs.getHead();
   while(tag)
   {
      tag->RDF_to_LDIF(*this, vocab, ldif, fetch_fh);
      tag = vocab.string_arcs.getNext();
   }

   if(rdf.getResourceTargets(*this, vocab.resource_arcs.section))
   {
      RDF_Resource r = rdf.getNextResource();
      while(r)
      {
         SP_NewsItem* Item = new SP_NewsItem(rdf, r);
         Item->RDF_to_LDIF(vocab, ldif, fetch_fh);
         delete Item;
         r = rdf.getNextResource();
      }
   }
   ldif.write();
}

SP_link::SP_link(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
ArcToString(_namespace, elementName, rdf, r)
{
}

SP_image::SP_image(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
ArcToResource(_namespace, elementName, rdf, r)
{
}

void SP_image::RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{
   /* Determine if an image Resource exists */
   RDF_Resource r_image = getSlotValue(Parent);
   if(r_image == NULL) return;
   char* id = resourceID(r_image);

   /* Determine new filename based on time and file extension */
   char *ext = strrchr(id, '.');
   if(!ext) return;
   char* unique_id = makeNewID();
   int filename_len = strlen(ext) + strlen(unique_id);
   char* filename = new char[filename_len + 1];
   sprintf(filename, "%s%s", unique_id, ext);

   /* Write out href and filename to fetcher file */
   int buf_len = strlen(id) + filename_len + 2;
   char* buf = new char[buf_len + 1];
   sprintf(buf, "%s %s\n", id, filename);
   PR_Write(fetch_fh, buf, buf_len);

   /* Write out LDIF attribute to LDIF file */
   char *attr_name = "image";
   ldif.pushAttribute(attr_name, filename);

   /* Get description (alt text) */
   char* description = vocab.string_arcs.description.getSlotValue(r_image);
   if(description)
   {
      char *suffix = "_alt";
      int key_len = strlen(attr_name) + strlen(suffix);
      char* key = new char[key_len + 1];
      sprintf(key, "%s%s", attr_name, suffix);
      ldif.pushAttribute(key, description);
      delete[] key;
   }

   /* Get link (to another URL) */
   char* link = vocab.string_arcs.link.getSlotValue(r_image);
   if(link)
   {
      char *suffix = "_link";
      int key_len = strlen(attr_name) + strlen(suffix);
      char* key = new char[key_len + 1];
      sprintf(key, "%s%s", attr_name, suffix);
      ldif.pushAttribute(key, link);
      delete[] key;
   }


   /* Free allocated strings */
   delete[] filename;
   delete[] buf;
}


SP_Item::SP_Item(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
Resource(_namespace, elementName, rdf, r)
{
}

void SP_Item::RDF_to_LDIF(Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{
   char *l = getSlotValue(vocab.string_arcs.title);
   l = l ? ldifTitle(l) : id;
   ldif.pushDN("l", l);
   ldif.write();

   ldif.clearAttributes();
   ldif.pushAttribute("changetype", "modify");
   ldif.pushAttribute("replace", "*");

   /* Get image */
   vocab.resource_arcs.image.RDF_to_LDIF(*this, vocab, ldif, fetch_fh);

  /* Get all string values */
   VocabElement *tag = vocab.string_arcs.getHead();
   while(tag)
   {
      tag->RDF_to_LDIF(*this, vocab, ldif, fetch_fh);
      tag = vocab.string_arcs.getNext();
   }
}

SP_NewsItem::SP_NewsItem(RDF_Wrapper& rdf, RDF_Resource r) :
SP_Item(rdf, r)
{
}

void SP_NewsItem::RDF_to_LDIF(Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh)
{
   char* title = vocab.string_arcs.title.getSlotValue(*this);
   char* description = vocab.string_arcs.description.getSlotValue(*this);

   /* Check for missing title or description */
   if(!title || !description)
      return;

   char buf[4096];
   sprintf(buf, "<A HREF=\"%s\">%s</A>%s", id, title, description);
   ldif.pushAttribute("headline", buf);
}



SP_Organization::SP_Organization(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
Resource(_namespace, elementName, rdf, r)
{
}

void SP_Organization::RDF_to_LDIF(Vocabulary& vocab, PRFileDesc* ldif_fh, PRFileDesc* fetch_fh)
{
   LDIF_Entry ldif(ldif_fh);

   /* Get the Organization */
   char* title = vocab.string_arcs.title.getSlotValue(*this);
   if(!title) {
      fprintf(stderr, "Syntax Error: Organization title not found.\n");
      return;
   }
   ldifTitle(title);

   ldif.pushAttribute("changetype", "add");
   ldif.pushAttribute("objectClass", "top");
   ldif.pushAttribute("objectClass", "extensibleObject");
   ldif.pushDN("o", ROOT_ORGANIZATION, FALSE);
   ldif.pushDN("ou", ldiftitle); /* Add ou field */
   ldif.write();

   ldif.clearAttributes();
   ldif.pushAttribute("changetype", "modify");
   ldif.pushAttribute("replace", "*");

   /* Get image */
   vocab.resource_arcs.image.RDF_to_LDIF(*this, vocab, ldif, fetch_fh);

  /* Get all string values */
   VocabElement *tag = vocab.string_arcs.getHead();
   while(tag)
   {
      tag->RDF_to_LDIF(*this, vocab, ldif, fetch_fh);
      tag = vocab.string_arcs.getNext();
   }

   ldif.write();
}

SP_section::SP_section(RDF_Wrapper& rdf, RDF_Resource r, char* elementName) :
SitePreview(),
ArcToResource(_namespace, elementName, rdf, r)
{
}


ResourceVocab::ResourceVocab(RDF_Wrapper& rdf) :
Channel(rdf, NULL),
Item(rdf, NULL),
Organization(rdf, NULL)
{
   add(&Channel);
   add(&Item);
   add(&Organization);
}


ResourceArcVocab::ResourceArcVocab(RDF_Wrapper& rdf) :
publisher(rdf, NULL),
image(rdf, NULL),
section(rdf, NULL)
{
   add(&publisher);
   add(&image);
   add(&section);
}

StringArcVocab::StringArcVocab(RDF_Wrapper& rdf) :
creator(rdf, NULL),
date(rdf, NULL),
description(rdf, NULL),
language(rdf, NULL),
source(rdf, NULL),
subject(rdf, NULL),
title(rdf, NULL),
aboutEvent(rdf, NULL),
aboutOrganization(rdf, NULL),
aboutPlace(rdf, NULL),
aboutPerson(rdf, NULL),
link(rdf, NULL)
{
   add(&creator);
   add(&date);
   add(&description);
   add(&language);
   add(&source);
   add(&subject);
   add(&title);
   add(&aboutEvent);
   add(&aboutOrganization);
   add(&aboutPlace);
   add(&aboutPerson);
   add(&link);
}

Vocabulary::Vocabulary(RDF_Wrapper& rdf) :
resources(rdf),
resource_arcs(rdf),
string_arcs(rdf)
{
}

/* URL constructor */
URL::URL() :
url(0),
len(0)
{
}

/* URL constructor */
URL::URL(char* source)
{
   set(source);
}

/* URL constructor */
URL::URL(URL& source)
{
   set(source);
}


/* Sets the URL, given a source string */
void URL::set(char* source)
{
   if(url) delete[] url;

   len = strlen(source);
   char* f = "";
   if(!strstr(source, "://"))
   {
#ifdef XP_WIN
      f = "file:///";
#else
      f = "file://";
#endif
      
      len += strlen(f);
   }

   url = new char[len + 1];
   sprintf(url, "%s%s", f, source);
}

/* Sets the URL, given a source URL */
void URL::set(URL& source)
{
   if(url) delete[] url;

   len = source.length();
   url = new char[len + 1];
   strcpy(url, source.string());
}

/* Appends a string to URL */
void URL::append(char* source)
{
   len += strlen(source);
   char* temp = new char[len];
   sprintf(temp, "%s%s", url, source);
   delete[] url;
   url = temp;
}

/* URL Destructor */
URL::~URL()
{
   if(url) delete[] url;
}


RDF_Wrapper::RDF_Wrapper() :
c(0),
isCopy(0)
{}

RDF_Wrapper::RDF_Wrapper(RDF_Wrapper& source) :
c(0),
isCopy(1)
{
   db = source;
}


int RDF_Wrapper::init()
{
   RDF_InitParamsStruct initParams = {0};
   RDF_Error err = RDF_Init(&initParams); 
   if (err)
   {
      perror("RDF_Init: "); 
      return 0; // bad, bad thing happened.
   }

   const char    *dataSources[] = { 
      "rdf:remoteStore", NULL 
   };
   db = RDF_GetDB(dataSources); 
   if (db == NULL)
   {
      perror("RDF_GetDB"); 
      return 0; // bad, bad thing happened.
   }
   return 1; // success.
}

RDF_Wrapper::~RDF_Wrapper()
{
   if(!isCopy)
     RDF_Shutdown();
}


/* Initialize static members */
SPF_APP* SPF_APP::pApp = NULL;

/* Application constructor */
SPF_APP::SPF_APP(int argc, char** argv) :
runnable(0),
fetch_fh(0),
ldif_fh(0),
vocab(0)
{
   if(!pApp)
      pApp = this;

   if(argc < 2)
   {
      displayUsage();
      return; // pretty bad thing happened.
   }

   if(!RDF_Wrapper::init())
      return;

   vocab = new Vocabulary(*this);

   in_url.set(argv[1]);
   runnable = 1;
}

/* Application destructor.  Called at program exit. */
SPF_APP::~SPF_APP()
{
   if(fetch_fh)
      PR_Close(fetch_fh);

   if(ldif_fh)
      PR_Close(ldif_fh);

   if(vocab)
      delete vocab;
}

/* Displays Application Usage Info */
void SPF_APP::displayUsage()
{
   printf("Usage:\n");
   printf("   spf2ldif <file.rdf>\n\n");
   printf("Output:\n");
   printf("   - file.ldif: an LDAP import file\n");
   printf("   - file.fetch: additional resources to be fetched\n\n");
   printf("Configuration:\n");
   printf("   - spf2ldif.conf: associates organizations with styles\n");
   printf("   - example:\n"); 
   printf("       Organization Name: news\n");
   printf("   - Currently supported styles:\n");
   printf("     - news\n");
   printf("     - default\n");
}

/* Application run method */
int SPF_APP::run()
{
   if(!runnable) return 1;

   readConfigFile("spf2ldif.conf");

   RDFFile file = readRDFFile (in_url.string(), vocab->resources.Channel, PR_TRUE, gRemoteStore);
   if (file && file->assertionCount > 0)
   {
      /* Make sure that both DC and SP namespaces are correct */
      if(ensureValidNameSpaces(file))
      {
         createOutputFiles();
         processRDF();
      }
      rdf_freefile(file);
   }
   else
   {
      printf("Error parsing file at url: %s\n", in_url.string());
      return 1;
   }

   return 0;
}

/* Creates new .LDIF and .FETCH files with same base name as RDF file */
void SPF_APP::createOutputFiles()
{
   char* fetch_fileURL = strreplacei(in_url.string(), ".rdf", ".fetch");
   char* ldif_fileURL = strreplacei(in_url.string(), ".rdf", ".ldif");
   PR_Delete(strrchr(ldif_fileURL, '/') + 1);
   PR_Delete(strrchr(fetch_fileURL, '/') + 1);
   fetch_fh = CallPROpenUsingFileURL(fetch_fileURL, PR_WRONLY | PR_CREATE_FILE, 0644);
   ldif_fh = CallPROpenUsingFileURL(ldif_fileURL, PR_WRONLY | PR_CREATE_FILE, 0644);
   free(fetch_fileURL);
   free(ldif_fileURL);
}

XP_Bool SPF_APP::ensureValidNameSpaces(RDFFile f)
{
   DublinCore dc;
   SitePreview sp;
   XP_Bool foundDC = FALSE, foundSP = FALSE;

   XMLNameSpace ns = f->namespaces;
   while (ns) {
     if (startsWith(ns->url, dc._namespace))
       foundDC = TRUE;
     if (startsWith(ns->url, sp._namespace))
       foundSP = TRUE;
     ns = ns->next;
   }

   if(!foundDC || !foundSP)
   {
      fprintf(stderr, "Invalid NameSpaces encountered.\n");
      fprintf(stderr, "Legal Namespaces:\n");
      fprintf(stderr, "\tDublin Core (DC): %s\n", dc._namespace);
      fprintf(stderr, "\tSite Preview (SP): %s\n", sp._namespace);
      return FALSE;
   }

   return TRUE;
}

void SPF_APP::readConfigFile(char* filename)
{
   FILE* file = fopen(filename, "r");
   char buf[1024];
   if(file)
   {
      while( fgets(buf, sizeof(buf), file) != NULL)
      {
         char* colon = strchr(buf, ':');
         if(!colon) continue;

         char* Value = strtok(colon + 1, " \n\r");
         if(Value)
         {
            *colon = 0;
            colon --;
            while(*colon == ' ')
            {
               *colon = 0;
               colon --;
            }
            char* key   = strdup(buf);
            char* value = strdup(Value);
            ChannelTable.put(key, value);
         }
      }
      fclose(file);
   }
}

/* Turns RDF into .LDIF and .FETCH */
void SPF_APP::processRDF()
{
   printf("processing SPF Data into LDIF... \n");

   SP_Organization* Org = OrgList.getHead();
   while(Org)
   {
      Org->RDF_to_LDIF(*vocab, ldif_fh, fetch_fh);
      Org = OrgList.getNext();
   }

   RDF_Resource r_Channel = ChannelsList.getHead();
   while(r_Channel)
   {
      SP_Channel* Channel = SP_Channel::newChannel(*vocab, *this, r_Channel);
      if(Channel)
      {
         Channel->RDF_to_LDIF(*vocab, ldif_fh, fetch_fh);
         delete Channel;
      }
      r_Channel = ChannelsList.getNext();
   }
}

/* Main.  Duh */
int main(int argc, char** argv)
{
   SPF_APP app(argc, argv);
   return app.run();
}


void notifySlotValueAdded(RDF_Resource u, RDF_Resource s, void* v, RDF_ValueType type)
{
   SPF_APP& app = *SPF_APP::pApp;

   if(type == RDF_RESOURCE_TYPE)
   {
      if (endsWith(app.vocab->resources.Organization.getTagName(), resourceID((RDF_Resource)v)))
      {
         SP_Organization* org = new SP_Organization(app, u);
         app.OrgList.add(org);
      }

      if (endsWith(app.vocab->resources.Channel.getTagName(), resourceID((RDF_Resource)v)))
      {
         app.ChannelsList.add(u);
      }
   }
}



LDIF_Entry::LDIF_Entry(PRFileDesc* file) :
DN(NULL),
fd(file)
{
}

/* Inserts 'key=value, ' at the beginning of DN. */
void LDIF_Entry::pushDN(char* key, char* value, XP_Bool addAttr)
{
   if(!key || !value) return;

   XP_Bool bFirstTime = DN ? FALSE : TRUE;
   int len = bFirstTime ? 0 : strlen(DN);
   len += strlen(key) + strlen(value) + strlen("=, \n");
   char* temp = new char[len + 1];
   char* escapedval = ldapDnEscape(value);

   if(bFirstTime)
      sprintf(temp, "%s=%s\n", key, escapedval);
   else
   {
      sprintf(temp, "%s=%s, %s", key, escapedval, DN);
      delete[] DN;
   }
   DN = temp;

   delete[] escapedval;

   if(addAttr)
      pushAttribute(key, value);
}

LDIF_Entry::LDIF_Entry(LDIF_Entry& source) :
DN(NULL)
{
   if(source.DN)
   {
      DN = new char[strlen(source.DN) + 1];
      strcpy(DN, source.DN);
   }

   fd = source.fd;

   Attribute* attr = source.attributes.getHead();
   while(attr)
   {
      Attribute* newattr = new Attribute(attr->key, attr->value);
      attributes.add(newattr);
      attr = source.attributes.getNext();
   }
}

LDIF_Entry::~LDIF_Entry()
{
   if(DN)
      delete[] DN;
   clearAttributes();
}

void LDIF_Entry::pushAttribute(char* key, char* value, XP_Bool Escape)
{
   if(!key || !value) return;

   char* val = Escape ? ldapDnEscape((char*)value) : value;

   Attribute* attr = new Attribute(key, val);
   attributes.add(attr);

   if(Escape)
      delete[] val;
}

void LDIF_Entry::clearAttributes()
{
   attributes.empty();
}

void LDIF_Entry::write(PRFileDesc* fh)
{
   if(!DN)
   {
      fprintf(stderr, "Tried to write an LDIF entry with no DN!\n");
      return;
   }

   if(fh == NULL) fh = fd;
   PR_Write(fh, "DN: ", 4);
   PR_Write(fh, DN, strlen(DN));
   Attribute* attr = attributes.getHead();
   while(attr){
      int len = strlen(attr->key) + strlen(attr->value) + strlen(": \n");
      char* buf = new char[len + 1];
      sprintf(buf, "%s: %s\n", attr->key, attr->value);
      PR_Write(fh, buf, len);
      delete[] buf;
      attr = attributes.getNext();
   }
   PR_Write(fh, "\n", 1);
}

void LDIF_Entry::popAttribute()
{
   attributes.getTail();
   attributes.remove(TRUE);
}
