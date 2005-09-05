
#ifndef nsDOMJSUtils_h__
#define nsDOMJSUtils_h__

#include "jsapi.h"
#include "nsIScriptContext.h"

// seems like overkill for just this 1 function - but let's see what else
// falls out first.
inline nsIScriptContext *
GetScriptContextFromJSContext(JSContext *cx)
{
  if (!(::JS_GetOptions(cx) & JSOPTION_PRIVATE_IS_NSISUPPORTS)) {
    return nsnull;
  }

  nsCOMPtr<nsIScriptContext> scx =
    do_QueryInterface(NS_STATIC_CAST(nsISupports *,
                                     ::JS_GetContextPrivate(cx)));

  // This will return a pointer to something that's about to be
  // released, but that's ok here.
  return scx;
}

#endif // nsDOMJSUtils_h__
