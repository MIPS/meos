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

extern void sigfpe(int) asm("sigfpe_paranoia2");
extern int main(void) asm("main_paranoia2");
extern double Sign(double) asm("Sign_paranoia2");
extern double Random(void) asm("Random_paranoia2");
extern void BadCond(int, char *) asm("BadCond_paranoia2");
extern void SqXMinX(int) asm("SqXMinX_paranoia2");
extern void TstCond(int, int, char *) asm("TstCond_paranoia2");
extern void notify(char *) asm("notify_paranoia2");
extern void Characteristics(void) asm("Characteristics_paranoia2");
extern void Heading(void) asm("Heading_paranoia2");
extern void History(void) asm("History_paranoia2");
extern void Instructions(void) asm("Instructions_paranoia2");
extern void IsYeqX(void) asm("IsYeqX_paranoia2");
extern void NewD(void) asm("NewD_paranoia2");
extern void Pause(void) asm("Pause_paranoia2");
extern void PrintIfNPositive(void) asm("PrintIfNPositive_paranoia2");
extern void SR3750(void) asm("SR3750_paranoia2");
extern void SR3980(void) asm("SR3980_paranoia2");
extern void TstPtUf(void) asm("TstPtUf_paranoia2");
extern void msglist(char **) asm("msglist_paranoia2");

extern double Zero asm("Zero_paranoia2");
extern double Half asm("Half_paranoia2");
extern double One asm("One_paranoia2");
extern double Two asm("Two_paranoia2");
extern double Three asm("Three_paranoia2");
extern double Four asm("Four_paranoia2");
extern double Five asm("Five_paranoia2");
extern double Eight asm("Eight_paranoia2");
extern double Nine asm("Nine_paranoia2");
extern double TwentySeven asm("TwentySeven_paranoia2");
extern double ThirtyTwo asm("ThirtyTwo_paranoia2");
extern double TwoForty asm("TwoForty_paranoia2");
extern double MinusOne asm("MinusOne_paranoia2");
extern double OneAndHalf asm("OneAndHalf_paranoia2");
extern int NoTrials asm("NoTrials_paranoia2");

extern int Indx asm("Indx_paranoia2");
extern char ch[8] asm("ch_paranoia2");
extern double AInvrse asm("AInvrse_paranoia2");
extern double A1 asm("A1_paranoia2");
extern double C asm("C_paranoia2");
extern double CInvrse asm("CInvrse_paranoia2");
extern double D asm("D_paranoia2");
extern double FourD asm("FourD_paranoia2");
extern double E0 asm("E0_paranoia2");
extern double E1 asm("E1_paranoia2");
extern double Exp2 asm("Exp2_paranoia2");
extern double E3 asm("E3_paranoia2");
extern double MinSqEr asm("MinSqEr_paranoia2");
extern double SqEr asm("SqEr_paranoia2");
extern double MaxSqEr asm("MaxSqEr_paranoia2");
extern double E9 asm("E9_paranoia2");
extern double Third asm("Third_paranoia2");
extern double F6 asm("F6_paranoia2");
extern double F9 asm("F9_paranoia2");
extern double H asm("H_paranoia2");
extern double HInvrse asm("HInvrse_paranoia2");
extern int I asm("I_paranoia2");
extern double StickyBit asm("StickyBit_paranoia2");
extern double J asm("J_paranoia2");
extern double MyZero asm("MyZero_paranoia2");
extern double Precision asm("Precision_paranoia2");
extern double Q asm("Q_paranoia2");
extern double Q9 asm("Q9_paranoia2");
extern double R asm("R_paranoia2");
extern double Random9 asm("Random9_paranoia2");
extern double T asm("T_paranoia2");
extern double Underflow asm("Underflow_paranoia2");
extern double S asm("S_paranoia2");
extern double OneUlp asm("OneUlp_paranoia2");
extern double UfThold asm("UfThold_paranoia2");
extern double U1 asm("U1_paranoia2");
extern double U2 asm("U2_paranoia2");
extern double V asm("V_paranoia2");
extern double V0 asm("V0_paranoia2");
extern double V9 asm("V9_paranoia2");
extern double W asm("W_paranoia2");
extern double X asm("X_paranoia2");
extern double X1 asm("X1_paranoia2");
extern double X2 asm("X2_paranoia2");
extern double X8 asm("X8_paranoia2");
extern double Random1 asm("Random1_paranoia2");
extern double Y asm("Y_paranoia2");
extern double Y1 asm("Y1_paranoia2");
extern double Y2 asm("Y2_paranoia2");
extern double Random2 asm("Random2_paranoia2");
extern double Z asm("Z_paranoia2");
extern double PseudoZero asm("PseudoZero_paranoia2");
extern double Z1 asm("Z1_paranoia2");
extern double Z2 asm("Z2_paranoia2");
extern double Z9 asm("Z9_paranoia2");
extern int ErrCnt[4] asm("ErrCnt_paranoia2");
extern int fpecount asm("fpecount_paranoia2");
extern int Milestone asm("Milestone_paranoia2");
extern int PageNo asm("PageNo_paranoia2");
extern int M asm("M_paranoia2");
extern int N asm("N_paranoia2");
extern int N1 asm("N1_paranoia2");
extern int GMult asm("GMult_paranoia2");
extern int GDiv asm("GDiv_paranoia2");
extern int GAddSub asm("GAddSub_paranoia2");
extern int RMult asm("RMult_paranoia2");
extern int RDiv asm("RDiv_paranoia2");
extern int RAddSub asm("RAddSub_paranoia2");
extern int RSqrt asm("RSqrt_paranoia2");
extern int Break asm("Break_paranoia2");
extern int Done asm("Done_paranoia2");
extern int NotMonot asm("NotMonot_paranoia2");
extern int Monot asm("Monot_paranoia2");
extern int Anomaly asm("Anomaly_paranoia2");
extern int IEEE asm("IEEE_paranoia2");
extern int SqRWrng asm("SqRWrng_paranoia2");
extern int UfNGrad asm("UfNGrad_paranoia2");
extern double Radix asm("Radix_paranoia2");
extern double BInvrse asm("BInvrse_paranoia2");
extern double RadixD2 asm("RadixD2_paranoia2");
extern double BMinusU2 asm("BMinusU2_paranoia2");

extern jmp_buf ovfl_buf asm("ovfl_buf_paranoia2");
extern void (*sigsave) () asm("sigsave_paranoia2");

#include "paranoiao.c"
