/* This stub function allows us to get around
   the linking error we were seeing on OSF 
   because of the svrcore component.  This 
   module should be deleted as soon as the 
   svrcore mess is cleaned up */

#ifdef OSF1
void SVRPLCY_InstallSSLPolicy()
{
	return;
}
#endif

