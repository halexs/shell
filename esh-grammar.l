/*
 * Tokens for esh.
 */
%{
#include <string.h>

/* lex.yy.c uses 'ECHO;' which is in termbits.h defined as 0x10
 * undefine this to avoid 'useless statement' warning.
 */
#ifdef ECHO
#undef ECHO
#endif /* ECHO */
%}
%%
[ \t]*		;
">>"		return GREATER_GREATER;
[|&;<>\n]	return *yytext;
[^|&;<>\n\t ]+ 	{ yylval.word = strdup(yytext); return WORD; }
%%
