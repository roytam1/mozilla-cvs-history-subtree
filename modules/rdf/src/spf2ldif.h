#ifndef _SPF2LDIF_
  #define _SPF2LDIF_

template <class T>
class List
{
public:
   struct Node;
   inline List() : head(0), tail(0), current(0) {}
   inline ~List() {
      while(head) {
         current = head;
         head = head->next;
         delete current;
      }
   }
   inline void add(T* newElement) {
      tail = new Node(newElement, tail, 0);
      if(!head) 
         head = tail;
   }
   inline void remove(XP_Bool bDeleteItem = FALSE) {
      if(current)
      {
         if(current == tail)
            tail = current->prev;
         if(current == head)
            head = current->next;
         if(current->prev)
            current->prev->next = current->next;
         if(current->next)
            current->next->prev = current->prev;
         if(bDeleteItem)
            delete current->value;
         delete current;
         current = tail;
      }
   }
   inline void empty(XP_Bool bDeleteItem = FALSE) {
      current = tail;
      while(current){
         remove(bDeleteItem);
      }
   }
   inline T* getHead() {
      current = head;
      return current ? current->value : (T*)NULL;
   }
   inline T* getTail() {
      current = tail;
      return current ? current->value : (T*)NULL;
   }
   inline T* getNext() {
      current = current ? current->next : NULL;
      return current ? current->value : (T*)NULL;
   }
   inline T* getPrev() {
      current = current ? current->prev : NULL;
      return current ? current->value : (T*)NULL;
   }
public:
   struct Node {
      Node(T* _value, Node* _prev, Node* _next) 
         : value(_value), prev(_prev), next(_next) {
         if(this->prev) this->prev->next = this;
         if(this->next) this->next->prev = this;
      }
      T*    value;
      Node* prev;
      Node* next;
   };
   Node* head, *tail, *current;
};


#define ROOT_ORGANIZATION "netcenter.com"
class LDIF_Entry
{
public:
   LDIF_Entry(PRFileDesc* file);
   LDIF_Entry(LDIF_Entry& source);
   ~LDIF_Entry();

   void pushDN(char* key, char* value, XP_Bool addAttr = TRUE);
   void pushAttribute(char* key, char* value, XP_Bool Escape=FALSE);
   void popAttribute();
   void clearAttributes();
   void write(PRFileDesc* file=NULL);
protected:
class Attribute
{
public:
   inline Attribute(const char* _key, const char* _value) : key(strdup(_key)), value(strdup(_value)) {}
   inline ~Attribute() {
      free(key);
      free(value);
   }
   char *key;
   char *value;
};

public:
   char* DN;
   List<Attribute> attributes;
   PRFileDesc* fd;
};

class RDF_Wrapper
{
public:
   RDF_Wrapper();
   RDF_Wrapper(RDF_Wrapper& source);
   virtual ~RDF_Wrapper();
   int init();
   inline char* getSlotValue(RDF_Resource source, RDF_Resource arcLabel) {
      return (char*)RDF_GetSlotValue(db, source, arcLabel, RDF_STRING_TYPE, PR_FALSE, PR_TRUE);
   }
   inline RDF_Resource getSlotResource(RDF_Resource source, RDF_Resource arcLabel) {
      return (RDF_Resource)RDF_GetSlotValue(db, source, arcLabel, RDF_RESOURCE_TYPE, PR_FALSE, PR_TRUE);
   }
   inline RDF_Resource getResource(char* name, XP_Bool create=TRUE) {
      return (RDF_Resource)RDF_GetResource(db, name, create);
   }
   inline RDF_Cursor getTargets(RDF_Resource source, RDF_Resource arcLabel) {
      c = RDF_GetTargets(db, source, arcLabel, RDF_STRING_TYPE, PR_TRUE);
      return c;
   }
   inline RDF_Cursor getResourceTargets(RDF_Resource source, RDF_Resource arcLabel) {
      c = RDF_GetTargets(db, source, arcLabel, RDF_RESOURCE_TYPE, PR_TRUE);
      return c;
   }
   inline char* getNext() {
      return (char *)RDF_NextValue(c);      
   }
   inline RDF_Resource getNextResource() {
      return (RDF_Resource)RDF_NextValue(c);      
   }

   inline operator RDF() {
      return db;
   }


protected:
   RDF db;
   RDF_Cursor c;
   XP_Bool isCopy;
};


class Vocabulary;

class VocabElement
{
public:
    VocabElement(char* _xmlnamespace, char* _elementName, RDF_Wrapper& _rdf, RDF_Resource _res);
   virtual ~VocabElement();
   virtual void RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
   inline char* getTagName() {return elementName; }
   inline char* getTagSuffix() {
       char* end = strrchr(elementName, '/');
       return end ? end + 1 : NULL;
   }
   inline operator RDF_Resource() { return res; }
   inline operator char*() { return id; }
protected:
   RDF_Wrapper& rdf;
   char* elementName;
   RDF_Resource res;
   char* id;
};

class ArcToString : public VocabElement
{
protected:
  ArcToString(char* _xmlnamespace, char* _elementName, RDF_Wrapper& _rdf, RDF_Resource _res);
  virtual ~ArcToString();
public:
  virtual void RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
  inline char* getSlotValue(RDF_Resource Parent) {
    return rdf.getSlotValue(Parent, *this);
  }
};

class ArcToResource : public VocabElement
{
protected:
  ArcToResource(char* _xmlnamespace, char* _elementName, RDF_Wrapper& _rdf, RDF_Resource _res);
  virtual ~ArcToResource();
public:
  virtual void RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
  inline RDF_Resource getSlotValue(RDF_Resource Parent) {
      return rdf.getSlotResource(Parent, *this);
  }
};

class Resource : public VocabElement
{
protected:
  Resource(char* _xmlnamespace, char* _elementName, RDF_Wrapper& _rdf, RDF_Resource _res);
  virtual ~Resource();
  char* ldiftitle;
  virtual char* constructLDIFTitle(char* title);
public:
  virtual void RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
  inline RDF_Resource getSlotValue(ArcToResource& arc) {
    return rdf.getSlotResource(*this, arc);
  }
  inline char* getSlotValue(ArcToString& arc) {
    return rdf.getSlotValue(*this, arc);
  }
  virtual inline char* ldifTitle(char* title=NULL) {
      return ldiftitle ? ldiftitle : constructLDIFTitle(title);
  }
};

class DublinCore
{
public:
   DublinCore(char* xmlnamespace="http://purl.oclc.org/metadata/dublin_core_elements/");
   virtual ~DublinCore();
   char* _namespace;
};

class SitePreview
{
public:
   SitePreview(char* xmlnamespace="http://www.netscape.com/sp/schema/");
   virtual ~SitePreview();
   char* _namespace;
};

class DC_creator : public DublinCore, public ArcToString
{
public:
   DC_creator(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "creator");
};

class DC_date : public DublinCore, public ArcToString
{
public:
   DC_date(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "date");
   virtual void RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
   time_t makeTime(char* DateString);
};


class DC_description : public DublinCore, public ArcToString
{
public:
   DC_description(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "description");
};

class DC_language : public DublinCore, public ArcToString
{
public:
   DC_language(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "language");
};

class DC_source : public DublinCore, public ArcToString
{
public:
   DC_source(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "source");
};

class DC_publisher : public DublinCore, public ArcToResource
{
public:
   DC_publisher(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "publisher");
};

class DC_subject : public DublinCore, public ArcToString
{
public:
   DC_subject(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "subject");
};
class DC_title : public DublinCore, public ArcToString
{
public:
   DC_title(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "title");
};
class SP_aboutEvent : public SitePreview, public ArcToString
{
public:
   SP_aboutEvent(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "aboutEvent");
};
class SP_aboutOrganization : public SitePreview, public ArcToString
{
public:
   SP_aboutOrganization(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "aboutOrganization");
};
class SP_aboutPlace : public SitePreview, public ArcToString
{
public:
   SP_aboutPlace(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "aboutPlace");
};
class SP_aboutPerson : public SitePreview, public ArcToString
{
public:
   SP_aboutPerson(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "aboutPerson");
};

class SP_Channel : public SitePreview, public Resource
{
public:
   SP_Channel(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "Channel");
   virtual void RDF_to_LDIF(Vocabulary& vocab, PRFileDesc* ldif_fh, PRFileDesc* fetch_fh);
   static SP_Channel* newChannel(Vocabulary& vocab, RDF_Wrapper& rdf, RDF_Resource r);
protected:
   virtual XP_Bool ldif_header(Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
   virtual XP_Bool ldif_body(Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
};

class SP_NewsChannel : public SP_Channel
{
public:
   SP_NewsChannel(RDF_Wrapper& rdf, RDF_Resource r);
   virtual void RDF_to_LDIF(Vocabulary& vocab, PRFileDesc* ldif_fh, PRFileDesc* fetch_fh);
};

class SP_image : public SitePreview, public ArcToResource
{
public:
   SP_image(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "image");
   virtual void RDF_to_LDIF(RDF_Resource Parent, Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
};
class SP_Item : public SitePreview, public Resource
{
public:
   SP_Item(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "Item");
   virtual void RDF_to_LDIF(Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
};
class SP_NewsItem : public SP_Item
{
public:
   SP_NewsItem(RDF_Wrapper& rdf, RDF_Resource r = NULL);
   virtual void RDF_to_LDIF(Vocabulary& vocab, LDIF_Entry& ldif, PRFileDesc* fetch_fh);
};
class SP_link : public SitePreview, public ArcToString
{
public:
   SP_link(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "link");
};
class SP_Organization : public SitePreview, public Resource
{
public:
   SP_Organization(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "Organization");
   virtual void RDF_to_LDIF(Vocabulary& vocab, PRFileDesc* ldif_fh, PRFileDesc* fetch_fh);
};
class SP_section : public SitePreview, public ArcToResource
{
public:
   SP_section(RDF_Wrapper& rdf, RDF_Resource r = NULL, char* elementName = "section");
};


/*
*/

class ResourceVocab : public List<VocabElement>
{
public:
   ResourceVocab(RDF_Wrapper& rdf);

   SP_Channel      Channel;
   SP_Item         Item;
   SP_Organization Organization;
};

class ResourceArcVocab : public List<VocabElement>
{
public:
   ResourceArcVocab(RDF_Wrapper& rdf);

   DC_publisher publisher;
   SP_image image;
   SP_section section;
};

class StringArcVocab : public List<VocabElement>
{
public:
   StringArcVocab(RDF_Wrapper& rdf);

   DC_creator           creator;
   DC_date              date;
   DC_description       description;
   DC_language          language;
   DC_source            source;
   DC_subject           subject;
   DC_title             title;
   SP_aboutEvent        aboutEvent;
   SP_aboutOrganization aboutOrganization;
   SP_aboutPlace        aboutPlace;
   SP_aboutPerson       aboutPerson;
   SP_link              link;
};

class Vocabulary
{
public:
   Vocabulary(RDF_Wrapper& rdf);

   ResourceVocab    resources;
   ResourceArcVocab resource_arcs;
   StringArcVocab   string_arcs;
};


class URL
{
public:
   URL();
   URL(char* source);
   URL(URL& source);
   virtual ~URL();
   void set(char* source);
   void set(URL& source);
   void append(char* source);
   inline char* string() { return url; }
   inline const int length() { return len; }
protected:
   char* url;
   int len;
};

extern "C" void notifySlotValueAdded(RDF_Resource u, RDF_Resource s, void* v, RDF_ValueType type);

class SPF_APP : public RDF_Wrapper
{
public:
   SPF_APP(int argc, char** argv);
   ~SPF_APP();
   int run();
   void displayUsage();
   HashTable                ChannelTable;
   static SPF_APP*          pApp;
               
private:
   int                      runnable;
   URL                      in_url;
   PRFileDesc*              fetch_fh;
   PRFileDesc*              ldif_fh; 
   Vocabulary*              vocab;
   List<RDF_ResourceStruct> ChannelsList;
   List<SP_Organization>    OrgList;

   XP_Bool ensureValidNameSpaces(RDFFile f);
   void createOutputFiles();
   void readConfigFile(char* filename);
   void processRDF();

   friend void notifySlotValueAdded(RDF_Resource u, RDF_Resource s, void* v, RDF_ValueType type);
};

#endif /* _SPF2LDIF_ */
