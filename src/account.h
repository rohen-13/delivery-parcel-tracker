#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stdbool.h>

/* Local demonstration session only; no server or stored passwords. */
typedef struct {
    bool signed_in;
    bool locked;
    char display_name[49];
} Account;

void AccountSignOut(Account *account);
bool AccountSignIn(Account *account, const char *username, const char *password);
bool AccountSetDisplayName(Account *account, const char *name);
bool AccountLock(Account *account);
bool AccountUnlock(Account *account, const char *username, const char *password);

#endif
