%{
/* $Id$ */
/* Copyright (c) 2001 Alex Hioreanu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* 
 * Basic lexer for use with flex (not tested with vanilla lex).  Nothing
 * very interesting here.
 *
 * Strings may not span multiple lines.  String literals may also have
 * character escapes in them, just like in C.  Numbers may not start
 * with a leading zero, since a leading zero means octal notation to
 * me, and we don't support octal/hex number literals.  A floating
 * point number can take any format as a C floating point literal.
 *
 * If flex allowed for the "non-greedy" closure operator (like every
 * modern regular expression library supplies), C-style comments could
 * be caught like this: */

#if 0 
/\*(.|\n)*?\*/ {;}
/\*(.|\n)* { printf("Unterminated comment on line %d\n", lineno); }
#endif 

/* But flex doesn't have '*?' so we have to write some inane code to
 * deal with C-style comments.
 */

#include "config.h"
#include "prefs.h"
#include "parser.h"

extern int line_number;
  
%}

DIGIT [0-9]

%% 

"\n" { line_number++; }

"/*" {
    int c;

    for (;;) {
        while ( (c = input()) != '*' && c != EOF && c != '\n');
        if (c == '*') {
            while ( (c = input()) == '*');
            if (c == '/') break;
        } else if (c == '\n') {
            line_number++;
        } else {
            fprintf(stderr, "AHWM: unterminated C-style comment on line %d\n",
                    line_number);
            break;
        }
    }
}

DisplayTitleBar { return TOK_DISPLAYTITLEBAR; /* ADDOPT 1: lexical symbol */ }
TitlebarFont { return TOK_TITLEBARFONT; }
Omnipresent { return TOK_OMNIPRESENT; }
DefaultWorkspace { return TOK_DEFAULTWORKSPACE; }
FocusPolicy { return TOK_FOCUS_POLICY; }
AlwaysOnTop { return TOK_ALWAYSONTOP; }
AlwaysOnBottom { return TOK_ALWAYSONBOTTOM; }
PassFocusClick { return TOK_PASSFOCUSCLICK; }
CycleBehaviour { return TOK_CYCLEBEHAVIOUR; }
CycleBehavior { return TOK_CYCLEBEHAVIOUR; }
ColorTitlebar { return TOK_COLORTITLEBAR; }
ColourTitlebar { return TOK_COLORTITLEBAR; }
ColorTitlebarNonFocused { return TOK_COLORTITLEBAR; }
ColourTitlebarNonFocused { return TOK_COLORTITLEBAR; }
ColorTitlebarFocused { return TOK_COLORTITLEBARFOCUSED; }
ColourTitlebarFocused { return TOK_COLORTITLEBARFOCUSED; }
ColorTitlebarText { return TOK_COLORTEXT; }
ColourTitlebarText { return TOK_COLORTEXT; }
ColorTitlebarTextNonFocused { return TOK_COLORTEXT; }
ColourTitlebarTextNonFocused { return TOK_COLORTEXT; }
ColorTitlebarTextFocused { return TOK_COLORTEXTFOCUSED; }
ColourTitlebarTextFocused { return TOK_COLORTEXTFOCUSED; }
NumberOfWorkspaces { return TOK_NWORKSPACES; }
DontBindMouse { return TOK_DONTBINDMOUSE; }
DontBindKeys { return TOK_DONTBINDKEYS; }
Sticky { return TOK_STICKY; }
TitlePosition { return TOK_TITLEPOSITION; }
KeepTransientsOnTop { return TOK_KEEPTRANSIENTSONTOP; }
RaiseDelay { return TOK_RAISEDELAY; }

SloppyFocus { return TOK_SLOPPY_FOCUS; }
ClickToFocus { return TOK_CLICK_TO_FOCUS; }
DontFocus { return TOK_DONT_FOCUS; }

SkipCycle { return TOK_SKIPCYCLE; }
RaiseImmediately { return TOK_RAISEIMMEDIATELY; }
RaiseOnCycleFinish { return TOK_RAISEONCYCLEFINISH; }
DontRaise { return TOK_DONTRAISE; }

DisplayLeft { return TOK_DISPLAYLEFT; }
DisplayRight { return TOK_DISPLAYRIGHT; }
DisplayCentered { return TOK_DISPLAYCENTERED; }
DontDisplay { return TOK_DONTDISPLAY; }

True { return TOK_TRUE; }
False { return TOK_FALSE; }

TransientFor { return TOK_TRANSIENTFOR; }
HasTransient { return TOK_HASTRANSIENT; }
Not { return TOK_NOT; }

IsShaped { return TOK_ISSHAPED; }
InWorkspace { return TOK_INWORKSPACE; }
WindowName { return TOK_WINDOWNAME; }
WindowClass { return TOK_WINDOWCLASS; }
WindowInstance { return TOK_WINDOWINSTANCE; }

Function { return TOK_FUNCTION; }

Menu { return TOK_MENU; }

BindKey { return TOK_BINDKEY; }
BindButton { return TOK_BINDBUTTON; }
BindDrag { return TOK_BINDDRAG; }
BindKeyRelease { return TOK_BINDKEYRELEASE; }
UnBindKey { return TOK_UNBINDKEY; }
UnBindButton { return TOK_UNBINDBUTTON; }
UnBindDrag { return TOK_UNBINDDRAG; }
UnBindKeyRelease { return TOK_UNBINDKEYRELEASE; }

Root { return TOK_ROOT; }
Frame { return TOK_FRAME; }
Titlebar { return TOK_TITLEBAR; }

SendToWorkspace { return TOK_SENDTOWORKSPACE; }
GoToWorkspace { return TOK_GOTOWORKSPACE; }
CycleNext { return TOK_CYCLENEXT; }
CyclePrevious { return TOK_CYCLEPREV; }
KillNicely { return TOK_KILLNICELY; }
KillWithExtremePrejudice { return TOK_KILLWITHEXTREMEPREJUDICE; }
Launch { return TOK_LAUNCH; }
Focus { return TOK_FOCUS; }
Maximize { return TOK_MAXIMIZE; }
MaximizeHorizontally { return TOK_MAXIMIZE_HORIZONTALLY; }
MaximizeVertically { return TOK_MAXIMIZE_VERTICALLY; }
Nop { return TOK_NOP; }
Quote { return TOK_QUOTE; }
MoveInteractively { return TOK_MOVEINTERACTIVELY; }
ResizeInteractively { return TOK_RESIZEINTERACTIVELY; }
MoveResize { return TOK_MOVERESIZE; }
Quit { return TOK_QUIT; }
Beep { return TOK_BEEP; }
Invoke { return TOK_INVOKE; }
ShowMenu { return TOK_SHOWMENU; }
Refresh { return TOK_REFRESH; }
Restart { return TOK_RESTART; }
Crash { return TOK_CRASH; }

Define { return TOK_DEFINE; }

";" { return TOK_SEMI; }
"=" { return TOK_EQUALS; }
"is!" { return TOK_SET_UNCONDITIONALLY; }
"{" { return TOK_LBRACE; }
"}" { return TOK_RBRACE; }
"," { return TOK_COMMA; }
"(" { return TOK_LPAREN; }
")" { return TOK_RPAREN; }

\"([^\"\n]|\\n|\\a|\\b|\\r|\\t|\\v|\\\"|\\\\)*\" {
    yylval.value_string = yytext;
    return TOK_STRING;
}

[-+]*(0|[1-9]{DIGIT}*) {
    yylval.value_int = atoi(yytext);
    return TOK_INTEGER;
}

({DIGIT}|[1-9]{DIGIT}+)(\.{DIGIT}+e[-+]?|\.|e[-+]?){DIGIT}+ {
    yylval.value_float = (float)atof(yytext);
    return TOK_FLOAT;
}

\#[^\n]*\n {line_number++;}
"//"[^\n]*\n {line_number++;}

[ \v\t\r]+ {;}

[a-zA-Z_][-a-zA-Z0-9]* {
    fprintf(stderr, "AHWM: Unrecognized keyword '%s' on line %d\n",
            yytext, line_number);
}
. { fprintf(stderr, "AHWM: Unrecognized character '%s' on line %d\n",
            yytext, line_number); }

%%

int line_number = 1;

#ifndef yywrap
int yywrap()
{
    return 1;
}
#endif
