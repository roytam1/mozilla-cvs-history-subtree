// -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; moz-jssh-buffer-globalobj: "getWindows()[0]" -*-
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

////////////////////////////////////////////////////////////////////////
// imports

Components.utils.importModule("gre:ClassUtils.js");
Components.utils.importModule("gre:RDFUtils.js");
Components.utils.importModule("gre:FileUtils.js");
Components.utils.importModule("gre:StringUtils.js");
Components.utils.importModule("gre:AsyncUtils.js");

////////////////////////////////////////////////////////////////////////
// globals

var wUserAgent = "zap/0.1.7"; // String to be sent in User-Agent
                              // header for SIP requests
var wNetUtils = Components.classes["@mozilla.org/zap/netutils;1"].getService(Components.interfaces.zapINetUtils);
var wPromptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
var wRDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);
var wRDFContainerUtils = Components.classes["@mozilla.org/rdf/container-utils;1"].createInstance(Components.interfaces.nsIRDFContainerUtils);

var wInteractPane;
var wURLField;

var wAlertBar;

var wSidebarTree;
var wSidebarDS;
var wEphemeralSidebarContainer;

var wConfig;
var wConfigDS;
var wServicesContainer;

var wIdentitiesDS;
var wIdentitiesContainer;
var wCurrentIdentity;
var wPasswordManager;
var wPasswordManagerInt;

var wContactsDS;
// all contacts live in the urn:mozilla:zap:contacts sequence:
var wContactsContainer;
// ... those displayed in the sidebar are also pointed to from the
// urn:mozilla:zap:friends sequence:
var wFriendsContainer;

var wCallsDS;
var wCallsContainer;

// an application-global ds for storing emphemeral state, e.g. whether
// a registration is active or not:
var wGlobalEphemeralDS = Components.classes["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].createInstance(Components.interfaces.nsIRDFDataSource);

var wSipStack;
var wSdpService;

var wErrorLog;

// helper to emit a warning to the current error sink:
function warning(message) {
  ErrorReporterSink.reporterFunction(message);
}

////////////////////////////////////////////////////////////////////////
// Initialization:

// this needs to be called inline here to load the datasources required by
// the PersistentRDFObject subclasses defined below:
ensureProfile();

function windowInit() {
  dump("Initializing zap main window...\n");

  // set up error logging:
  // XXX this should be configurable (enable/disable)
  wErrorLog = openFileForWriting(getProfileFile("zap.log"), true);
  ErrorReporterSink.reporterFunction = function (mes) {
    if (wErrorLog)
      wErrorLog.write(mes,mes.length);
    else
      dump(mes);
  };
  
  wInteractPane = document.getElementById("interactpane");
  wURLField = document.getElementById("url_field");
  wAlertBar = document.getElementById("alertbar");
  
  initConfig();
  initContacts();
  initCalls();
  initSidebar();
  wMediaPipeline.init();
  initSipStack();
  initIdentities();
  
  dump("... Done initializing zap main window\n");
}

function windowClose() {
  cleanupSipStack();
  wMediaPipeline.cleanup();
  
  // close error log:
  wErrorLog.close();
  wErrorLog = null;
}

function ensureProfile() {
  // XXX At some point we need proper profile migration.
  // For now just copy files from profile defaults dir if they are not
  // found in the profile dir:
  if (!ensureProfileFile("calls.rdf") ||
      !ensureProfileFile("config.rdf") ||
      !ensureProfileFile("contacts.rdf") ||
      !ensureProfileFile("identities.rdf") ||
      !ensureProfileFile("sidebar.rdf")) {
    alert("Profile Error!");
  }

  // load datasources from profile:
  wCallsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("calls.rdf"));
  wConfigDS = wRDF.GetDataSourceBlocking(getProfileFileURL("config.rdf"));
  wContactsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("contacts.rdf"));
  wIdentitiesDS = wRDF.GetDataSourceBlocking(getProfileFileURL("identities.rdf"));
  wSidebarDS = wRDF.GetDataSourceBlocking(getProfileFileURL("sidebar.rdf"));
}

////////////////////////////////////////////////////////////////////////
// UI Commands:

function cmdAbout() {
  window.openDialog("chrome://zap/content/about.xul",
                    "about",
                    "chrome,modal,alwaysRaised",
                    null);
}

function cmdExit() {
  goQuitApplication();
}

function cmdGo() {
  loadPage("chrome://zap/content/make-call.xul", true);
}

function cmdGenericRequest() {
  loadPage("chrome://zap/content/generic-request.xul", false);
}

function cmdRegisterRequest() {
  loadPage("chrome://zap/content/register-request.xul", false);
}
  
////////////////////////////////////////////////////////////////////////
// Interaction pane:

function loadPage(url, force)
{
  clearSidebarSelection();
  if (wInteractPane.getAttribute("src") != url)
    wInteractPane.setAttribute("src", url);
  else if (force)
    reloadPage();
}

function reloadPage()
{
  wInteractPane.webNavigation.reload(0);
}

////////////////////////////////////////////////////////////////////////
// Config:

function initConfig() {
  wServicesContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wServicesContainer.Init(wConfigDS,
                          wRDF.GetResource("urn:mozilla:zap:services"));
  
  wConfig = Config.instantiate();
  wConfig.initWithResource(wRDF.GetResource("urn:mozilla:zap:config"));
}

var Config = makeClass("Config", PersistentRDFObject);
Config.prototype.datasources["default"] = wConfigDS;
Config.rdfResourceAttrib("urn:mozilla:zap:identity",
                         "urn:mozilla:zap:initial_identity");
Config.rdfLiteralAttrib("urn:mozilla:zap:instance_id", "");
Config.rdfLiteralAttrib("urn:mozilla:zap:ringtone", "zap:d=32,o=5,b=140:b4,b4,b4,b4,b4,b4,b4,b4,b4,b4,b4,b4,8p.,f4,f4,8p.,f4,f4,1p"); //"nuages_gris:d=4,o=4,b=180:p,d,g,2c#5,d5,a#,g,p,d,g,c#5,c#5,d5,a#,g,1p");
Config.rdfLiteralAttrib("urn:mozilla:zap:sip_port_base", "5060");
// time constants for flow failure recovery (draft-ietf-sip-outbound-01.txt 4.3):
Config.rdfLiteralAttrib("urn:mozilla:zap:registration_recovery_max_time", "1800");
Config.rdfLiteralAttrib("urn:mozilla:zap:registration_recovery_base_time_all_fail", "30");
Config.rdfLiteralAttrib("urn:mozilla:zap:registration_recovery_base_time_not_failed", "60");

Config.rdfLiteralAttrib("urn:mozilla:zap:dnd_code", "480"); // Temporarily unavail.
Config.rdfLiteralAttrib("urn:mozilla:zap:dnd_headers", ""); // additional headers for DND response

// pool of services, indexed by resource id:
var wServices = {};

function getService(resourceID) {
  var service = wServices[resourceID];
  if (!service) {
    service = Service.instantiate();
    service.initWithResource(wRDF.GetResource(resourceID));
    wServices[resourceID] = service;
  }
  return service;  
}

var Service = makeClass("Service", PersistentRDFObject);
Service.prototype.datasources["default"] = wConfigDS;
Service.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:domain", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:route1", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:route2", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:route3", "");
Service.rdfLiteralAttrib("urn:mozilla:zap:route4", "");
// address of STUN server for this service. (if empty we just use the domain name):
Service.rdfLiteralAttrib("urn:mozilla:zap:stun_server", "");
// whether or not we should use our mapped address as determined by a
// STUN request in contact addresses of registrations:
Service.rdfLiteralAttrib("urn:mozilla:zap:use_stun_contact_address", "false");
// whether or not we should send OPTIONS requests to keep alive the connection:
Service.rdfLiteralAttrib("urn:mozilla:zap:options_keep_alive", "false");
Service.rdfLiteralAttrib("urn:mozilla:zap:suggested_registration_interval", "");
// whether or not the top (loose) route header should be removed from
// (non-dialog) requests:
Service.rdfLiteralAttrib("urn:mozilla:zap:elide_destination_route_header", "false");

Service.fun(
  function getStunServer() {
    var s = this["urn:mozilla:zap:stun_server"];
    if (!s) s = this["urn:mozilla:zap:domain"];
    return s;
  });

Service.fun(
  function getDefaultRoute() {
    // XXX
    return [];
  });

Service.spec(
  function createFromDocument(doc) {
    // Temporarily defer flushing until later, so that the container
    // changes get flushed as well:
    this.autoflush = false;
    this._Service_createFromDocument(doc);
    this.autoflush = true;
    
    // Append to services container:
    wServicesContainer.AppendElement(this.resource);

    this.flush();
  });

////////////////////////////////////////////////////////////////////////
// Identities:

// pool of identities, indexed by resource id:
var wIdentities = {};

function getIdentity(resourceID) {
  var identity = wIdentities[resourceID];
  if (!identity) {
    identity = Identity.instantiate();
    identity.initWithResource(wRDF.GetResource(resourceID));
    wIdentities[resourceID] = identity;
  }
  return identity;
}

function getIdentityByAOR(uri) {
  for (var id in wIdentities) {
    if (wIdentities[id].getAOR().equals(uri))
      return wIdentities[id];
  }
  return null;
}

function initIdentities() {
  wPasswordManager = Components.classes["@mozilla.org/passwordmanager;1"].
    createInstance(Components.interfaces.nsIPasswordManager);
  wPasswordManagerInt = wPasswordManager.QueryInterface(Components.interfaces.nsIPasswordManagerInternal);
  wIdentitiesContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wIdentitiesContainer.Init(wIdentitiesDS,
                           wRDF.GetResource("urn:mozilla:zap:identities"));

  // set the current identity:
  var identitiesList = document.getElementById("identities_list_popup");
  identitiesList.database.AddDataSource(wIdentitiesDS);
  identitiesList.builder.rebuild();
  // select correct identity in identities list:
  document.getElementById("identities_list").selectedItem = document.getElementById(wConfig["urn:mozilla:zap:identity"]);
  // make sure out stack is configured for this identity profile:
  wCurrentIdentity = getIdentity(wConfig["urn:mozilla:zap:identity"]);
  currentIdentityUpdated();

  // register all identities that want automatic registration:
  var identity_resources = wIdentitiesContainer.GetElements();
  while (identity_resources.hasMoreElements()) {
    var identity = getIdentity(identity_resources.getNext().QueryInterface(Components.interfaces.nsIRDFResource).Value);
    if (identity["urn:mozilla:zap:automatic_registration"]=="true")
      registerIdentity(identity);
  }
}

// called when the user selects a different identity in the identities_list:
function identityChange() {
  var l = document.getElementById("identities_list").selectedItem.getAttribute("id");
  if (wConfig["urn:mozilla:zap:identity"] == l)
    return;
  else {
    wConfig["urn:mozilla:zap:identity"] = l;
  }
  wCurrentIdentity = getIdentity(wConfig["urn:mozilla:zap:identity"]);
  currentIdentityUpdated();
}

// we call this function every time the current identity profile is
// updated or a new one is selected so that we can reconfigure the
// stack from here. Alternatively we could listen in on the identity
// and config datasources, which we should probably do at some point.
function currentIdentityUpdated() {
}

// helper to determine our public address and call the success or
// failure continuation as appropriate:
function withMappedStunAddress(stunServer, success, failure) {
  wNetUtils.resolveMappedAddress(
    {
      onAddressResolveComplete: function(mappedAddress, status) {
        if (status == Components.results.NS_OK)
          success(mappedAddress);
        else
          failure(mappedAddress);
      }
    },
    stunServer);
}

//----------------------------------------------------------------------
// Identity class

var Identity = makeClass("Identity",
                         PersistentRDFObject, SupportsImpl, AsyncObject);
Identity.addInterfaces(Components.interfaces.zapISipCredentialsProvider);

Identity.prototype.datasources["default"] = wIdentitiesDS;
Identity.prototype.datasources["global-ephemeral"] = wGlobalEphemeralDS;

Identity.rdfResourceAttrib("urn:mozilla:zap:sidebarparent",
                           "urn:mozilla:zap:identities");
Identity.rdfLiteralAttrib("urn:mozilla:zap:chromepage",
                          "chrome://zap/content/identity.xul");
Identity.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "sip:thisis@anonymous.invalid");
Identity.rdfLiteralAttrib("urn:mozilla:zap:nodetype", "identity");
Identity.rdfLiteralAttrib("urn:mozilla:zap:display_name", "");
Identity.rdfLiteralAttrib("urn:mozilla:zap:organization", "");
Identity.rdfLiteralAttrib("urn:mozilla:zap:preference", "0.1");
Identity.rdfLiteralAttrib("urn:mozilla:zap:automatic_registration", "true");
Identity.rdfLiteralAttrib("urn:mozilla:zap:authentication_username", "");
Identity.rdfResourceAttrib("urn:mozilla:zap:service",
                           "urn:mozilla:zap:automatic_service");
Identity.rdfLiteralAttrib("urn:mozilla:zap:is_registered",
                          "false", "global-ephemeral");

// This will be set after we have attempted to resolve our stun
// address (if requested). Once this condition is reached, the
// Identity can be used for making calls, registering, etc (although
// it might not be fully operational).
Identity.addCondition("InitializationAttempted");

Identity.spec(
  function initWithResource(r) {
    this._Identity_initWithResource(r);
    this.service = getService(this.getServiceId());
    this.hostAddress = wSipStack.hostAddress;
    if (this.service["urn:mozilla:zap:use_stun_contact_address"] == "true")
      this.resolveStunAddress();
    else
      this.InitializationAttempted = true;
  });

Identity.fun(
  function resolveStunAddress() {
    var me = this;
    withMappedStunAddress(this.service.getStunServer(),
                          function(addr) {
                            me.hostAddress = addr;
                            me.InitializationAttempted = true;
                          },
                          function(addr) {
                            // unsuccessful
                            // -> reschedule in 10s
                            schedule(function() {
                                       // go ahead with registrations etc anyway: 
                                       me.InitializationAttempted = true;
                                       me.resolveStunAddress();
                                     },
                                     10000);
                          });
  });

Identity.spec(
  function createFromDocument(doc) {
    // Temporarily defer flushing until later, so that the container
    // changes get flushed as well:
    this.autoflush = false;
    this._Identity_createFromDocument(doc);
    this.autoflush = true;
    
    // Append to identities:
    wIdentitiesContainer.AppendElement(this.resource);

    this.flush();
  });

// zapISipCredentialsProvider methods:
Identity.fun(
  function getCredentialsForRealm(realm, username, password,
                                  hasRejectedCredentials) {
    var is_our_realm = (realm == this.getHost());
    // first check if we have stored values for the current session:
    if (is_our_realm && this.session_credentials) {
      username.value = this.session_credentials.username;
      password.value = this.session_credentials.password;
      if (!hasRejectedCredentials) return true;
    }
    else {
      // try to get the credentials from the password manager:
      
      if (is_our_realm) {
        // fill in username from identity database
        username.value = this["urn:mozilla:zap:username"];
        if (!username.value)
          username.value = this.getUser();
      }
      
      try {
        wPasswordManagerInt.findPasswordEntry(realm, username.value, null,
                                              {}, username, password);
        if (!hasRejectedCredentials) return true;
      }
      catch(e) {
      }
    }
    
    // either we don't know the credentials yet or we have previously
    // tried them and they have been rejected. in either case we want
    // to prompt:

    var saveCredentials = {value:is_our_realm};
    if (!wPromptService.promptUsernameAndPassword(null, "Enter credentials",
                                                  "Enter credentials for realm "+
                                                  realm+":",
                                                  username,
                                                  password,
                                                  "Save credentials for future sessions",
                                                  saveCredentials))
      return false;

    // remember credentials for this session:
    if (is_our_realm) {
      this.session_credentials = {};
      this.session_credentials.username = username.value;
      this.session_credentials.password = password.value;
    }
    if (saveCredentials) {
      // store credentials for future sessions:
      if (is_our_realm) {
        if (username.value != this.getUser()) {
          this["urn:mozilla:zap:username"] = username.value;
        }
      }
      try {
        // addUser doesn't update when there already is an existing entry.
        // -> try removing first
        wPasswordManager.removeUser(realm, username.value);
      } catch(e) {}
      wPasswordManager.addUser(realm, username.value, password.value);
    }
        
    return true;
  });

// Return user part of the AOR (or empty string if the AOR can't
// be parsed)
Identity.fun(
  function getUser() {
    // XXX these try/catch blocks will be redundant once we hook up
    // syntax checking in PersistentRDFObject
    try {
      var user = this.getAOR().QueryInterface(Components.interfaces.zapISipSIPURI).user;
    }
    catch(e) {
      this._dump("Can't determine user from "+this["http://home.netscape.com/NC-rdf#Name"]);
      return "";
    }
    return user;
  });

// Return host part of the AOR (or empty string if the AOR can't be
// parsed)
Identity.fun(
  function getHost() {
    // XXX these try/catch blocks will be redundant once we hook up
    // syntax checking in PersistentRDFObject
    try {
      var host = this.getAOR().QueryInterface(Components.interfaces.zapISipSIPURI).host;
    }
    catch(e) {
      this._dump("Can't determine domain from "+this["http://home.netscape.com/NC-rdf#Name"]);
      return "";
    }
    return host;
  });

// Get the AOR URI object for this Identity:
Identity.fun(
  function getAOR() {
    // XXX these try/catch blocks will become redundant once we hook
    // up syntax checking in PersistentRDFObject
    try {
      var aor = wSipStack.syntaxFactory.deserializeURI(this["http://home.netscape.com/NC-rdf#Name"]);
    }
    catch(e) {
      this._dump("Can't parse AOR "+this["http://home.netscape.com/NC-rdf#Name"]);
      return null;
    }
    return aor;
  });

// Get From Address for non-REGISTER requests:
Identity.fun(
  function getFromAddress() {
    return wSipStack.syntaxFactory.createAddress(this["urn:mozilla:zap:display_name"], this.getAOR());
  });

// this will be initially be set to our internal address and
// asynchronously resolved to our public STUN address if our
// associated service has urn:mozilla:zap:use_stun_contact_address set
// to true:
Identity.obj("hostAddress", null);

// contact address used for registration
Identity.fun(
  function getRegisterContactAddress() {
    // Synthesize from the 'From Address', adjusting host and port. 
    // XXX We DO want to keep the From address's display name, but what about
    // uri parameters???
    var addr = this.getFromAddress();
    var uri = addr.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
    uri.host = this.hostAddress;
    if (wSipStack.listeningPort == 5060)
      uri.port = "";
    else
      uri.port = wSipStack.listeningPort;

    // The zid will allow us to identify the identity for incoming
    // calls. The hex encoding (which will be encoded again for
    // transmission!) is necessary since uri parameters are not case
    // sensitive, but rdf resource ids are...    
    // XXX this is only needed when there are several identities/aor.
    // investigate whether this is a feasible use-case
    // uri.setURIParameter("zid", octetToHex(this.resource.Value));
    
    return addr;
  });

// Returns a structure {contact, routeset} with routing information
// for non-REGISTER requests:
Identity.fun(
  function getRoutingInfo() {
    var routingInfo;
    if (this.registrationGroup)
      routingInfo = this.registrationGroup.getRoutingInfo();

    if (!routingInfo) {
      this._dump("using unreliable routing for non-registered identity");
      return this._constructFallbackRoutingInfo();
    }

    return routingInfo;
  });

// Constructs request client flags based on this identity's service:
Identity.fun(
  function getRCFlags() {
    var flags = 0;
    if (this.service["urn:mozilla:zap:elide_destination_route_header"]=="true")
      flags |= Components.interfaces.zapISipUAStack.ELIDE_DESTINATION_ROUTE_HEADER;
    return flags;
  });

Identity.fun(
  function _constructFallbackRoutingInfo() {
    return {
      contact : this.getRegisterContactAddress(),
      routeset: this.service.getDefaultRoute()
    };
  });


// Work out which Service this Identity uses and return its ID.
Identity.fun(
  function getServiceId() {
    var service_id = this["urn:mozilla:zap:service"];
    if (service_id != "urn:mozilla:zap:automatic_service")
      return service_id;
    //else...
    // we need to try to figure out the service from the AOR:
    // walk through known Services:
    var services = wServicesContainer.GetElements();
    var resourceDomain = wRDF.GetResource("urn:mozilla:zap:domain");
    var resourceTheDomain = wRDF.GetLiteral(this.getHost());
    while (services.hasMoreElements()) {
      var service = services.getNext().QueryInterface(Components.interfaces.nsIRDFResource);
      if (wConfigDS.HasAssertion(service,
                                 resourceDomain,
                                 resourceTheDomain, true)) {
        return service.Value;
      }
    }
    // nope, we don't know the service; use the default service:
    return "urn:mozilla:zap:default_service";
  });

////////////////////////////////////////////////////////////////////////
// Registration:

// hash of registration groups indexed by identity resource:

// XXX We currently use one registration group per identity. This is a
// bit wasteful if we have several identities with the same
// AOR. Investigate whether this is common enough to care.
var wRegistrations = {};

// set up or refresh a registration group for the given identity
function registerIdentity(identity) {
  warning("registering "+identity["http://home.netscape.com/NC-rdf#Name"]+"\n");
  var registrationGroup = wRegistrations[identity.resource.Value];
  if (registrationGroup) {
    // We already have a registration group for this identity. We
    // are probably getting called because the user has modified the
    // identity or the service associated with the identity.
    // -> Clear old registration group before creating a new one
    warning("unregistering "+identity["http://home.netscape.com/NC-rdf#Name"]+"\n");
    registrationGroup.unregister();
  }
  // create a new registration group :
  registrationGroup = RegistrationGroup.instantiate();
  if (registrationGroup.init(identity))
    wRegistrations[identity.resource.Value] = registrationGroup;
}

function unregisterIdentity(identity) {
  var registrationGroup = wRegistrations[identity.resource.Value];
  if (!registrationGroup) return; // not registered
  
  warning("unregistering "+registrationGroup.identity["http://home.netscape.com/NC-rdf#Name"]+"\n");
  registrationGroup.unregister();
  delete wRegistrations[identity.resource.Value];
}

// called whenever the user edits a service
function notifyServiceUpdated(service_resource) {
  var service_id = service_resource.Value;
  // (re-)register any identities that use this service and have
  // automatic registration or are currently registered:
  for (var identity in wIdentities) {
    if (wIdentities[identity].service.resource.Value == service_id &&
        (wIdentities[identity]["urn:mozilla:zap:is_registered"]=="true" ||
         wIdentities[identity]["urn:mozilla:zap:automatic_registration"]=="true"))
      registerIdentity(wIdentities[identity]);
  }
}

//----------------------------------------------------------------------

var RegistrationGroup = makeClass("RegistrationGroup", ErrorReporter);

RegistrationGroup.obj("registrations", null);

// true if we are currently unregistering:
RegistrationGroup.obj("unregistering", false);

RegistrationGroup.fun(
  function init(identity) {
    this.identity = identity;
    identity.registrationGroup = this;
    var me = this;
    this.identity.whenInitializationAttempted(function() {
                                                if (!me.unregistering)
                                                  me.init2();
                                              });
    return true;
  });

RegistrationGroup.fun(
  function init2() {
    this.domain = this.identity.service["urn:mozilla:zap:domain"];
    
    if (!this.domain) {
      // the service isn't bound to a particular domain (probably
      // because it is the 'default' service.
      // -> use the identity's host:
      this.domain = this.identity.getHost();
    }
  
    if (this.domain == "anonymous.invalid") {
      // don't attempt to register our 'anonymous' identity
      return false;
    }
      
    this.domain = "sip:"+this.domain;
    
    // parse domain into SIP syntax objects:
    try {
      this.domain = wSipStack.syntaxFactory.deserializeURI(this.domain);
    } catch(e) { warning("Domain parse error: "+this.domain+"\n"); return false; }
    
    this.interval = this.identity.service["urn:mozilla:zap:suggested_registration_interval"];

    // add a registration for each flow:
    this.registrations = [];
    var flow_counter = 0;
    for (var i=1; i<=4; ++i) {
      var route = this.identity.service["urn:mozilla:zap:route"+i];
      if (!route) continue;
      try {
        route = wSipStack.syntaxFactory.deserializeRouteSet(route, {});
      } catch(e) { warning("Invalid route set: "+route+"\n"); continue; }
      var registration = Registration.instantiate();
      registration.init(this,
                        route,
                        ++flow_counter);
      this.registrations.push(registration);
    }
    if (!flow_counter) {
      // The service didn't specify any specific registration routes.
      // -> Try domain:
      try {
        var route = wSipStack.syntaxFactory.deserializeRouteSet("<"+this.domain.serialize()+">", {});
      } catch(e) { this._warning("Invalid route set: <"+this.domain.serialize()+">"); return false; }
      var registration = Registration.instantiate();
      registration.init(this,
                        route,
                        1);
      this.registrations.push(registration);
    }

    return true;
  });

RegistrationGroup.fun(
  function unregister() {
    // make sure that we don't act upon pending notifications:
    this.unregistering = true;
    // mark our identity as un-registered:
    this.identity["urn:mozilla:zap:is_registered"] = "false";
    // walk through existing registrations and unregister:
    if (this.registrations) {
      this.registrations.forEach(function(r) {
                                   r.unregister();
                                 });
    }
  });

RegistrationGroup.fun(
  function getRoutingInfo() {
    if (!this.registrations) return null;
    // find first active flow:
    for (var i=0,l=this.registrations.length; i<l; ++i) {
      if (this.registrations[i].registered) {
        var contactAddr = this.registrations[i].gruu;
        if (!contactAddr) {
          // hmm, no gruu. best we can do is use our registered
          // contact address and hope that our outbound route does
          // some helping with any NATs:
          contactAddr = this.identity.getRegisterContactAddress();
        }
        return {
          contact : contactAddr,
          routeset: this.registrations[i].outboundRoute
        };
      }
    }
    return null;
  });

RegistrationGroup.fun(
  function notifyRegistrationSuccess(registration) {
    if (this.unregistering) return;
    this._dump("Registration to "+this.identity["http://home.netscape.com/NC-rdf#Name"]+" over flow "+registration.flowid+" succeeded");
    if (this.identity["urn:mozilla:zap:is_registered"] != "true") {
      // mark our identity as registered:      
      this.identity["urn:mozilla:zap:is_registered"] = "true";
    }
  });

RegistrationGroup.fun(
  function notifyRegistrationFailure(registration) {
    if (this.unregistering) return;
    this._dump("Registration to "+this.identity["http://home.netscape.com/NC-rdf#Name"]+" over flow "+registration.flowid+" has failed");
    this.recoverFromRegistrationFailure(registration);
  });

RegistrationGroup.fun(
  function notifyFlowFailure(registration) {
    if (this.unregistering) return;
    this._dump("Flow "+registration.flowid+" to "+this.identity["http://home.netscape.com/NC-rdf#Name"]+" has failed");
    this.recoverFromRegistrationFailure(registration);
  });

RegistrationGroup.fun(
  function recoverFromRegistrationFailure(registration) {
    // recover as described in draft-ietf-outbound-01.txt 4.3:
    var some_active = this.registrations.some(function(r) {
                                                return r.registered;
                                              });
    var base_time;
    if (some_active) {
      base_time = wConfig["urn:mozilla:zap:registration_recovery_base_time_not_failed"];
    }
    else {
      // mark our identity as un-registered:
      this.identity["urn:mozilla:zap:is_registered"] = "false";
    
      base_time = wConfig["urn:mozilla:zap:registration_recovery_base_time_all_fail"];
    }
    base_time = parseFloat(base_time);

    var max_time = parseFloat(wConfig["urn:mozilla:zap:registration_recovery_max_time"]);

    var wait_time_ms = Math.min(max_time, base_time*Math.pow(2, registration.failureCount))*1000*(0.5+0.5*Math.random());
    this._dump("reregistering "+this.identity["http://home.netscape.com/NC-rdf#Name"]+", flow "+registration.flowid+" in "+wait_time_ms+"ms");
    registration.scheduleRefresh(wait_time_ms);
  });

//----------------------------------------------------------------------

var Registration = makeClass("Registration", SupportsImpl);
Registration.addInterfaces(Components.interfaces.zapISipNonInviteRCListener,
                           Components.interfaces.zapISipFlowMonitor,
                           Components.interfaces.nsITimerCallback);

// is this flow currently registered?
Registration.obj("registered", false);

// how many consecutive times has this flow failed?
Registration.obj("failureCount", 0);

// the last response received from the registrar
Registration.obj("lastResponse", null);

// if we are currently registered, this will contain the contact
// header as returned by the registrar:
Registration.obj("contact", null);

// this will contain our GRUU if we are currently registered and the
// registrar returned one:
Registration.obj("gruu", null);

// our default outbound route. might be different to the current
// outbound route (see below) if there is a Service-Route in force.
Registration.obj("defaultOutboundRoute", null);

// this will contain the default route or the service route (RFC3608)
// returned by the registrar:
Registration.obj("outboundRoute", null);

Registration.fun(
  function init(group, route, flowid) {
    this._dump("group="+group+", route="+route+", flowid="+flowid);
    this.group = group;
    this.flowid = flowid;
    this.defaultOutboundRoute = route;
    this.outboundRoute = route;
    
    this._initRC();
    this.registrationRC.listener = this;
    this.registrationRC.sendRequest();
  });

Registration.fun(
  function _maybeMonitorFlow(flow) {
    if (this.flow) {
      if (this.flow == flow)
        return; // we're already monitoring this flow
      this._unmonitorFlow();
    }
    // check if we should monitor the current flow
    var monitorType = null;
    if (this.outboundRoute.length &&
        this.outboundRoute[0].uri.QueryInterface(Components.interfaces.zapISipSIPURI).hasURIParameter("sip-stun")) {
      monitorType = Components.interfaces.zapISipFlow.IETF_SIP_OUTBOUND_01_MONITOR;
    }
    else if (this.group.identity.service["urn:mozilla:zap:options_keep_alive"]=="true") {
      monitorType = Components.interfaces.zapISipFlow.OPTIONS_MONITOR;
    }

    if (monitorType != null) {
      this.flow = flow;
      this.flow.addFlowMonitor(this, monitorType);
    }
  });

Registration.fun(
  function _unmonitorFlow() {
    if (this.flow) {
      this.flow.removeFlowMonitor(this);
      delete this.flow;
    }
  });

// helper to initialize a register request client:
Registration.fun(
  function _initRC() {
    var route = this.outboundRoute;
    this.registrationRC =
      wSipStack.createRegisterRequestClient(this.group.domain,
                                            this.group.identity.getAOR(),
                                            this.group.identity.getRegisterContactAddress(),
                                            route,
                                            route.length,
                                            this.group.identity.getRCFlags());

    var contact = this.registrationRC.request.getTopContactHeader().QueryInterface(Components.interfaces.zapISipContactHeader);

    if (this.group.identity["urn:mozilla:zap:preference"]) {
      // add a q-value
      contact.setParameter("q", this.group.identity["urn:mozilla:zap:preference"]);
    }
    
    if (this.group.interval) {
      // add an expires parameter
      contact.setParameter("expires", this.group.interval);
    }
    
    // XXX if (use_outbound)
    contact.setParameter("+sip.instance", '"<'+wSipStack.instanceID+'>"');
    contact.setParameter("flow-id", this.flowid);
  });    

Registration.fun(
  function unregister() {
    if (this.refreshTimer) {
      this.refreshTimer.cancel();
      delete this.refreshTimer;
    }
    this._unmonitorFlow();
    
    if (this.registrationRC.listener) {
      // We are currently waiting for a registration to complete.
      // XXX what we want to do is cancel the RC.
      this.registrationRC.listener = null;
    }
    else if (this.registered) {
      // XXX maybe we shouldn't send unregistrations.
      var contact = this.registrationRC.request.getTopContactHeader();
      contact.setParameter("expires", "0");
      this.registrationRC.sendRequest();
    }
    delete this.registrationRC;
  });

// Helper to compare the first hop in the new route to the remote ip
// address. Returns 'true' if the first hop has changed. 
function firstHopChanged(newRoute, oldRoute, currentFlow) {
  if (!newRoute.length) return (oldRoute.length>0);
  if (!oldRoute.length) return true;
  //XXX this needs some more thought
  // See http://www.croczilla.com/zap/codedocs/notes A.
  // var hop1 = newRoute[0].uri.QueryInterface(Components.interfaces.zapISipSIPURI);
  //   if (hop1.host == currentFlow.remoteAddress &&
  //       (hop1.port == "" || hop1.port == currentFlow.remotePort))
  //     return false; // endpoint addresses are equal
  var uri1 = newRoute[0].uri.QueryInterface(Components.interfaces.zapISipSIPURI);
  var uri2 = oldRoute[0].uri.QueryInterface(Components.interfaces.zapISipSIPURI);
  return !uri1.equals(uri2);
}

// Update the current request route from the service route on the
// given registration response (which can be null - in which case the
// preloaded route will be restored).  Returns 'true' if the first hop
// in the new route is different to the remote end of the flow over
// which the response was received. See http://www.croczilla.com/zap/codedocs/notes A.
Registration.fun(
  function _updateRequestRoute(response, flow) {
    var newRoute = null;
    var serviceRouteHeaders = response.getHeaders("Service-Route", {});
    if (serviceRouteHeaders.length) {
      newRoute = [];
      serviceRouteHeaders.forEach(
        function(h) {
          newRoute.push(h.QueryInterface(Components.interfaces.zapISipServiceRouteHeader).address);
        });
    }
    else
      newRoute = this.defaultOutboundRoute;
          
    var oldRoute = this.outboundRoute;
    if (oldRoute != newRoute) {
      this.outboundRoute = newRoute;
      // Use the service route as a new route for *all* outbound
      // request over this flow, including future REGISTERs
      // (RFC3608, draft-rosenberg-sip-route-construct-00).
      
      // We need to create a new request client with the updated route:
      var oldRequest = this.registrationRC.request;
      this._initRC();
      var newRequest = this.registrationRC.request;
      // copy credentials:
      oldRequest.getHeaders("Authorization", {}).forEach(
        function(h) {newRequest.appendHeader(h);});
      oldRequest.getHeaders("Proxy-Authorization", {}).forEach(
        function(h) {newRequest.appendHeader(h);});
    }
    
    return firstHopChanged(newRoute, oldRoute, flow);
  });

// helper to extract a gruu from a contact header
function extractGRUU(contact)
{
  if (!contact.hasParameter("gruu")) return null;
  try {
    var gruu = /"(.*)"/(contact.getParameter("gruu"))[1];
    gruu = wSipStack.syntaxFactory.deserializeAddress(gruu);
    return gruu;
  }
  catch (e) { warning("gruu parse error: "+e); }
  return null;
}

// zapISipNonInviteRCListener methods:
Registration.fun(
  function notifyResponseReceived(rc, dialog, response, flow) {
    this.lastResponse = response;
          
    if (response.statusCode[0] == "1")
      return; // just a provisional response
    rc.listener = null;
    if ((response.statusCode == "401" ||
         response.statusCode == "407") &&
        wSipStack.authentication.addAuthorizationHeaders(this.group.identity,
                                                         response,
                                                         rc.request)) {
      // retry with credentials:
      rc.listener = this;
      rc.sendRequest();
      return;
    }
    else if (response.statusCode == "423") {
      // interval too brief. see RFC3261 10.2.8
      // we need to modify our request and retry
      var contact = rc.request.getTopContactHeader().QueryInterface(Components.interfaces.zapISipContactHeader);
      // look for a min-expires header:
      var minExpires = response.getTopHeader("Min-Expires");
      if (minExpires) {
        minExpires = minExpires.QueryInterface(Components.interfaces.zapISipMinExpiresHeader);
        // remember minimum interval for future registrations:
        this.group.interval = minExpires.deltaSeconds.toString();
        contact.setParameter("expires", this.group.interval);
      }
      else {
        this._dump("warning: 424 response without Min-Expires header");
        delete this.group.interval;
        contact.removeParameter("expires");
      }
      // retry the ammended request:
      rc.listener = this;
      rc.sendRequest();
      return;
    }
    else if (response.statusCode[0] == "2") {
      // find our registration among the returned contact headers:
      var contacts = response.getHeaders("Contact", {});
      var contact;
      var myURI = rc.request.getTopContactHeader().QueryInterface(Components.interfaces.zapISipContactHeader).address.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
      for (var i=0,l=contacts.length; i<l; ++i) {
        var c = contacts[i].QueryInterface(Components.interfaces.zapISipContactHeader);
        try {
          var uri = c.address.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
          if (uri.equals(myURI)) {
            // we've found a match
            contact = c;
            break;
          }
          else {
            this._dump(myURI.serialize()+" != "+uri.serialize());
          }
        }
        catch(e) {
          // there was an error parsing the uri in the contact header. ignore.
          this._dump("exception during registration response parsing: "+e);
        }
      }
      if (contact) {
        // looks like we're registered ok. let's see when the
        // registration expires (RFC3261 10.2.4):
        var expires = contact.getParameter("expires");
        if (!expires) {
          var expiresHeader = response.getTopHeader("Expires");
          if (expiresHeader) {
            expires = expiresHeader.QueryInterface(Components.interfaces.zapISipExpiresHeader).deltaSeconds;
          }
        }
        else {
          // the 'expires' uri parameter is a string (in contrast to a
          // deltaSeconds value which is already an integer). parse it:
          expires = parseInt(expires);
        }
        
        if (expires != null && !isNaN(expires) && expires != 0) {
          // we've got a valid registration!
          // update our state:
          this.contact = contact;
          this.gruu = extractGRUU(contact);

          var hopChanged = this._updateRequestRoute(response, flow);

          if (hopChanged) {
            // The new service route has a first hop that is different
            // to the one we were using.
            // Send the request again using the updated route:
            this.registrationRC.listener = this;
            this.registrationRC.sendRequest();
            return;
          }
          
          if (!this.registered) {
            this.failureCount = 0;
            this.registered = true;
            // inform our group:            
            this.group.notifyRegistrationSuccess(this);
          }
          // set up a refresh timer:
          this.scheduleRefresh(expires*1000*0.9);
          this._maybeMonitorFlow(flow);
          return;
        }
      }
    }
    // failure:
    this.registered = false;
    this.contact = null;
    this.gruu = null;
    ++this.failureCount;
    this._unmonitorFlow();
    // let our registration group handle recovery:
    this.group.notifyRegistrationFailure(this);
  });

Registration.fun(
  function scheduleRefresh(delay_ms) {
    if (this.refreshTimer) {
      this.refreshTimer.cancel();
    }
    else {
      this.refreshTimer =
        Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer);
    }
    this.refreshTimer.initWithCallback(this, delay_ms,
                                       Components.interfaces.nsITimer.TYPE_ONE_SHOT);
  });

// nsITimerCallback methods:
Registration.fun(
  function notify(timer) {
    this.registrationRC.listener = this;
    try {
      this.registrationRC.sendRequest();
    } catch(e) {
      // the rc might still be busy with the last request.
      this._warning("racing registrations");
    }
  });

// zapISipFlowMonitor methods:

// void flowChanged(in zapISipFlow flow, in unsigned short changeFlags);
Registration.fun(
  function flowChanged(flow, changeFlags) {
    // stop monitoring the flow; recover as described in
    // draft-ietf-sip-outbound-01.txt
    this._unmonitorFlow();
    this.registered = false;
    // we leave recovery up to our group:
    ++this.failureCount;
    this.group.notifyFlowFailure(this);
  });

////////////////////////////////////////////////////////////////////////
// Contacts:

function initContacts() {
  wContactsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wContactsContainer.Init(wContactsDS,
                          wRDF.GetResource("urn:mozilla:zap:contacts"));
  wFriendsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wFriendsContainer.Init(wContactsDS,
                         wRDF.GetResource("urn:mozilla:zap:friends"));
}

var Contact = makeClass("Contact", PersistentRDFObject);

Contact.prototype.datasources["default"] = wContactsDS;

Contact.rdfResourceAttrib("urn:mozilla:zap:sidebarparent",
                          "urn:mozilla:zap:friends");
Contact.rdfLiteralAttrib("urn:mozilla:zap:chromepage",
                         "chrome://zap/content/contacts.xul");
Contact.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:nodetype", "contact");
Contact.rdfLiteralAttrib("urn:mozilla:zap:sip1", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:sip2", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:organization", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:email", "");
Contact.rdfLiteralAttrib("urn:mozilla:zap:homepage", "");
// if isfriend is true, the contact will be displayed in the sidebar:
Contact.rdfLiteralAttrib("urn:mozilla:zap:isfriend", "true");

Contact.spec(
  function createFromDocument(doc) {
    // Temporarily defer flushing until later, so that the container
    // changes get flushed as well:
    this.autoflush = false;
    this._Contact_createFromDocument(doc);
    this.autoflush = true;
    
    // Append to contacts:
    wContactsContainer.AppendElement(this.resource);
    // ... and if it's a friend, to the friends list as well:
    if (this["urn:mozilla:zap:isfriend"] == "true")
      wFriendsContainer.AppendElement(this.resource);

    this.flush();
  });

Contact.spec(
  function updateFromDocument(doc) {
    // Temporarily defer flushing until later, so that the container
    // changes get flushed as well:
    this.autoflush = false;
    this._Contact_updateFromDocument(doc);
    this.autoflush = true;
    // add/remove from friends container:
    var index = wFriendsContainer.IndexOf(this.resource);
    if (this["urn:mozilla:zap:isfriend"] == "true") {
      if (index == -1)
        wFriendsContainer.AppendElement(this.resource);
    }
    else {
      if (index != -1)
        wFriendsContainer.RemoveElementAt(index, true);
    }
    this.flush();
  });

////////////////////////////////////////////////////////////////////////
// Sidebar:

/*
  Sidebar RDF datasource:
                           zap:
                     sidebarparent _n
                            __     __
                           |  |   |  |
                           V  |   |  V
 +------------+         +---------------+    zap:     +----------+
 |  resource: |   _n    |   resource:   | chromepage  | literal: |
 | zap:sidebar|-------->|  sidebarnode  |------------>|   url    |
 +------------+         +---------------+             +----------+
                          |           |
                          | NC:Name   | zap:nodetype
                          V           V
                   +----------+  +----------+
                   | literal: |  | literal: |
                   |   name   |  |  type    |
                   +----------+  +----------+
*/

// initialize wSidebarDS and wSidebarTree:
function initSidebar() {
  // Construct an ephemeral sidebar container. Its content will be
  // merged with the static sibar content from sidebar.rdf.
  wEphemeralSidebarContainer = wRDFContainerUtils.MakeBag(wGlobalEphemeralDS,
                                                          wRDF.GetResource("urn:mozilla:zap:sidebar"));

  wSidebarTree = document.getElementById("sidebar-tree");
  wSidebarTree.database.AddDataSource(wGlobalEphemeralDS);
  wSidebarTree.database.AddDataSource(wCallsDS);
  wSidebarTree.database.AddDataSource(wSidebarDS);
  wSidebarTree.database.AddDataSource(wIdentitiesDS);
  wSidebarTree.database.AddDataSource(wContactsDS);

  wSidebarTree.builder.rebuild();
  selectSidebarNode("urn:mozilla:zap:home");
}

function getSelectedSidebarResource()
{
  return getSelectedResource(wSidebarTree);
}

function getSidebarResourceAttrib(source, attrib) {
  var property = wRDF.GetResource(attrib);
  var target = wSidebarTree.database.GetTarget(source, property, true);
  if (target)
    target = target.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
  return target;
}

function getSidebarResourceParent(source) {
  var property = wRDF.GetResource("urn:mozilla:zap:sidebarparent");
  var target = wSidebarTree.database.GetTarget(source, property, true);
  if (target)
    target = target.QueryInterface(Components.interfaces.nsIRDFResource);
  return target;
}

function clearSidebarSelection()
{
  var selection = wSidebarTree.view.selection;
  if (selection)
    selection.clearSelection();
}

function ensureSidebarNode(uri) {
  var resource = wRDF.GetResource(uri);
  var index = wSidebarTree.builder.getIndexOfResource(resource);
  if (index == -1) {
    // The node isn't in the tree. Try recursively expanding ancestors:
    var parent = getSidebarResourceParent(resource);
    if (parent) {
      var parentIndex = ensureSidebarNode(parent.Value);
      wSidebarTree.builder.toggleOpenState(parentIndex);
      index = wSidebarTree.builder.getIndexOfResource(resource);
    }
  }
  return index;
}

function selectSidebarNode(uri)
{
  var index = ensureSidebarNode(uri);
  if (index < 0) return;
  wSidebarTree.view.selection.select(index);
  wSidebarTree.treeBoxObject.ensureRowIsVisible(index);
}

// called by the sidebar whenever its selection changes:
function sidebarSelectionChange()
{
  var page;
  var selection = getSelectedSidebarResource();
  if (!selection) return;

  page = getSidebarResourceAttrib(selection, "urn:mozilla:zap:chromepage");
  if (!page) page = "about:blank";

  if (page != wInteractPane.getAttribute("src"))
    wInteractPane.setAttribute("src", page);
  else
    wInteractPane.webNavigation.reload(0);
}

////////////////////////////////////////////////////////////////////////
// MediaPipeline

var wMediaPipeline = {
  init : function() {
    try {
      // We use getService() instead of createInstance() to get a globally
      // well-known media graph instance:
      this.mediagraph = Components.classes["@mozilla.org/zap/mediagraph;1"].getService(Components.interfaces.zapIMediaGraph);

      // 8000Hz, 20ms, 1 channel audio pipe:      
      this.audioin = this.mediagraph.addNode("audioin", null);
      this.asplitter = this.mediagraph.addNode("splitter", null);
      this.audioout = this.mediagraph.addNode("audioout", null);
      this.amixer = this.mediagraph.addNode("audio-mixer", null);
      this.A = this.mediagraph.connect(this.audioin, null,
                                       this.asplitter, null);
      this.B = this.mediagraph.connect(this.amixer, null,
                                       this.audioout, null);
      this.mediagraph.setAlias("ain", this.asplitter);
      this.mediagraph.setAlias("aout", this.amixer);
    }
    catch(e) {
      dump("Error during media pipeline initialization: "+e+"\n");
    }
  },
  
  cleanup: function() {
    try {
      this.mediagraph.shutdown();
    }
    catch(e) {}
    delete this.mediagraph;
  }
  
}

////////////////////////////////////////////////////////////////////////
// SipStack


function initSipStack() {
  wSdpService = Components.classes["@mozilla.org/zap/sdpservice;1"].
    getService(Components.interfaces.zapISdpService);
  
  wSipStack = Components.classes["@mozilla.org/zap/sipstack;1"].createInstance(Components.interfaces.zapISipUAStack);

  // make sure we generate a UA instance id now if we haven't got one
  // already: (see draft-ietf-sip-gruu-05.txt)
  if (!wConfig["urn:mozilla:zap:instance_id"]) {
    var uuidgen = Components.classes["@mozilla.org/zap/uuid-generator;1"].getService(Components.interfaces.zapIUUIDGenerator);
    wConfig["urn:mozilla:zap:instance_id"] = uuidgen.generateUUIDURNString();
  }
  
  wSipStack.init(wUAHandler,
                 makePropertyBag({$instance_id:wConfig["urn:mozilla:zap:instance_id"],
                                  $port_base:wConfig["urn:mozilla:zap:sip_port_base"],
                                  $methods: "OPTIONS,INVITE,ACK,CANCEL,BYE",
                                  $extensions: "path,gruu", // path: RFC3327, gruu: draft-ietf-sip-gruu-05.txt
                                  $user_agent: wUserAgent
                                 }));
  document.getElementById("status_text").label = "Listening for UDP/TCP SIP traffic on port "+wSipStack.listeningPort;

  // set up logging to profile_dir/zap.sbox:
  wSipTrafficLogger.logOutputStream = openFileForWriting(getProfileFile("zap.sbox"),
                                                         true);
  wSipStack.transport.transceiver.trafficMonitor = wSipTrafficLogger;
}

function cleanupSipStack() {
  wSipTrafficLogger.logOutputStream.close();
  wSipTrafficLogger.logOutputStream = null;
}

// traffic logger for logging the raw sip traffic to profile_dir/zap.sbox
var wSipTrafficLogger = {
  logOutputStream : null,
  notifyPacket: function(data, protocol, localAddress, localPort,
                         remoteAddress, remotePort, outbound) {
    if (!this.logOutputStream) return;
    var entry = "- From "+localAddress+":"+localPort;
    if (outbound)
      entry += "-->";
    else
      entry += "<--";
    entry += remoteAddress+":" + remotePort + " " + protocol + " ";
    entry += data.length + "bytes " + (new Date()).toUTCString() + "\n";
    if (wNetUtils.snoopStunPacket(data) != -2) {
      entry += octetToHex(data, " ", 16);
      // XXX log or not?
      return;
    }
    else
      entry += data;
    entry += "\n\n";

    this.logOutputStream.write(entry, entry.length);
  }
};


// our UA request handler:
var wUAHandler = {
  handleNonInviteRequest: function(rs) {
    return false;
  },

  handleInviteRequest: function(rs) {
    var ic = InboundCall.instantiate();
    ic.createNew(true);
    ic.receiveCall(rs);
    return true;
  }
};

////////////////////////////////////////////////////////////////////////
// Calls

// initialize wCallsDS, wCallsContainer:
function initCalls() {
  wCallsDS = wRDF.GetDataSourceBlocking(getProfileFileURL("calls.rdf"));

  wCallsContainer = Components.classes["@mozilla.org/rdf/container;1"].
    createInstance(Components.interfaces.nsIRDFContainer);
  wCallsContainer.Init(wCallsDS,
                              wRDF.GetResource("urn:mozilla:zap:calls"));
}

// hash of active calls, indexed by resource name:
var wActiveCalls = {};

// get the Call object for the given resource. This will either be a
// new Call instance or the cached instance in wActiveCalls if the
// call is currently active:
function getCall(resource) {
  var call = wActiveCalls[resource.Value];
  if (!call) {
    call = Call.instantiate();
    call.initWithResource(resource);
  }
  return call;
}

//----------------------------------------------------------------------
// Call class

var Call = makeClass("Call", PersistentRDFObject);

Call.prototype.datasources["default"] = wCallsDS;
Call.prototype.datasources["global-ephemeral"] = wGlobalEphemeralDS;
Call.addInMemoryDS("ephemeral");

// persistent attributes:
Call.rdfLiteralAttrib("urn:mozilla:zap:status", ""); // last received/sent SIP status code
Call.rdfLiteralAttrib("urn:mozilla:zap:remote", ""); // remote SIP address
Call.rdfLiteralAttrib("urn:mozilla:zap:local", ""); // local SIP address
Call.rdfLiteralAttrib("urn:mozilla:zap:subject", ""); // SIP Subject header value
Call.rdfLiteralAttrib("urn:mozilla:zap:callid", ""); // SIP Call-ID header value
Call.rdfLiteralAttrib("urn:mozilla:zap:timestamp", ""); // yyyy-mm-dd hh:mm:ss (XXX timezone ???)
Call.rdfLiteralAttrib("urn:mozilla:zap:duration", "-"); // duration in seconds

// ephemeral attributes:
Call.rdfLiteralAttrib("urn:mozilla:zap:active", "false", "global-ephemeral");
Call.rdfLiteralAttrib("urn:mozilla:zap:session-running", "false", "global-ephemeral");
Call.rdfLiteralAttrib("http://home.netscape.com/NC-rdf#Name",
                      "", "global-ephemeral"); // name to be shown for active calls in the sidebar; initialized from remote address trigger
Call.rdfLiteralAttrib("urn:mozilla:zap:sidebarindex", "0050",
                      "global-ephemeral"); // ensure active calls are at the top of the sidebar
Call.rdfLiteralAttrib("urn:mozilla:zap:chromepage",
                      "chrome://zap/content/call.xul", "global-ephemeral");

// This is to provide an entrypoint for template recursion. see
// e.g. calls.xul for usage and comments in
// RDFUtils.js::rdfPointerAttrib:
Call.rdfPointerAttrib("urn:mozilla:zap:root",
                      "urn:mozilla:zap:current-call",
                      "ephemeral");

// triggers:
Call.rdfAttribTrigger(
  "urn:mozilla:zap:remote",
  function(prop, val) {
    this._dump("TRIGGER remote ->"+val);
    this["http://home.netscape.com/NC-rdf#Name"] = val;
  });

Call.rdfAttribTrigger(
  "urn:mozilla:zap:session-running",
  function(prop, val) {
    this._dump("TRIGGER session-running -> "+val);
    var me = this;
    if (val == "true") {
      this.sessionStart = new Date();
      this.sessionTimer = schedule(
        function(reschedule) {
          var duration_ms = (new Date()) - me.sessionStart;
          me["urn:mozilla:zap:duration"] = Math.round(duration_ms/1000);
          reschedule();
        }, 1000);
    }
    else if (this.sessionTimer) {
      this.sessionTimer.cancel();
      delete this.sessionTimer;
      delete this.sessionStart;
    }
  });

Call.rdfAttribTrigger(
  "urn:mozilla:zap:active",
  function(prop, val) {
    this._dump("TRIGGER active -> "+val);
    if (val == "true") {
      wEphemeralSidebarContainer.AppendElement(this.resource);
    }
    else {
      wEphemeralSidebarContainer.RemoveElement(this.resource, false);
    }
  });

// set timestamp to current date/time:
Call.fun(
  function setTimestamp() {
    var now = new Date();
    this["urn:mozilla:zap:timestamp"] =
      now.getFullYear() + "-" +
      padleft((now.getMonth()+1).toString(), "0", 2) + "-" +
      padleft(now.getDate().toString(), "0", 2) + " " +
      padleft(now.getHours().toString(), "0", 2) + ":" +
      padleft(now.getMinutes().toString(), "0", 2) + ":" +
      padleft(now.getSeconds().toString(), "0", 2);
  });

//----------------------------------------------------------------------
// Active call

var ActiveCall = makeClass("ActiveCall", Call, SupportsImpl);
ActiveCall.addInterfaces(Components.interfaces.zapISipDialogListener);

// Mediasession and dialog will only be valid for our subclasses
// OutboundCall and InboundCall while a call is in progress:
ActiveCall.obj("mediasession", null);
ActiveCall.obj("dialog", null);

ActiveCall.spec(
  function createFromDocument(doc) {
    // Temporarily defer flushing until later, so that the container
    // changes get flushed as well:
    this.autoflush = false;
    this._ActiveCall_createFromDocument(doc);
    this.autoflush = true;
    
    // append to calls container:
    wCallsContainer.AppendElement(this.resource);    
    
    // mark as active:
    this["urn:mozilla:zap:active"] = "true";
    
    // enter into active calls hash:
    wActiveCalls[this.resource.Value] = this;

    this.flush();
  });

ActiveCall.spec(
  function createNew(addAssertions) {
    // Temporarily defer flushing until later, so that the container
    // changes get flushed as well:
    this.autoflush = false;
    this._ActiveCall_createNew(addAssertions);
    this.autoflush = true;
    
    // append to calls container:
    wCallsContainer.AppendElement(this.resource);

    // mark as active:
    this["urn:mozilla:zap:active"] = "true";
    
    // enter into active calls hash:
    wActiveCalls[this.resource.Value] = this;

    this.flush();
  });

// clean up after call has terminated
ActiveCall.fun(
  function terminated() { 
    this["urn:mozilla:zap:active"] = "false";
    this["urn:mozilla:zap:session-running"] = "false";
    delete wActiveCalls[this.resource.Value];
    if (this.mediasession) {
      this.mediasession.shutdown();
      this.mediasession = null;
    }
    this.dialog = null;
  });

// zapISipDialogListener methods:
ActiveCall.fun(
  function notifyDialogTerminated(dialog) {
    this["urn:mozilla:zap:status"] = "Terminated";
    this.terminated();
  });

//----------------------------------------------------------------------
// OutboundCall class

var OutboundCall = makeClass("OutboundCall", ActiveCall);

OutboundCall.rdfLiteralAttrib("urn:mozilla:zap:direction", "outbound");

OutboundCall.fun(
  function makeCall(toAddress, identity) {
    this.identity = identity;
    this.routingInfo = identity.getRoutingInfo();
    this.callHandler = OutboundCallHandler.instantiate();
    this.callHandler.call = this;

    var rc = wSipStack.createInviteRequestClient(toAddress,
                                                 identity.getFromAddress(),
                                                 this.routingInfo.contact,
                                                 this.routingInfo.routeset,
                                                 this.routingInfo.routeset.length,
                                                 identity.getRCFlags());
    rc.listener = this.callHandler;
    this.callHandler.rc = rc;

    var subject = this["urn:mozilla:zap:subject"];
    if (subject) {
      try {
        var s = wSipStack.syntaxFactory.createHeader("Subject").QueryInterface(Components.interfaces.zapISipSubjectHeader);
        s.subject = subject;
        rc.request.appendHeader(s);
      }
      catch(e) {
        this._warning("Exception during Subject header construction: "+e);
      }
    }
    this["urn:mozilla:zap:local"] = identity.getFromAddress().serialize();
    this["urn:mozilla:zap:callid"] = rc.request.getCallIDHeader().callID;
    this.setTimestamp();
    var offer = this.mediasession.generateSDPOffer();

    rc.request.setContent("application", "sdp", offer.serialize());
    rc.sendInvite(this.callHandler);
  });

//----------------------------------------------------------------------
// OutboundCallHandler
// CALLING -> 2XX_RECEIVED -> TERMINATED

var OutboundCallHandler = makeClass("OutboundCallHandler",
                                    SupportsImpl, StateMachine);
OutboundCallHandler.addInterfaces(Components.interfaces.zapISipInviteResponseHandler,
                                  Components.interfaces.zapISipInviteRCListener);

// zapISipInviteResponseHandler methods:
OutboundCallHandler.fun(
  function handle2XXResponse(rc, dialog, response, ackTemplate) {
    // we have a positive answer for this request, so we won't
    // resend. make sure that notifyTerminated really terminates from
    // now on:
    this.changeState("2XX_RECEIVED"); 
    
    if (this.call.dialog) {
      this._dump("received additional dialog for outbound call");
      // we already have a dialog. this must be a subsequent dialog
      // from a forking proxy. send a bye immediately:
      // XXX probably not a good idea to do this before sending the ack
      dialog.createNonInviteRequestClient("BYE").sendRequest();
      return ackTemplate;
    }

    // try to parse answer:
    var sd;
    try {
      sd = wSdpService.deserializeSessionDescription(response.body);
    }
    catch(e) {
      this.call["urn:mozilla:zap:status"] = "Can't parse other party's session description";
      // send a BYE:
      // XXX need a way to send ACK first
      dialog.createNonInviteRequestClient("BYE").sendRequest();
      return ackTemplate;
    }

    // we've got a parsed answer; process it:
    try {
      this.call.mediasession.processSDPAnswer(sd);
    }
    catch(e) {
      // XXX add verbose error
      this.call["urn:mozilla:zap:status"] = "Session negotiation failed";
      
      // send a BYE:
      // XXX need a way to send ACK first
      dialog.createNonInviteRequestClient("BYE").sendRequest();

      return ackTemplate;
    }

    // finally. we've cleared all the hurdles:
    this.call.mediasession.startSession();
    this.call.dialog = dialog;
    this.call.dialog.listener = this.call;
    this.call["urn:mozilla:zap:session-running"] = "true";
    this.call["urn:mozilla:zap:status"] = "Connected";
    return ackTemplate;
  });

// zapISipInviteRCListener methods:
OutboundCallHandler.fun(
  function notifyResponseReceived(rc, dialog, response, flow) {
    this._dump(response.statusCode+" response for call="+this.call);
    if (response.statusCode[0] == "1")
      this.call["urn:mozilla:zap:status"] =
        response.statusCode + " " + response.reasonPhrase;
    else if (response.statusCode[0] == "2") {
      // we handle 2XX codes in handle2XXResponse
    }
    else if (response.statusCode == "401" ||
             response.statusCode == "407") {
      if (wSipStack.authentication.
          addAuthorizationHeaders(wCurrentIdentity,
                                  response,
                                  rc.request)) {
        // we've got new credentials -> retry
        rc.sendInvite(this);
      }
    }
//     else if (response.statusCode == "301" ||
//              ...) {
//       // retry with new destination
//     }
    else {
      this.call["urn:mozilla:zap:status"] =
        response.statusCode + " " + response.reasonPhrase;
      this.terminate();
    }
  });

OutboundCallHandler.statefun(
  "2XX_RECEIVED",
  function notifyTerminated(rc) {
    this._dump("terminated in 2XX_RECEIVED state. call="+this.call);
    this.terminate();
  });

OutboundCallHandler.statefun(
  "*",
  function notifyTerminated(rc) {
    this._dump("ignoring responsehandler termination.");
  });

OutboundCallHandler.fun(
  function terminate() {
    this.changeState("TERMINATED");
    if (!this.call.dialog) {
      this.call.terminated();
    }
    delete this.call.callHandler;
    delete this.call;
    delete this.rc;
  });

//----------------------------------------------------------------------
// InboundCall

var InboundCall = makeClass("InboundCall", ActiveCall);

InboundCall.rdfLiteralAttrib("urn:mozilla:zap:direction", "inbound");

InboundCall.fun(
  function receiveCall(rs) {
    this.setTimestamp();
    this["urn:mozilla:zap:remote"] = rs.request.getFromHeader().address.serialize();
    this["urn:mozilla:zap:local"] = rs.request.getToHeader().address.serialize();
    this["urn:mozilla:zap:callid"] = rs.request.getCallIDHeader().callID;
    var subjectHeader = rs.request.getTopHeader("Subject");
    if (subjectHeader) {
      this["urn:mozilla:zap:subject"] = subjectHeader.QueryInterface(Components.interfaces.zapISipSubjectHeader).subject;
    }
    this.callHandler = InboundCallHandler.instantiate();
    this.callHandler.handleCall(this, rs);
  });

//----------------------------------------------------------------------
// InboundCallHandler

var InboundCallHandler = makeClass("InboundCallHandler", SupportsImpl);
InboundCallHandler.addInterfaces(Components.interfaces.zapISipInviteRSListener);

InboundCallHandler.fun(
  function handleCall(call, rs) {
    this.call = call;
    this.rs = rs;
    rs.listener = this;

    // Work out identity based on To-address
    this.call.identity = null;
    try {
      var uri = rs.request.getToHeader().address.uri.QueryInterface(Components.interfaces.zapISipSIPURI);
      this.call.identity = getIdentityByAOR(uri);
    }
    catch(e) { /* To address probably wasn't a SIP URI -> fall through */ }

    if (!this.call.identity)
      this.call.identity = wCurrentIdentity; // XXX maybe need a separate config for this?
    
    this.contact = this.call.identity.getRoutingInfo().contact;
    
    // reject based on busy settings:
    if (document.getElementById("button_dnd").checked) {
      this._respond(wConfig["urn:mozilla:zap:dnd_code"],
                    wConfig["urn:mozilla:zap:dnd_headers"]);
      return;
    }

    // check if there is a media offer in the request:
    // XXXif (){}
    
    // parse offer; check if it's acceptable:
    try {
      var offer = wSdpService.deserializeSessionDescription(rs.request.body);
      this.call.mediasession = Components.classes["@mozilla.org/zap/mediasession;1"]
        .createInstance(Components.interfaces.zapIMediaSession);
      this.call.mediasession.init("zap",
                                  this.call.identity.hostAddress,
                                  this.call.identity.hostAddress);
      this.answer = this.call.mediasession.processSDPOffer(offer);
    }
    catch(e) {
      // Session negotiation failed.
      // Send 488 (Not acceptable) (RFC3261 13.3.1.3)
      // XXX set a Warning header
      this._respond("488");
      return;
    }

    // the offer is acceptable; we have an answer at hand. alert the user:
    this._respond("180");
    this.alertUser();
  });

InboundCallHandler.fun(
  function alertUser() {
    // construct a new alert box:
    this.alertBox = this.createAltertBox();
    showAlert(this.alertBox);

    // start the ringer:
    this.ringer = wMediaPipeline.mediagraph.addNode("rtttl-player",
                                                    makePropertyBag(
                                                      {$rtttl:wConfig["urn:mozilla:zap:ringtone"]}));
    wMediaPipeline.mediagraph.connect(this.ringer, null,
                                      "aout", null);
  });

// helper to create a label element:
function createLabel(value) {
  var l = document.createElement("label");
  l.setAttribute("value", value);
  return l;
}

InboundCallHandler.fun(
  function createAltertBox() {
    var me = this;
    var rv = document.createElement("groupbox");
    rv.setAttribute("class", "alertbox");
    rv.setAttribute("onclick", "this.handleClick();");
    rv.handleClick = function() { loadPage("chrome://zap/content/call.xul?resource="+escape(me.call.resource.Value)); };
    
    var caption = document.createElement("caption");
    caption.setAttribute("label", "Incoming call for "+this.call["urn:mozilla:zap:local"]);
    rv.appendChild(caption);

    var hbox = document.createElement("hbox");
    
    // 2 column grid:
    var grid = document.createElement("grid");
    grid.setAttribute("flex", "1");
    var columns = document.createElement("columns");
    var column = document.createElement("column");
    columns.appendChild(column);
    column = document.createElement("column");
    column.setAttribute("flex", "1");
    columns.appendChild(column);
    grid.appendChild(columns);

    // grid rows:
    var rows = document.createElement("rows");
    var row = document.createElement("row");
    row.appendChild(createLabel("Caller:"));
    row.appendChild(createLabel(this.call["urn:mozilla:zap:remote"]));
    rows.appendChild(row);
    if (this.call["urn:mozilla:zap:subject"]) {
      row = document.createElement("row");
      row.appendChild(createLabel("Subject:"));
      row.appendChild(createLabel(this.call["urn:mozilla:zap:subject"]));
      rows.appendChild(row);
    }
    grid.appendChild(rows);

    hbox.appendChild(grid);

    // button box:
    var buttons = document.createElement("hbox");
    var button = document.createElement("button");
    button.setAttribute("label", "Accept");
    button.setAttribute("oncommand", "this.accept();");
    button.accept = function() { me.acceptCall(); };
    buttons.appendChild(button);
    button = document.createElement("button");
    button.setAttribute("label", "Reject");
    button.setAttribute("onclick", "event.cancelBubble = true;");
    button.setAttribute("oncommand", "this.reject();");
    button.reject = function() { me.rejectCall(); };
    buttons.appendChild(button);

    hbox.appendChild(buttons);

    rv.appendChild(hbox);
    
    return rv;    
  });

InboundCallHandler.fun(
  function unalertUser() {
    if (this.alertBox) {
      hideAlert(this.alertBox);
      this.alertBox = null;
    }
    if (this.ringer) {
      wMediaPipeline.mediagraph.removeNode(this.ringer);
      delete this.ringer;
    }
  });

InboundCallHandler.fun(
  function acceptCall() {
    // formulate accepting response:
    var resp = this.rs.formulateResponse("200", this.contact);
    // append session description:
    resp.setContent("application", "sdp", this.answer.serialize());
    this.call["urn:mozilla:zap:status"] = resp.statusCode+" "+resp.reasonPhrase;
    this.unalertUser();
    this.call.mediasession.startSession();
    this.call.dialog = this.rs.sendResponse(resp);
    this.call.dialog.listener = this.call;    
    this.call["urn:mozilla:zap:session-running"] = "true";
  });

InboundCallHandler.fun(
  function rejectCall() {
    this._respond("603");
  });

InboundCallHandler.fun(
  function _respond(code, extra_headers) {
    var resp = this.rs.formulateResponse(code, this.contact);

    if (extra_headers) {
      // add in extra header, converting line endings appropriately:
      var responseTxt = resp.serialize();
      responseTxt = responseTxt.substring(0, responseTxt.length-2); // strip last CRLF
      responseTxt += extra_headers;
      responseTxt = responseTxt.replace(/\r/g, "");
      responseTxt = responseTxt.replace(/\n/g, "\r\n");
      responseTxt += "\r\n\r\n";
      resp = wSipStack.syntaxFactory.deserializeMessage(responseTxt).QueryInterface(Components.interfaces.zapISipResponse);
    }
    
    this.call["urn:mozilla:zap:status"] = resp.statusCode+" "+resp.reasonPhrase;
    this.unalertUser();
    return this.rs.sendResponse(resp);
  });
  
// zapISipInviteRSListener methods:
InboundCallHandler.fun(
  function notifyCancelled(rs) {
    this.unalertUser();
    this.call["urn:mozilla:zap:status"] = "Missed";
  });

InboundCallHandler.fun(
  function notifyACKReceived(rs, ack) {
    this.unalertUser();
    this.call["urn:mozilla:zap:status"] = "Confirmed";
  });

InboundCallHandler.fun(
  function notifyTerminated(rs) {
    if (!this.call.dialog) {
      this.call.terminated();
    }
    this.call.callHandler = null;
    this.call = null;
    this.rs = null;
  });

//----------------------------------------------------------------------
// Alerting

function showAlert(node) {
  wAlertBar.appendChild(node);
  showVSlideBar("alertbar", true);
}

function hideAlert(node) {
  wAlertBar.removeChild(node);
  if (!wAlertBar.hasChildNodes())
    hideVSlideBar("alertbar", true);
}
