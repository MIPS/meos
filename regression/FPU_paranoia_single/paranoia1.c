/***(C)2009***************************************************************
*
* Copyright (C) 2009 MIPS Tech, LLC
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holder nor the names of its
* contributors may be used to endorse or promote products derived from this
* software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
****(C)2009**************************************************************/

/*************************************************************************
*
*   Description:	FPU paranoia test
*
*************************************************************************/

#include "MEOS.h"
#include <setjmp.h>

extern void sigfpe(int) asm("sigfpe_paranoia1");
extern int main(void) asm("main_paranoia1");
extern float Sign(float) asm("Sign_paranoia1");
extern float Random(void) asm("Random_paranoia1");
extern void BadCond(int, char *) asm("BadCond_paranoia1");
extern void SqXMinX(int) asm("SqXMinX_paranoia1");
extern void TstCond(int, int, char *) asm("TstCond_paranoia1");
extern void notify(char *) asm("notify_paranoia1");
extern void Characteristics(void) asm("Characteristics_paranoia1");
extern void Heading(void) asm("Heading_paranoia1");
extern void History(void) asm("History_paranoia1");
extern void Instructions(void) asm("Instructions_paranoia1");
extern void IsYeqX(void) asm("IsYeqX_paranoia1");
extern void NewD(void) asm("NewD_paranoia1");
extern void Pause(void) asm("Pause_paranoia1");
extern void PrintIfNPositive(void) asm("PrintIfNPositive_paranoia1");
extern void SR3750(void) asm("SR3750_paranoia1");
extern void SR3980(void) asm("SR3980_paranoia1");
extern void TstPtUf(void) asm("TstPtUf_paranoia1");
extern void msglist(char **) asm("msglist_paranoia1");

extern float Zero asm("Zero_paranoia1");
extern float Half asm("Half_paranoia1");
extern float One asm("One_paranoia1");
extern float Two asm("Two_paranoia1");
extern float Three asm("Three_paranoia1");
extern float Four asm("Four_paranoia1");
extern float Five asm("Five_paranoia1");
extern float Eight asm("Eight_paranoia1");
extern float Nine asm("Nine_paranoia1");
extern float TwentySeven asm("TwentySeven_paranoia1");
extern float ThirtyTwo asm("ThirtyTwo_paranoia1");
extern float TwoForty asm("TwoForty_paranoia1");
extern float MinusOne asm("MinusOne_paranoia1");
extern float OneAndHalf asm("OneAndHalf_paranoia1");
extern int NoTrials asm("NoTrials_paranoia1");

extern int Indx asm("Indx_paranoia1");
extern char ch[8] asm("ch_paranoia1");
extern float AInvrse asm("AInvrse_paranoia1");
extern float A1 asm("A1_paranoia1");
extern float C asm("C_paranoia1");
extern float CInvrse asm("CInvrse_paranoia1");
extern float D asm("D_paranoia1");
extern float FourD asm("FourD_paranoia1");
extern float E0 asm("E0_paranoia1");
extern float E1 asm("E1_paranoia1");
extern float Exp2 asm("Exp2_paranoia1");
extern float E3 asm("E3_paranoia1");
extern float MinSqEr asm("MinSqEr_paranoia1");
extern float SqEr asm("SqEr_paranoia1");
extern float MaxSqEr asm("MaxSqEr_paranoia1");
extern float E9 asm("E9_paranoia1");
extern float Third asm("Third_paranoia1");
extern float F6 asm("F6_paranoia1");
extern float F9 asm("F9_paranoia1");
extern float H asm("H_paranoia1");
extern float HInvrse asm("HInvrse_paranoia1");
extern int I asm("I_paranoia1");
extern float StickyBit asm("StickyBit_paranoia1");
extern float J asm("J_paranoia1");
extern float MyZero asm("MyZero_paranoia1");
extern float Precision asm("Precision_paranoia1");
extern float Q asm("Q_paranoia1");
extern float Q9 asm("Q9_paranoia1");
extern float R asm("R_paranoia1");
extern float Random9 asm("Random9_paranoia1");
extern float T asm("T_paranoia1");
extern float Underflow asm("Underflow_paranoia1");
extern float S asm("S_paranoia1");
extern float OneUlp asm("OneUlp_paranoia1");
extern float UfThold asm("UfThold_paranoia1");
extern float U1 asm("U1_paranoia1");
extern float U2 asm("U2_paranoia1");
extern float V asm("V_paranoia1");
extern float V0 asm("V0_paranoia1");
extern float V9 asm("V9_paranoia1");
extern float W asm("W_paranoia1");
extern float X asm("X_paranoia1");
extern float X1 asm("X1_paranoia1");
extern float X2 asm("X2_paranoia1");
extern float X8 asm("X8_paranoia1");
extern float Random1 asm("Random1_paranoia1");
extern float Y asm("Y_paranoia1");
extern float Y1 asm("Y1_paranoia1");
extern float Y2 asm("Y2_paranoia1");
extern float Random2 asm("Random2_paranoia1");
extern float Z asm("Z_paranoia1");
extern float PseudoZero asm("PseudoZero_paranoia1");
extern float Z1 asm("Z1_paranoia1");
extern float Z2 asm("Z2_paranoia1");
extern float Z9 asm("Z9_paranoia1");
extern int ErrCnt[4] asm("ErrCnt_paranoia1");
extern int fpecount asm("fpecount_paranoia1");
extern int Milestone asm("Milestone_paranoia1");
extern int PageNo asm("PageNo_paranoia1");
extern int M asm("M_paranoia1");
extern int N asm("N_paranoia1");
extern int N1 asm("N1_paranoia1");
extern int GMult asm("GMult_paranoia1");
extern int GDiv asm("GDiv_paranoia1");
extern int GAddSub asm("GAddSub_paranoia1");
extern int RMult asm("RMult_paranoia1");
extern int RDiv asm("RDiv_paranoia1");
extern int RAddSub asm("RAddSub_paranoia1");
extern int RSqrt asm("RSqrt_paranoia1");
extern int Break asm("Break_paranoia1");
extern int Done asm("Done_paranoia1");
extern int NotMonot asm("NotMonot_paranoia1");
extern int Monot asm("Monot_paranoia1");
extern int Anomaly asm("Anomaly_paranoia1");
extern int IEEE asm("IEEE_paranoia1");
extern int SqRWrng asm("SqRWrng_paranoia1");
extern int UfNGrad asm("UfNGrad_paranoia1");
extern float Radix asm("Radix_paranoia1");
extern float BInvrse asm("BInvrse_paranoia1");
extern float RadixD2 asm("RadixD2_paranoia1");
extern float BMinusU2 asm("BMinusU2_paranoia1");

extern jmp_buf ovfl_buf asm("ovfl_buf_paranoia1");
extern void (*sigsave) () asm("sigsave_paranoia1");

#include "paranoiao.c"
