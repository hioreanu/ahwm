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
#define	TOK_DEFAULTWORKSPACE	258
#define	TOK_NUMBEROFWORKSPACES	259
#define	TOK_TRUE	260
#define	TOK_FALSE	261
#define	TOK_TRANSIENTFOR	262
#define	TOK_HASTRANSIENT	263
#define	TOK_NOT	264
#define	TOK_ISSHAPED	265
#define	TOK_INWORKSPACE	266
#define	TOK_WINDOWCLASS	267
#define	TOK_WINDOWNAME	268
#define	TOK_WINDOWINSTANCE	269
#define	TOK_FUNCTION	270
#define	TOK_MENU	271
#define	TOK_BINDKEY	272
#define	TOK_BINDBUTTON	273
#define	TOK_UNBINDKEY	274
#define	TOK_UNBINDBUTTON	275
#define	TOK_ROOT	276
#define	TOK_FRAME	277
#define	TOK_TITLEBAR	278
#define	TOK_MOVETOWORKSPACE	279
#define	TOK_GOTOWORKSPACE	280
#define	TOK_ALTTAB	281
#define	TOK_KILLNICELY	282
#define	TOK_KILLWITHEXTREMEPREJUDICE	283
#define	TOK_LAUNCH	284
#define	TOK_FOCUS	285
#define	TOK_MAXIMIZE	286
#define	TOK_NOP	287
#define	TOK_QUOTE	288
#define	TOK_MOVEINTERACTIVELY	289
#define	TOK_RESIZEINTERACTIVELY	290
#define	TOK_MOVERESIZE	291
#define	TOK_QUIT	292
#define	TOK_BEEP	293
#define	TOK_INVOKE	294
#define	TOK_SHOWMENU	295
#define	TOK_REFRESH	296
#define	TOK_SEMI	297
#define	TOK_EQUALS	298
#define	TOK_LBRACE	299
#define	TOK_RBRACE	300
#define	TOK_COMMA	301
#define	TOK_LPAREN	302
#define	TOK_RPAREN	303
#define	TOK_STRING	304
#define	TOK_INTEGER	305
#define	TOK_FLOAT	306


extern YYSTYPE yylval;
