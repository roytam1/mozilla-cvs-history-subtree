*** ldif.h	Tue Feb  8 11:06:53 2000
--- /export/ws/ws_csdk_branch_41sdk/ns/netsite/ldap/include/ldif.h	Tue Apr 25 12:44:23 2000
***************
*** 17,22 ****
--- 17,24 ----
  extern "C" {
  #endif
  
+ #define LDIF_VERSION_ONE        1	/* LDIF standard version */
+ 
  #define LDIF_MAX_LINE_WIDTH      76      /* maximum length of LDIF lines */
  
  /*
***************
*** 36,48 ****
--- 38,61 ----
      ((tlen) + 4 + LDIF_BASE64_LEN(vlen) \
      + ((LDIF_BASE64_LEN(vlen) + tlen + 3) / LDIF_MAX_LINE_WIDTH * 2 ))
  
+ /*
+  * Options for ldif_put_type_and_value_with_options() and 
+  * ldif_type_and_value_with_options().
+  */
+ #define LDIF_OPT_NOWRAP			0x01UL
+ #define LDIF_OPT_VALUE_IS_URL		0x02UL
+ #define LDIF_OPT_MINIMAL_ENCODING	0x04UL
  
  int ldif_parse_line( char *line, char **type, char **value, int *vlen);
  char * ldif_getline( char **next );
  void ldif_put_type_and_value( char **out, char *t, char *val, int vlen );
  void ldif_put_type_and_value_nowrap( char **out, char *t, char *val, int vlen );
+ void ldif_put_type_and_value_with_options( char **out, char *t, char *val,
+ 	int vlen, unsigned long options );
  char *ldif_type_and_value( char *type, char *val, int vlen );
  char *ldif_type_and_value_nowrap( char *type, char *val, int vlen );
+ char *ldif_type_and_value_with_options( char *type, char *val, int vlen,
+ 	unsigned long options );
  int ldif_base64_decode( char *src, unsigned char *dst );
  int ldif_base64_encode( unsigned char *src, char *dst, int srclen,
  	int lenused );
