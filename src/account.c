#include "account.h"
#include <string.h>

static bool CredentialsValid(const char *username, const char *password)
{
    return username && password && strcmp(username,"demo")==0 && strcmp(password,"parcel123")==0;
}

void AccountSignOut(Account *account)
{
    memset(account, 0, sizeof(*account));
}

bool AccountSignIn(Account *account, const char *username, const char *password)
{
    AccountSignOut(account);
    if (!CredentialsValid(username,password)) return false;
    account->signed_in = true;
    strcpy(account->display_name, "Parcel operator");
    return true;
}

bool AccountSetDisplayName(Account *account, const char *name)
{
    if (!account->signed_in || account->locked || !name) return false;
    size_t length = strlen(name);
    if (length == 0 || length >= sizeof(account->display_name)) return false;
    bool has_text = false;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)name[i];
        if (c < 32 || c > 126) return false;
        if (c != ' ') has_text = true;
    }
    if (!has_text) return false;
    memmove(account->display_name, name, length + 1);
    return true;
}

bool AccountLock(Account *account)
{
    if(!account->signed_in)return false;
    account->locked=true;
    return true;
}

bool AccountUnlock(Account *account,const char *username,const char *password)
{
    if(!account->signed_in || !account->locked || !CredentialsValid(username,password))return false;
    account->locked=false;
    return true;
}
