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
#define	TOK_TITLEPOSITION	273
#define	TOK_SLOPPY_FOCUS	274
#define	TOK_CLICK_TO_FOCUS	275
#define	TOK_DONT_FOCUS	276
#define	TOK_SKIPCYCLE	277
#define	TOK_RAISEIMMEDIATELY	278
#define	TOK_RAISEONCYCLEFINISH	279
#define	TOK_DONTRAISE	280
#define	TOK_DISPLAYLEFT	281
#define	TOK_DISPLAYRIGHT	282
#define	TOK_DISPLAYCENTERED	283
#define	TOK_DONTDISPLAY	284
#define	TOK_TRUE	285
#define	TOK_FALSE	286
#define	TOK_TRANSIENTFOR	287
#define	TOK_HASTRANSIENT	288
#define	TOK_NOT	289
#define	TOK_ISSHAPED	290
#define	TOK_INWORKSPACE	291
#define	TOK_WINDOWCLASS	292
#define	TOK_WINDOWNAME	293
#define	TOK_WINDOWINSTANCE	294
#define	TOK_FUNCTION	295
#define	TOK_MENU	296
#define	TOK_BINDKEY	297
#define	TOK_BINDBUTTON	298
#define	TOK_BINDDRAG	299
#define	TOK_BINDKEYRELEASE	300
#define	TOK_UNBINDKEY	301
#define	TOK_UNBINDBUTTON	302
#define	TOK_UNBINDDRAG	303
#define	TOK_UNBINDKEYRELEASE	304
#define	TOK_ROOT	305
#define	TOK_FRAME	306
#define	TOK_TITLEBAR	307
#define	TOK_SENDTOWORKSPACE	308
#define	TOK_GOTOWORKSPACE	309
#define	TOK_CYCLENEXT	310
#define	TOK_CYCLEPREV	311
#define	TOK_KILLNICELY	312
#define	TOK_KILLWITHEXTREMEPREJUDICE	313
#define	TOK_LAUNCH	314
#define	TOK_FOCUS	315
#define	TOK_MAXIMIZE	316
#define	TOK_MAXIMIZE_HORIZONTALLY	317
#define	TOK_MAXIMIZE_VERTICALLY	318
#define	TOK_NOP	319
#define	TOK_QUOTE	320
#define	TOK_MOVEINTERACTIVELY	321
#define	TOK_RESIZEINTERACTIVELY	322
#define	TOK_MOVERESIZE	323
#define	TOK_QUIT	324
#define	TOK_BEEP	325
#define	TOK_INVOKE	326
#define	TOK_SHOWMENU	327
#define	TOK_REFRESH	328
#define	TOK_SEMI	329
#define	TOK_EQUALS	330
#define	TOK_SET_UNCONDITIONALLY	331
#define	TOK_LBRACE	332
#define	TOK_RBRACE	333
#define	TOK_COMMA	334
#define	TOK_LPAREN	335
#define	TOK_RPAREN	336
#define	TOK_DEFINE	337
#define	TOK_STRING	338
#define	TOK_INTEGER	339
#define	TOK_FLOAT	340


extern YYSTYPE yylval;
