#ifndef UI_TYPOGRAPHY_H
#define UI_TYPOGRAPHY_H
#include "raylib.h"
/* Installed native fonts are loaded, not copied or redistributed. */
static inline Font ParcelFont(bool heading)
{
    const char *path=heading?"/System/Library/Fonts/Supplemental/Trebuchet MS Bold.ttf":"/System/Library/Fonts/Supplemental/Trebuchet MS.ttf";
    if(!FileExists(path))path=heading?"/System/Library/Fonts/Supplemental/Arial Bold.ttf":"/System/Library/Fonts/Supplemental/Arial.ttf";
    Font result=FileExists(path)?LoadFontEx(path,64,NULL,0):GetFontDefault();
    SetTextureFilter(result.texture,TEXTURE_FILTER_BILINEAR);
    return result;
}
#endif
