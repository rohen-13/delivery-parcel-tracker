#ifndef DISPATCH_UI_H
#define DISPATCH_UI_H
#include "raylib.h"
void DispatchUIInit(void);
void DispatchUIReset(void);
void DispatchUISetViewport(float scale, Vector2 offset);
/* Actions: 0 stays on dispatch, 1 opens monitoring, 2 opens the shared profile. */
int DispatchUIDraw(void);
void DispatchUIClose(void);
#endif
