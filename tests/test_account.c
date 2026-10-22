#include "../src/account.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    Account a;
    AccountSignOut(&a);
    assert(!a.signed_in && a.display_name[0] == '\0');
    assert(!AccountSetDisplayName(&a, "Operator"));
    assert(!AccountSignIn(&a, "demo", "wrong"));
    assert(!AccountSignIn(&a, "", "parcel123"));
    assert(!AccountSignIn(&a, NULL, NULL));
    assert(AccountSignIn(&a, "demo", "parcel123"));
    assert(a.signed_in && strcmp(a.display_name, "Parcel operator") == 0);
    assert(AccountSetDisplayName(&a, "Oleksandr"));
    assert(!AccountSetDisplayName(&a, "   "));
    assert(!AccountSetDisplayName(&a, "Invalid\nname"));
    assert(!AccountSetDisplayName(&a, "1234567890123456789012345678901234567890123456789"));
    assert(strcmp(a.display_name, "Oleksandr") == 0);
    assert(AccountLock(&a) && a.signed_in && a.locked);
    assert(!AccountSetDisplayName(&a, "Changed while locked"));
    assert(!AccountUnlock(&a,"demo","wrong") && a.locked);
    assert(!AccountUnlock(&a,"other","parcel123") && a.locked);
    assert(!AccountUnlock(&a,NULL,NULL) && a.locked);
    assert(strcmp(a.display_name,"Oleksandr")==0);
    assert(AccountUnlock(&a,"demo","parcel123"));
    assert(!a.locked && a.signed_in && strcmp(a.display_name,"Oleksandr")==0);
    assert(AccountLock(&a));
    AccountSignOut(&a);
    assert(!a.signed_in && a.display_name[0] == '\0');
    assert(!a.locked && !AccountLock(&a));
    assert(!AccountUnlock(&a,"demo","parcel123"));
    assert(AccountSignIn(&a, "demo", "parcel123"));
    assert(strcmp(a.display_name, "Parcel operator") == 0);
    assert(!AccountSignIn(&a, "demo", "incorrect"));
    assert(!a.signed_in);
    puts("Account checks passed: credentials, profile validation, lock/unlock preservation and sign-out.");
    return 0;
}
