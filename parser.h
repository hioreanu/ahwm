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
    definition *value_definition;
    funclist *value_funclist;
} YYSTYPE;
#define	TOK_DISPLAYTITLEBAR	257
#define	TOK_OMNIPRESENT	258
#define	TOK_DEFAULTWORKSPACE	259
#define	TOK_FOCUS_POLICY	260
#define	TOK_ALWAYSONTOP	261
#define	TOK_ALWAYSONBOTTOM	262
#define	TOK_PASSFOCUSCLICK	263
#define	TOK_CYCLEBEHAVIOUR	264
#define	TOK_COLORTITLEBAR	265
#define	TOK_COLORTITLEBARFOCUSED	266
#define	TOK_COLORTEXT	267
#define	TOK_COLORTEXTFOCUSED	268
#define	TOK_NWORKSPACES	269
#define	TOK_DONTBINDMOUSE	270
#define	TOK_DONTBINDKEYS	271
#define	TOK_SLOPPY_FOCUS	272
#define	TOK_CLICK_TO_FOCUS	273
#define	TOK_DONT_FOCUS	274
#define	TOK_SKIPCYCLE	275
#define	TOK_RAISEIMMEDIATELY	276
#define	TOK_RAISEONCYCLEFINISH	277
#define	TOK_DONTRAISE	278
#define	TOK_TRUE	279
#define	TOK_FALSE	280
#define	TOK_TRANSIENTFOR	281
#define	TOK_HASTRANSIENT	282
#define	TOK_NOT	283
#define	TOK_ISSHAPED	284
#define	TOK_INWORKSPACE	285
#define	TOK_WINDOWCLASS	286
#define	TOK_WINDOWNAME	287
#define	TOK_WINDOWINSTANCE	288
#define	TOK_FUNCTION	289
#define	TOK_MENU	290
#define	TOK_BINDKEY	291
#define	TOK_BINDBUTTON	292
#define	TOK_BINDDRAG	293
#define	TOK_BINDKEYRELEASE	294
#define	TOK_UNBINDKEY	295
#define	TOK_UNBINDBUTTON	296
#define	TOK_UNBINDDRAG	297
#define	TOK_UNBINDKEYRELEASE	298
#define	TOK_ROOT	299
#define	TOK_FRAME	300
#define	TOK_TITLEBAR	301
#define	TOK_SENDTOWORKSPACE	302
#define	TOK_GOTOWORKSPACE	303
#define	TOK_CYCLENEXT	304
#define	TOK_CYCLEPREV	305
#define	TOK_KILLNICELY	306
#define	TOK_KILLWITHEXTREMEPREJUDICE	307
#define	TOK_LAUNCH	308
#define	TOK_FOCUS	309
#define	TOK_MAXIMIZE	310
#define	TOK_MAXIMIZE_HORIZONTALLY	311
#define	TOK_MAXIMIZE_VERTICALLY	312
#define	TOK_NOP	313
#define	TOK_QUOTE	314
#define	TOK_MOVEINTERACTIVELY	315
#define	TOK_RESIZEINTERACTIVELY	316
#define	TOK_MOVERESIZE	317
#define	TOK_QUIT	318
#define	TOK_BEEP	319
#define	TOK_INVOKE	320
#define	TOK_SHOWMENU	321
#define	TOK_REFRESH	322
#define	TOK_SEMI	323
#define	TOK_EQUALS	324
#define	TOK_SET_UNCONDITIONALLY	325
#define	TOK_LBRACE	326
#define	TOK_RBRACE	327
#define	TOK_COMMA	328
#define	TOK_LPAREN	329
#define	TOK_RPAREN	330
#define	TOK_DEFINE	331
#define	TOK_STRING	332
#define	TOK_INTEGER	333
#define	TOK_FLOAT	334


extern YYSTYPE yylval;
