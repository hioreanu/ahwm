%{                                                      /* -*-Text-*- */
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
 * Parser definition file for use with bison (not tested with yacc).
 * Should have no s/r or r/r conflicts and doesn't use right recursion
 * anywhere.  I opted for clarity in lieu of brevity because this
 * stuff is complex enough without overloading rules, structures or
 * names.  A simple little parser, no "gotchas".
 */

#include "config.h"

#include "keyboard-mouse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef PARSER_DEBUG
#define parse_debug(x) printf(x)
#else
#define parse_debug(x) /* */
#endif

/* ADDOPT 2: define token */
%}

%token TOK_DISPLAYTITLEBAR
%token TOK_OMNIPRESENT
%token TOK_DEFAULTWORKSPACE
%token TOK_FOCUS_POLICY
%token TOK_ALWAYSONTOP
%token TOK_ALWAYSONBOTTOM
%token TOK_PASSFOCUSCLICK
%token TOK_CYCLEBEHAVIOUR
%token TOK_COLORTITLEBAR
%token TOK_COLORTITLEBARFOCUSED
%token TOK_COLORTEXT
%token TOK_COLORTEXTFOCUSED
%token TOK_NWORKSPACES
%token TOK_DONTBINDMOUSE
%token TOK_DONTBINDKEYS
%token TOK_STICKY
%token TOK_TITLEPOSITION
%token TOK_KEEPTRANSIENTSONTOP

%token TOK_SLOPPY_FOCUS
%token TOK_CLICK_TO_FOCUS
%token TOK_DONT_FOCUS

%token TOK_SKIPCYCLE
%token TOK_RAISEIMMEDIATELY
%token TOK_RAISEONCYCLEFINISH
%token TOK_DONTRAISE

%token TOK_DISPLAYLEFT
%token TOK_DISPLAYRIGHT
%token TOK_DISPLAYCENTERED
%token TOK_DONTDISPLAY

%token TOK_TRUE
%token TOK_FALSE

%token TOK_TRANSIENTFOR
%token TOK_HASTRANSIENT
%token TOK_NOT

%token TOK_ISSHAPED
%token TOK_INWORKSPACE
%token TOK_WINDOWCLASS
%token TOK_WINDOWNAME
%token TOK_WINDOWINSTANCE

%token TOK_FUNCTION

%token TOK_MENU

%token TOK_BINDKEY
%token TOK_BINDBUTTON
%token TOK_BINDDRAG
%token TOK_BINDKEYRELEASE
%token TOK_UNBINDKEY
%token TOK_UNBINDBUTTON
%token TOK_UNBINDDRAG
%token TOK_UNBINDKEYRELEASE

%token TOK_ROOT
%token TOK_FRAME
%token TOK_TITLEBAR

%token TOK_SENDTOWORKSPACE
%token TOK_GOTOWORKSPACE
%token TOK_CYCLENEXT
%token TOK_CYCLEPREV
%token TOK_KILLNICELY
%token TOK_KILLWITHEXTREMEPREJUDICE
%token TOK_LAUNCH
%token TOK_FOCUS
%token TOK_MAXIMIZE
%token TOK_MAXIMIZE_HORIZONTALLY
%token TOK_MAXIMIZE_VERTICALLY
%token TOK_NOP
%token TOK_QUOTE
%token TOK_MOVEINTERACTIVELY
%token TOK_RESIZEINTERACTIVELY
%token TOK_MOVERESIZE
%token TOK_QUIT
%token TOK_BEEP
%token TOK_INVOKE
%token TOK_SHOWMENU
%token TOK_REFRESH

%token TOK_SEMI
%token TOK_EQUALS
%token TOK_SET_UNCONDITIONALLY
%token TOK_LBRACE
%token TOK_RBRACE
%token TOK_COMMA
%token TOK_LPAREN
%token TOK_RPAREN

%token TOK_DEFINE

%{
#include "prefs.h"
line *make_line(int type, void *dollar_one);
char *make_string(char *s);
%}

%union {
    int value_int;
    float value_float;
    char *value_string;
    line *value_line;
    option *value_option;
    context *value_context;
    type *value_type;
    keybinding *value_keybinding;
    keyunbinding *value_keyunbinding;
    mousebinding *value_mousebinding;
    mouseunbinding *value_mouseunbinding;
    function *value_function;
    arglist *value_arglist;
    definition *value_definition;
    funclist *value_funclist;
}

%token <value_string> TOK_STRING
%token <value_int> TOK_INTEGER
%token TOK_FLOAT

%type <value_int> boolean context_option context_name
%type <value_int> option_name focus_enumeration cycle_enumeration
%type <value_int> position_enumeration location function_name
%type <value_line> line config_file config
%type <value_option> option
%type <value_context> context
%type <value_type> type
%type <value_keybinding> keybinding
%type <value_keyunbinding> keyunbinding
%type <value_mousebinding> mousebinding
%type <value_mouseunbinding> mouseunbinding
%type <value_function> function;
%type <value_arglist> arglist;
%type <value_definition> definition;
%type <value_funclist> function_list;

%%

config_file: config { preferences = $1; }

config: /* empty */ { $$ = NULL; }
     | config line
       {
           line *tmp;
           parse_debug("CONFIG\n");
           if ($1 != NULL) {
               tmp = $1;
               while (tmp->line_next != NULL) tmp = tmp->line_next;
               tmp->line_next = $2;
               $$ = $1;
           } else {
               $$ = $2;
           }
       }

line: option TOK_SEMI { $$ = make_line(OPTION, $1); }
    | context { $$ = make_line(CONTEXT, $1); }
    | keybinding TOK_SEMI { $$ = make_line(KEYBINDING, $1); }
    | keyunbinding TOK_SEMI { $$ = make_line(KEYUNBINDING, $1); }
    | mousebinding TOK_SEMI { $$ = make_line(MOUSEBINDING, $1); }
    | mouseunbinding TOK_SEMI { $$ = make_line(MOUSEUNBINDING, $1); }
    | definition { $$ = make_line(DEFINITION, $1); }
    | TOK_SEMI { $$ = make_line(INVALID_LINE, NULL); }
    | error TOK_SEMI
      {
          extern int line_number;
          fprintf(stderr, "XWM: parse error on line %d.  Ignoring statement.\n",
                  line_number);
          $$ = make_line(INVALID_LINE, NULL);
      }
option: option_name TOK_EQUALS type
        {
            option *opt;
            parse_debug("OPTION\n");
            opt = malloc(sizeof(option));
            if (opt != NULL) {
                opt->option_name = $1;
                opt->option_value = $3;
                opt->option_setting = UserSet;
            }
            $$ = opt;
        }
      | option_name TOK_SET_UNCONDITIONALLY type
        {
            option *opt;
            parse_debug("OPTION\n");
            opt = malloc(sizeof(option));
            if (opt != NULL) {
                opt->option_name = $1;
                opt->option_value = $3;
                opt->option_setting = UserOverridden;
            }
            $$ = opt;
        }

/* ADDOPT 3: define as option */
option_name: TOK_DISPLAYTITLEBAR { $$ = DISPLAYTITLEBAR; }
           | TOK_OMNIPRESENT { $$ = OMNIPRESENT; }
           | TOK_DEFAULTWORKSPACE { $$ = DEFAULTWORKSPACE; }
           | TOK_FOCUS_POLICY { $$ = FOCUSPOLICY; }
           | TOK_ALWAYSONTOP { $$ = ALWAYSONTOP; }
           | TOK_ALWAYSONBOTTOM { $$ = ALWAYSONBOTTOM; }
           | TOK_PASSFOCUSCLICK { $$ = PASSFOCUSCLICK; }
           | TOK_CYCLEBEHAVIOUR { $$ = CYCLEBEHAVIOUR; }
           | TOK_COLORTITLEBAR { $$ = COLORTITLEBAR; }
           | TOK_COLORTITLEBARFOCUSED { $$ = COLORTITLEBARFOCUSED; }
           | TOK_COLORTEXT { $$ = COLORTEXT; }
           | TOK_COLORTEXTFOCUSED { $$ = COLORTEXTFOCUSED; }
           | TOK_NWORKSPACES { $$ = NWORKSPACES; }
           | TOK_DONTBINDMOUSE { $$ = DONTBINDMOUSE; }
           | TOK_DONTBINDKEYS { $$ = DONTBINDKEYS; }
           | TOK_STICKY { $$ = STICKY; }
           | TOK_TITLEPOSITION { $$ = TITLEPOSITION; }
           | TOK_KEEPTRANSIENTSONTOP { $$ = KEEPTRANSIENTSONTOP; }

type: boolean
      {
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = BOOLEAN;
              typ->type_value.intval = $1;
          }
          $$ = typ;
      }
    | TOK_STRING
      {
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = STRING;
              typ->type_value.stringval = make_string($1);
          }
          $$ = typ;
      }
    | TOK_INTEGER
      {
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = INTEGER;
              typ->type_value.intval = $1;
          }
          $$ = typ;
      }
    | focus_enumeration
      {
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = FOCUS_ENUM;
              typ->type_value.focus_enum = $1;
          }
          $$ = typ;
      }
    | cycle_enumeration
      {
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = CYCLE_ENUM;
              typ->type_value.cycle_enum = $1;
          }
          $$ = typ;
      }
    | position_enumeration
      {
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = POSITION_ENUM;
              typ->type_value.position_enum = $1;
          }
          $$ = typ;
      }

focus_enumeration: TOK_SLOPPY_FOCUS { $$ = TYPE_SLOPPY_FOCUS; }
                 | TOK_CLICK_TO_FOCUS { $$ = TYPE_CLICK_TO_FOCUS; }
                 | TOK_DONT_FOCUS { $$ = TYPE_DONT_FOCUS; }

cycle_enumeration: TOK_SKIPCYCLE { $$ = TYPE_SKIP_CYCLE }
                 | TOK_RAISEIMMEDIATELY { $$ = TYPE_RAISE_IMMEDIATELY }
                 | TOK_RAISEONCYCLEFINISH { $$ = TYPE_RAISE_ON_CYCLE_FINISH }
                 | TOK_DONTRAISE { $$ = TYPE_DONT_RAISE }

position_enumeration: TOK_DISPLAYLEFT { $$ = TYPE_DISPLAY_LEFT; }
                    | TOK_DISPLAYRIGHT { $$ = TYPE_DISPLAY_RIGHT; }
                    | TOK_DISPLAYCENTERED { $$ = TYPE_DISPLAY_CENTERED; }
                    | TOK_DONTDISPLAY { $$ = TYPE_DONT_DISPLAY; }    

boolean: TOK_TRUE { $$ = 1; }
       | TOK_FALSE { $$ = 0; }

context: context_option context_name type TOK_LBRACE config TOK_RBRACE
         {
             context *cntxt;
             parse_debug("CONTEXT\n");
             cntxt = malloc(sizeof(context));
             if (cntxt != NULL) {
                 cntxt->context_selector = $1 | $2;
                 cntxt->context_value = $3;
                 cntxt->context_lines = $5;
             }
             $$ = cntxt;
         }

context_name: TOK_ISSHAPED { $$ = SEL_ISSHAPED; }
            | TOK_INWORKSPACE { $$ = SEL_INWORKSPACE; }
            | TOK_WINDOWNAME { $$ = SEL_WINDOWNAME; }
            | TOK_WINDOWCLASS { $$ = SEL_WINDOWCLASS; }
            | TOK_WINDOWINSTANCE { $$ = SEL_WINDOWINSTANCE; }

context_option: /* empty */ { $$ = 0; }
              | TOK_NOT { $$ = SEL_NOT; }
              | TOK_TRANSIENTFOR { $$ = SEL_TRANSIENTFOR; }
              | TOK_HASTRANSIENT { $$ = SEL_HASTRANSIENT; }

keybinding: TOK_BINDKEY TOK_STRING function
            {
                keybinding *kb;
                kb = malloc(sizeof(keybinding));
                if (kb != NULL) {
                    kb->keybinding_string = make_string($2);
                    kb->keybinding_function = $3;
                    kb->keybinding_depress = KEYBOARD_DEPRESS;
                }
                $$ = kb;
            }
            | TOK_BINDKEYRELEASE TOK_STRING function
            {
                keybinding *kb;
                kb = malloc(sizeof(keybinding));
                if (kb != NULL) {
                    kb->keybinding_string = make_string($2);
                    kb->keybinding_function = $3;
                    kb->keybinding_depress = KEYBOARD_RELEASE;
                }
                $$ = kb;
            }

mousebinding: TOK_BINDBUTTON location TOK_STRING function
              {
                  mousebinding *mb;
                  mb = malloc(sizeof(mousebinding));
                  if (mb != NULL) {
                      mb->mousebinding_string = make_string($3);
                      mb->mousebinding_location = $2;
                      mb->mousebinding_function = $4;
                      mb->mousebinding_depress = MOUSE_RELEASE;
                  }
                  $$ = mb;
              }
              | TOK_BINDDRAG location TOK_STRING function
              {
                  mousebinding *mb;
                  mb = malloc(sizeof(mousebinding));
                  if (mb != NULL) {
                      mb->mousebinding_string = make_string($3);
                      mb->mousebinding_location = $2;
                      mb->mousebinding_function = $4;
                      mb->mousebinding_depress = MOUSE_DEPRESS;
                  }
                  $$ = mb;
              }
              
keyunbinding: TOK_UNBINDKEY TOK_STRING
              {
                  keyunbinding *kub;
                  kub = malloc(sizeof(keyunbinding));
                  if (kub != NULL) {
                      kub->keyunbinding_string = make_string($2);
                      kub->keyunbinding_depress = 1;
                  }
                  $$ = kub;
              }
              | TOK_UNBINDKEYRELEASE TOK_STRING
              {
                  keyunbinding *kub;
                  kub = malloc(sizeof(keyunbinding));
                  if (kub != NULL) {
                      kub->keyunbinding_string = make_string($2);
                      kub->keyunbinding_depress = 0;
                  }
                  $$ = kub;
              }

mouseunbinding: TOK_UNBINDBUTTON location TOK_STRING
                {
                    mouseunbinding *mub;
                    mub = malloc(sizeof(mouseunbinding));
                    if (mub != NULL) {
                        mub->mouseunbinding_string = make_string($3);
                        mub->mouseunbinding_location = $2;
                        mub->mouseunbinding_depress = 0;
                    }
                    $$ = mub;
                }
                | TOK_UNBINDDRAG location TOK_STRING
                {
                    mouseunbinding *mub;
                    mub = malloc(sizeof(mouseunbinding));
                    if (mub != NULL) {
                        mub->mouseunbinding_string = make_string($3);
                        mub->mouseunbinding_location = $2;
                        mub->mouseunbinding_depress = 1;
                    }
                    $$ = mub;
                }

location: TOK_ROOT { $$ = MOUSE_ROOT; }
        | TOK_FRAME { $$ = MOUSE_FRAME; }
        | TOK_TITLEBAR { $$ = MOUSE_TITLEBAR; }

function: function_name TOK_LPAREN arglist TOK_RPAREN
          {
              function *f = malloc(sizeof(function));
              if (f != NULL) {
                  f->function_type = $1;
                  f->function_args = $3;
              }
              $$ = f;
          }
function: function_name TOK_LPAREN TOK_RPAREN
          {
              function *f = malloc(sizeof(function));
              if (f != NULL) {
                  f->function_type = $1;
                  f->function_args = NULL;
              }
              $$ = f;
          }

function_name: TOK_SENDTOWORKSPACE { $$ = SENDTOWORKSPACE; }
             | TOK_GOTOWORKSPACE { $$ = GOTOWORKSPACE; }
             | TOK_CYCLENEXT { $$ = CYCLENEXT; }
             | TOK_CYCLEPREV { $$ = CYCLEPREV; }
             | TOK_KILLNICELY { $$ = KILLNICELY; }
             | TOK_KILLWITHEXTREMEPREJUDICE { $$ = KILLWITHEXTREMEPREJUDICE; }
             | TOK_LAUNCH { $$ = LAUNCH; }
             | TOK_FOCUS { $$ = FOCUS; }
             | TOK_MAXIMIZE { $$ = MAXIMIZE; }
             | TOK_MAXIMIZE_HORIZONTALLY { $$ = MAXIMIZE_H; }
             | TOK_MAXIMIZE_VERTICALLY { $$ = MAXIMIZE_V; }
             | TOK_NOP { $$ = NOP; }
             | TOK_QUOTE { $$ = QUOTE; }
             | TOK_MOVEINTERACTIVELY { $$ = MOVEINTERACTIVELY; }
             | TOK_RESIZEINTERACTIVELY { $$ = RESIZEINTERACTIVELY; }
             | TOK_MOVERESIZE { $$ = MOVERESIZE; }
             | TOK_QUIT { $$ = QUIT; }
             | TOK_BEEP { $$ = BEEP; }
             | TOK_INVOKE { $$ = INVOKE; }
             | TOK_SHOWMENU { $$ = SHOWMENU; }
             | TOK_REFRESH { $$ = REFRESH; }

arglist: arglist TOK_COMMA type
         {
             arglist *tmp;
             arglist *al;
             if ($1 != NULL) {
                 al = malloc(sizeof(arglist));
                 if (al != NULL) {
                     al->arglist_arg = $3;
                     al->arglist_next = NULL;
                     tmp = $1;
                     while (tmp->arglist_next != NULL)
                         tmp = tmp->arglist_next;
                     tmp->arglist_next = al;
                 }
             }
             $$ = $1;
         }
       | type
         {
             arglist *al = malloc(sizeof(arglist));
             if (al != NULL) {
                 al->arglist_arg = $1;
                 al->arglist_next = NULL;
             }
             $$ = al;
         }

definition: TOK_DEFINE TOK_STRING TOK_LBRACE function_list TOK_RBRACE
            {
                definition *def = malloc(sizeof(definition));
                if (def != NULL) {
                    def->funclist = $4;
                    def->identifier = make_string($2);
                }
                $$ = def;
            }

function_list: function TOK_SEMI function_list
               {
                   funclist *fl = malloc(sizeof(funclist));
                   if (fl != NULL) {
                       fl->func = $1;
                       fl->next = $3;
                   }
                   $$ = fl;
               }
             | function TOK_SEMI
               {
                   funclist *fl = malloc(sizeof(funclist));
                   if (fl != NULL) {
                       fl->func = $1;
                       fl->next = NULL;
                   }
                   $$ = fl;
               }

%%

/*
 * Changes:
 * "blah \"blah" foo qux
 * To:
 * blah "blah
 * Returns newly-malloced string (which is perhaps over-malloced)
 */
char *make_string(char *s)
{
    char *n, *np, *sp, c;

    parse_debug(("String is '%s'\n", s));
    assert(s[0] == '"');
    n = malloc(strlen(s) - 2 + 1); /* minus quotes, plus NUL */
    if (n == NULL) return NULL;
    c = '\0';                   /* c is previous char examined */
    np = n;
    for (sp = s+1; *sp != '\0'; sp++) {
        if (c == '\\') {
            /* deal with escape characters */
            switch (*sp) {
                case 'n':
                    *np++ = '\n';
                    break;
                case 'a':
                    *np++ = '\a';
                    break;
                case 'b':
                    *np++ = '\b';
                    break;
                case 'r':
                    *np++ = '\r';
                    break;
                case 't':
                    *np++ = '\t';
                    break;
                case 'v':
                    *np++ = '\v';
                    break;
                case '"':
                    *np++ = '"';
                    break;
                default:
                    *np++ = '\\';
                    *np++ = *sp;
            }
        } else if (*sp == '"') {
            break;
        } else if (*sp == '\\') {
            ;
        } else {
            *np++ = *sp;
        }
        c = *sp;
    }
    *np = '\0';
    parse_debug(("String is now '%s'\n", n));
    return n;
}
       
line *make_line(int type, void *dollar_one)
{
    line *ln;
    parse_debug("LINE\n");
    ln = malloc(sizeof(line));
    if (ln != NULL) {
        ln->line_type = type;
        ln->line_value.option = dollar_one;
        ln->line_next = NULL;
    }
    return ln;
}
              
int yyerror(char *s)
{
    fprintf(stderr, "%s\n", s);
}

line *preferences = NULL;

#ifdef PARSER_DEBUG
int main(int argc, char **argv)
{
    yyparse();
    printf("preferences = 0x%lx\n", preferences);
}
#endif
