/*
 *  main.cpp
 *  CellSpuPipe
 *
 *  Created by Asher Hensley on 02/27/17.
 *  Copyright 2017. All rights reserved.
 *
 *  MIT License
 *
 *  Copyright (c) 2016 Asher A. Hensley Research
 *  $Revision: 1.0 $  $Date: 2016/11/03 19:23:54 $
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a 
 *  copy of this software and associated documentation files (the 
 *  "Software"), to deal in the Software without restriction, including 
 *  without limitation the rights to use, copy, modify, merge, publish, 
 *  distribute, sublicense, and/or sell copies of the Software, and to 
 *  permit persons to whom the Software is furnished to do so, subject to 
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included 
 *  in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 *  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 *  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 *  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ILB.h"
#include "IF.h"
#include "ID.h"
#include "Issue.h"
#include "RF.h"
#include "EvenPipe.h"
#include "OddPipe.h"

int sc_main (int argc, char * argv[]) {
	
	//Clock
	sc_clock clock("CLOCK",1,SC_NS);
	
	//ILB-IF Signals
	sc_signal <bool> cache_valid[2];
	sc_signal <sc_uint<32> > cache_tags[2];
	sc_signal <sc_uint<32> > cache_inst[2][32];
	
	//IF-ID Signals
	sc_signal<sc_uint<32> > I1, I2, PC1, PC2;
	
	//ID-Issue Signals
	sc_signal<sc_uint<32> > info1[4], info2[4];
	sc_signal<int> rabc1[3],rabc2[3];
	
	//Issue-RF Signals
	sc_signal<sc_uint<32> > info11[4], info22[4];
	sc_signal<int> rabc11[3],rabc22[3];
	sc_signal<bool> issue_stall;
	
	//RF-Pipe Signals
	sc_signal<sc_biguint<128> > A1, B1, C1, T1;
	sc_signal<sc_biguint<128> > A2, B2, C2, T2;
	sc_signal<sc_uint<32> > info111[4], info222[4];
	
	//EvenPipe Signals
	sc_signal<sc_biguint<128> > Result1[8];
	sc_signal<sc_uint<8> > Result_Destination1[8];
	sc_signal<sc_uint<8> > Stage_When_Ready1[8];
	sc_signal<bool> Reg_Write1[8];
	sc_signal<sc_uint<8> > Unit_Id1[8];
	
	//OddPipe Signals
	sc_signal<sc_biguint<128> > Result2[8];
	sc_signal<sc_uint<8> > Result_Destination2[8];
	sc_signal<sc_uint<8> > Stage_When_Ready2[8];
	sc_signal<bool> Reg_Write2[8];
	sc_signal<sc_uint<8> > Unit_Id2[8];
	sc_signal<sc_uint<32> > Miss_Intructions[8][33];
	sc_signal<sc_int<32> > pc_plus_offset[8];
	sc_signal<sc_uint<3> > flush;
	
	//Instantiations
	ILB ILB1("ILB1");
	IF IF1("IF1");
	ID ID1("ID1");
	ID ID2("ID2");
	Issue Issue1("Issue1");
 	RF RF1("RF1");
	EvenPipe EvenPipe1("EvenPipe1"); 
	OddPipe OddPipe1("OddPipe1"); 
	
	//ILB1 Connections
	ILB1.clock(clock);
	ILB1.cache_valid[0](cache_valid[0]);
	ILB1.cache_valid[1](cache_valid[1]);
	ILB1.cache_tags[0](cache_tags[0]);
	ILB1.cache_tags[1](cache_tags[1]);
	for (int jj=0; jj<32; jj++) {
		ILB1.Miss_Intructions[0][jj](Miss_Intructions[7][jj]);
		ILB1.cache_inst[0][jj](cache_inst[0][jj]);
		ILB1.cache_inst[1][jj](cache_inst[1][jj]);
	}
	ILB1.Miss_Intructions[0][32](Miss_Intructions[7][32]);
	
	//IF1 Connections
	IF1.clock(clock);
	IF1.stall(issue_stall);
	IF1.I1(I1);
	IF1.I2(I2);
	IF1.PC1(PC1);
	IF1.PC2(PC2);
	IF1.new_pc(pc_plus_offset[3]);
	IF1.cache_valid[0](cache_valid[0]);
	IF1.cache_valid[1](cache_valid[1]);
	IF1.cache_tags[0](cache_tags[0]);
	IF1.cache_tags[1](cache_tags[1]);
	for (int jj=0; jj<32; jj++) {
		IF1.cache_inst[0][jj](cache_inst[0][jj]);
		IF1.cache_inst[1][jj](cache_inst[1][jj]);
	}
	
	//ID1 Connections
	ID1.clock(clock);
	ID1.stall(issue_stall);
	ID1.PCin(PC1);
	ID1.I(I1);
	ID1.new_pc(pc_plus_offset[3]);
	for (int ii=0; ii<4; ii++) {
		ID1.info[ii](info1[ii]);
	}
	for (int ii=0; ii<3; ii++) {
		ID1.rabc[ii](rabc1[ii]);
	}
	
	//ID2 Connections
	ID2.clock(clock);
	ID2.stall(issue_stall);
	ID2.PCin(PC2);
	ID2.I(I2);
	ID2.new_pc(pc_plus_offset[3]);
	for (int ii=0; ii<4; ii++) {
		ID2.info[ii](info2[ii]);
	}
	for (int ii=0; ii<3; ii++) {
		ID2.rabc[ii](rabc2[ii]);
	}
	
	//Issue1 Connections
	Issue1.clock(clock);
	Issue1.stall(issue_stall);
	Issue1.new_pc(pc_plus_offset[3]);
	for (int ii=0; ii<4; ii++) {
		Issue1.info1_in[ii](info1[ii]);
		Issue1.info2_in[ii](info2[ii]);
		Issue1.info1_out[ii](info11[ii]);
		Issue1.info2_out[ii](info22[ii]);
		Issue1.info1_RF[ii](info11[ii]); //Feedback
		Issue1.info2_RF[ii](info22[ii]); //Feedback
		Issue1.info1_RFa[ii](info111[ii]); //Feedback
		Issue1.info2_RFa[ii](info222[ii]); //Feedback
	}
	for (int ii=0; ii<3; ii++) {
		Issue1.rabc1_in[ii](rabc1[ii]);
		Issue1.rabc2_in[ii](rabc2[ii]);
		Issue1.rabc1_out[ii](rabc11[ii]);
		Issue1.rabc2_out[ii](rabc22[ii]);
	}
	for (int ii=0; ii<8; ii++) {
		Issue1.Result1[ii](Result1[ii]);
		Issue1.Result_Destination1[ii](Result_Destination1[ii]);
		Issue1.Reg_Write1[ii](Reg_Write1[ii]);
		Issue1.Stage_When_Ready1[ii](Stage_When_Ready1[ii]);
		Issue1.Result2[ii](Result2[ii]);
		Issue1.Result_Destination2[ii](Result_Destination2[ii]);
		Issue1.Reg_Write2[ii](Reg_Write2[ii]);
		Issue1.Stage_When_Ready2[ii](Stage_When_Ready2[ii]);
	}
	
	//RF1 Connections
	RF1.clock(clock);
	RF1.new_pc(pc_plus_offset[3]);
	for (int ii=0; ii<4; ii++) {
		RF1.info1_in[ii](info11[ii]);
		RF1.info2_in[ii](info22[ii]);
		RF1.info1_out[ii](info111[ii]);
		RF1.info2_out[ii](info222[ii]);
	}
	for (int ii=0; ii<3; ii++) {
		RF1.rabc1[ii](rabc11[ii]);
		RF1.rabc2[ii](rabc22[ii]);
	}
	RF1.A1(A1);
	RF1.B1(B1);
	RF1.C1(C1);
	RF1.T1(T1);
	RF1.A2(A2);
	RF1.B2(B2);
	RF1.C2(C2);
	RF1.T2(T2);
	for (int ii=0; ii<8; ii++) {
		RF1.Result1[ii](Result1[ii]);
		RF1.Result_Destination1[ii](Result_Destination1[ii]);
		RF1.Reg_Write1[ii](Reg_Write1[ii]);
		RF1.Result2[ii](Result2[ii]);
		RF1.Result_Destination2[ii](Result_Destination2[ii]);
		RF1.Reg_Write2[ii](Reg_Write2[ii]);
	}
	
	//EvenPipe1 Connections
	EvenPipe1.clock(clock);
	EvenPipe1.A(A1);
	EvenPipe1.B(B1);
	EvenPipe1.C(C1);
	EvenPipe1.T(T1);
	EvenPipe1.flush(flush);

	for (int ii=0; ii<4; ii++) {
		EvenPipe1.info[ii](info111[ii]);
	}
	
	for (int ii=0; ii<8; ii++) {
		EvenPipe1.Result[ii](Result1[ii]);
		EvenPipe1.Result_Destination[ii](Result_Destination1[ii]);
		EvenPipe1.Stage_When_Ready[ii](Stage_When_Ready1[ii]);
		EvenPipe1.Reg_Write[ii](Reg_Write1[ii]);
		EvenPipe1.Unit_Id[ii](Unit_Id1[ii]);
	}
	
	//OddPipe1 Connections
	OddPipe1.clock(clock);
	OddPipe1.A(A2);
	OddPipe1.B(B2);
	OddPipe1.C(C2);
	OddPipe1.T(T2);
	OddPipe1.flush(flush);
	for (int ii=0; ii<4; ii++) {
		OddPipe1.info[ii](info222[ii]);
	}
	
	for (int ii=0; ii<8; ii++) {
		OddPipe1.Result[ii](Result2[ii]);
		OddPipe1.Result_Destination[ii](Result_Destination2[ii]);
		OddPipe1.Stage_When_Ready[ii](Stage_When_Ready2[ii]);
		OddPipe1.Reg_Write[ii](Reg_Write2[ii]);
		OddPipe1.Unit_Id[ii](Unit_Id2[ii]);
		OddPipe1.pc_plus_offset[ii](pc_plus_offset[ii]);
		
		for (int jj=0; jj<33; jj++) {
			OddPipe1.Miss_Instructions[ii][jj](Miss_Intructions[ii][jj]);
		}
	}
	
	//Run
    sc_start(200,SC_NS);
    return 0;
}
