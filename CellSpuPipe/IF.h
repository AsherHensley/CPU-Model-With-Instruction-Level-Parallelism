/*
 *  IF.h
 *  CellSpuPipe Instruction Fetch Module
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

#include "stdio.h"
#include "systemc.h"

SC_MODULE(IF) {

	//Inputs
	sc_in<bool> clock, stall;
	sc_in<sc_int<32> > new_pc;
	sc_in <bool> cache_valid[2];
	sc_in <sc_uint<32> > cache_tags[2];
	sc_in <sc_uint<32> > cache_inst[2][32];
	
	//Outputs
	sc_out<sc_uint<32> > I1, I2, PC1, PC2;
	
	//Variables
	unsigned int haltCode, nopCode, lnopCode, missCode;
	bool haltSet, missSent;
	sc_uint<32> PC;
	fpos_t pc1,pc2;
	
	//Methods
	void fetch(){
		
		//If Branch:
		if(new_pc.read()>0){
			PC = new_pc.read()-1;
			haltSet = false;
			PC1.write(0);
			PC2.write(0);
			I1.write(0);
			I2.write(0);
			cout<<"<BRANCH: PC="<< new_pc.read()-1<< ">"<<endl;
		}
		else {
			
			//If Not Stalled:
			if(stall.read()==0){
				
				//Set Default Codes
				unsigned int Code1 = nopCode;
				unsigned int Code2 = lnopCode;
				
				//Read From File
				if(!haltSet){
					
					//I1 Physical Address
					sc_uint<32> inst_addr = PC;
					int offset = inst_addr.range(6,2);
					int index = inst_addr.range(7,7);
					int tag = inst_addr.range(14,8);
					
					//Fetch Instruction 1
					sc_uint<32> i1;
					if(cache_valid[index].read()==0 || cache_tags[index].read()!=tag){
						if(missSent==0){
							i1 = missCode;
							missSent = true;
							cout << "<INSTRUCTION MISS>" << endl;
						}
						else {
							i1 = 0;
						}
					}
					else {
						i1 = cache_inst[index][offset];
						if(missSent==true){
							cout << "<INSTRUCTION BLOCK WRITTEN TO CACHE>" << endl;
						}
						missSent = false;
					}
					
					//Update PC
					PC1.write(PC);
					if(missSent==false){
						PC = PC+4;
					}
					
					//Chk for Halt Instruction
					if(i1==haltCode){
						haltSet = true;
					}
					else {
						
						//Assign
						Code1 = i1;
						
						//I2 Physical Address
						sc_uint<32> inst_addr = PC;
						int offset = inst_addr.range(6,2);
						int index = inst_addr.range(7,7);
						int tag = inst_addr.range(14,8);
						
						//Fetch Instruction 2
						sc_uint<32> i2;
						if(cache_valid[index].read()==0 || cache_tags[index].read()!=tag){
							if(missSent==0){
								i2 = missCode;
								missSent = true;
								cout << "INSTRUCTION MISS" << endl;
							}
							else {
								i2 = 0;
							}
						}
						else {
							i2 = cache_inst[index][offset];
							if(missSent==true){
								cout << "INSTRUCTIONS WRITTEN TO CACHE" << endl;
							}
							missSent = false;
						}
						
						//Update PC
						PC2.write(PC);
						if(missSent==false){
							PC = PC+4;
						}
						
						//Chk for Halt Instruction
						if(i2==haltCode){
							haltSet = true;
						}	
						else {
							
							//Assign
							Code2 = i2;
						}
					}
				}
				
				//Miss Logic
				if(Code1==missCode){
					Code2 = nopCode;
				}
				
				//Assign Outputs
				I1.write(Code1);
				I2.write(Code2);	
			}	
		}
	}
	
	//Constructor
	SC_CTOR(IF){
		
		//Methods
		SC_METHOD(fetch);
		sensitive << clock.pos();
		
		//Constants
		PC = 0;
		missCode = 2147483648;
		haltCode = 2063597568;
		nopCode = 1075838976;
		lnopCode = 2097152;
		haltSet = false;
		missSent = false;
		
	}
};






