#include "nsCOMPtr.h"
#include "nsISVGRendererRegion.h"
#include "nsISVGRectangleSink.h"
#include "nsSVGCairoRegion.h"

class nsSVGCairoRectRegion : public nsISVGRendererRegion
{
protected:
  friend nsresult NS_NewSVGCairoRectRegion(nsISVGRendererRegion** result,
                                           int x, int y,
                                           int width, int height);
  nsSVGCairoRectRegion(int x, int y, int w, int h);
  
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsISVGRendererRegion interface:
  NS_DECL_NSISVGRENDERERREGION

private:
  int mX, mY, mWidth, mHeight;
};

//----------------------------------------------------------------------
// implementation:

nsresult
NS_NewSVGCairoRectRegion(nsISVGRendererRegion** result, int x, int y,
                         int width, int height)
{
  *result = new nsSVGCairoRectRegion(x, y, width, height);
  
  if (!*result) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*result);
  return NS_OK;
}

nsSVGCairoRectRegion::nsSVGCairoRectRegion(int x, int y, int w, int h) :
    mX(x), mY(y), mWidth(w), mHeight(h)
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGCairoRectRegion)
NS_IMPL_RELEASE(nsSVGCairoRectRegion)

NS_INTERFACE_MAP_BEGIN(nsSVGCairoRectRegion)
  NS_INTERFACE_MAP_ENTRY(nsISVGRendererRegion)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsISVGRendererRegion methods:

/** Implements nsISVGRendererRegion combine(in nsISVGRendererRegion other); */
NS_IMETHODIMP
nsSVGCairoRectRegion::Combine(nsISVGRendererRegion *other,
                              nsISVGRendererRegion **_retval)
{
  nsSVGCairoRectRegion *_other = static_cast<nsSVGCairoRectRegion*>(other); // ok, i know i'm being bad

  int x1 = PR_MIN(mX, _other->mX);
  int y1 = PR_MIN(mY, _other->mY);
  int x2 = PR_MAX(mX+mWidth, _other->mX+_other->mWidth);
  int y2 = PR_MAX(mY+mHeight, _other->mHeight+_other->mHeight);
  
  return NS_NewSVGCairoRectRegion(_retval, x1, y1, x2-x1, y2-y1);
}

/** Implements void getRectangleScans(in nsISVGRectangleSink sink); */
NS_IMETHODIMP
nsSVGCairoRectRegion::GetRectangleScans(nsISVGRectangleSink *sink)
{
  sink->SinkRectangle(mX, mY, mWidth, mHeight);
  return NS_OK;
}
