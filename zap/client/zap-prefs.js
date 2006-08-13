pref("toolkit.defaultChromeURI", "chrome://zap/content/zap.xul");
pref("toolkit.singletonWindowType", "zap_mainwin");
pref("general.useragent.extra.zap", "zap/0.2.3");
pref("signon.SignonFileName", "credentials.txt");
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
