typedef union {
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
} YYSTYPE;
#define	TOK_DISPLAYTITLEBAR	257
#define	TOK_OMNIPRESENT	258
#define	TOK_SKIP_ALT_TAB	259
#define	TOK_DEFAULTWORKSPACE	260
#define	TOK_NUMBEROFWORKSPACES	261
#define	TOK_FOCUS_POLICY	262
#define	TOK_SLOPPY_FOCUS	263
#define	TOK_CLICK_TO_FOCUS	264
#define	TOK_DONT_FOCUS	265
#define	TOK_TRUE	266
#define	TOK_FALSE	267
#define	TOK_TRANSIENTFOR	268
#define	TOK_HASTRANSIENT	269
#define	TOK_NOT	270
#define	TOK_ISSHAPED	271
#define	TOK_INWORKSPACE	272
#define	TOK_WINDOWCLASS	273
#define	TOK_WINDOWNAME	274
#define	TOK_WINDOWINSTANCE	275
#define	TOK_FUNCTION	276
#define	TOK_MENU	277
#define	TOK_BINDKEY	278
#define	TOK_BINDBUTTON	279
#define	TOK_BINDDRAG	280
#define	TOK_BINDKEYRELEASE	281
#define	TOK_UNBINDKEY	282
#define	TOK_UNBINDBUTTON	283
#define	TOK_UNBINDDRAG	284
#define	TOK_UNBINDKEYRELEASE	285
#define	TOK_ROOT	286
#define	TOK_FRAME	287
#define	TOK_TITLEBAR	288
#define	TOK_MOVETOWORKSPACE	289
#define	TOK_GOTOWORKSPACE	290
#define	TOK_ALTTAB	291
#define	TOK_KILLNICELY	292
#define	TOK_KILLWITHEXTREMEPREJUDICE	293
#define	TOK_LAUNCH	294
#define	TOK_FOCUS	295
#define	TOK_MAXIMIZE	296
#define	TOK_NOP	297
#define	TOK_QUOTE	298
#define	TOK_MOVEINTERACTIVELY	299
#define	TOK_RESIZEINTERACTIVELY	300
#define	TOK_MOVERESIZE	301
#define	TOK_QUIT	302
#define	TOK_BEEP	303
#define	TOK_INVOKE	304
#define	TOK_SHOWMENU	305
#define	TOK_REFRESH	306
#define	TOK_SEMI	307
#define	TOK_EQUALS	308
#define	TOK_LBRACE	309
#define	TOK_RBRACE	310
#define	TOK_COMMA	311
#define	TOK_LPAREN	312
#define	TOK_RPAREN	313
#define	TOK_STRING	314
#define	TOK_INTEGER	315
#define	TOK_FLOAT	316


extern YYSTYPE yylval;
