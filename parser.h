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
#define	TOK_ALWAYSONTOP	263
#define	TOK_ALWAYSONBOTTOM	264
#define	TOK_PASSFOCUSCLICK	265
#define	TOK_SLOPPY_FOCUS	266
#define	TOK_CLICK_TO_FOCUS	267
#define	TOK_DONT_FOCUS	268
#define	TOK_TRUE	269
#define	TOK_FALSE	270
#define	TOK_TRANSIENTFOR	271
#define	TOK_HASTRANSIENT	272
#define	TOK_NOT	273
#define	TOK_ISSHAPED	274
#define	TOK_INWORKSPACE	275
#define	TOK_WINDOWCLASS	276
#define	TOK_WINDOWNAME	277
#define	TOK_WINDOWINSTANCE	278
#define	TOK_FUNCTION	279
#define	TOK_MENU	280
#define	TOK_BINDKEY	281
#define	TOK_BINDBUTTON	282
#define	TOK_BINDDRAG	283
#define	TOK_BINDKEYRELEASE	284
#define	TOK_UNBINDKEY	285
#define	TOK_UNBINDBUTTON	286
#define	TOK_UNBINDDRAG	287
#define	TOK_UNBINDKEYRELEASE	288
#define	TOK_ROOT	289
#define	TOK_FRAME	290
#define	TOK_TITLEBAR	291
#define	TOK_SENDTOWORKSPACE	292
#define	TOK_GOTOWORKSPACE	293
#define	TOK_ALTTAB	294
#define	TOK_KILLNICELY	295
#define	TOK_KILLWITHEXTREMEPREJUDICE	296
#define	TOK_LAUNCH	297
#define	TOK_FOCUS	298
#define	TOK_MAXIMIZE	299
#define	TOK_NOP	300
#define	TOK_QUOTE	301
#define	TOK_MOVEINTERACTIVELY	302
#define	TOK_RESIZEINTERACTIVELY	303
#define	TOK_MOVERESIZE	304
#define	TOK_QUIT	305
#define	TOK_BEEP	306
#define	TOK_INVOKE	307
#define	TOK_SHOWMENU	308
#define	TOK_REFRESH	309
#define	TOK_SEMI	310
#define	TOK_EQUALS	311
#define	TOK_LBRACE	312
#define	TOK_RBRACE	313
#define	TOK_COMMA	314
#define	TOK_LPAREN	315
#define	TOK_RPAREN	316
#define	TOK_STRING	317
#define	TOK_INTEGER	318
#define	TOK_FLOAT	319


extern YYSTYPE yylval;
