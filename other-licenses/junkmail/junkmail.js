// the junkmail plugin object itself
//
const NS_JUNKMAIL_CONTRACTID = 
    '@mozilla.org/messenger/filter-plugin;1?name=junkmail';
const NS_JUNKMAIL_CID =
    Components.ID('{c9a94b7c-a67d-11d6-8039-00008646b737}');

var headerTestsByName = {};
var gPrefs;
var gTargetFolder;
var gLogStream;

// types of tests
//
const TEST_HEADER_REGEXP_MATCH = 1;
const TEST_HEADER_REGEXP_NOMATCH = 2;
const TEST_HEADER_EXISTS = 3;
const TEST_HEADER_EVAL = 4;

// Table-driven parser for each type of spam-assassin config file
// line.  Each function expects a string which starts at the first
// non-whitespace character after the keyword token.
//
const parseConfigLine = {

    describe: function(line) {
        // if this is a describe line and a corresponding test object exists, 
        // attach a describe property to the test 
        //
        const describeRE = /^(\S+)\s+(.*)$/;
        var matches = describeRE.exec(line);
        if (matches) { 
            if ( matches[1] in headerTestsByName) { 
                headerTestsByName[matches[1]].describe = matches[2];
                return;
            }
            // debug("ignored describe line for unknown test: " + matches[1] 
            // + "\n");
        } else {
            debug("error parsing describe line: '" + line + "'\n");
        }
    },

    header: function(line) {

        // get the test name
        //
        var testNameRE = /^(\S+)\s+/g;
        testNameRE.lastIndex = 0;
        var matches = testNameRE.exec(line);
        if (matches) {
            var testName = matches[1];
            line = line.substring(testNameRE.lastIndex); // move down the line
        } else {
            debug("error parsing header config line: '" + line + "'\n");
            return;
        }

        if ( testName[0] == '_' && testName[1] == '_') {
            // debug("subrule skipped\n");
            return;
        }
        var test = new Object();
        test.name = testName;

        // get the operator
        //
        var operatorRE = /^(\S+)\:/g;
        operatorRE.lastIndex = 0;
        matches = operatorRE.exec(line);
        if (matches) {

            line = line.substring(operatorRE.lastIndex);
            switch ( matches[1] ) {

            case "eval":
                // XXX use RE here to catch malformed
                // XXX security check input
                test.operator = TEST_HEADER_EVAL;
                test.evalString = line;
                headerTestsByName[testName] = test;
                break;

            case "exists":
                var headerNameRE = /^(\S+)/;
                matches = headerNameRE.exec(line);
                if (matches) {
                    // build an existance test
                    //
                    test.operator = TEST_HEADER_EXISTS;
                    test.headerName = matches[1];

                    // add this test to the lists
                    //
                    headerTestsByName[testName] = test;
                } else {
                    debug ("error parsing exists header: '" + line + "'\n");
                }
                break;

            case "rbleval":
                //debug("not parsed: '" + line + "'\n");
                break;
            case "rblreseval":
                //debug("not parsed: '" + line + "'\n");
                break;
            }
        } else {
            // this must be a regexp matching rule.

            // split out the arguments 
            // XXX need lookahead assertion to see that \/ won't trigger
            // end-of-RE match
            //
            const headerArgsRE = /^(\S+)\s+(\=~|\!~)\s+\/(.*)\/(\S*)\s*(.*)/;
            var headerArgs = headerArgsRE.exec(line);
            if (!headerArgs) {
                debug("parse error: " + testName + "\n");
                return;
            }

            // JS RegExps don't support all Perl RegExp flags.  Ignore
            // tests containing these.
            const knownREFlags = /^[igm]*$/;
            if (!knownREFlags.test(headerArgs[4])) {
                // debug("ignored (unsupported regexp flag):" + matches[2] 
                // + "\n");
                return;
            }

            // create a object for this test, and add it to the list of tests
            // for this header
            //
            test.headerName = headerArgs[1];
            switch (headerArgs[2]) {
            case "=~":
                test.operator = TEST_HEADER_REGEXP_MATCH;
                break;
            case "!~":
                test.operator = TEST_HEADER_REGEXP_NOMATCH;
                break;
            default:
                debug("error parsing header operator: " + headerArgs[2]);
            }

            test.regexp = new RegExp(headerArgs[3], headerArgs[4]);

            // parse the trailing unset group, if present
            //
            if (headerArgs[5]) {
                const unsetRE = /^\[if-unset\:\s+(.*)\]/;
                matches = unsetRE.exec(headerArgs[5]);
                if (matches && matches[1]) {
                    test.unset = matches[1];
                    // debug("unset clause: " + matches[1] + "\n");
                } else {
                    // XXX either a parse or comment, probably
                }
            } 

            headerTestsByName[testName] = test;
            return;
        }
    }, 

    score: function(line) {
        // if this is a score line and a corresponding test object exists, 
        // attach a score property to the test 
        //
        const scoreRE = /^(\S+)\s+(\S+)\s*/;
        var matches = scoreRE.exec(line);
        if (matches) { 
            if ( matches[1] in headerTestsByName) { 
                headerTestsByName[matches[1]].score = Number(matches[2]);
                return;
            }
            // debug("ignored score line for unknown test: " + matches[1] 
            // + "\n");
        } else {
            debug("error parsing score line: '" + line + "'\n");
        }
    }
}
 
// This is a dummy message filter which gets passed back to ApplyFilterHit()
// I think I'm gonna propose that nsIMsgFilter get split into two interfaces
// for cleaniness.
//
function dummyFilter() {}

dummyFilter.prototype = 
{
    get action() {
        return Components.interfaces.nsMsgFilterAction.MoveToFolder;
    },

    get actionTargetFolderUri() {
        return gTargetFolder;
    }
}

var df = new dummyFilter();

function nsJunkmail() {}

nsJunkmail.prototype = {

    init: function() {

        if (!gPrefs) {
            gPrefs = Components.classes["@mozilla.org/preferences-service;1"]
                .getService(Components.interfaces.nsIPrefService)
                .getBranch(null);
        }

        // XXX needs to be a wstring pref, actually.  Both of the following
        // prefs probably should be associated if with an identity, if not
        // an account or server
        //
        gTargetFolder = gPrefs.getCharPref("mail.junkmail.target_spam_folder");

        if (!gLogStream) {
            var logFileName = gPrefs.getCharPref("mail.junkmail.logfile");

            var logFile = Components.classes["@mozilla.org/file/local;1"].
                createInstance(Components.interfaces.nsILocalFile);
           
            logFile.initWithPath(logFileName);

            // XXX need to use binary output stream here?
            // XXX use nsIBufferedOutputStream?
            gLogStream = Components.classes
                ["@mozilla.org/network/file-output-stream;1"]
                .createInstance(Components.interfaces.nsIFileOutputStream);

            // XXX commentary on the flags & constants
            gLogStream.init(logFile, 0x02|0x10|0x80, 420, 0);
        }

        var spamAssassinRulesPath = 
            gPrefs.getCharPref("mail.junkmail.spamassassin_rules_dir");
        var rulesDir = Components.classes["@mozilla.org/file/local;1"].
            createInstance(Components.interfaces.nsILocalFile);
        rulesDir.initWithPath(spamAssassinRulesPath);

        // verify that this is really a directory
        //
        if (!rulesDir.isDirectory()) {
            debug("nsJunkmail::init(): rulesDir is not a directory\n");
            throw Components.results.NS_ERROR_FAILURE;
        }
        
        // get a list of the rules files
        //
        var dirEntries = rulesDir.directoryEntries;
        var confFiles = new Array();

        while (dirEntries.hasMoreElements()) {
            var rulesFile = dirEntries.getNext().QueryInterface(
                Components.interfaces.nsIFile);

            // if this isn't a directory and ends in .cf, add it to the list
            // of config files, and isn't a test file from the cvs version
            // of spam assassin...  XXX should remove for non-debug builds
            //
            if ( rulesFile.leafName.search(/.cf$/) != -1 
                 && ! ( rulesFile.leafName[0] == '7'
                        && rulesFile.leafName[1] == '0' )) {
                confFiles.push(rulesFile);
            }
        }

        // compare nsIFiles by name for sorting purposes
        function comparator(a, b) { 
            if ( a.leafName < b.leafName ) {
                return -1;
            } else if (a.leafName > b.leafName) {
                return 1;
            } else {
                return 0;
            }
        }

        confFiles.sort(comparator);        // files should be read in order

        for (rulesFile in confFiles) {
            
            // XXX use nsIBufferedInputStream?
            var rulesIStream = Components.classes[
                "@mozilla.org/network/file-input-stream;1"].
                createInstance(Components.interfaces.nsIFileInputStream);

            const PR_READONLY = 0x1;
            rulesIStream.init(confFiles[rulesFile], PR_READONLY, 0,
                        Components.interfaces.nsIFileInputStream.CLOSE_ON_EOF);

            // use the read-one-line-at-a-time interface
            //
            var rulesLineStream = rulesIStream.QueryInterface(
                Components.interfaces.nsILineInputStream);

            var line = {};
            var i = 0; 
            do {
                // XXX check what happens on 0-length file
                this.parse_config_line(line.value);
            } while (rulesLineStream.readLine(line));
        }
    },
    
    parse_config_line: function(line) {

        // nothing but whitespace || starts with comment
        const ignoreRE = /^\s*$|^\s*#/;
        if (ignoreRE.test(line)) {
            return;
        }

        // XXX strip trailing comments
        var keywordRE = /^\s*(\S+)\s*/g;
        keywordRE.lastIndex = 0;
        var matches = keywordRE.exec(line);

        // XXX err-chk matches[1] contains something
        if (matches[1] in parseConfigLine) {
            parseConfigLine[matches[1]](line.substring(
                                            keywordRE.lastIndex));
        } else {
            // debug("not parsed: " + line + "\n");
        }
    },

    filterMsgByHeaders: function(aCount, aHeaders, aListener, aMsgWindow) { 

        var headersByName = {};
        var msgScore = 0.0;

        // returns all values of a header, joined with \n
        //
        function getHeaderVals(aHeaderName) {
            if (aHeaderName in headersByName) {
                return headersByName[aHeaderName].join("\n");
            } else {
                return undefined;
            }
        }

        // returns all values of a header, joined with \n
        //
        function getHeaderValsArray(aHeaderName) {
            if (aHeaderName in headersByName) {
                return headersByName[aHeaderName];
            } else {
                return undefined;
            }
        }


        // execute a single test to a single header value, updating msgScore as
        // we go (mmm, lexical-scoping & side-effects)
        //
        function executeTest(aTest) {

            // XXX get rid of [0]s, probably

            // XXX header names currently case-sensitive, probably an issue

            var testResult;
            var headerValues;

            switch (aTest.operator) { 
            case TEST_HEADER_REGEXP_MATCH:

                // get the header we're trying to test, or the default value
                // for this test, or just skip this test
                //
                headerValues = getHeaderVals(aTest.headerName);
                if ( !headerValues ) {
                    if ("unset" in aTest) {
                        headerValues = aTest.unset;
                    } else {
                        return;
                    }
                }

                testResult = aTest.regexp.test(headerValues);
                break;

            case TEST_HEADER_REGEXP_NOMATCH:

                // get the header we're trying to test, or the default value
                // for this test, or just skip this test
                //
                headerValues = getHeaderVals(aTest.headerName);
                if ( !headerValues ) {
                    if ("unset" in aTest) {
                        headerValues = aTest.unset;
                    } else {
                        return;
                    }
                }

                testResult = !aTest.regexp.test(headerValues);
                break;

            case TEST_HEADER_EXISTS:
                headerValues = getHeaderVals(aTest.headerName);
                if ( !headerValues ) {
                    return;
                }

                testResult = true;
                break;

            case TEST_HEADER_EVAL:
                try { 
                    testResult = eval(aTest.evalString.toString());
                } catch (ex if ex instanceof ReferenceError) {
                    // XXX shouldn't even be in tests table if we don't support
                } 
                break;
            }

            // if the testResult is true and this test has an associated
            // score...
            //
            // XXX see if .describe is defined before using?
            //
            if ( testResult && ("score" in aTest) ) {

                msgScore += aTest.score;
                var logMsg =  " " + aTest.name + " (" + aTest.score + "): " 
                    + ("describe" in aTest ? aTest.describe : "") + "\n";
                gLogStream.write(logMsg, logMsg.length);
            } else {
                // debug("  " + aTest.name + " (" + aTest.score + ")\n");
            }
        }

        // these functions are used by evals
        //
        function check_for_bad_helo() {

            // XXX rule out of date

            var authWarning = getHeaderVals("X-Authentication-Warning");
            if ( !authWarning ) {
                return 0;
            }

            const badHeloRE = /host \S+ \[(\S+)\] claimed to be.*\[(\S+)\]/i;
            var matches = badHeloRE.exec(authWarning);

            if ( matches && matches.length == 3 && matches[1] != matches[2]) {
                return 1;
            }
            return 0;
        }

        function check_from_name_eq_from_address() {
            const fromRE = /\"(\S+)\" <(\S+)>/;
            var matches = fromRE.exec(getHeaderVals("From"));
            if ( matches && matches.length == 3 && matches[1] == matches[2]) {
                return 1;
            }

            return 0;
        }

        function check_for_unique_subject_id() {

            var subject = getHeaderVals("Subject");

            if (subject.search( /[-_\.\s]{7,}([-a-z0-9]{4,})$/ ) >= 0
                || subject.search( /\s{10,}(?:\S\s)?(\S+)$/ ) >= 0 
                || subject.search( 
                    /\s{3,}[-:\#\(\[]+([-a-z0-9]{4,})[\]\)]+$/ ) >=0
                || subject.search( /\s{3,}[:\#\(\[]*([0-9]{4,})[\]\)]*$/ ) >= 0
                || subject.search( /\s{3,}[-:\#]([a-z0-9]{5,})$/) >= 0

                // (7217vPhZ0-478TLdy5829qicU9-0@26) and similar
                || subject.search( /\(([-\w]{7,}\@\d+)\)$/ ) >= 0

                // Seven or more digits at the end of a subject is almost
                // certainly an id.
                || subject.search( /\b(\d{7,})\s*$/ ) >= 0

                // stuff at end of line after "!" or "?" is usually an id
                || subject.search( /[!\?]\s*(\d{4,}|\w+(-\w+)+)\s*$/ ) >= 0

                // 9095IPZK7-095wsvp8715rJgY8-286-28 and similar
                || subject.search( /\b(\w{7,}-\w{7,}-\d+-\d+)\s*$/ ) >= 0

                // #30D7 and similar
                || subject.search( /\s#\s*([a-f0-9]{4,})\s*$/ ) >= 0) {

                // XXX exempt online purchases

                // XXX need to do word_is_in_dict check here
                return 1;
            }
            return 0;
        }

        function check_lots_of_cc_lines() {
            var vals = getHeaderValsArray('Cc');
            if ( vals && vals.length > 20 ) {
                return 1;
            } 

            return 0;
        }

        function check_subject_for_lotsa_8bit_chars() {
            return 0;
        }

        function are_more_high_bits_set(str) {

        }

        // Build an table of arrays of lightly munged headers by
        // name.  Leading whitespace on the header value has been eliminated,
        // and continuations replaced by single space characters.
        //
        var headerRE = /^(\S+)\:\s+/g;  // global cause we want lastIndex set

        for (var headerLine in aHeaders) {

            if (aHeaders[headerLine] == "") {
                continue;   // ignore any empty lines
            }

            headerRE.lastIndex = 0;     // clear lastIndex
            var matches = headerRE.exec(aHeaders[headerLine]);
            try {
                var headerName = matches[1];
            } catch (ex) {
                debug("_EXCEPTION_: failed to match '" + aHeaders[headerLine]
                      +"'\n");
            }

            if (! (headerName in headersByName)) {
                headersByName[headerName] = new Array();
            }
            
            // push header value, with continuations replaced, onto 
            // the array of like-named headers
            //
            headersByName[headerName].push(
                aHeaders[headerLine].substring(headerRE.lastIndex)
                .replace(/\n\s+/g, " "));
        }

        var fromVals = getHeaderVals("From");
        if (fromVals) {
            var logMsg = "From: " + fromVals + "\n";
            gLogStream.write(logMsg, logMsg.length);
        }
        var subjVals = getHeaderVals("Subject");
        if (subjVals) {
            logMsg = "Subject: " + subjVals + "\n";
            gLogStream.write(logMsg, logMsg.length);
        }

        // execute all the tests.  at some point, we'll do this in
        // a sorted order, so that we can bail out as soon as
        // the msgScore hits a certain threshold.
        //
        for ( testName in headerTestsByName )
        {
            executeTest(headerTestsByName[testName]);
        }

        logMsg = "total message score: " + msgScore + "\n\n";
        gLogStream.write(logMsg, logMsg.length);

        // XXX don't hardcode this
        //
        if (msgScore >= 3.1) {
            aListener.applyFilterHit(df, aMsgWindow);
        }
    }
}


// XXX better way to do singleton?
var Junkmail = null;

var nsJunkmailModule = 
{
    registerSelf: function (compMgr, fileSpec, location, type) {
        debug("*** Registering spam whacker components" +
              " (all right -- a JavaScript module!)\n");
        
        compMgr = compMgr.QueryInterface(
            Components.interfaces.nsIComponentRegistrar);
        
        compMgr.registerFactoryLocation(
                NS_JUNKMAIL_CID, 
                'Spam Whacker', 
                NS_JUNKMAIL_CONTRACTID, 
                fileSpec, location, 
                type);
    },

    getClassObject: function(compMgr, cid, iid) {
        if (!iid.equals(Components.interfaces.nsIFactory)) {
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
        }

        if (cid.equals(NS_JUNKMAIL_CID)) {
            return this.nsJunkmailFactory;    
        } else {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }
    },

    nsJunkmailFactory: {
        createInstance: function(outer, iid) {
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;
      
            if (Junkmail == null) {
                Junkmail = new nsJunkmail();
            }

            return Junkmail;
        }
    },

    // because of the way JS components work (the JS garbage-collector
    // keeps track of all the memory refs and won't unload until appropriate)
    // this ends up being a dummy function; it can always return true.
    //
    canUnload: function(compMgr) { return true; }

}

function NSGetModule(compMgr, fileSpec) { return nsJunkmailModule; }
