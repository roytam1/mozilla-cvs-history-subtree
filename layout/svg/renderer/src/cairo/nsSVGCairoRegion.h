#ifndef __NS_SVGCAIRO_REGION_H__
#define __NS_SVGCAIRO_REGION_H__

class nsISVGRendererRegion;

//----------------------------------------------------------------------
// region constructors:

nsresult NS_NewSVGCairoRectRegion(nsISVGRendererRegion** result,
                                  int x, int y, int width, int height);

#endif // __NS_SVGCAIRO_REGION_H__
