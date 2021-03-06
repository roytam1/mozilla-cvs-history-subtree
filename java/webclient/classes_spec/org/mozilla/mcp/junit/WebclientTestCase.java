/*
 * $Id$
 */

/* 
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Sun
 * Microsystems, Inc. Portions created by Sun are
 * Copyright (C) 1999 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): Ed Burns &lt;edburns@acm.org&gt;
 */

package org.mozilla.mcp.junit;

// WebclientTestCase.java

import java.util.ArrayList;
import java.util.List;

import java.io.IOException;
import java.io.File;
import java.util.logging.Logger;

import junit.framework.TestCase;
import junit.framework.TestSuite;
import junit.framework.TestResult;
import org.mozilla.mcp.CompareFiles;

import org.mozilla.mcp.THTTPD;
import org.mozilla.webclient.BrowserControlFactory;

/**
 *
 *  <p>WebclientTestCase extends <code>junit.framework.TestCase</code>
 *  and allows using MCP from a JUnit test.  It makes assertions that
 *  verify preconditions for running MCP.</p>
 *
 * <p>This class currently has a number of undocumented and unsupported
 * features that can be useful if you take the time to look at the
 * source.  Specifically, it has the ability to capture output from
 * running the testcase, compare that output with a golden file, and it
 * has a trivial HTTP server built in so webclient automated tests can
 * run without any extra server baggage.</p>
 *
 * @version $Id$
 * 
 *
 */

public abstract class WebclientTestCase extends TestCase
{
//
// Protected Constants
//

public static final String WEBCLIENTSTUB_LOG_MODULE = "webclientstub";
public static final String WEBCLIENT_LOG_MODULE = "webclient";
public static String OUTPUT_FILE_ROOT = null;
public static final String TEST_LOG = "org.mozilla.mcp.junit";
public static final String TEST_LOG_STRINGS = "org.mozilla.mcp.junit.TestLogStrings";

public static final Logger LOGGER = getLogger(TEST_LOG);

//
// Class Variables
//

    static THTTPD.ServerThread serverThread;

//
// Instance Variables
//

// Attribute Instance Variables

// Relationship Instance Variables

//
// Constructors and Initializers    
//

public WebclientTestCase()
{
    super("WebclientTestCase");
}

public WebclientTestCase(String name)
{
    super(name);
}

//
// Class methods
//

public static Logger getLogger( String loggerName ) {
    return Logger.getLogger(loggerName, TEST_LOG_STRINGS );
}


//
// Methods From TestCase
//

public void setUp()
{
    verifyPreconditions();
    verifyOutputFileRootIsSet();
    
    LOGGER.info(this.getClass().getName() + " setUp()");
    
}

public void tearDown()
{
    LOGGER.info(this.getClass().getName() + " tearDown()");
}

//
// General Methods
//

public static TestSuite createServerTestSuite() {
    verifyOutputFileRootIsSet();
    TestSuite result = new TestSuite() {
	    public void run(TestResult result) {
		serverThread = 
		    new THTTPD.ServerThread("LocalHTTPD",
					    new File (OUTPUT_FILE_ROOT), -1);
		serverThread.start();
		serverThread.P();
		super.run(result);
		try {
		    BrowserControlFactory.appTerminate();
		}
		catch (Exception e) {
		    fail();
		}
		serverThread.stopRunning();
	    }
	};
	return result;
}

/**

* assertTrue that the string logModuleName is a correct log module
* string as specified in pr_log.h, and that its value is at least n.

*/

protected void verifyLogModuleValueIsAtLeastN(String logModuleName, int n)
{
    int i = 0;
    String logModuleValue = null;
    assertTrue(null != (logModuleValue = 
			System.getProperty("NSPR_LOG_MODULES")));
    
    assertTrue(-1 != 
	       (i = logModuleValue.indexOf(logModuleName + ":")));
    try {
	i = Integer.
	    valueOf(logModuleValue.substring(i + logModuleName.length() + 1,
					     i + logModuleName.length() + 2)).
	    intValue();
	assertTrue(i >= n); 
    }
    catch (Throwable e) {
	e.printStackTrace();
	assertTrue(false);
    }
	
}

protected static void verifyBinDirSet()
{
    assertTrue("BROWSER_BIN_DIR is not set",
	       null != System.getProperty("BROWSER_BIN_DIR"));
}

protected static String getBrowserBinDir() {
    return System.getProperty("BROWSER_BIN_DIR");
}

protected static String getOutputFileRoot() {
    return OUTPUT_FILE_ROOT;
}


/**

* assertTrue that NSPR_LOG_FILE is set.

*/

protected String verifyOutputFileIsSet()
{
    String logFileValue = null;

    assertTrue(null != (logFileValue = 
			System.getProperty("NSPR_LOG_FILE")));
    return logFileValue;
    
}

private static void verifyOutputFileRootIsSet() {
    if (null != OUTPUT_FILE_ROOT) {
        return;
    }
    OUTPUT_FILE_ROOT = System.getProperty("build.test.results.dir");
    assertNotNull(OUTPUT_FILE_ROOT);

    File outputRoot = new File(OUTPUT_FILE_ROOT);
    assertTrue(outputRoot.exists());

    
}

/**

* This implementation checks that the proper environment vars are set.

*/

protected void verifyPreconditions()
{
    String nsprLogModules = null;

    // make sure we have at least PR_LOG_DEBUG set
    verifyLogModuleValueIsAtLeastN(WEBCLIENTSTUB_LOG_MODULE, 4);
    verifyLogModuleValueIsAtLeastN(WEBCLIENT_LOG_MODULE, 4);
    verifyBinDirSet();
    if (sendOutputToFile()) {
	verifyOutputFileIsSet();
    }
}

public boolean verifyExpectedOutput()
{
    boolean result = false;
    CompareFiles cf = new CompareFiles();
    String errorMessage = null;
    String outputFileName = null;
    String correctFileName = null;
    
    // If this testcase doesn't participate in file comparison
    if (!this.sendOutputToFile() && 
	(null == this.getExpectedOutputFilename())) {
	return true;
    }
    
    if (this.sendOutputToFile() ) {
        outputFileName = verifyOutputFileIsSet();
    } 
    correctFileName = OUTPUT_FILE_ROOT + this.getExpectedOutputFilename();
    
    errorMessage = "File Comparison failed: diff -u " + outputFileName + " " + 
        correctFileName;
    
    ArrayList ignoreList = null;
    String [] ignore = null;
    
    if (null != (ignore = this.getLinesToIgnore())) {
	ignoreList = new ArrayList();
	for (int i = 0; i < ignore.length; i++) {
	    ignoreList.add(ignore[i]);
	}
    }
    
    try {
	result = cf.filesIdentical(outputFileName, correctFileName,ignoreList,
				   getIgnorePrefix(), getIgnoreWarnings(),
				   getIgnoreKeywords()); 
    }
    catch (IOException e) {
	System.out.println(e.getMessage());
	e.printStackTrace();
    }

    if (!result) {
	System.out.println(errorMessage);
    }
    System.out.println("VERIFY:"+result); 
    return result;
}

/**

* @return the name of the expected output filename for this testcase.

*/

public String getExpectedOutputFilename() { return null; }

public String [] getLinesToIgnore() { return null; }

public List getIgnoreKeywords() {
    ArrayList result = new ArrayList();
    result.add("nativeBinDir");
    return result;
}

public boolean getIgnorePrefix() { return true; }

public boolean getIgnoreWarnings() { return true; }

public boolean sendOutputToFile() { return false; }



} // end of class WebclientTestCase
