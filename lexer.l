%{
/*
 * $Id$
 * Flex-based lexical analyzer.
 */

/* 
 * Nothing very interesting here.
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
            error("Unterminated C-style comment\n");
            break;
        }
    }
}

DisplayTitleBar { return TOK_DISPLAYTITLEBAR; }
DefaultWorkspace { return TOK_DEFAULTWORKSPACE; }

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
UnBindKey { return TOK_UNBINDKEY; }
UnBindButton { return TOK_UNBINDBUTTON; }

Root { return TOK_ROOT; }
Frame { return TOK_FRAME; }
Titlebar { return TOK_TITLEBAR; }

MoveToWorkspace { return TOK_MOVETOWORKSPACE; }
GoToWorkspace { return TOK_GOTOWORKSPACE; }
AltTab { return TOK_ALTTAB; }
KillNicely { return TOK_KILLNICELY; }
KillWithExtremePrejudice { return TOK_KILLWITHEXTREMEPREJUDICE; }
Launch { return TOK_LAUNCH; }
Focus { return TOK_FOCUS; }
Maximize { return TOK_MAXIMIZE; }
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

";" { return TOK_SEMI; }
"=" { return TOK_EQUALS; }
"{" { return TOK_LBRACE; }
"}" { return TOK_RBRACE; }
"," { return TOK_COMMA; }
"(" { return TOK_LPAREN; }
")" { return TOK_RPAREN; }

\"([^\"\\\n]|\\n|\\\"|\\\\)*\" {
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

\#[^\n]*\n {;}
"//"[^\n]*\n {;}

[ \v\t\r\n]+ {;}

[a-zA-Z_][-a-zA-Z0-9]* {
    fprintf(stderr, "XWM: Unrecognized keyword '%s'\n", yytext);
}
. { fprintf(stderr, "XWM: Unrecognized character '%s'\n", yytext); }

%%

int line_number = 1;

#ifndef yywrap
int yywrap()
{
    return 1;
}
#endif
