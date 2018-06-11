/*
 *  RF.h
 *  CellSpuPipe Register File Module
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

#include "systemc.h"

SC_MODULE(RF) {
	
	//Inputs
	sc_in<bool> clock;
	sc_in<sc_uint<32> > info1_in[4],info2_in[4];	//info = [PC,opcode,rt,value];
	sc_in<int> rabc1[3], rabc2[3];					//rabc = [ra,rb,rc];
	sc_in<sc_int<32> > new_pc;
	
	//Forwarding Inputs
	sc_in<sc_biguint<128> > Result1[8], Result2[8];
	sc_in<sc_uint<8> > Result_Destination1[8], Result_Destination2[8];
	sc_in<bool> Reg_Write1[8], Reg_Write2[8];
	
	//Outputs
	sc_out<sc_biguint<128> > A1, B1, C1, T1;
	sc_out<sc_biguint<128> > A2, B2, C2, T2;
	sc_out<sc_uint<32> > info1_out[4],info2_out[4];
	
	//Variables
	sc_biguint<128> regfile[128];
	unsigned int nopCode, lnopCode;
	FILE *fp;
	
	//Read Method
	void read(){
		
		//Init 'temp' Array
		sc_biguint<128> temp[8];
		for (int ii=0; ii<8; ii++) {
			temp[ii] = 0;
		}
		
		//Write 'info' to Temp Buffers
		sc_uint<32> info1_buf[4],info2_buf[4];
		for (int ii=0; ii<4; ii++) {
			info1_buf[ii] = info1_in[ii].read();
			info2_buf[ii] = info2_in[ii].read();
		}
		
		//Chk Branch Condition
		if (new_pc.read()>0) {
			
			//Flush Info Arrays
			info1_buf[0] = 0; //PC
			info1_buf[1] = nopCode; //opcode
			info1_buf[2] = 0; //rt
			info1_buf[3] = 0; //value
			
			info2_buf[0] = 0; //PC
			info2_buf[1] = lnopCode; //opcode
			info2_buf[2] = 0; //rt
			info2_buf[3] = 0; //value
			
		}
		else {
			
			//Register Data Needed
			int rvec[8];
			rvec[0] = rabc1[0].read();
			rvec[1] = rabc1[1].read();
			rvec[2] = rabc1[2].read();
			rvec[3] = info1_in[2].read();
			rvec[4] = rabc2[0].read();
			rvec[5] = rabc2[1].read();
			rvec[6] = rabc2[2].read();
			rvec[7] = info2_in[2].read();
			
			//Register Loop
			for (int ii = 0; ii<8; ii++) {
				
				//Chk for Valid Registers
				if (rvec[ii]>=0) {
					
					//Chk Pipes
					bool forward_success = false;
					for (int jj = 0; jj<8; jj++) {
						
						//EvenPipe Forwarding
						if (Result_Destination1[jj].read()==rvec[ii] & Result_Destination1[jj].read()>0) {
							if (Reg_Write1[jj].read()==1) {
								temp[ii] = Result1[jj]; 
								forward_success = true;
								//std::cout << "Forward Success" << endl;
							}
							else {
								std::cout << rvec[ii] << endl;
								std::cout << "WARNING: Result in EvenPipe not ready for forwarding" << endl;
							}
							break;
						}
						
						//OddPipe Forwarding
						if (Result_Destination2[jj].read()==rvec[ii] & Result_Destination2[jj].read()>0) {
							if (Reg_Write2[jj].read()==1) {
								temp[ii] = Result2[jj];
								forward_success = true;
								//std::cout << "Forward Success" << endl;
							}
							else {
								std::cout << "WARNING: Result in OddPipe not ready for forwarding" << endl;
							}
							break;
						}
					}
					
					//Read from Register File (If not found in pipeline)
					if (!forward_success) {
						temp[ii] = regfile[rvec[ii]]; 
					}
				}
			}
		}

		//Assign CH 1 Data
		A1.write(temp[0]);
		B1.write(temp[1]);
		C1.write(temp[2]);
		T1.write(temp[3]);
		
		//Assign CH 2 Data
		A2.write(temp[4]);
		B2.write(temp[5]);
		C2.write(temp[6]);
		T2.write(temp[7]);
		
		//Assign Info
		for (int ii = 0; ii<4; ii++) {
			info1_out[ii].write(info1_buf[ii]);
			info2_out[ii].write(info2_buf[ii]);
		}
		
	}
	
	//Write Method
	void write(){
		
		//Target Registers
		int rt1 = Result_Destination1[7].read();
		int rt2 = Result_Destination2[7].read();
		
		//Register Write Enables
		bool rw1 = Reg_Write1[7].read();
		bool rw2 = Reg_Write2[7].read();
		
		//Chk if Writing to Same Register
		if (rt1==rt2 & rt1>0 & rt2>0 & rw1==1 & rw2==1) {
			std::cout << "WARNING: Even/Odd Pipe Trying to Write to Same Register" << endl;
		}
		
		//EvenPipe Write
		if (rt1>0 & rw1==1) {
			if (rt1<128) {
				regfile[rt1] = Result1[7].read();
			}
			else {
				std::cout << "WRITE FAIL: Out of Bounds Target Register (EvenPipe)" << endl;
			}	
		}
		else {
			if (rt1==0 & rw1==1) {
				std::cout << "WRITE FAIL: Cannot Write to Register 0 (EvenPipe)" << endl;
			}
		}

		//OddPipe Write
		if (rt2>0 & rw2==1) {
			if (rt2<128) {
				regfile[rt2] = Result2[7].read();
			}
			else {
				std::cout << "WRITE FAIL: Out of Bounds Target Register (OddPipe)" << endl;
			}
		}
		else {
			if (rt2==0 & rw2==1) {
				std::cout << "WRITE FAIL: Cannot Write to Register 0 (OddPipe)" << endl;
			}
		}
	}
	
	//Constructor
	SC_CTOR(RF){
		
		//Methods
		SC_METHOD(read);
		sensitive << clock.pos();
		
		SC_METHOD(write);
		sensitive << clock.pos();
		
		//Variables
		nopCode = 513;
		lnopCode = 1;
		
		//Open Register.bin
		fp = fopen("/Users/asherhensley/Documents/GradSchool/ESE_545/project_part2/CellSpuPipe/Registers.bin","r");
		if (NULL==fp) {
			std::cout << "ERROR: Could not open Register.bin" << endl;
		}
		
		//Init Registers
		unsigned int I;
		for (int ii=0; ii<128; ii++) {
			for (int jj=4; jj>0; jj--) {
				fread(&I,sizeof(I),1,fp);
				regfile[ii].range(32*jj-1,32*jj-32) = I;
			}
		}

	}
};