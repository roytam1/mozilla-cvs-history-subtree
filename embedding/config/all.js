pref("general.useragent.locale", "chrome://navigator/locale/navigator.properties");
pref("general.useragent.contentlocale", "chrome://navigator-region/locale/region.properties");
pref("general.useragent.misc", "rv:1.3b");

pref("browser.cache.memory.enable",         true);
pref("browser.cache.memory.capacity",       1024);
pref("browser.cache.disk_cache_ssl",        false);

// 0 = once-per-session, 1 = each-time, 2 = never, 3 = when-appropriate/automatically
pref("browser.cache.check_doc_frequency",   3);

pref("browser.display.use_document_fonts",  1);  // 0 = never, 1 = quick, 2 = always
pref("browser.display.use_document_colors", true);
pref("browser.display.use_system_colors",   false);
pref("browser.display.foreground_color",    "#000000");
pref("browser.display.background_color",    "#FFFFFF");
pref("browser.display.force_inline_alttext", false); // true = force ALT text for missing images to be layed out inline
// 0 = no external leading, 
// 1 = use external leading only when font provides, 
// 2 = add extra leading both internal leading and external leading are zero
pref("browser.display.normal_lineheight_calc_control", 2);
pref("browser.display.show_image_placeholders", true); // true = show image placeholders while image is loaded and when image is broken
pref("browser.anchor_color",                "#0000EE");
pref("browser.visited_color",               "#551A8B");
pref("browser.underline_anchors",           true);
pref("browser.blink_allowed",               true);
pref("browser.enable_automatic_image_resizing", false);

pref("browser.display.use_focus_colors",    false);
pref("browser.display.focus_background_color", "#117722");
pref("browser.display.focus_text_color",     "#ffffff");
pref("browser.display.focus_ring_width",     1);
pref("browser.display.focus_ring_on_anything", false);

pref("browser.sessionhistory.max_entries", 1);

// loading and rendering of framesets and iframes
pref("browser.frames.enabled", true);

// form submission
pref("browser.forms.submit.backwards_compatible", true);

// gfx widgets
pref("nglayout.widget.mode", 2);
pref("nglayout.widget.gfxscrollbars", true);

// css2 hover pref
pref("nglayout.events.showHierarchicalHover", false);

// dispatch left clicks only to content in browser (still allows clicks to chrome/xul)
pref("nglayout.events.dispatchLeftClickOnly", true);

// whether or not to use xbl form controls
pref("nglayout.debug.enable_xbl_forms", false);

// size of scrollbar snapping region
pref("slider.snapMultiplier", 6);

// URI fixup prefs
pref("browser.fixup.alternate.enabled", true);
pref("browser.fixup.alternate.prefix", "www.");
pref("browser.fixup.alternate.suffix", ".com");

// Default Capability Preferences: Security-Critical! 
// Editing these may create a security risk - be sure you know what you're doing
//pref("capability.policy.default.barprop.visible.set", "UniversalBrowserWrite");

pref("capability.policy.default_policynames", "mailnews");
pref("capability.policy.policynames", "");

pref("capability.policy.default.DOMException.code", "allAccess");
pref("capability.policy.default.DOMException.message", "allAccess");
pref("capability.policy.default.DOMException.name", "allAccess");
pref("capability.policy.default.DOMException.result", "allAccess");
pref("capability.policy.default.DOMException.toString", "allAccess");

pref("capability.policy.default.History.back", "allAccess");
pref("capability.policy.default.History.current", "UniversalBrowserRead");
pref("capability.policy.default.History.forward", "allAccess");
pref("capability.policy.default.History.go", "allAccess");
pref("capability.policy.default.History.item", "UniversalBrowserRead");
pref("capability.policy.default.History.next", "UniversalBrowserRead");
pref("capability.policy.default.History.previous", "UniversalBrowserRead");
pref("capability.policy.default.History.toString", "UniversalBrowserRead");

pref("capability.policy.default.HTMLDocument.close", "allAccess");
pref("capability.policy.default.HTMLDocument.open", "allAccess");

pref("capability.policy.default.Location.hash.set", "allAccess");
pref("capability.policy.default.Location.href.set", "allAccess");
pref("capability.policy.default.Location.reload", "allAccess");
pref("capability.policy.default.Location.replace", "allAccess");

pref("capability.policy.default.Navigator.preference", "allAccess");
pref("capability.policy.default.Navigator.preferenceinternal.get", "UniversalPreferencesRead");
pref("capability.policy.default.Navigator.preferenceinternal.set", "UniversalPreferencesWrite");

pref("capability.policy.default.Window.blur", "allAccess");
pref("capability.policy.default.Window.close", "allAccess");
pref("capability.policy.default.Window.closed", "allAccess");
pref("capability.policy.default.Window.Components", "allAccess");
pref("capability.policy.default.Window.document", "allAccess");
pref("capability.policy.default.Window.focus", "allAccess");
pref("capability.policy.default.Window.frames", "allAccess");
pref("capability.policy.default.Window.history", "allAccess");
pref("capability.policy.default.Window.length", "allAccess");
pref("capability.policy.default.Window.location", "allAccess");
pref("capability.policy.default.Window.opener", "allAccess");
pref("capability.policy.default.Window.parent", "allAccess");
pref("capability.policy.default.Window.self", "allAccess");
pref("capability.policy.default.Window.top", "allAccess");
pref("capability.policy.default.Window.window", "allAccess");

// Restrictions on the DOM for mail/news - see bugs 66938 and 84545
pref("capability.policy.mailnews.sites", "mailbox: imap: news:");

pref("capability.policy.mailnews.*.attributes.get", "noAccess");
pref("capability.policy.mailnews.*.baseURI.get", "noAccess");
pref("capability.policy.mailnews.*.data.get", "noAccess");
pref("capability.policy.mailnews.*.getAttribute", "noAccess");
pref("capability.policy.mailnews.*.getNamedItem", "noAccess");
pref("capability.policy.mailnews.*.host.get", "noAccess");
pref("capability.policy.mailnews.*.hostname.get", "noAccess");
pref("capability.policy.mailnews.*.href.get", "noAccess");
pref("capability.policy.mailnews.*.innerHTML.get", "noAccess");
pref("capability.policy.mailnews.*.lowSrc.get", "noAccess");
pref("capability.policy.mailnews.*.nodeValue.get", "noAccess");
pref("capability.policy.mailnews.*.pathname.get", "noAccess");
pref("capability.policy.mailnews.*.protocol.get", "noAccess");
pref("capability.policy.mailnews.*.src.get", "noAccess");
pref("capability.policy.mailnews.*.substringData.get", "noAccess");
pref("capability.policy.mailnews.*.text.get", "noAccess");
pref("capability.policy.mailnews.*.title.get", "noAccess");
pref("capability.policy.mailnews.DOMException.toString", "noAccess");
pref("capability.policy.mailnews.HTMLAnchorElement.toString", "noAccess");
pref("capability.policy.mailnews.HTMLDocument.domain", "noAccess");
pref("capability.policy.mailnews.HTMLDocument.URL", "noAccess");
pref("capability.policy.mailnews.Location.toString", "noAccess");
pref("capability.policy.mailnews.Range.toString", "noAccess");
pref("capability.policy.mailnews.Window.blur", "noAccess");
pref("capability.policy.mailnews.Window.focus", "noAccess");
pref("capability.policy.mailnews.Window.innerWidth.set", "noAccess");
pref("capability.policy.mailnews.Window.innerHeight.set", "noAccess");
pref("capability.policy.mailnews.Window.moveBy", "noAccess");
pref("capability.policy.mailnews.Window.moveTo", "noAccess");
pref("capability.policy.mailnews.Window.name.set", "noAccess");
pref("capability.policy.mailnews.Window.outerHeight.set", "noAccess");
pref("capability.policy.mailnews.Window.outerWidth.set", "noAccess");
pref("capability.policy.mailnews.Window.resizeBy", "noAccess");
pref("capability.policy.mailnews.Window.resizeTo", "noAccess");
pref("capability.policy.mailnews.Window.screenX.set", "noAccess");
pref("capability.policy.mailnews.Window.screenY.set", "noAccess");
pref("capability.policy.mailnews.Window.sizeToContent", "noAccess");
pref("capability.policy.mailnews.document.load", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.channel", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.responseXML", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.responseText", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.status", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.statusText", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.abort", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.getAllResponseHeaders", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.getResponseHeader", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.open", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.send", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.setRequestHeader", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.readyState", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.overrideMimeType", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.onload", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.onerror", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.onreadystatechange", "noAccess");
pref("capability.policy.mailnews.XMLSerializer.serializeToString", "noAccess");
pref("capability.policy.mailnews.XMLSerializer.serializeToStream", "noAccess");
pref("capability.policy.mailnews.DOMParser,parseFromString", "noAccess");
pref("capability.policy.mailnews.DOMParser,parseFromStream", "noAccess");
pref("capability.policy.mailnews.SOAPCall.transportURI", "noAccess");
pref("capability.policy.mailnews.SOAPCall.verifySourceHeader", "noAccess");
pref("capability.policy.mailnews.SOAPCall.invoke", "noAccess");
pref("capability.policy.mailnews.SOAPCall.asyncInvoke", "noAccess");
pref("capability.policy.mailnews.SOAPResponse.fault", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.styleURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getAssociatedEncoding", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.setEncoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getEncoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.setDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.setDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.defaultEncoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.defaultDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.schemaCollection", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.encode", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.decode", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.mapSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.unmapSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getInternalSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getExternalSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPFault.element", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultNamespaceURI", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultCode", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultString", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultActor", "noAccess");
pref("capability.policy.mailnews.SOAPFault.detail", "noAccess");
pref("capability.policy.mailnews.SOAPHeaderBlock.actorURI", "noAccess");
pref("capability.policy.mailnews.SOAPHeaderBlock.mustUnderstand", "noAccess");
pref("capability.policy.mailnews.SOAPParameter", "noAccess");
pref("capability.policy.mailnews.SOAPPropertyBagMutator.propertyBag", "noAccess");
pref("capability.policy.mailnews.SOAPPropertyBagMutator.addProperty", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.load", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.loadAsync", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.processSchemaElement", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.onLoad", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.onError", "noAccess");

// XMLExtras
pref("capability.policy.default.XMLHttpRequest.channel", "noAccess");
pref("capability.policy.default.DOMParser.parseFromStream", "noAccess");

// Scripts & Windows prefs
pref("browser.block.target_new_window",     false);
pref("dom.disable_cookie_get",              false);
pref("dom.disable_cookie_set",              false);
pref("dom.disable_image_src_set",           false);
pref("dom.disable_open_during_load",        false);
pref("dom.disable_window_flip",             false);
pref("dom.disable_window_move_resize",      false);
pref("dom.disable_window_status_change",    false);

pref("dom.disable_window_open_feature.titlebar",    false);
pref("dom.disable_window_open_feature.close",       false);
pref("dom.disable_window_open_feature.toolbar",     false);
pref("dom.disable_window_open_feature.location",    false);
pref("dom.disable_window_open_feature.directories", false);
pref("dom.disable_window_open_feature.personalbar", false);
pref("dom.disable_window_open_feature.menubar",     false);
pref("dom.disable_window_open_feature.scrollbars",  false);
pref("dom.disable_window_open_feature.resizable",   false);
pref("dom.disable_window_open_feature.minimizable", false);
pref("dom.disable_window_open_feature.status",      false);

pref("dom.allow_scripts_to_close_windows",          false);

pref("javascript.enabled",                  true);
pref("javascript.allow.mailnews",           false);
pref("javascript.options.strict",           false);
pref("javascript.options.showInConsole",    true);

// advanced prefs
pref("advanced.always_load_images",         true);
pref("security.enable_java",                true);
pref("image.animation_mode",                "normal");

// <http>
pref("network.http.version", "1.1");	  // default
// pref("network.http.version", "1.0");   // uncomment this out in case of problems
// pref("network.http.version", "0.9");   // it'll work too if you're crazy
// keep-alive option is effectively obsolete. Nevertheless it'll work with
// some older 1.0 servers:

pref("network.http.proxy.version", "1.1");    // default
// pref("network.http.proxy.version", "1.0"); // uncomment this out in case of problems
                                              // (required if using junkbuster proxy)

// enable caching of http documents
pref("network.http.use-cache", true);

// this preference can be set to override the socket type used for normal
// HTTP traffic.  an empty value indicates the normal TCP/IP socket type.
pref("network.http.default-socket-type", "");

pref("network.http.keep-alive", true); // set it to false in case of problems
pref("network.http.proxy.keep-alive", true);
pref("network.http.keep-alive.timeout", 300);

// limit the absolute number of http connections.
pref("network.http.max-connections", 24);

// limit the absolute number of http connections that can be established per
// host.  if a http proxy server is enabled, then the "server" is the proxy
// server.  Otherwise, "server" is the http origin server.
pref("network.http.max-connections-per-server", 8);

// if network.http.keep-alive is true, and if NOT connecting via a proxy, then
// a new connection will only be attempted if the number of active persistent
// connections to the server is less then max-persistent-connections-per-server.
pref("network.http.max-persistent-connections-per-server", 2);

// if network.http.keep-alive is true, and if connecting via a proxy, then a
// new connection will only be attempted if the number of active persistent
// connections to the proxy is less then max-persistent-connections-per-proxy.
pref("network.http.max-persistent-connections-per-proxy", 4);

// amount of time (in seconds) to suspend pending requests, before spawning a
// new connection, once the limit on the number of persistent connections per
// host has been reached.  however, a new connection will not be created if
// max-connections or max-connections-per-server has also been reached.
pref("network.http.request.max-start-delay", 10);

// http specific network timeouts (XXX currently unused)
pref("network.http.connect.timeout",  30);	// in seconds
pref("network.http.request.timeout", 120);	// in seconds

// Headers
pref("network.http.accept.default", "text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,video/x-mng,image/png,image/jpeg,image/gif;q=0.2,*/*;q=0.1");

pref("network.http.sendRefererHeader",      2); // 0=don't send any, 1=send only on clicks, 2=send on image requests as well

// Controls whether we send HTTPS referres to other HTTPS sites.
// By default this is enabled for compatibility (see bug 141641)
pref("network.http.sendSecureXSiteReferrer", true);

// Maximum number of consecutive redirects before aborting.
pref("network.http.redirection-limit", 20);

// Enable http compression: comment this out in case of problems with 1.1
pref("network.http.accept-encoding" ,"gzip,deflate,compress;q=0.9");

pref("network.http.pipelining"      , false);
pref("network.http.proxy.pipelining", false);

// Always pipeling the very first request:  this will only work when you are
// absolutely sure the the site or proxy you are browsing to/through support
// pipelining; the default behavior will be that the browser will first make
// a normal, non-pipelined request, then  examine  and remember the responce
// and only the subsequent requests to that site will be pipeline
pref("network.http.pipelining.firstrequest", false);

// Max number of requests in the pipeline
pref("network.http.pipelining.maxrequests" , 4);

pref("network.http.proxy.ssl.connect",true);
// </http>

// This preference controls whether or not internationalized domain names (IDN)
// are handled.  IDN requires a nsIIDNService implementation.
pref("network.enableIDN", true);

// This preference controls whether or not URLs with UTF-8 characters are
// escaped.  Set this preference to TRUE for strict RFC2396 conformance.
pref("network.standard-url.escape-utf8", true);

// Idle timeout for ftp control connections - 5 minute default
pref("network.ftp.idleConnectionTimeout", 300);

// directory listing format - constants are defined in nsIDirectoryListing.idl
// Do not set this to 0...
pref("network.dir.format", 2);

// enables the prefetch service (i.e., prefetching of <link rel="next"> URLs).
pref("network.prefetch-next", true);

pref("network.hosts.socks_server",          "");
pref("network.hosts.socks_serverport",      1080);
pref("network.hosts.socks_conf",            "");
pref("network.image.imageBehavior",         0); // 0-Accept, 1-dontAcceptForeign, 2-dontUse
pref("network.image.warnAboutImages",       false);
pref("network.proxy.autoconfig_url",        "");
pref("network.proxy.type",                  0);
pref("network.proxy.ftp",                   "");
pref("network.proxy.ftp_port",              0);
pref("network.proxy.gopher",                "");
pref("network.proxy.gopher_port",           0);
pref("network.proxy.news",                  "");
pref("network.proxy.news_port",             0);
pref("network.proxy.http",                  "");
pref("network.proxy.http_port",             0);
pref("network.proxy.wais",                  "");
pref("network.proxy.wais_port",             0);
pref("network.proxy.ssl",                   "");
pref("network.proxy.ssl_port",              0);
pref("network.proxy.socks",                 "");
pref("network.proxy.socks_port",            0);
pref("network.proxy.socks_version",         5);
pref("network.proxy.no_proxies_on",         "localhost, 127.0.0.1");
pref("network.online",                      true); //online/offline
pref("network.cookie.cookieBehavior",       3); // 0-Accept, 1-dontAcceptForeign, 2-dontUse, 3-p3p
pref("network.cookie.disableCookieForMailNews", true); // disable all cookies for mail
pref("network.cookie.warnAboutCookies",     false);
pref("network.cookie.lifetime.enabled",     false);
pref("network.cookie.lifetime.behavior",    0);
pref("network.cookie.lifetime.days",        90);

// The following default value is for p3p medium mode.
// See extensions/cookie/resources/content/p3p.xul for the definitions of low/medium/hi
pref("network.cookie.p3p",                  "ffffaaaa");
pref("network.cookie.p3plevel",             1); // 0=low, 1=medium, 2=high, 3=custom

pref("network.enablePad",                   false); // Allow client to do proxy autodiscovery
pref("network.enableIDN",                   false); // Turn on/off IDN (Internationalized Domain Name) resolution
pref("converter.html2txt.structs",          true); // Output structured phrases (strong, em, code, sub, sup, b, i, u)
pref("converter.html2txt.header_strategy",  1); // 0 = no indention; 1 = indention, increased with header level; 2 = numbering and slight indention

pref("intl.accept_languages",               "chrome://navigator/locale/navigator.properties");
pref("intl.accept_charsets",                "iso-8859-1,*,utf-8");
pref("intl.collationOption",                "chrome://navigator-platform/locale/navigator.properties");
pref("intl.menuitems.alwaysappendacceskeys","chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.static",     "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more1",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more2",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more3",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more4",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more5",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.mailedit",           "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.cache",      "");
pref("intl.charsetmenu.mailview.cache",     "");
pref("intl.charsetmenu.composer.cache",     "");
pref("intl.charsetmenu.browser.cache.size", 5);
pref("intl.charset.detector",               "chrome://navigator/locale/navigator.properties");
pref("intl.charset.default",                "chrome://navigator-platform/locale/navigator.properties");
pref("intl.content.langcode",               "chrome://communicator-region/locale/region.properties");
pref("intl.locale.matchOS",                 false);
// fallback charset list for Unicode conversion (converting from Unicode)
// currently used for mail send only to handle symbol characters (e.g Euro, trademark, smartquotes)
// for ISO-8859-1
pref("intl.fallbackCharsetList.ISO-8859-1", "windows-1252");
pref("font.language.group",                 "chrome://navigator/locale/navigator.properties");

pref("signed.applets.codebase_principal_support", false);
pref("security.checkloaduri", true);
pref("security.xpconnect.plugin.unrestricted", true);

// Modifier key prefs: default to Windows settings,
// menu access key = alt, accelerator key = control.
// Use 17 for Ctrl, 18 for Alt, 224 for Meta, 0 for none. Mac settings in macprefs.js
pref("ui.key.accelKey", 17);
pref("ui.key.generalAccessKey", 18);
pref("ui.key.menuAccessKey", 18);
pref("ui.key.menuAccessKeyFocuses", false);
pref("ui.key.saveLink.shift", true); // true = shift, false = meta

// Middle-mouse handling
pref("middlemouse.paste", false);
pref("middlemouse.openNewWindow", true);
pref("middlemouse.contentLoadURL", false);
pref("middlemouse.scrollbarPosition", false);

// Clipboard behavior
pref("clipboard.autocopy", false);

// 0=lines, 1=pages, 2=history , 3=text size
pref("mousewheel.withnokey.action",0);
pref("mousewheel.withnokey.numlines",1);	
pref("mousewheel.withnokey.sysnumlines",true);
pref("mousewheel.withcontrolkey.action",0);
pref("mousewheel.withcontrolkey.numlines",1);
pref("mousewheel.withcontrolkey.sysnumlines",true);
pref("mousewheel.withshiftkey.action",0);
pref("mousewheel.withshiftkey.numlines",1);
pref("mousewheel.withshiftkey.sysnumlines",false);
pref("mousewheel.withaltkey.action",2);
pref("mousewheel.withaltkey.numlines",1);
pref("mousewheel.withaltkey.sysnumlines",false);

pref("profile.confirm_automigration",true);

pref("prefs.converted-to-utf8",false);

// --------------------------------------------------
// IBMBIDI 
// --------------------------------------------------
//
// ------------------
//  Text Direction
// ------------------
// 1 = directionLTRBidi *
// 2 = directionRTLBidi
pref("bidi.direction", 1);
// ------------------
//  Text Type
// ------------------
// 1 = charsettexttypeBidi *
// 2 = logicaltexttypeBidi
// 3 = visualtexttypeBidi
pref("bidi.texttype", 1);
// ------------------
//  Controls Text Mode
// ------------------
// 1 = logicalcontrolstextmodeBidiCmd
// 2 = visualcontrolstextmodeBidi <-- NO LONGER SUPPORTED
// 3 = containercontrolstextmodeBidi *
pref("bidi.controlstextmode", 1);
// ------------------
//  Clipboard Text Mode
// ------------------
//  1 = logicalclipboardtextmodeBidi
// 2 = visiualclipboardtextmodeBidi
// 3 = sourceclipboardtextmodeBidi *
pref("bidi.clipboardtextmode", 3);
// ------------------
//  Numeral Style
// ------------------
// 1 = regularcontextnumeralBidi *
// 2 = hindicontextnumeralBidi
// 3 = arabicnumeralBidi
// 4 = hindinumeralBidi
pref("bidi.numeral", 1);
// ------------------
//  Support Mode
// ------------------
// 1 = mozillaBidisupport *
// 2 = OsBidisupport
// 3 = disableBidisupport
pref("bidi.support", 1);
// ------------------
//  Charset Mode
// ------------------
// 1 = doccharactersetBidi *
// 2 = defaultcharactersetBidi
pref("bidi.characterset", 1);


pref("browser.throbber.url","chrome://navigator-region/locale/region.properties");

// used for double-click word selection behavior. Win will override.
pref("layout.word_select.eat_space_to_next_word", false);
pref("layout.word_select.stop_at_punctuation", true);

// pref to force frames to be resizable
pref("layout.frames.force_resizability", false);

// pref to permit users to make verified SOAP calls by default
pref("capability.policy.default.SOAPCall.invokeVerifySourceHeader", "allAccess");

// if true, allow plug-ins to override internal imglib decoder mime types in full-page mode
pref("plugin.override_internal_types", false);
pref("plugin.expose_full_path", false); // if true navigator.plugins reveals full path

// See bug 136985.  Gives embedders a pref to hook into to show
// a popup blocker if they choose.
pref("browser.popups.showPopupBlocker", true);

// Pref to control whether the viewmanager code does double-buffering or not
// See http://bugzilla.mozilla.org/show_bug.cgi?id=169483 for further details...
pref("viewmanager.do_doublebuffering", false);


