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
#define	TOK_TITLEBARFONT	258
#define	TOK_OMNIPRESENT	259
#define	TOK_DEFAULTWORKSPACE	260
#define	TOK_FOCUS_POLICY	261
#define	TOK_ALWAYSONTOP	262
#define	TOK_ALWAYSONBOTTOM	263
#define	TOK_PASSFOCUSCLICK	264
#define	TOK_CYCLEBEHAVIOUR	265
#define	TOK_COLORTITLEBAR	266
#define	TOK_COLORTITLEBARFOCUSED	267
#define	TOK_COLORTEXT	268
#define	TOK_COLORTEXTFOCUSED	269
#define	TOK_NWORKSPACES	270
#define	TOK_DONTBINDMOUSE	271
#define	TOK_DONTBINDKEYS	272
#define	TOK_STICKY	273
#define	TOK_TITLEPOSITION	274
#define	TOK_KEEPTRANSIENTSONTOP	275
#define	TOK_SLOPPY_FOCUS	276
#define	TOK_CLICK_TO_FOCUS	277
#define	TOK_DONT_FOCUS	278
#define	TOK_SKIPCYCLE	279
#define	TOK_RAISEIMMEDIATELY	280
#define	TOK_RAISEONCYCLEFINISH	281
#define	TOK_DONTRAISE	282
#define	TOK_DISPLAYLEFT	283
#define	TOK_DISPLAYRIGHT	284
#define	TOK_DISPLAYCENTERED	285
#define	TOK_DONTDISPLAY	286
#define	TOK_TRUE	287
#define	TOK_FALSE	288
#define	TOK_TRANSIENTFOR	289
#define	TOK_HASTRANSIENT	290
#define	TOK_NOT	291
#define	TOK_ISSHAPED	292
#define	TOK_INWORKSPACE	293
#define	TOK_WINDOWCLASS	294
#define	TOK_WINDOWNAME	295
#define	TOK_WINDOWINSTANCE	296
#define	TOK_FUNCTION	297
#define	TOK_MENU	298
#define	TOK_BINDKEY	299
#define	TOK_BINDBUTTON	300
#define	TOK_BINDDRAG	301
#define	TOK_BINDKEYRELEASE	302
#define	TOK_UNBINDKEY	303
#define	TOK_UNBINDBUTTON	304
#define	TOK_UNBINDDRAG	305
#define	TOK_UNBINDKEYRELEASE	306
#define	TOK_ROOT	307
#define	TOK_FRAME	308
#define	TOK_TITLEBAR	309
#define	TOK_SENDTOWORKSPACE	310
#define	TOK_GOTOWORKSPACE	311
#define	TOK_CYCLENEXT	312
#define	TOK_CYCLEPREV	313
#define	TOK_KILLNICELY	314
#define	TOK_KILLWITHEXTREMEPREJUDICE	315
#define	TOK_LAUNCH	316
#define	TOK_FOCUS	317
#define	TOK_MAXIMIZE	318
#define	TOK_MAXIMIZE_HORIZONTALLY	319
#define	TOK_MAXIMIZE_VERTICALLY	320
#define	TOK_NOP	321
#define	TOK_QUOTE	322
#define	TOK_MOVEINTERACTIVELY	323
#define	TOK_RESIZEINTERACTIVELY	324
#define	TOK_MOVERESIZE	325
#define	TOK_QUIT	326
#define	TOK_BEEP	327
#define	TOK_INVOKE	328
#define	TOK_SHOWMENU	329
#define	TOK_REFRESH	330
#define	TOK_RESTART	331
#define	TOK_SEMI	332
#define	TOK_EQUALS	333
#define	TOK_SET_UNCONDITIONALLY	334
#define	TOK_LBRACE	335
#define	TOK_RBRACE	336
#define	TOK_COMMA	337
#define	TOK_LPAREN	338
#define	TOK_RPAREN	339
#define	TOK_DEFINE	340
#define	TOK_STRING	341
#define	TOK_INTEGER	342
#define	TOK_FLOAT	343


extern YYSTYPE yylval;
