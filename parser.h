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
#define	TOK_STICKY	272
#define	TOK_SLOPPY_FOCUS	273
#define	TOK_CLICK_TO_FOCUS	274
#define	TOK_DONT_FOCUS	275
#define	TOK_SKIPCYCLE	276
#define	TOK_RAISEIMMEDIATELY	277
#define	TOK_RAISEONCYCLEFINISH	278
#define	TOK_DONTRAISE	279
#define	TOK_TRUE	280
#define	TOK_FALSE	281
#define	TOK_TRANSIENTFOR	282
#define	TOK_HASTRANSIENT	283
#define	TOK_NOT	284
#define	TOK_ISSHAPED	285
#define	TOK_INWORKSPACE	286
#define	TOK_WINDOWCLASS	287
#define	TOK_WINDOWNAME	288
#define	TOK_WINDOWINSTANCE	289
#define	TOK_FUNCTION	290
#define	TOK_MENU	291
#define	TOK_BINDKEY	292
#define	TOK_BINDBUTTON	293
#define	TOK_BINDDRAG	294
#define	TOK_BINDKEYRELEASE	295
#define	TOK_UNBINDKEY	296
#define	TOK_UNBINDBUTTON	297
#define	TOK_UNBINDDRAG	298
#define	TOK_UNBINDKEYRELEASE	299
#define	TOK_ROOT	300
#define	TOK_FRAME	301
#define	TOK_TITLEBAR	302
#define	TOK_SENDTOWORKSPACE	303
#define	TOK_GOTOWORKSPACE	304
#define	TOK_CYCLENEXT	305
#define	TOK_CYCLEPREV	306
#define	TOK_KILLNICELY	307
#define	TOK_KILLWITHEXTREMEPREJUDICE	308
#define	TOK_LAUNCH	309
#define	TOK_FOCUS	310
#define	TOK_MAXIMIZE	311
#define	TOK_MAXIMIZE_HORIZONTALLY	312
#define	TOK_MAXIMIZE_VERTICALLY	313
#define	TOK_NOP	314
#define	TOK_QUOTE	315
#define	TOK_MOVEINTERACTIVELY	316
#define	TOK_RESIZEINTERACTIVELY	317
#define	TOK_MOVERESIZE	318
#define	TOK_QUIT	319
#define	TOK_BEEP	320
#define	TOK_INVOKE	321
#define	TOK_SHOWMENU	322
#define	TOK_REFRESH	323
#define	TOK_SEMI	324
#define	TOK_EQUALS	325
#define	TOK_SET_UNCONDITIONALLY	326
#define	TOK_LBRACE	327
#define	TOK_RBRACE	328
#define	TOK_COMMA	329
#define	TOK_LPAREN	330
#define	TOK_RPAREN	331
#define	TOK_DEFINE	332
#define	TOK_STRING	333
#define	TOK_INTEGER	334
#define	TOK_FLOAT	335


extern YYSTYPE yylval;
