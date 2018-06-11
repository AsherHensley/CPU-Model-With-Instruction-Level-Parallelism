/*
 *  EvenPipe.h
 *  CellSpuPipe Even Pipeline Operations Module
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

SC_MODULE(EvenPipe) {
	
	//Inputs
	sc_in<bool> clock;
	sc_in<sc_uint<32> > info[4]; //info = [PC,opcode,rt,value];
	sc_in<sc_biguint<128> > A, B, C, T;
	sc_in<sc_uint<3> > flush;
	
	//Outputs
	sc_out<sc_biguint<128> > Result[8];
	sc_out<sc_uint<8> > Result_Destination[8];
	sc_out<sc_uint<8> > Stage_When_Ready[8];
	sc_out<bool> Reg_Write[8];
	sc_out<sc_uint<8> > Unit_Id[8];
	
	//Variables
	int simple_fixed_op_latency1;
	int simple_fixed_op_latency2;
	int single_precision_op_latency1;
	int single_precision_op_latency2;
	int byte_op_latency;
	int nop_unit_id;
	int simple_fixed_op_unit_id1;
	int simple_fixed_op_unit_id2;
	int single_precision_op_unit_id1;
	int single_precision_op_unit_id2;
	int byte_op_unit_id; 
	bool print_double;
	FILE *fp;
	int f1,f2;
	int print_counter;
	
	//Update Method
	void update(){
		
		//Process New Instruction
		unsigned int opcode = info[1].read();
		unsigned int rt = info[2].read();
		sc_bv<32> value_bv = info[3].read();
		sc_bv<128> result_bv;
		sc_bv<32> temp_bv;
		sc_int<32> a, b, c;
		sc_int<32> au, bu;
		sc_int<16> a16, b16, c16;
		sc_bv<32> a_bv, b_bv;
		sc_uint<64> a64, b64, t64;
		sc_biguint<128> t128;
		
		//Flush
		int stop_stage = 0;
		if (flush.read()>0) {
			opcode = 0;
			for (int ii=0; ii<=flush.read(); ii++) {
				Result[ii].write(0);
				Result_Destination[ii].write(0);
				Stage_When_Ready[ii].write(0);
				Reg_Write[ii].write(0);
				Unit_Id[ii].write(nop_unit_id);
			}
			stop_stage = flush.read();
		}
		
		//Shift Pipe Data
		for (int ii=7; ii>stop_stage; ii--) {
				
			Result[ii].write(Result[ii-1].read());
			Result_Destination[ii].write(Result_Destination[ii-1].read());
			Stage_When_Ready[ii].write(Stage_When_Ready[ii-1].read());
			Reg_Write[ii].write(Reg_Write[ii-1].read());
			Unit_Id[ii].write(Unit_Id[ii-1].read());
				
			if (Stage_When_Ready[ii-1].read()==ii) {
				Reg_Write[ii].write(true);
			}
		}

		//Byte-Op Vars
		sc_bv<8> value_bv_byte, a_bv_byte, b_bv_byte;
		sc_uint<8> value_byte, a_byte, b_byte;
		sc_bv<10> value_bv_byte2, a_bv_byte2, b_bv_byte2;
		sc_uint<10> value_byte2, a_byte2, b_byte2;
		
		//Floating Point Vars
		bool convert_single = false;
		bool convert_double = false;
		int expo_a, expo_b, expo_c, expo_t;
		float single_a[4], single_b[4], single_c[4], single_result[4];
		double double_a[2], double_b[2], double_T[2], double_result[2];
		
		//Run
		int cur_unit_id;
		int ptr;
		switch (opcode) {
				
				
			//--------------------
			// No-Ops
			//--------------------
			case 0:
				cur_unit_id = nop_unit_id;
				break;
				
			case 513:
				cur_unit_id = nop_unit_id;
				break;
				
				
			//*** SIMPLE-FIXED-1 OPS ***//
				
				
			//--------------------	
			// Add Word
			//--------------------
			case 192:
				
				//Add
				for (int ii = 0; ii<128; ii=ii+32) {
					a = A.read().range(ii+32-1,ii);
					b = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = a+b;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Add Word Immediate
			//--------------------
			case 28:
				
				//Sign Extend
				for (int ii=10; ii<32; ii++) {
					value_bv.range(ii,ii) = value_bv.range(ii-1,ii-1);
				}
				b = value_bv;
				
				//Add
				for (int ii = 0; ii<128; ii=ii+32) {
					a = A.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = a+b;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Subtract From Word
			//--------------------
			case 64:
				
				//Subtract
				for (int ii = 0; ii<128; ii=ii+32) {
					a = A.read().range(ii+32-1,ii);
					b = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = a-b;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Subtract From Word Immediate
			//--------------------
			case 13:
				
				//Sign Extend
				for (int ii=10; ii<32; ii++) {
					value_bv.range(ii,ii) = value_bv.range(ii-1,ii-1);
				}
				b = value_bv;
				
				//Subtract
				for (int ii = 0; ii<128; ii=ii+32) {
					a = A.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = b-a;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
			
			//--------------------
			// Count Leading Zeros
			//--------------------
			case 677:
				
				//Count
				for (int ii = 0; ii<128; ii=ii+32) {
					temp_bv = A.read().range(ii+32-1,ii);
					bool lz_set = false;
					int lz_cnt = 0;
					for (int jj = 31; jj>=0; jj--) {
						if (temp_bv.range(jj,jj)==1 & !lz_set) {
							result_bv.range(ii+32-1,ii) = lz_cnt;
							lz_set = true;
						}
						lz_cnt++;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// And Word 
			//--------------------
			case 193:
				
				//And
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					b_bv = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = a_bv & b_bv;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// And Word Immediate
			//--------------------
			case 20:
				
				//Sign Extend
				for (int ii=10; ii<32; ii++) {
					value_bv.range(ii,ii) = value_bv.range(ii-1,ii-1);
				}
				
				//And
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = a_bv & value_bv;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Or Word 
			//--------------------
			case 65:
				
				//Or
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					b_bv = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = a_bv | b_bv;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Or Word Immediate
			//--------------------
			case 4:
				
				//Sign Extend
				for (int ii=10; ii<32; ii++) {
					value_bv.range(ii,ii) = value_bv.range(ii-1,ii-1);
				}
				
				//Or
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = a_bv | value_bv;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Exclusive Or Word 
			//--------------------
			case 577:
				
				//Exclusive Or
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					b_bv = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = a_bv ^ b_bv;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Exclusive Or Word Immediate
			//--------------------
			case 68:
				
				//Sign Extend
				for (int ii=10; ii<32; ii++) {
					value_bv.range(ii,ii) = value_bv.range(ii-1,ii-1);
				}
				
				//Exclusive Or
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = a_bv ^ value_bv;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Nand Word 
			//--------------------
			case 201:
				
				//Nand
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					b_bv = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = ~(a_bv & b_bv);
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Nor Word 
			//--------------------
			case 73:
				
				//Nor
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					b_bv = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = ~(a_bv | b_bv);
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Compare Equal Word 
			//--------------------
			case 960:
				
				//Compare
				for (int ii = 0; ii<128; ii=ii+32) {
					a = A.read().range(ii+32-1,ii);
					b = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = 0;
					if (a==b) {
						result_bv.range(ii+32-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Compare Equal Word Immediate
			//--------------------
			case 124:
				
				//Sign Extend
				for (int ii=10; ii<32; ii++) {
					value_bv.range(ii,ii) = value_bv.range(ii-1,ii-1);
				}
				
				//Compare
				for (int ii = 0; ii<128; ii=ii+32) {
					a = A.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = 0;
					if (a==value_bv) {
						result_bv.range(ii+32-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;	
				
				
			//--------------------
			// Compare Greater Than Word 
			//--------------------
			case 576:
				
				//Compare
				for (int ii = 0; ii<128; ii=ii+32) {
					a = A.read().range(ii+32-1,ii);
					b = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = 0;
					if (a>b) {
						result_bv.range(ii+32-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;	
			
				
			//--------------------
			// Compare Greater Than Word Immediate
			//--------------------
			case 76:
				
				//Sign Extend
				for (int ii=10; ii<32; ii++) {
					value_bv.range(ii,ii) = value_bv.range(ii-1,ii-1);
				}
				
				//Compare
				b = value_bv;
				for (int ii = 0; ii<128; ii=ii+32) {
					a = A.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = 0;
					if (a>b) {
						result_bv.range(ii+32-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Double Floating Compare Equal  
			//--------------------
			case 963:
				
				//Compare
				for (int ii = 0; ii<128; ii=ii+64) {
					a64 = A.read().range(ii+64-1,ii);
					b64 = B.read().range(ii+64-1,ii);
					result_bv.range(ii+64-1,ii) = 0;
					if (a64==b64) {
						result_bv.range(ii+64-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Double Floating Compare Greater Than  
			//--------------------
			case 707:
				
				//Compare
				ptr = 0;
				for (int ii = 0; ii<128; ii=ii+64) {
					
					//Convert to Double
					a64 = A.read().range(ii+64-1,ii);
					b64 = B.read().range(ii+64-1,ii);
					double_a[ptr] = 0;
					double_b[ptr] = 0;
					for (int jj=51; jj>=0; jj--) {
						double_a[ptr] = double_a[ptr] + pow(2,jj-52) * a64.range(jj,jj);
						double_b[ptr] = double_b[ptr] + pow(2,jj-52) * b64.range(jj,jj);
					}
					expo_a = a64.range(62,52)-1023;
					expo_b = b64.range(62,52)-1023;
					double_a[ptr] = pow(-1,a64.range(63,63)) * (1 + double_a[ptr]) * pow(2,expo_a);
					double_b[ptr] = pow(-1,b64.range(63,63)) * (1 + double_b[ptr]) * pow(2,expo_b);
					
					//Compare
					result_bv.range(ii+64-1,ii) = 0;
					if (double_a[ptr]>double_b[ptr]) {
						result_bv.range(ii+64-1,ii) = -1;
					}
					ptr++;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Floating Compare Equal  
			//--------------------
			case 962:
				
				//Compare
				for (int ii = 0; ii<128; ii=ii+32) {
					a = A.read().range(ii+32-1,ii);
					b = B.read().range(ii+32-1,ii);
					result_bv.range(ii+32-1,ii) = 0;
					if (a==b) {
						result_bv.range(ii+32-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Floating Compare Greater Than  
			//--------------------
			case 706:
				
				//Compare
				ptr = 0;
				for (int ii = 0; ii<128; ii=ii+32) {
					
					//Convert to Single
					a = A.read().range(ii+32-1,ii);
					b = B.read().range(ii+32-1,ii);
					single_a[ptr] = 0;
					single_b[ptr] = 0;
					for (int jj=22; jj>=0; jj--) {
						single_a[ptr] = single_a[ptr] + pow(2,jj-23) * a.range(jj,jj);
						single_b[ptr] = single_b[ptr] + pow(2,jj-23) * b.range(jj,jj);
					}
					expo_a = a.range(30,23)-127;
					expo_b = b.range(30,23)-127;
					single_a[ptr] = pow(-1,a.range(31,31)) * (1 + single_a[ptr]) * pow(2,expo_a);
					single_b[ptr] = pow(-1,b.range(31,31)) * (1 + single_b[ptr]) * pow(2,expo_b);
					
					//Compare
					result_bv.range(ii+32-1,ii) = 0;
					if (single_a[ptr]>single_b[ptr]) {
						result_bv.range(ii+32-1,ii) = -1;
					}
					ptr++;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// And Byte Immediate
			//--------------------
			case 22:
				
				//Immediate
				value_bv_byte = value_bv.range(7,0);

				//And
				for (int ii = 0; ii<128; ii=ii+8) {
					a_bv_byte = A.read().range(ii+8-1,ii);
					result_bv.range(ii+8-1,ii) = a_bv_byte & value_bv_byte;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Or Byte Immediate
			//--------------------
			case 6:
				
				//Immediate
				value_bv_byte = value_bv.range(7,0);
				
				//Or
				for (int ii = 0; ii<128; ii=ii+8) {
					a_bv_byte = A.read().range(ii+8-1,ii);
					result_bv.range(ii+8-1,ii) = a_bv_byte | value_bv_byte;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Exclusive Or Byte Immediate
			//--------------------
			case 70:
				
				//Immediate
				value_bv_byte = value_bv.range(7,0);
				
				//Exclusive Or
				for (int ii = 0; ii<128; ii=ii+8) {
					a_bv_byte = A.read().range(ii+8-1,ii);
					result_bv.range(ii+8-1,ii) = a_bv_byte ^ value_bv_byte;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Compare Equal Byte
			//--------------------
			case 976:
				
				//Compare
				for (int ii = 0; ii<128; ii=ii+8) {
					a_bv_byte = A.read().range(ii+8-1,ii);
					b_bv_byte = B.read().range(ii+8-1,ii);
					result_bv.range(ii+8-1,ii) = 0;
					if (a_bv_byte==b_bv_byte) {
						result_bv.range(ii+8-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//--------------------
			// Compare Equal Byte Immediate
			//--------------------
			case 126:
				
				//Immediate
				value_bv_byte = value_bv.range(7,0);
				
				//Compare
				for (int ii = 0; ii<128; ii=ii+8) {
					a_bv_byte = A.read().range(ii+8-1,ii);
					result_bv.range(ii+8-1,ii) = 0;
					if (a_bv_byte==value_bv_byte) {
						result_bv.range(ii+8-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;

				
			//--------------------
			// Compare Greater Than Byte
			//--------------------
			case 592:
				
				//Compare
				for (int ii = 0; ii<128; ii=ii+8) {
					a_byte = A.read().range(ii+8-1,ii);
					b_byte = B.read().range(ii+8-1,ii);
					result_bv.range(ii+8-1,ii) = 0;
					if (a_byte>b_byte) {
						result_bv.range(ii+8-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
			
			//--------------------
			// Compare Greater Than Byte Immediate
			//--------------------
			case 78:
				
				//Compare
				for (int ii = 0; ii<128; ii=ii+8) {
					a_byte = A.read().range(ii+8-1,ii);
					value_bv_byte = value_bv.range(7,0);
					b_byte = value_bv_byte;
					result_bv.range(ii+8-1,ii) = 0;
					if (a_byte>b_byte) {
						result_bv.range(ii+8-1,ii) = -1;
					}
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id1;
				break;
				
				
			//*** SIMPLE-FIXED-2 OPS ***//
				
				
			//--------------------
			// Shift Left Word
			//--------------------
			case 91:
				
				//Shift Each Word
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					b_bv = B.read().range(ii+32-1,ii);
					b = b_bv & 63;
					if (b>0) {
						for (int jj=0; jj<b; jj++) {
							a_bv = (a_bv.range(30,0),false);
						}
					}
					result_bv.range(ii+32-1,ii) = a_bv;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id2;
				break;
				
				
			//--------------------
			// Rotate Word
			//--------------------
			case 88:
				
				//Rotate Each Word
				for (int ii = 0; ii<128; ii=ii+32) {
					a_bv = A.read().range(ii+32-1,ii);
					b_bv = B.read().range(ii+32-1,ii);
					b = b_bv & 31;
					if (b>0) {
						for (int jj=0; jj<b; jj++) {
							a_bv = (a_bv.range(30,0),a_bv.range(31,31));
						}
					}
					result_bv.range(ii+32-1,ii) = a_bv;
				}
				
				//Set Unit Id
				cur_unit_id = simple_fixed_op_unit_id2;
				break;
				
				
			//*** SINGLE-PRECSIONS-1 OPS ***//
				
				
			//--------------------	
			// Multiply
			//--------------------
			case 964:
				
				//Multiply
				for (int ii = 0; ii<128; ii=ii+32) {
					a16 = A.read().range(ii+16-1,ii);
					b16 = B.read().range(ii+16-1,ii);
					result_bv.range(ii+32-1,ii) = a16*b16;
				}
				
				//Set Unit Id
				cur_unit_id = single_precision_op_unit_id1;
				break;	
			
				
			//--------------------	
			// Multiply Immediate
			//--------------------
			case 116:
				
				//Sign Extend
				for (int ii=10; ii<16; ii++) {
					value_bv.range(ii,ii) = value_bv.range(ii-1,ii-1);
				}
				b16 = value_bv;
				
				//Multiply
				for (int ii = 0; ii<128; ii=ii+32) {
					a16 = A.read().range(ii+16-1,ii);
					result_bv.range(ii+32-1,ii) = a16*b16;
				}
				
				//Set Unit Id
				cur_unit_id = single_precision_op_unit_id1;
				break;	
				
				
			//--------------------	
			// Integer Multiply and Add
			//--------------------
			case 12:
				
				//Multiply
				for (int ii = 0; ii<128; ii=ii+32) {
					a16 = A.read().range(ii+16-1,ii);
					b16 = B.read().range(ii+16-1,ii);
					c16 = C.read().range(ii+16-1,ii);
					result_bv.range(ii+32-1,ii) = a16*b16+c16;
				}
				
				//Set Unit Id
				cur_unit_id = single_precision_op_unit_id1;
				break;
				
				
			//*** SINGLE-PRECSIONS-2 OPS ***//
				
					
			//--------------------
			// Floating Add, Subtract, Multiply, Multiply/Add, Multiply/Subtract
			//--------------------
			case 708:
			case 709:
			case 710:	
			case 14:
			case 15:
				
				ptr = 0;
				for (int ii = 0; ii<128; ii=ii+32) {
					
					//Convert to Bit-Vector to Single
					a = A.read().range(ii+32-1,ii);
					b = B.read().range(ii+32-1,ii);
					c = C.read().range(ii+32-1,ii);
					single_a[ptr] = 0;
					single_b[ptr] = 0;
					single_c[ptr] = 0;
					for (int jj=22; jj>=0; jj--) {
						single_a[ptr] = single_a[ptr] + pow(2,jj-23) * a.range(jj,jj);
						single_b[ptr] = single_b[ptr] + pow(2,jj-23) * b.range(jj,jj);
						single_c[ptr] = single_c[ptr] + pow(2,jj-23) * c.range(jj,jj);
					}
					expo_a = a.range(30,23)-127;
					expo_b = b.range(30,23)-127;
					expo_c = c.range(30,23)-127;
					single_a[ptr] = pow(-1,a.range(31,31)) * (1 + single_a[ptr]) * pow(2,expo_a);
					single_b[ptr] = pow(-1,b.range(31,31)) * (1 + single_b[ptr]) * pow(2,expo_b);
					single_c[ptr] = pow(-1,c.range(31,31)) * (1 + single_c[ptr]) * pow(2,expo_c);
					
					//Add
					if (opcode==708) {
						single_result[ptr] = single_a[ptr]+single_b[ptr];
					}
					
					//Subtract
					if (opcode==709) {
						single_result[ptr] = single_a[ptr]-single_b[ptr];
					}
					
					//Multiply
					if (opcode==710) {
						single_result[ptr] = single_a[ptr]*single_b[ptr];
					}
					
					//Multiply/Add
					if (opcode==14) {
						single_result[ptr] = single_a[ptr]*single_b[ptr] + single_c[ptr];
					}
					
					//Multiply/Subtract
					if (opcode==15) {
						single_result[ptr] = single_a[ptr]*single_b[ptr] - single_c[ptr];
					}
					
					ptr++;

				}
				
				//Set Unit Id
				convert_single = true;
				cur_unit_id = single_precision_op_unit_id2;
				break;
			
	
			//--------------------
			// Double Floating Add, Subtract, Multiply, Multiply/Add, Multiply/Subtract
			//--------------------	
			case 716:
			case 717:
			case 718:
			case 860:
			case 861:
				
				//Read Register RT
				t128 = 0;
				if (opcode==860 | opcode==861) {
					t128 = T.read();
				}
				
				ptr = 0;
				for (int ii = 0; ii<128; ii=ii+64) {
					
					//Convert to Bit Vector to Double
					a64 = A.read().range(ii+64-1,ii);
					b64 = B.read().range(ii+64-1,ii);
					t64 = t128.range(ii+64-1,ii);
					double_a[ptr] = 0;
					double_b[ptr] = 0;
					double_T[ptr] = 0;
					for (int jj=51; jj>=0; jj--) {
						double_a[ptr] = double_a[ptr] + pow(2,jj-52) * a64.range(jj,jj);
						double_b[ptr] = double_b[ptr] + pow(2,jj-52) * b64.range(jj,jj);
						double_T[ptr] = double_T[ptr] + pow(2,jj-52) * t64.range(jj,jj);
					}
					expo_a = a64.range(62,52)-1023;
					expo_b = b64.range(62,52)-1023;
					expo_t = t64.range(62,52)-1023;
					double_a[ptr] = pow(-1,a64.range(63,63)) * (1 + double_a[ptr]) * pow(2,expo_a);
					double_b[ptr] = pow(-1,b64.range(63,63)) * (1 + double_b[ptr]) * pow(2,expo_b);
					double_T[ptr] = pow(-1,t64.range(63,63)) * (1 + double_T[ptr]) * pow(2,expo_t);
					
					//Add
					if (opcode==716) {
						double_result[ptr] = double_a[ptr]+double_b[ptr];
					}
					
					//Subtract
					if (opcode==717) {
						double_result[ptr] = double_a[ptr]-double_b[ptr];
					}
					
					//Multiply
					if (opcode==718) {
						double_result[ptr] = double_a[ptr]*double_b[ptr];
					}
					
					//Multiply/Add
					if (opcode==860) {
						double_result[ptr] = double_a[ptr]*double_b[ptr]+double_T[ptr];
					}
					
					//Multiply/Subtract
					if (opcode==861) {
						double_result[ptr] = double_a[ptr]*double_b[ptr]-double_T[ptr];
					}
					
					ptr++;
					
				}

				
				//Set Unit Id
				convert_double = true;
				cur_unit_id = single_precision_op_unit_id2;
				break;

				
			//*** BYTE OPS ***//
			//--------------------	
			// Count Ones in Bytes
			//--------------------
			case 692:
				
				//Loop Over Bytes
				for (int ii = 0; ii<128; ii=ii+8) {
					a_bv_byte = A.read().range(ii+8-1,ii);
					a = 0;
					for (int jj=0; jj<8; jj++) {
						if (a_bv_byte.range(jj,jj)==1) {
							a++;
						}
					}
					result_bv.range(ii+8-1,ii) = a;
				}
				
				//Set Unit Id
				cur_unit_id = byte_op_unit_id;
				break;
				
				
			//--------------------	
			// Average Bytes
			//--------------------
			case 211:
				
				//Loop Over Bytes
				for (int ii = 0; ii<128; ii=ii+8) {
					a_byte2.range(7,0) = A.read().range(ii+8-1,ii);
					b_byte2.range(7,0) = B.read().range(ii+8-1,ii);
					a_byte2 = a_byte2+b_byte2+1;
					a_bv_byte2 = a_byte2;

					result_bv.range(ii+8-1,ii) = a_bv_byte2.range(8,1);
				}
				
				//Set Unit Id
				cur_unit_id = byte_op_unit_id;
				break;
				
			
			//--------------------	
			// Absolute Different in Bytes
			//--------------------
			case 83:
				
				//Loop Over Bytes
				for (int ii = 0; ii<128; ii=ii+8) {
					a_byte = A.read().range(ii+8-1,ii);
					b_byte = B.read().range(ii+8-1,ii);
					if (a_byte>b_byte) {
						a_byte = a_byte-b_byte;
					}
					else {
						if (a_byte<b_byte) {
							a_byte = b_byte-a_byte;
						}
						else {
							a_byte = 0;
						}
					}

					a_bv_byte = a_byte;
					result_bv.range(ii+8-1,ii) = a_bv_byte;
				}
				
				//Set Unit Id
				cur_unit_id = byte_op_unit_id;
				break;
			
				
			//--------------------	
			// Sum Bytes Into Half Words
			//--------------------
			case 595:
				
				//Loop Over Words
				for (int ii = 0; ii<128; ii=ii+32) {
					au = A.read().range(ii+32-1,ii);
					bu = B.read().range(ii+32-1,ii);
					
					//Loop Over Bytes
					a16 = 0;
					b16 = 0;
					for (int jj = 0; jj<32; jj=jj+8) {
						a16 = a16+au.range(jj+8-1,jj);
						b16 = b16+bu.range(jj+8-1,jj);
					}
					
					//Assign
					result_bv.range(ii+32-1,ii+16) = b16;
					result_bv.range(ii+16-1,ii) = a16;
				}
				
				//Set Unit Id
				cur_unit_id = byte_op_unit_id;
				break;
				
				
			default:
				cout << "UNKNOWN OPCODE FOUND (EVEN PIPE):" << opcode << endl;
				
				break;
		}
		
		//Float Conversion to Bit Vector
		if (convert_single) {
			
			//Components
			sc_bv<1> single_sb;
			sc_uint<23> single_frac;
			sc_int<8> single_expo;
			float single_conv_temp;
			
			for (int ii=0; ii<4; ii++) {
				
				//Sign-Bit
				single_sb = false;
				if (single_result[ii]<0) {
					single_sb = true;
					single_result[ii] = -single_result[ii];
				}
				
				//Exponent
				single_expo = 0;
				if (single_result[ii]<1 & single_result[ii]!=0) {
					while (single_result[ii]<1) {
						single_result[ii] = single_result[ii]*2;
						single_expo = single_expo-1;
					}
				}
				else {
					while (single_result[ii]>2) {
						single_result[ii] = single_result[ii]/2;
						single_expo = single_expo+1;
					}
				}
				single_expo = single_expo+127;
				
				//Mantissa
				single_frac = 0;
				single_result[ii] = single_result[ii]-1;
				for (int jj = 22; jj>=0; jj--) {
					single_conv_temp = single_result[ii]*2;
					if (single_conv_temp>1) {
						single_frac.range(jj,jj) = true;
						single_conv_temp = single_conv_temp-1;
					}
					single_result[ii] = single_conv_temp; 
				}
				
				//Assign
				result_bv.range(31+32*ii,31+32*ii) = single_sb;
				result_bv.range(30+32*ii,23+32*ii) = single_expo;
				result_bv.range(22+32*ii,0+32*ii) = single_frac;
			}	
		}

		//Double Conversion to Bit Vector
		if (convert_double) {
			
			//Components
			sc_bv<1> double_sb;
			sc_uint<52> double_frac;
			sc_int<11> double_expo;
			double double_conv_temp;
			
			for (int ii=0; ii<2; ii++) {
				
				//Sign-Bit
				double_sb = false;
				if (double_result[ii]<0) {
					double_sb = true;
					double_result[ii] = -double_result[ii];
				}
				
				//Exponent
				double_expo = 0;
				if (double_result[ii]<1 & double_result[ii]!=0) {
					while (double_result[ii]<1) {
						double_result[ii] = double_result[ii]*2;
						double_expo = double_expo-1;
					}
				}
				else {
					while (double_result[ii]>2) {
						double_result[ii] = double_result[ii]/2;
						double_expo = double_expo+1;
					}
				}
				double_expo = double_expo+1023;
				
				//Mantissa
				double_frac = 0;
				double_result[ii] = double_result[ii]-1;
				for (int jj = 51; jj>=0; jj--) {
					double_conv_temp = double_result[ii]*2;
					if (double_conv_temp>1) {
						double_frac.range(jj,jj) = true;
						double_conv_temp = double_conv_temp-1;
					}
					double_result[ii] = double_conv_temp; 
				}
				
				//Assign
				result_bv.range(63+64*ii,63+64*ii) = double_sb;
				result_bv.range(62+64*ii,52+64*ii) = double_expo;
				result_bv.range(51+64*ii,0+64*ii) = double_frac;
				
			}
		}
		
		//Assignments
		switch (cur_unit_id) {
			
			//No-Op
			case 9:
				Result[0].write(0);
				Result_Destination[0].write(0);
				Stage_When_Ready[0].write(0);
				Reg_Write[0].write(false);
				Unit_Id[0].write(cur_unit_id);
				break;
			
			//Simple Fixed 1
			case 1:
				Result[0].write(result_bv);
				Result_Destination[0].write(rt);
				Stage_When_Ready[0].write(simple_fixed_op_latency1);
				Reg_Write[0].write(false);
				Unit_Id[0].write(cur_unit_id);
				break;
			
			//Simple Fixed 2
			case 2:
				Result[0].write(result_bv);
				Result_Destination[0].write(rt);
				Stage_When_Ready[0].write(simple_fixed_op_latency2);
				Reg_Write[0].write(false);
				Unit_Id[0].write(cur_unit_id);
				break;
			
			//Single Precision 1
			case 3:
				Result[0].write(result_bv);
				Result_Destination[0].write(rt);
				Stage_When_Ready[0].write(single_precision_op_latency1);
				Reg_Write[0].write(false);
				Unit_Id[0].write(cur_unit_id);
				break;
				
			//Single Precision 2
			case 4:
				Result[0].write(result_bv);
				Result_Destination[0].write(rt);
				Stage_When_Ready[0].write(single_precision_op_latency2);
				Reg_Write[0].write(false);
				Unit_Id[0].write(cur_unit_id);
				break;
				
			//Byte
			case 5:
				Result[0].write(result_bv);
				Result_Destination[0].write(rt);
				Stage_When_Ready[0].write(byte_op_latency);
				Reg_Write[0].write(false);
				Unit_Id[0].write(cur_unit_id);
				break;

			default:
				break;
		}
	}
	
	//Print Method
	void print() {
		
		//Clock
		if (print_counter<10) {
			std::cout << print_counter << "  |";
		}
		else {
			if (print_counter<100) {
				std::cout << print_counter << " |";
			}
			else {
				std::cout << print_counter << "|";
			}
		}
		print_counter++;
		
		//Unit ID
		for (int ii=0; ii<7; ii++) {
			std::cout << Unit_Id[ii].read() << ",";
		}
		std::cout << "|";
		
		//Stage_When_Ready
		for (int ii=0; ii<7; ii++) {
			if (Stage_When_Ready[ii].read()>0) {
				std::cout << Stage_When_Ready[ii].read()+1 << ",";
			}
			else {
				std::cout << 0 << ",";
			}
		}
		std::cout << "|";
		
		//Reg_Write
		for (int ii=0; ii<7; ii++) {
			std::cout << Reg_Write[ii].read() << ",";
		}
		std::cout << "|";

		//Result_Destination
		for (int ii=0; ii<7; ii++) {
			if (Result_Destination[ii].read()<10) {
				std::cout << " " <<Result_Destination[ii].read() << ",";
			}
			else {
				std::cout << Result_Destination[ii].read() << ",";
			}

			
		}
		std::cout << "|";
		
		//Flush
		std::cout << flush.read() << " |";

		//Result
		if (print_double) {
			for (int ii=0; ii<7; ii++) {
				sc_biguint<64> temp;
				temp = Result[ii].read().range(127,64);
				std::cout << "["<< temp << ",";
				temp = Result[ii].read().range(63,0);
				std::cout << temp << "],";
			}
		}
		else {
			for (int ii=0; ii<7; ii++) {
				
				if (Result[ii].read().range(31,0)!=Result[ii].read().range(63,32) & Unit_Id[ii].read()==single_precision_op_unit_id2) {
					sc_int<64> temp;
					temp = Result[ii].read().range(63,0);
					std::cout << temp << ",";
				}
				else {
					sc_int<32> temp;
					temp = Result[ii].read().range(31,0);
					std::cout << temp << ",";
				} 
			}
		}
		std::cout << endl;
	}
	
	//Constructir
	SC_CTOR(EvenPipe){
		
		//Methods
		SC_METHOD(update);
		sensitive << clock.pos();
		
		//Printer Setup
		fp = fopen("/Users/asherhensley/Documents/GradSchool/ESE_545/project_part2/CellSpuPipe/Printout.bin","r");
		if (NULL==fp) {
			std::cout << "ERROR: Could not open Printout.bin" << endl;
		}
		fread(&f1,sizeof(f1),1,fp);
		fread(&f2,sizeof(f1),1,fp);
		if (f1==2) {
			SC_METHOD(print);
			sensitive << clock.pos();
		}
		if (f2==32) {
			print_double = false;
		}
		else {
			print_double = true;
		}
		
		//Variables
		simple_fixed_op_latency1 = 2-1;
		simple_fixed_op_latency2 = 4-1;
		single_precision_op_latency1 = 7-1;
		single_precision_op_latency2 = 6-1;
		byte_op_latency = 4-1;
		
		nop_unit_id = 9;
		simple_fixed_op_unit_id1 = 1;
		simple_fixed_op_unit_id2 = 2;
		single_precision_op_unit_id1 = 3;
		single_precision_op_unit_id2 = 4;
		byte_op_unit_id = 5;
		
		print_counter = 1;
		
		//Print Header
		if (f1==2) {
			std::cout << "EVEN PIPE PRINTOUT:"<<endl;
			std::cout << "---|";
			std::cout << "--------------|";
			std::cout << "--------------|";
			std::cout << "--------------|";
			std::cout << "---------------------|";
			std::cout << "--|";
			std::cout << "-------------- " << endl;
			
			std::cout << "Clk|";
			std::cout << "Unit Id       |";
			std::cout << "Stage Ready   |";
			std::cout << "Register Write|";
			std::cout << "Result Destination   |";
			std::cout << "Fl|";
			std::cout << "Result         "<<endl;
			
			std::cout << "---|";
			std::cout << "--------------|";
			std::cout << "--------------|";
			std::cout << "--------------|";
			std::cout << "---------------------|";
			std::cout << "--|";
			std::cout << "-------------- " << endl;
		}
	}
};







