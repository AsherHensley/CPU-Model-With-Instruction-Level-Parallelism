/*
 *  ID.h
 *  CellSpuPipe Instruction Decode Module
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

SC_MODULE(ID) {
	
	//Inputs
	sc_in<bool> clock, stall;
	sc_in<sc_uint<32> > PCin, I;
	sc_in<sc_int<32> > new_pc;
	
	//Outputs
	sc_out<sc_uint<32> > info[4]; //info = [PC,opcode,rt,value];
	sc_out<int> rabc[3];
	
	//Variables
	FILE *fp;
	unsigned int nopWord, lnopWord, missWord;
	unsigned int nopCode, lnopCode;
	
	//Methods
	void decode() {
		
		if (new_pc.read()>0) {
			
			//Flush Info
			info[0].write(0); //PC
			info[1].write(0); //opcode
			info[2].write(0); //rt
			info[3].write(0); //value
			
			//FlushRegisters
			rabc[0] = -1;
			rabc[1] = -1;
			rabc[2] = -1;
		}
		else {
			if (stall.read()==0) {
				
				//Read PC
				fpos_t pc = PCin.read();
				info[0].write(PCin.read()); //PC
				
				//Get Instruction Format
				unsigned int fmt;
				fmt = 1;
				if (I.read()!=nopWord & I.read()!=lnopWord & I.read()!=missWord) {
					fsetpos(fp,&pc);
					fread(&fmt,sizeof(fmt),1,fp);
				}
				
				//Parse Instruction
				switch (fmt) {
					case 1:
						
						//Info
						info[1].write(I.read().range(31,21)); //opcode
						info[2].write(I.read().range(6,0));   //rt
						info[3].write(0);                     //value
						
						//Registers
						rabc[0] = I.read().range(13,7);
						rabc[1] = I.read().range(20,14);
						rabc[2] = -1;
						break;
						
					case 2:
						
						//Info
						info[1].write(I.read().range(31,23)); //opcode
						info[2].write(I.read().range(6,0));   //rt
						info[3].write(I.read().range(22,7));  //value
						
						//Registers
						rabc[0] = -1;
						rabc[1] = -1;
						rabc[2] = -1;
						break;
						
					case 3:
						
						//Info
						info[1].write(I.read().range(31,24)); //opcode
						info[2].write(I.read().range(6,0));   //rt
						info[3].write(I.read().range(23,14)); //value
						
						//Registers
						rabc[0] = I.read().range(13,7);
						rabc[1] = -1;
						rabc[2] = -1;
						break;
						
					case 4:
						
						//Info
						info[1].write(I.read().range(31,28)); //opcode
						info[2].write(I.read().range(27,21)); //rt
						info[3].write(0);					  //value
						
						//Registers
						rabc[0] = I.read().range(13,7);
						rabc[1] = I.read().range(20,14);
						rabc[2] = I.read().range(6,0);
						break;
						
					default:
						break;
				}
			}
		}		
	}
			
	//Constructor
	SC_CTOR(ID){
		
		//Methods
		SC_METHOD(decode);
		sensitive << clock.pos();
		
		//Variables
		missWord = 2147483648;
		nopWord = 1075838976;
		lnopWord = 2097152;
		nopCode = 513;
		lnopCode = 1;
		
		//Open Format.bin
		fp = fopen("/Users/asherhensley/Documents/GradSchool/ESE_545/project_part2/CellSpuPipe/Format.bin","r");
		if (NULL==fp) {
			std::cout << "ERROR: Could not open Format.bin" << endl;
		}
	}
	
};






