#ifndef InputBuffer_h
#define InputBuffer_h

/*
 * InputBuffer - filters and buffers characters from a Reader.
 */

#include <string>
#include <vector>
#include <iostream.h>
#include <sstream>
#include <stdio.h>
#include <fstream>
#include "characterclasses.h"

namespace esc {
namespace v1 {

using namespace CharacterClasses;

class InputBuffer {
public:
	std::string* lineA;// = new StringBuffer();
    std::string* lineB; // = new StringBuffer();
    std::string* curr_line;
	std::string* prev_line;
    int curr_line_offset,prev_line_offset;
    int lineA_offset, lineB_offset;
private:
	std::string text;
	std::vector<int> line_breaks; // = new int[1000];
	std::ofstream* out;
public:
	std::istream& in;
	std::ostream& err;
	std::string&  origin;

    int     pos;
    int     colPos, lnNum; // <0,0> is the position of the first character.

/*
    void setOut(std::string filename) {
		out = new std::ofstream(filename,std::ios::binary);
    }
*/
    static void main(int argc, char* argv[]) {
        test_retract();
        test_markandcopy();
        test_getLineText();
    }

	InputBuffer( std::istream& in, std::ostream& err, std::string& origin ) 
		: lineA(new std::string()), lineB(new std::string()), curr_line(0), prev_line(0), in(in), err(err), 
		  origin(origin), pos(0), lnNum(-1), colPos(0) {
		line_breaks.resize(1000,0);
    }

    InputBuffer( std::istream& in, std::ostream& err, std::string& origin, int pos ) 
		: lineA(new std::string()), lineB(new std::string()), curr_line(0), prev_line(0), in(in), err(err), 
		  origin(origin), pos(pos), lnNum(-1), colPos(0) {
		line_breaks.resize(1000,0);
    }

    /*
     * read()
     *
     * Read the next character from the input reader and store it in a buffer
     * of the full text.
     */

    int read() {
        int c = in.get();
        text.insert(text.end(),(unsigned short)c);
        pos++;
        return c;
    }

    /*
     * text()
     */

	std::string& source() {
        return text;
    }

    /*
     * nextchar() -- 
     *
     * The basic function of nextchar() is to fetch the next character,
     * in the input array, increment next and return the fetched character.
     *
     * To simplify the Scanner, this method also does the following:
     * 1. normalizes certain special characters to a common character.
     * 2. skips unicode format control characters (category Cf).
     * 3. keeps track of line breaks, line position and line number.
     * 4. treats <cr>+<lf> as a single line terminator.
     * 5. returns 0 when the end of input is reached.
     */
      
    int nextchar() {

        int c = -1;

        // If the last character was at the end of a line,
        // then swap buffers and fill the new one with characters
        // from the input reader.

        if( curr_line==0 || colPos==curr_line->size() ) {

            lnNum++;
            colPos=0;

            // If the current character is a newline, then read
            // the next line of input into the other input buffer.
            
            if( curr_line == lineA ) {
                if( lineB == 0 ) {
                    lineB = new std::string();
                    lineB_offset = pos;
                }
                curr_line = lineB; curr_line_offset = lineB_offset;
                prev_line = lineA; prev_line_offset = lineA_offset;
                lineA = 0;
            } else {
                if( lineA == 0 ) {
                    lineA = new std::string();
                    lineA_offset = pos;
                }
                curr_line = lineA; curr_line_offset = lineA_offset;
                prev_line = lineB; prev_line_offset = lineB_offset;
                lineB = 0;
            }

            while(c != '\n' && c != 0) {

                c = read();

                if( false ) {
                    //Debugger.trace( "c = " + c );
                }

                // Skip Unicode 3.0 format-control (general category Cf in
                // Unicode Character Database) characters. 

                while(true) {

                    switch(c) {
                        case 0x070f: // SYRIAC ABBREVIATION MARK
                        case 0x180b: // MONGOLIAN FREE VARIATION SELECTOR ONE
                        case 0x180c: // MONGOLIAN FREE VARIATION SELECTOR TWO
                        case 0x180d: // MONGOLIAN FREE VARIATION SELECTOR THREE
                        case 0x180e: // MONGOLIAN VOWEL SEPARATOR
                        case 0x200c: // ZERO WIDTH NON-JOINER
                        case 0x200d: // ZERO WIDTH JOINER
                        case 0x200e: // LEFT-TO-RIGHT MARK
                        case 0x200f: // RIGHT-TO-LEFT MARK
                        case 0x202a: // LEFT-TO-RIGHT EMBEDDING
                        case 0x202b: // RIGHT-TO-LEFT EMBEDDING
                        case 0x202c: // POP DIRECTIONAL FORMATTING
                        case 0x202d: // LEFT-TO-RIGHT OVERRIDE
                        case 0x202e: // RIGHT-TO-LEFT OVERRIDE
                        case 0x206a: // INHIBIT SYMMETRIC SWAPPING
                        case 0x206b: // ACTIVATE SYMMETRIC SWAPPING
                        case 0x206c: // INHIBIT ARABIC FORM SHAPING
                        case 0x206d: // ACTIVATE ARABIC FORM SHAPING
                        case 0x206e: // NATIONAL DIGIT SHAPES
                        case 0x206f: // NOMINAL DIGIT SHAPES
                        case 0xfeff: // ZERO WIDTH NO-BREAK SPACE
                        case 0xfff9: // INTERLINEAR ANNOTATION ANCHOR
                        case 0xfffa: // INTERLINEAR ANNOTATION SEPARATOR
                        case 0xfffb: // INTERLINEAR ANNOTATION TERMINATOR
                            c = read();
                            continue; // skip it.
                        default:
                            break;
                    }
                    break; // out of while loop.
                }                  


                if( c == 0x000a && prev_line->size()!=0 && (*prev_line)[prev_line->size()-1] == (unsigned short)0x000d ) {

                    // If this is one of those funny double line terminators,
                    // Then ignore the second character by reading on.

                    line_breaks[lnNum]=pos; // adjust if forward.
                    c = read();
                } 
                
                // Line terminators.

                if( c == 0x000a ||
                    c == 0x000d ||
                    c == 0x2028 ||
                    c == 0x2029 ) {

                    curr_line->insert(curr_line->end(),(char)c);
                    c = '\n';
                    line_breaks[lnNum+1]=pos;
                
                // White space

                } else if( c == 0x0009 ||
                           c == 0x000b ||
                           c == 0x000c ||
                           c == 0x0020 ||
                           c == 0x00a0 ||
                           false /* other cat Zs' */  ) {

                    c = ' ';
                    curr_line->insert(curr_line->end(),(char)c);
                
                // End of line

                } else if( c == -1 ) {

                    c = 0;
                    curr_line->insert(curr_line->end(),(char)c);
                
                // All other characters.
                
                } else {

                    // Use c as is.
                    curr_line->insert(curr_line->end(),(char)c);

                }
            }
        }

        // Get the next character.

        int ln  = lnNum;
        int col = colPos;

        if( curr_line->size()!=0 && (*curr_line)[colPos] == 0 ) {
            c = 0;
            colPos++;
        } else if( colPos == curr_line->size()-1 ) {
            c = '\n';
            colPos++;
        } else {
            c = (*curr_line)[colPos++];
        }

        if( out ) {
            //out.println("Ln " + ln + ", Col " + col + ": nextchar " + Integer.toHexString(c) + " = " + (char)c + " @ " + positionOfNext());
        }
        //if( debug || debug_nextchar ) {
            //Debugger.trace("Ln " + ln + ", Col " + col + ": nextchar " + Integer.toHexString(c) + " = " + (char)c + " @ " + positionOfNext());
        //}

        return c;
    }

    /**
     * test_nextchar() -- 
     * Return an indication of how nextchar() performs
     * in various situations, relative to expectations.
     */

    bool test_nextchar() {
        return true;
    }

    /**
     * time_nextchar() -- 
     * Return the milliseconds taken to do various ordinary
     * tasks with nextchar().
     */

    int time_nextchar() {
        return 0;
    }

    /**
     * retract
     *
     * Backup one character position in the input. If at the beginning
     * of the line, then swap buffers and point to the last character
     * in the other buffer.
     */

    void retract() {
        colPos--;
        if( colPos<0 ) {
            if( curr_line == prev_line ) {
                // Can only retract over one line.
                throw; // new Exception("Can only retract past one line at a time.");
            } else if( curr_line == lineA ) {
                curr_line = lineB = prev_line;
            } else {
                curr_line = lineA = prev_line;
            }
            lnNum--;
            colPos = curr_line->size()-1;
            curr_line_offset = prev_line_offset;
        }
        //if( debug || debug_retract ) {
            //Debugger.trace("Ln " + lnNum + ", Col " + colPos + ": retract");
        //}
        return;
    }
    
    static bool test_retract() {
        //Debugger.trace("test_retract");

		std::istringstream input("abc\ndef\nghi"); 
		std::ofstream err("test.err");

		std::string name("test");
		
		InputBuffer* inbuf = new InputBuffer(input,err,name);
        int c=-1;
		int i;
        for(i=0;i<9;i++) {
           c = inbuf->nextchar();
        }
        for(i=0;i<3;i++) {
           inbuf->retract();
           c = inbuf->nextchar();
           inbuf->retract();
        }
        while(c!=0) {
           c = inbuf->nextchar();
        }
        return true;
    }

    /**
     * classOfNext
     */

    unsigned char classOfNext() {

        // return the Unicode character class of the current
        // character, which is pointed to by 'next-1'.

        return CharacterClasses::data[curr_line->at(colPos-1)];
    }

    /**
     * positionOfNext
     */

    int positionOfNext() {
        return curr_line_offset+colPos-1;
    }

    /**
     * positionOfMark
     */

    int positionOfMark() {
        return line_breaks[markLn]+markCol;
    }

    /**
     * mark
     */

    int markCol;
    int markLn;

    int mark() {
        markLn  = (lnNum==-1)?0:lnNum; // In case nextchar hasn't been called yet.
        markCol = colPos;

        //if( debug ) {
        //    Debugger.trace("Ln " + markLn + ", Col " + markCol + ": mark");
        //}

        return markCol;
    }

    /*
     * copy
     */

	std::string* copy() {
		std::string* buf = new std::string();

        //if( debug ) {
        //    Debugger.trace("Ln " + lnNum + ", Col " + colPos + ": copy " + buf);
        //}

        if(markLn!=lnNum || markCol>colPos) {
            throw; // new Exception("Internal error: InputBuffer.copy() markLn = " + markLn + ", lnNum = " + lnNum + ", markCol = " +
                     //                  markCol + ", colPos = " + colPos );
        }

        for (int i = markCol-1; i < colPos; i++) {
            buf->insert(buf->end(),curr_line->at(i));
        }

        return buf;
    }

    static bool test_markandcopy() {
		std::istringstream input("abc\ndef\nghijklmnopqrst\nuvwxyz"); 
		std::string name("test");
		std::ofstream err("test.err");
		InputBuffer* inbuf = new InputBuffer(input,err,name);

		std::string* s;
        int c=-1;
        int i;
        for(i=0;i<10;i++) {
           c = inbuf->nextchar();
        }
        inbuf->mark();
        for(;i<13;i++) {
           c = inbuf->nextchar();
        }
        s = inbuf->copy();
        if( *s == "ijk") {
            printf("1: passed: %s\n", s->c_str());
        } else {
            printf("1: failed: %s\n", s->c_str());
        }
        return true;
    }

	std::string getLineText(int pos) {
        int i,len,a;
        for(i = 0; (a=line_breaks.at(i)) <= pos && i <= lnNum; i++)
            ;
        
        int offset = line_breaks.at(i-1);
        
        for(len = offset ; (text.at(len)!=(char)-1 &&
                       text.at(len)!=0x0a &&
                       text.at(len)!=0x0d &&
                       text.at(len)!=0x2028 &&
                       text.at(len)!=0x2029) ; len++) {
        }

		std::string buf = std::string(len+1,0);
        int count = text.copy(buf.begin(),len-offset,offset);
		return buf;
    }

    static bool test_getLineText() {
		std::istringstream input("abc\ndef\nghi"); 
		std::string name("test");
		std::ofstream err("test.err");
		InputBuffer* inbuf = new InputBuffer(input,err,name);
        int c=-1;
		int i;
        for(i=0;i<9;i++) {
           c = inbuf->nextchar();
        }
        for(i=0;i<3;i++) {
           inbuf->retract();
           c = inbuf->nextchar();
           inbuf->retract();
        }
        while(c!=0) {
           c = inbuf->nextchar();
        }
        for(i=0;i<inbuf->text.length()-1;i++) {
			std::string buf = inbuf->getLineText(i);
			printf("text @ %d: %s\n",i,buf.c_str());
        }
        return true;
    }

    int getColPos(int pos) {
        //Debugger.trace("pos " + pos);
        int i;
        for(i = 0; line_breaks.at(i) <= pos && i <= lnNum; i++)
            ;
        
        int offset = line_breaks.at(i-1);
        //Debugger.trace("offset " + offset);
        
        return pos-offset;
    }

    int getLnNum(int pos) {
        int i;
        for(i = 0; line_breaks.at(i) <= pos && i <= lnNum; i++)
            ;
        return i-1;
    }
};
}
}

#endif // InputBuffer_h

/*
 * Written by Jeff Dyer
 * Copyright (c) 1998-2001 by Mountain View Compiler Company.
 * All Rights Reserved.
 */
