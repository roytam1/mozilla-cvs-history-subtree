450	ldapssl_client_init
451	ldapssl_init
452	ldapssl_install_routines
453	ldapssl_clientauth_init
454	ldapssl_enable_clientauth
456	ldapssl_advclientauth_init
457	ldapssl_pkcs_init
458	ldapssl_err2string
# the last Windows ordinal number that has been reserved for SSL is 469.

# Windows ordinals 1100-1150 are reserved for privately/non-published
# exported routines
# Temporarily export SVRCORE/NSS functions due
# to two versions getting confused
1100    SVRCORE_RegisterPinObj
1101    SVRCORE_CreateStdPinObj
1102    SVRCORE_CreateFilePinObj
1103    PK11_FreeSlot
1104    PK11_GetInternalKeySlot
1105    PK11_GetTokenName

