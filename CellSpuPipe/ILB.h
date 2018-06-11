/*
 *  ILB.h
 *  CellSpuPipe Instruction Line Buffer Module
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

SC_MODULE(ILB) {
	
	//Inputs
	sc_in<bool> clock;
	sc_in<sc_uint<32> > Miss_Intructions[1][33];
	
	//Outputs
	sc_out <bool> cache_valid[2];
	sc_out <sc_uint<32> > cache_tags[2];
	sc_out <sc_uint<32> > cache_inst[2][32];
	
	
	//Methods
	void update(){
		if (Miss_Intructions[0][0].read()>0) {
			
			//Parse PC
			sc_uint<32> inst_addr = Miss_Intructions[0][32].read();
			//int offset = inst_addr.range(6,2);
			int index = inst_addr.range(7,7);
			int tag = inst_addr.range(14,8);
			//cout << tag << "," << index << "," << offset << endl;
			
			//Update Cache
			cache_valid[index].write(true);
			cache_tags[index].write(tag);
			for (int ii=0; ii<32; ii++) {
				cache_inst[index][ii].write(Miss_Intructions[0][ii].read());
			}
		}
	}
		
	//Constructor
	SC_CTOR(ILB){
		
		//Method
		SC_METHOD(update);
		sensitive << clock.pos();
		
	}
	
};
