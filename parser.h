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
#define	TOK_COLORTITLEBAR	266
#define	TOK_COLORTITLEBARFOCUSED	267
#define	TOK_COLORTEXT	268
#define	TOK_COLORTEXTFOCUSED	269
#define	TOK_SLOPPY_FOCUS	270
#define	TOK_CLICK_TO_FOCUS	271
#define	TOK_DONT_FOCUS	272
#define	TOK_SKIPCYCLE	273
#define	TOK_RAISEIMMEDIATELY	274
#define	TOK_RAISEONCYCLEFINISH	275
#define	TOK_DONTRAISE	276
#define	TOK_TRUE	277
#define	TOK_FALSE	278
#define	TOK_TRANSIENTFOR	279
#define	TOK_HASTRANSIENT	280
#define	TOK_NOT	281
#define	TOK_ISSHAPED	282
#define	TOK_INWORKSPACE	283
#define	TOK_WINDOWCLASS	284
#define	TOK_WINDOWNAME	285
#define	TOK_WINDOWINSTANCE	286
#define	TOK_FUNCTION	287
#define	TOK_MENU	288
#define	TOK_BINDKEY	289
#define	TOK_BINDBUTTON	290
#define	TOK_BINDDRAG	291
#define	TOK_BINDKEYRELEASE	292
#define	TOK_UNBINDKEY	293
#define	TOK_UNBINDBUTTON	294
#define	TOK_UNBINDDRAG	295
#define	TOK_UNBINDKEYRELEASE	296
#define	TOK_ROOT	297
#define	TOK_FRAME	298
#define	TOK_TITLEBAR	299
#define	TOK_SENDTOWORKSPACE	300
#define	TOK_GOTOWORKSPACE	301
#define	TOK_ALTTAB	302
#define	TOK_KILLNICELY	303
#define	TOK_KILLWITHEXTREMEPREJUDICE	304
#define	TOK_LAUNCH	305
#define	TOK_FOCUS	306
#define	TOK_MAXIMIZE	307
#define	TOK_NOP	308
#define	TOK_QUOTE	309
#define	TOK_MOVEINTERACTIVELY	310
#define	TOK_RESIZEINTERACTIVELY	311
#define	TOK_MOVERESIZE	312
#define	TOK_QUIT	313
#define	TOK_BEEP	314
#define	TOK_INVOKE	315
#define	TOK_SHOWMENU	316
#define	TOK_REFRESH	317
#define	TOK_SEMI	318
#define	TOK_EQUALS	319
#define	TOK_LBRACE	320
#define	TOK_RBRACE	321
#define	TOK_COMMA	322
#define	TOK_LPAREN	323
#define	TOK_RPAREN	324
#define	TOK_STRING	325
#define	TOK_INTEGER	326
#define	TOK_FLOAT	327


extern YYSTYPE yylval;
