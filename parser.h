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
#define	TOK_DEFAULTWORKSPACE	259
#define	TOK_NUMBEROFWORKSPACES	260
#define	TOK_FOCUS_POLICY	261
#define	TOK_ALWAYSONTOP	262
#define	TOK_ALWAYSONBOTTOM	263
#define	TOK_PASSFOCUSCLICK	264
#define	TOK_CYCLEBEHAVIOUR	265
#define	TOK_SLOPPY_FOCUS	266
#define	TOK_CLICK_TO_FOCUS	267
#define	TOK_DONT_FOCUS	268
#define	TOK_SKIPCYCLE	269
#define	TOK_RAISEIMMEDIATELY	270
#define	TOK_RAISEONCYCLEFINISH	271
#define	TOK_DONTRAISE	272
#define	TOK_TRUE	273
#define	TOK_FALSE	274
#define	TOK_TRANSIENTFOR	275
#define	TOK_HASTRANSIENT	276
#define	TOK_NOT	277
#define	TOK_ISSHAPED	278
#define	TOK_INWORKSPACE	279
#define	TOK_WINDOWCLASS	280
#define	TOK_WINDOWNAME	281
#define	TOK_WINDOWINSTANCE	282
#define	TOK_FUNCTION	283
#define	TOK_MENU	284
#define	TOK_BINDKEY	285
#define	TOK_BINDBUTTON	286
#define	TOK_BINDDRAG	287
#define	TOK_BINDKEYRELEASE	288
#define	TOK_UNBINDKEY	289
#define	TOK_UNBINDBUTTON	290
#define	TOK_UNBINDDRAG	291
#define	TOK_UNBINDKEYRELEASE	292
#define	TOK_ROOT	293
#define	TOK_FRAME	294
#define	TOK_TITLEBAR	295
#define	TOK_SENDTOWORKSPACE	296
#define	TOK_GOTOWORKSPACE	297
#define	TOK_ALTTAB	298
#define	TOK_KILLNICELY	299
#define	TOK_KILLWITHEXTREMEPREJUDICE	300
#define	TOK_LAUNCH	301
#define	TOK_FOCUS	302
#define	TOK_MAXIMIZE	303
#define	TOK_NOP	304
#define	TOK_QUOTE	305
#define	TOK_MOVEINTERACTIVELY	306
#define	TOK_RESIZEINTERACTIVELY	307
#define	TOK_MOVERESIZE	308
#define	TOK_QUIT	309
#define	TOK_BEEP	310
#define	TOK_INVOKE	311
#define	TOK_SHOWMENU	312
#define	TOK_REFRESH	313
#define	TOK_SEMI	314
#define	TOK_EQUALS	315
#define	TOK_LBRACE	316
#define	TOK_RBRACE	317
#define	TOK_COMMA	318
#define	TOK_LPAREN	319
#define	TOK_RPAREN	320
#define	TOK_STRING	321
#define	TOK_INTEGER	322
#define	TOK_FLOAT	323


extern YYSTYPE yylval;
