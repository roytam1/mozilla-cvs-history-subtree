#include <stdio.h>
#include "nsString.h"

void test_assign_helper(const nsACString& in, nsACString &_retval)
  {
    _retval = in;
  }

PRBool test_assign()
  {
    nsCString result;
    test_assign_helper(NS_LITERAL_CSTRING("a") + NS_LITERAL_CSTRING("b"), result);
    PRBool r = strcmp(result.get(), "ab") == 0;
    if (!r)
      printf("[result=%s]\n", result.get());
    return r;
  }

PRBool test1()
  {
    NS_NAMED_LITERAL_STRING(empty, "");
    const nsAString& aStr = empty;

    nsAutoString buf(aStr);

    PRInt32 n = buf.FindChar(',');

    n = buf.Length();

    buf.Cut(0, n + 1);
    n = buf.FindChar(',');

    if (n != kNotFound)
      printf("n=%d\n", n);

    return n == kNotFound;
  }

PRBool test2()
  {
    nsCString data("hello world");
    const nsACString& aStr = data;

    nsCString temp(aStr);
    temp.Cut(0, 6);

    PRBool r = strcmp(temp.get(), "world") == 0;
    if (!r)
      printf("[temp=%s]\n", temp.get());
    return r;
  }

PRBool test_find()
  {
    nsCString src("<!DOCTYPE blah blah blah>");

    PRInt32 i = src.Find("DOCTYPE", PR_TRUE, 2, 1);
    if (i == 2)
      return PR_TRUE;

    printf("i=%d\n", i);
    return PR_FALSE;
  }

PRBool test_rfind()
  {
    const char text[] = "<!DOCTYPE blah blah blah>";
    nsCString src(text);
    PRInt32 i = src.RFind("bLaH", PR_TRUE, 3, -1); 
    if (i == -1)
      return PR_TRUE;

    printf("i=%d\n", i);
    return PR_FALSE;
  }

PRBool test_rfind_2()
  {
    const char text[] = "<!DOCTYPE blah blah blah>";
    nsCString src(text);
    PRInt32 i = src.RFind("TYPE", PR_FALSE, 5, -1); 
    if (i == 5)
      return PR_TRUE;

    printf("i=%d\n", i);
    return PR_FALSE;
  }

PRBool test_rfind_3()
  {
    const char text[] = "urn:mozilla:locale:en-US:necko";
    nsCAutoString value(text);
    PRInt32 i = value.RFind(":");
    if (i == 24)
      return PR_TRUE;

    printf("i=%d\n", i);
    return PR_FALSE;
  }

PRBool test_distance()
  {
    const char text[] = "abc-xyz";
    nsCString s(text);
    nsCString::const_iterator begin, end;
    s.BeginReading(begin);
    s.EndReading(end);
    size_t d = Distance(begin, end);
    PRBool r = (d == sizeof(text)-1);
    if (!r)
      printf("d=%u\n", d);
    return r;
  }

PRBool test_length()
  {
    const char text[] = "abc-xyz";
    nsCString s(text);
    size_t d = s.Length();
    PRBool r = (d == sizeof(text)-1);
    if (!r)
      printf("d=%u\n", d);
    return r;
  }

PRBool test_trim()
  {
    const char text[] = " a\t    $   ";
    const char set[] = " \t$";

    nsCString s(text);
    s.Trim(set);
    PRBool r = strcmp(s.get(), "a") == 0;
    if (!r)
      printf("[s=%s]\n", s.get());
    return r;
  }

PRBool test_replace_substr()
  {
    const char text[] = "abc-ppp-qqq-ppp-xyz";
    nsCString s(text);
    s.ReplaceSubstring("ppp", "www");
    PRBool r = strcmp(s.get(), "abc-www-qqq-www-xyz") == 0;
    if (!r)
      printf("[s=%s]\n", s.get());
    return r;
  }

PRBool test_strip_ws()
  {
    const char text[] = " a    $   ";
    nsCString s(text);
    s.StripWhitespace();
    PRBool r = strcmp(s.get(), "a$") == 0;
    if (!r)
      printf("[s=%s]\n", s.get());
    return r;
  }

PRBool test_equals_ic()
  {
    nsCString s;
    PRBool r = s.EqualsIgnoreCase("view-source");
    if (r)
      printf("[r=%d]\n", r);
    return !r;
  }

PRBool test_cbufdesc()
  {
    const char text[] = "hello world";
    CBufDescriptor bufdesc(text, PR_TRUE, sizeof(text)-1);
    nsCAutoString s(bufdesc);
    if (strcmp(s.get(), text) == 0)
      return PR_TRUE;
    
    printf("[s=%s]\n", s.get());
    return PR_FALSE;
  }

PRBool test_concat()
  {
    nsCString bar("bar");
    const nsACString& barRef = bar;

    const nsPromiseFlatCString& result =
        PromiseFlatCString(NS_LITERAL_CSTRING("foo") +
                           NS_LITERAL_CSTRING(",") +
                           barRef);
    if (strcmp(result.get(), "foo,bar") == 0)
      return PR_TRUE;

    printf("[result=%s]\n", result.get());
    return PR_FALSE;
  }

PRBool test_concat_2()
  {
    nsCString fieldTextStr("xyz");
    nsCString text("text");
    const nsACString& aText = text;

    nsCAutoString result = fieldTextStr + aText;

    if (strcmp(result.get(), "xyztext") == 0)
      return PR_TRUE;
    
    printf("[result=%s]\n", result.get());
    return PR_FALSE;
  }

//----

typedef PRBool (*TestFunc)();

static const struct Test
  {
    const char* name;
    TestFunc    func;
  }
tests[] =
  {
    { "test_assign", test_assign },
    { "test1", test1 },
    { "test2", test2 },
    { "test_find", test_find },
    { "test_rfind", test_rfind },
    { "test_rfind_2", test_rfind_2 },
    { "test_rfind_3", test_rfind_3 },
    { "test_distance", test_distance },
    { "test_length", test_length },
    { "test_trim", test_trim },
    { "test_replace_substr", test_replace_substr },
    { "test_strip_ws", test_strip_ws },
    { "test_equals_ic", test_equals_ic },
    { "test_cbufdesc", test_cbufdesc },
    { "test_concat", test_concat },
    { "test_concat_2", test_concat_2 },
    { nsnull, nsnull }
  };

//----

int main()
  {
    for (const Test* t = tests; t->name != nsnull; ++t)
      {
        printf("%20s : %s\n", t->name, t->func() ? "SUCCESS" : "FAILURE");
      }
    
    return 0;
  }
