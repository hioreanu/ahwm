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
#define	TOK_KEEPTRANSIENTSONTOP	274
#define	TOK_SLOPPY_FOCUS	275
#define	TOK_CLICK_TO_FOCUS	276
#define	TOK_DONT_FOCUS	277
#define	TOK_SKIPCYCLE	278
#define	TOK_RAISEIMMEDIATELY	279
#define	TOK_RAISEONCYCLEFINISH	280
#define	TOK_DONTRAISE	281
#define	TOK_DISPLAYLEFT	282
#define	TOK_DISPLAYRIGHT	283
#define	TOK_DISPLAYCENTERED	284
#define	TOK_DONTDISPLAY	285
#define	TOK_TRUE	286
#define	TOK_FALSE	287
#define	TOK_TRANSIENTFOR	288
#define	TOK_HASTRANSIENT	289
#define	TOK_NOT	290
#define	TOK_ISSHAPED	291
#define	TOK_INWORKSPACE	292
#define	TOK_WINDOWCLASS	293
#define	TOK_WINDOWNAME	294
#define	TOK_WINDOWINSTANCE	295
#define	TOK_FUNCTION	296
#define	TOK_MENU	297
#define	TOK_BINDKEY	298
#define	TOK_BINDBUTTON	299
#define	TOK_BINDDRAG	300
#define	TOK_BINDKEYRELEASE	301
#define	TOK_UNBINDKEY	302
#define	TOK_UNBINDBUTTON	303
#define	TOK_UNBINDDRAG	304
#define	TOK_UNBINDKEYRELEASE	305
#define	TOK_ROOT	306
#define	TOK_FRAME	307
#define	TOK_TITLEBAR	308
#define	TOK_SENDTOWORKSPACE	309
#define	TOK_GOTOWORKSPACE	310
#define	TOK_CYCLENEXT	311
#define	TOK_CYCLEPREV	312
#define	TOK_KILLNICELY	313
#define	TOK_KILLWITHEXTREMEPREJUDICE	314
#define	TOK_LAUNCH	315
#define	TOK_FOCUS	316
#define	TOK_MAXIMIZE	317
#define	TOK_MAXIMIZE_HORIZONTALLY	318
#define	TOK_MAXIMIZE_VERTICALLY	319
#define	TOK_NOP	320
#define	TOK_QUOTE	321
#define	TOK_MOVEINTERACTIVELY	322
#define	TOK_RESIZEINTERACTIVELY	323
#define	TOK_MOVERESIZE	324
#define	TOK_QUIT	325
#define	TOK_BEEP	326
#define	TOK_INVOKE	327
#define	TOK_SHOWMENU	328
#define	TOK_REFRESH	329
#define	TOK_SEMI	330
#define	TOK_EQUALS	331
#define	TOK_SET_UNCONDITIONALLY	332
#define	TOK_LBRACE	333
#define	TOK_RBRACE	334
#define	TOK_COMMA	335
#define	TOK_LPAREN	336
#define	TOK_RPAREN	337
#define	TOK_DEFINE	338
#define	TOK_STRING	339
#define	TOK_INTEGER	340
#define	TOK_FLOAT	341


extern YYSTYPE yylval;
