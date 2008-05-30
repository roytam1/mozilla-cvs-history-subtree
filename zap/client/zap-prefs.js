#filter substitution
pref("toolkit.defaultChromeURI", "chrome://zap/content/zap.xul");
pref("toolkit.singletonWindowType", "zap_mainwin");
pref("general.useragent.extra.zap", "zap/@ZAP_APP_VERSION@");
pref("signon.SignonFileName", "credentials.txt");
pref("signon.SignonFileName2", "credentials2.txt");
pref("signon.rememberSignons", true);
pref("signon.debug", false);
pref("general.useragent.locale", "en-US");
pref("browser.preferences.instantApply", false);
pref("browser.preferences.animateFadeIn", false);
// make sure http, etc go through the external protocol handler:
pref("network.protocol-handler.expose-all", false);
// suppress external-load warning for standard browser schemes
pref("network.protocol-handler.warn-external.http", false);
pref("network.protocol-handler.warn-external.https", false);
pref("network.protocol-handler.warn-external.ftp", false);
pref("network.protocol-handler.warn-external.mailto", false);
