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
#define	TOK_TRUE	259
#define	TOK_FALSE	260
#define	TOK_TRANSIENTFOR	261
#define	TOK_HASTRANSIENT	262
#define	TOK_NOT	263
#define	TOK_ISSHAPED	264
#define	TOK_INWORKSPACE	265
#define	TOK_WINDOWCLASS	266
#define	TOK_WINDOWNAME	267
#define	TOK_WINDOWINSTANCE	268
#define	TOK_FUNCTION	269
#define	TOK_MENU	270
#define	TOK_BINDKEY	271
#define	TOK_BINDBUTTON	272
#define	TOK_UNBINDKEY	273
#define	TOK_UNBINDBUTTON	274
#define	TOK_ROOT	275
#define	TOK_FRAME	276
#define	TOK_TITLEBAR	277
#define	TOK_MOVETOWORKSPACE	278
#define	TOK_GOTOWORKSPACE	279
#define	TOK_ALTTAB	280
#define	TOK_KILLNICELY	281
#define	TOK_KILLWITHEXTREMEPREJUDICE	282
#define	TOK_LAUNCH	283
#define	TOK_FOCUS	284
#define	TOK_MAXIMIZE	285
#define	TOK_NOP	286
#define	TOK_QUOTE	287
#define	TOK_MOVEINTERACTIVELY	288
#define	TOK_RESIZEINTERACTIVELY	289
#define	TOK_MOVERESIZE	290
#define	TOK_QUIT	291
#define	TOK_BEEP	292
#define	TOK_INVOKE	293
#define	TOK_SHOWMENU	294
#define	TOK_REFRESH	295
#define	TOK_SEMI	296
#define	TOK_EQUALS	297
#define	TOK_LBRACE	298
#define	TOK_RBRACE	299
#define	TOK_COMMA	300
#define	TOK_LPAREN	301
#define	TOK_RPAREN	302
#define	TOK_STRING	303
#define	TOK_INTEGER	304
#define	TOK_FLOAT	305


extern YYSTYPE yylval;
