#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void main()
{
	printf("\33[2J");
	printf("				**************************\n");
	printf("						  ELIZA\n");
	printf("					CREATIVE COMPUTING\n");
	printf("				  MORRISTOWN, NEW JERSEY\n");
	printf("\n");
	printf("				   ADAPTED FOR IBM PC BY\n");
	printf
	    ("					PATRICIA DANIELSON AND PAUL HASHFIELD\n");
	printf("\n");
	printf
	    ("				PLEASE DON'T USE COMMAS OR PERIODS IN YOUR INPUTS\n");
	printf("\n");
	printf("				*************************\n");
	printf("\n\n\n");
	/*****INITIALIZATION**********/
	const char *KEYWORDS[] =
	    { "", "CAN YOU ", "CAN I ", "YOU ARE ", "YOU'RE ", "I DON'T ",
		"I FEEL ", "WHY DON'T YOU ", "WHY CAN'T I ", "ARE YOU ", "I CAN'T ",
		"I AM ", "I'M ", "YOU ", "I WANT ", "WHAT ", "HOW ", "WHO ", "WHERE ",
		"WHEN ", "WHY ", "NAME ", "CAUSE ", "SORRY ", "DREAM ", "HELLO ", "HI ",
		"MAYBE ", "NO", "YOUR ", "ALWAYS ", "THINK ", "ALIKE ", "YES ",
		"FRIEND ", "COMPUTER", "NOKEYFOUND"
	};
	const char *WORDIN[] =
	    { "", " ARE ", " WERE ", " YOU ", " YOUR", " I'VE ", " I'M ", " ME " };
	const char *WORDOUT[] =
	    { "", " AM ", " WAS ", " I ", " MY ", " YOU'VE ", " YOU'RE ", " YOU " };
	const char *REPLIES[] = {
		"",
		"DON'T YOU BELIEVE THAT I CAN*",
		"PERHAPS YOU WOULD LIKE TO BE LIKE ME*",
		"YOU WANT ME TO BE ABLE TO*",
		"PERHAPS YOU DON'T WANT TO*",
		"DO YOU WANT TO BE ABLE TO*",
		"WHAT MAKES YOU THINK I AM*",
		"DOES IT PLEASE YOU TO BELIEVE I AM*",
		"PERHAPS YOU WOULD LIKE TO BE*",
		"DO YOU SOMETIMES WISH YOU WERE*",
		"DON'T YOU REALLY*", "WHY DON'T YOU*",
		"DO YOU WISH TO BE ABLE TO*",
		"DOES THAT TROUBLE YOU*", "DO YOU OFTEN FEEL*",
		"DO YOU OFTEN FEEL*", "DO YOU ENJOY FEELING*",
		"DO YOU REALLY BELIEVE I DON'T*",
		"PERHAPS IN GOOD TIME I WILL*",
		"DO YOU WANT ME TO*",
		"DO YOU THINK YOU SHOULD BE ABLE TO*",
		"WHY CAN'T YOU*",
		"WHY ARE YOU INTERESTED IN WHETHER OR NOT I AM*",
		"WOULD YOU PREFER IF I WERE NOT*",
		"PERHAPS IN YOUR FANTASIES I AM*",
		"HOW DO YOU KNOW YOU CAN'T*", "HAVE YOU TRIED?",
		"PERHAPS YOU CAN NOW*",
		"DID YOU COME TO ME BECAUSE YOU ARE*",
		"HOW LONG HAVE YOU BEEN*",
		"DO YOU BELIEVE IT IS NORMAL TO BE*",
		"DO YOU ENJOY BEING*",
		"WE WERE DISCUSSING YOU--NOT ME.", "OH, I*",
		"YOU'RE NOT REALLY TALKING ABOUT ME, ARE YOU?",
		"WHAT WOULD IT MEAN TO YOU IF YOU GOT*",
		"WHY DO YOU WANT*", "SUPPOSE YOU SOON GOT*",
		"WHAT IF YOU NEVER GOT*",
		"I SOMETIMES ALSO WANT*", "WHY DO YOU ASK?",
		"DOES THAT QUESTION INTEREST YOU?",
		"WHAT ANSWER WOULD PLEASE YOU THE MOST?",
		"WHAT DO YOU THINK?",
		"ARE SUCH QUESTIONS ON YOUR MIND OFTEN?",
		"WHAT IS IT THAT YOU REALLY WANT TO KNOW?",
		"HAVE YOU ASKED ANYONE ELSE?",
		"HAVE YOU ASKED SUCH QUESTIONS BEFORE?",
		"WHAT ELSE COMES TO MIND WHEN YOU ASK THAT?",
		"NAMES DON'T INTEREST ME.",
		"I DON'T CARE ABOUT NAMES --PLEASE GO ON.",
		"IS THAT THE REAL REASON?",
		"DON'T ANY OTHER REASONS COME TO MIND?",
		"DOES THAT REASON EXPLAIN ANYTHING ELSE?",
		"WHAT OTHER REASONS MIGHT THERE BE?",
		"PLEASE DON'T APOLOGIZE!",
		"APOLOGIES ARE NOT NECESSARY.",
		"WHAT FEELINGS DO YOU HAVE WHEN YOU APOLOGIZE?",
		"DON'T BE SO DEFENSIVE!",
		"WHAT DOES THAT DREAM SUGGEST TO YOU?",
		"DO YOU DREAM OFTEN?",
		"WHAT PERSONS APPEAR IN YOUR DREAMS?",
		"ARE YOU DISTURBED BY YOUR DREAMS?",
		"HOW DO YOU DO ...PLEASE STATE YOUR PROBLEM.",
		"YOU DON'T SEEM QUITE CERTAIN.",
		"WHY THE UNCERTAIN TONE?",
		"CAN'T YOU BE MORE POSITIVE?",
		"YOU AREN'T SURE?", "DON'T YOU KNOW?",
		"ARE YOU SAYING NO JUST TO BE NEGATIVE?",
		"YOU ARE BEING A BIT NEGATIVE.", "WHY NOT?",
		"ARE YOU SURE?", "WHY NO?",
		"WHY ARE YOU CONCERNED ABOUT MY*",
		"WHAT ABOUT YOUR OWN*",
		"CAN YOU THINK OF A SPECIFIC EXAMPLE?", "WHEN?",
		"WHAT ARE YOU THINKING OF?", "REALLY, ALWAYS?",
		"DO YOU REALLY THINK SO?",
		"BUT YOU ARE NOT SURE YOU*",
		"DO YOU DOUBT YOU*", "IN WHAT WAY?",
		"WHAT RESEMBLANCE DO YOU SEE?",
		"WHAT DOES THE SIMILARITY SUGGEST TO YOU?",
		"WHAT OTHER CONNECTIONS DO YOU SEE?",
		"COULD THERE REALLY BE SOME CONNECTION?",
		"HOW?", "YOU SEEM QUITE POSITIVE.",
		"ARE YOU SURE?", "I SEE.", "I UNDERSTAND.",
		"WHY DO YOU BRING UP THE TOPIC OF FRIENDS?",
		"DO YOUR FRIENDS WORRY YOU?",
		"DO YOUR FRIENDS PICK ON YOU?",
		"ARE YOU SURE YOU HAVE ANY FRIENDS?",
		"DO YOU IMPOSE ON YOUR FRIENDS?",
		"PERHAPS YOUR LOVE FOR FRIENDS WORRIES YOU.",
		"DO COMPUTERS WORRY YOU?",
		"ARE YOU TALKING ABOUT ME IN PARTICULAR?",
		"ARE YOU FRIGHTENED BY MACHINES?",
		"WHY DO YOU MENTION COMPUTERS?",
		"WHAT DO YOU THINK MACHINES HAVE TO DO WITH YOUR PROBLEM?",
		"DON'T YOU THINK COMPUTERS CAN HELP PEOPLE?",
		"WHAT IS IT ABOUT MACHINES THAT WORRIES YOU?",
		"SAY, DO YOU HAVE ANY PSYCHOLOGICAL PROBLEMS?",
		"WHAT DOES THAT SUGGEST TO YOU?", "I SEE.",
		"I'M NOT SURE I UNDERSTAND YOU FULLY.",
		"COME COME ELUCIDATE YOUR THOUGHTS.",
		"CAN YOU ELABORATE ON THAT?", "THAT IS QUITE INTERESTING."
	};
	int S[] =
	    { 0, 1, 4, 6, 6, 10, 14, 17, 20, 22, 25, 28, 28, 32, 35, 40, 40, 40,
	  40, 40, 40, 49, 51, 55, 59, 63, 63, 64, 69, 74, 76, 80, 83, 90, 93,
	  99, 106 };
	int R[] =
	    { 0, 1, 4, 6, 6, 10, 14, 17, 20, 22, 25, 28, 28, 32, 35, 40, 40, 40,
	  40, 40, 40, 49, 51, 55, 59, 63, 63, 64, 69, 74, 76, 80, 83, 90, 93,
	  99, 106 };
	int N[] =
	    { 0, 3, 5, 9, 9, 13, 16, 19, 21, 24, 27, 31, 31, 34, 39, 48, 48, 48,
	  48, 48, 48, 50, 54, 58, 62, 63, 63, 68, 73, 75, 79, 82, 89, 92, 98,
	  105, 111 };
	int N1 = 36, N2 = 14;
	printf("HI! I'M ELIZA. WHAT'S YOUR PROBLEM?\n");
	/**********************************
	*******USER INPUT SECTION**********
	**********************************/
	static char buf[1025], I[1029], P[1029], C[1029], C2[1029];
	unsigned int L;
      input:
	while (!fgets(buf, sizeof(buf), stdin));
	for (L = 1; L < strlen(buf); L++) if (buf[L] == '\n') buf[L] = 0;
	sprintf(I, "  %s  ", buf);
	for (L = 1; L < strlen(I); L++) I[L] = (char)toupper((int)I[L]);
	/* GET RID OF APOSTROPHES */
	for (L = 1; L < strlen(I); L++) {
		//while (I[L] == '\'') {int L2; for (L2 = L; I[L2] != 0; L2++) I[L2] = I[L2 + 1];}
		if (L + 4 > strlen(I))
			break;
		if (strncmp(I + L, "SHUT", 4) != 0)
			continue;
		printf("O.K. IF YOU FEEL THAT WAY I'LL SHUT UP....\n");
		abort();
	}
	if (strcmp(I, P) == 0) {
		printf("PLEASE DON'T REPEAT YOURSELF!\n");
		goto input;
	}
	/**********************************
	********FIND KEYWORD IN I$*********
	**********************************/
	int K;
	const char *F;
	for (K = 1; K < N1; K++) {
		if (strlen(I) > strlen(KEYWORDS[K]))
		for (L = 1; L < strlen(I) - strlen(KEYWORDS[K]) + 1; L++) {
			if (strncmp(I + L, KEYWORDS[K], strlen(KEYWORDS[K])) != 0)
				continue;
			if ((K == 13)
			    &&
			    (strncmp(I + L, KEYWORDS[29], strlen(KEYWORDS[29]))
			     == 0))
				K = 29;
			F = KEYWORDS[K];
			goto conjugate;
		}
	}
	K = 36;
	goto reply;		/* WE DIDN'T FIND ANY KEYWORDS */
	/*****************************************
	**TAKE PART OF STRING AND CONJUGATE IT****
	**USING THE LIST OF STRINGS TO BE SWAPPED*
	*****************************************/
      conjugate:
	sprintf(C, "%s", I + strlen(I) - (strlen(I) - strlen(F) - L + 1));
	int X;
	for (X = 1; X < N2 / 2; X++) {
		for (L = 1; L < strlen(C); L++) {
			if (((L + strlen(WORDIN[X])) <= strlen(C))
			    && (strncmp(C + L, WORDIN[X], strlen(WORDIN[X])) ==
				0)) {
				strcpy(C2, C);
				sprintf(C, "%.*s%s%s", L - 1, C2, WORDOUT[X],
					C2 + L + strlen(WORDIN[X]));
				L += strlen(WORDOUT[X]);
				continue;
			}
			if (((L + strlen(WORDOUT[X])) <= strlen(C))
			    && (strncmp(C + L, WORDOUT[X], strlen(WORDOUT[X]))
				== 0)) {
				strcpy(C2, C);
				sprintf(C, "%.*s%s%s", L - 1, C2, WORDIN[X],
					C2 + L + strlen(WORDOUT[X]));
				L += strlen(WORDIN[X]);
			}
		}
	}
	if (C[2] == ' ') {
		strcpy(C2, C);
		sprintf(C, "%s", C + 1);
	}
	/* ONLY 1 SPACE */
	for (L = 1; L < strlen(C); L++) {
		while (C[L] == '!') {
			strcpy(C2, C);
			sprintf(C, "%.*s%s", L - 1, C, C + L + 1);
		}
	}
	/*********************************************
	**NOW USING THE KEYWORD NUMBER (K) GET REPLY**
	*********************************************/
      reply:
	F = REPLIES[R[K]];
	R[K] = R[K] + 1;
	if (R[K] > N[K])
		R[K] = S[K];
	if (F [ strlen(F) - 1] != '*') {
		printf("%s\n", F);
		strcpy(P, I);
		goto input;
	}
	if (strcmp(C, "   ") == 0) {
		printf("YOU WILL HAVE TO ELABORATE MORE FOR ME TO HELP YOU\n");
		goto input;
	}
	printf("%.*s %s\n",(int) strlen(F) - 1, F, C);
	strcpy(P, I);
	goto input;
}
