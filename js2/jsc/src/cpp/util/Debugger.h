#ifndef Debugger_h
#define Debugger_h

#include <fstream>

namespace esc {
namespace v1 {

struct Debugger {

	static std::ofstream* dbg_prev;
	static std::ofstream* dbg;

	static void trace(std::string str) {
		*dbg << str.c_str() << '\n';
	}

	static void trace(const char* str) {
		*dbg << str << '\n';
	}

	static void trace(const char* str1, const char* str2) {
		*dbg << str1 << str2 << '\n';
	}

	static void init(std::ofstream* ds) {
		dbg_prev = dbg;
		dbg = ds;
	}

	static void reset() {
		dbg = dbg_prev;
	}

#if 0
  public static PrintStream dbg = null;
  private static boolean useSystemOut = false;

  public static void setOutFile(String filename) {
      try {
      PrintStream out = new PrintStream( new FileOutputStream( filename ) );
      System.setOut( out );
	  //System.setErr( outfile );
      } catch ( Exception e ) {
          e.printStackTrace();
      }
  }

  public static void setErrFile(String filename) {
      try {
      PrintStream err = new PrintStream( new FileOutputStream( filename ) );
      //System.setOut( out );
	  System.setErr( err );
      } catch ( Exception e ) {
          e.printStackTrace();
      }
  }

  public static void setDbgFile(String filename) {
      try {
      if( dbg!=null) {
          dbg.close();
      }
      dbg = new PrintStream( new FileOutputStream( filename ) );
      //System.setOut( outfile );
	  //System.setErr( outfile );
      } catch ( Exception e ) {
          e.printStackTrace();
      }
  }
#endif
};
}
}

#endif // Debugger_h

/*
 * Written by Jeff Dyer
 * Copyright (c) 1998-2001 by Mountain View Compiler Company.
 * All Rights Reserved.
 */
