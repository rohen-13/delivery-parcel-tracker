#ifndef ROUND_STORE_H
#define ROUND_STORE_H
#include "round.h"
/* Versioned local checkpoint; no account data or cached route geometry.
 * Save atomically replaces path. Failed load leaves round untouched. */
bool RoundSave(const DeliveryRound *round,const char *path);
bool RoundLoad(DeliveryRound *round,const char *path);
#endif
