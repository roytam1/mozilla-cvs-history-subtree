/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef mkhttp_h__
#define mkhttp_h__

#include "mktcp.h"
#include "rosetta.h"

#if defined(SMOOTH_PROGRESS)
class nsHTTPTransfer;
#endif

/* definitions of state for the state machine design
 */
typedef enum {
    HTTP_START_CONNECT,
    HTTP_FINISH_CONNECT,
    HTTP_SEND_PROXY_TUNNEL_REQUEST,
    HTTP_BEGIN_UPLOAD_FILE,
    HTTP_SEND_REQUEST,
    HTTP_SEND_POST_DATA,
    HTTP_PARSE_FIRST_LINE,
    HTTP_PARSE_MIME_HEADERS,
    HTTP_SETUP_STREAM,
    HTTP_BEGIN_PUSH_PARTIAL_CACHE_FILE,
    HTTP_PUSH_PARTIAL_CACHE_FILE,
    HTTP_PULL_DATA,
    HTTP_DONE,
    HTTP_ERROR_DONE,
    HG93634
    HTTP_FREE
} StatesEnum;

/* structure to hold data about a tcp connection
 * to a news host
 */
typedef struct _HTTPConnection {
    char   *hostname;       /* hostname string (may contain :port) */
    PRFileDesc *sock;       /* socket */
    XP_Bool busy;           /* is the connection in use already? */
    XP_Bool prev_cache;     /* did this connection come from the cache? */
    XP_Bool secure;         /* is it a secure connection? */
    XP_Bool doNotSendConHdr;
} HTTPConnection;

typedef enum {
    POINT_NINE,
    ONE_POINT_O,
    ONE_POINT_ONE
} HTTP_Version;

/* structure to hold data pertaining to the active state of
 * a transfer in progress.
 *
 */
typedef struct _HTTPConData {
    StatesEnum  next_state;       /* the next state or action to be taken */
    char *      proxy_server;     /* name of proxy server if any */
    char *      proxy_conf;       /* proxy config ptr from proxy autoconfig */
    char *      line_buffer;      /* temporary string storage */
    char *      server_headers;   /* headers from the server for 
                                   * the proxy client */
    char *      orig_host;        /* if the host gets modified
                                   * for my "netscape" -> "www.netscape.com"
                                   * hack, the original host gets put here */
    XP_File     partial_cache_fp;
    int32       partial_needed;   /* the part missing from the cache */

    int32       total_size_of_files_to_post;
    int32       total_amt_written;

    int32       line_buffer_size; /* current size of the line buffer */
  
    HTTPConnection *connection;   /* struct to hold info about connection */

    NET_StreamClass * stream; /* The output stream */
    Bool     pause_for_read;   /* Pause now for next read? */
    Bool     send_http1;       /* should we send http/1.1? */
    Bool     acting_as_proxy;  /* are we acting as a proxy? */
    Bool     server_busy_retry; /* should we retry the get? */
    Bool     posting;          /* are we posting? */
    Bool     doing_redirect;   /* are we redirecting? */
    Bool     save_redirect_method;   /* don't change METHOD when redirecting */
    Bool     sent_authorization;     /* did we send auth with the request? */
    Bool     sent_proxy_auth;    /* did we send proxy auth with the req? */
    Bool     authorization_required; /* did we get a 401 auth return code? */
    Bool     proxy_auth_required;    /* did we get a 407 auth return code? */
    Bool     destroy_graph_progress; /* destroy graph progress? */
    Bool     destroy_file_upload_progress_dialog;  
    HTTP_Version  protocol_version;       /* .9 1.0 or 1.1 */ 
    int32    original_content_length; /* the content length at the time of
                                       * calling graph progress */
    TCP_ConData *tcp_con_data;  /* Data pointer for tcp connect state machine */
    Bool         use_proxy_tunnel;          /* should we use a proxy tunnel? */
    Bool         proxy_tunnel_setup_done;   /* is setup done */
    Bool         use_copy_from_cache;    /* did we get a 304? */
    Bool         displayed_some_data;    /* have we displayed any data? */
    Bool         save_connection;        /* save this connection for reuse? */
    Bool         partial_cache_file;
    Bool         reuse_stream;
    Bool         connection_is_valid;
#ifdef XP_WIN
    Bool         calling_netlib_all_the_time;  /* is SetCallNetlibAllTheTime set? */
#endif
    void        *write_post_data_data;   /* a data object 
                                          * for the WritePostData function */

#if defined(SMOOTH_PROGRESS)
    nsHTTPTransfer* transfer;
#endif
} HTTPConData;



#endif /* mkhttp_h__ */
