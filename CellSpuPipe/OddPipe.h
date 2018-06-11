/*
 *  OddPipe.h
 *  CellSpuPipe Odd Pipeline Operations Module
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

SC_MODULE(OddPipe) {
	
	//Inputs
	sc_in<bool> clock;
	sc_in<sc_uint<32> > info[4]; //info = [PC,opcode,rt,value];
	sc_in<sc_biguint<128> > A, B, C, T;
	
	//Outputs
	sc_out<sc_biguint<128> > Result[8];
	sc_out<sc_uint<8> > Result_Destination[8];
	sc_out<sc_uint<8> > Stage_When_Ready[8];
	sc_out<bool> Reg_Write[8];
	sc_out<sc_uint<8> > Unit_Id[8];
	sc_out<sc_uint<32> > Miss_Instructions[8][33];
	sc_out<sc_int<32> > pc_plus_offset[8];
	sc_out<sc_uint<3> > flush;
	
	//Variables
	int permute_op_latency;
	int local_store_latency;
	int branch_op_latency;
	int nop_unit_id;
	int permute_op_unit_id;
	int local_store_unit_id;
	int branch_op_unit_id;
	bool print_double;
	FILE *fp;
	int f1,f2;
	int print_counter;
	
	//32 KByte Local Store 
	sc_biguint<128> local_store[2048];
	
	//File Handles
	FILE *fp1;

	//Update Method
	void update(){
		
		//Process New Instruction
		unsigned int PC = info[0].read();
		unsigned int opcode = info[1].read();
		unsigned int rt = info[2].read();
		sc_bv<32> value_bv = info[3].read();
		
		//Shift Pipe Data
		bool flush_write = false;
		for (int ii=7; ii>0; ii--) {
			
			Result[ii].write(Result[ii-1].read());
			Result_Destination[ii].write(Result_Destination[ii-1].read());
			Stage_When_Ready[ii].write(Stage_When_Ready[ii-1].read());
			Reg_Write[ii].write(Reg_Write[ii-1].read());
			Unit_Id[ii].write(Unit_Id[ii-1].read());
			pc_plus_offset[ii].write(pc_plus_offset[ii-1]);
			
			if (Stage_When_Ready[ii-1].read()==ii & Unit_Id[ii-1].read()!=8) {
				Reg_Write[ii].write(true);
			}

			for (int jj=0; jj<33; jj++) {
				Miss_Instructions[ii][jj].write(Miss_Instructions[ii-1][jj].read());
			}
			
			if (ii==branch_op_latency+1 & pc_plus_offset[ii-1].read()>0) {
				flush.write(3);
				flush_write = true;
			}
			
		}
		
		if (!flush_write) {
			flush.write(0);
		}
		
		sc_int<32> a, b;
		sc_uint<32> a32, b32;
		sc_bv<32> a_bv32, b_bv32;
		sc_bv<128> a_bv, b_bv, result_bv;
		sc_uint<8> byte_offset;
		sc_bv<128> zero_pad = 0;
		sc_uint<32> temp;
		sc_bv<32> branch_offset_bv;
		sc_int<32> branch_target = 0;
		bool branch_taken = false;
		sc_uint<32> local_store_index;
		bool local_store_write = false;
		
		//Run
		int cur_unit_id;
		int ptr;
		double shift = 0;
		switch (opcode) {
				
				
			//--------------------
			// No-Op
			//--------------------
			case 0:
			case 1:
				cur_unit_id = nop_unit_id;
				break;
				

			//*** PERMUTE OPS ***//
				
				
			//--------------------
			// Shift Left Quadword
			//--------------------
			case 479:
				
				//Byte Offset
				byte_offset = B.read().range(4,0);
				
				//Shift
				a_bv = A.read();
				if (byte_offset>0) {
					for (int jj=0; jj<byte_offset*8; jj++) {
						b_bv = (a_bv.range(126,0),false);
						a_bv = b_bv;
					}
				}
				result_bv = a_bv;

				//Set Unit Id
				cur_unit_id = permute_op_unit_id;
				break;
				
				
			//--------------------
			// Rotate Quadword
			//--------------------
			case 476:
				
				//Byte Offset
				byte_offset = B.read().range(3,0);
				
				//Rotate
				a_bv = A.read();
				if (byte_offset>0) {
					for (int jj=0; jj<byte_offset*8; jj++) {
						b_bv = (a_bv.range(126,0),a_bv.range(127,127));
						a_bv = b_bv;
					}
				}
				result_bv = a_bv;
				
				//Set Unit Id
				cur_unit_id = permute_op_unit_id;
				break;
				
				
			//*** LOCAL STORE OPS ***//	
				
				
			//--------------------
			// Miss
			//--------------------
			case 1024:

				//Fetch 32 Instructions From Local Store
				shift = PC/16;
				shift = floor(shift);
				ptr = 0;
				for (int jj=shift; jj<shift+8; jj++) {
					for (int ii = 0; ii<128; ii=ii+32) {
						temp = local_store[jj].range(ii+32-1,ii);
						Miss_Instructions[0][ptr].write(temp);
						ptr++;
					}
				}
				
				//Append PC
				Miss_Instructions[0][ptr].write(PC);
				
				//Set Unit Id
				cur_unit_id = nop_unit_id;
				break;
				
				
			//--------------------
			// Store Quadword (x-form)
			//--------------------
			case 324:
				
				//Compute & Quantize Address
				a = A.read().range(31,0);
				b = B.read().range(31,0);
				a_bv32 = a+b;
				a_bv32.range(3,0) = 0;
				local_store_index = a_bv32;
				local_store_index = local_store_index/16;				
				local_store_write = true;
				
				//Set Unit Id
				cur_unit_id = local_store_unit_id;
				break;
				
				
			//--------------------
			// Load Quadword (x-form)
			//--------------------
			case 452:
				
				//Compute & Quantize Address
				a = A.read().range(31,0);
				b = B.read().range(31,0);
				a_bv32 = a+b;
				a_bv32.range(3,0) = 0;
				local_store_index = a_bv32;
				local_store_index = local_store_index/16;				
				local_store_write = false;

				//Set Unit Id
				cur_unit_id = local_store_unit_id;
				break;
				
				
			//--------------------
			// Store Quadword (a-form)
			//--------------------
			case 69:
				
				//Compute & Quantize Address
				a_bv32 = (value_bv.range(15,0),false,false);
				for (int ii = 18; ii<32; ii++) {
					a_bv32.range(ii,ii) = a_bv32.range(ii-1,ii-1);
				}
				a_bv32.range(3,0) = 0;
				local_store_index = a_bv32;
				local_store_index = local_store_index/16;				
				local_store_write = true;
				
				//Set Unit Id
				cur_unit_id = local_store_unit_id;
				break;
				
				
			//--------------------
			// Load Quadword (a-form)
			//--------------------
			case 97:
				
				//Compute & Quantize Address
				a_bv32 = (value_bv.range(15,0),false,false);
				for (int ii = 18; ii<32; ii++) {
					a_bv32.range(ii,ii) = a_bv32.range(ii-1,ii-1);
				}
				a_bv32.range(3,0) = 0;
				local_store_index = a_bv32;
				local_store_index = local_store_index/16;				
				local_store_write = false;
				
				//Set Unit Id
				cur_unit_id = local_store_unit_id;
				break;
				
				
			//*** BRANCH OPS ***//		
				
				
			//--------------------
			// Branch Relative
			//--------------------
			case 100:
				
				//Compute Branch Target
				branch_taken = true;
				branch_offset_bv = (value_bv.range(15,0),false,false);
				for (int ii = 18; ii<32; ii++) {
					branch_offset_bv.range(ii,ii) = branch_offset_bv.range(ii-1,ii-1);
				}
				branch_target = branch_offset_bv;
				branch_target = branch_target+PC;
				
				//Set Unit Id
				cur_unit_id = branch_op_unit_id;
				break;
				
				
			//--------------------
			// Branch Absolute
			//--------------------
			case 96:
				
				//Compute Branch Target
				branch_taken = true;
				branch_offset_bv = (value_bv.range(15,0),false,false);
				for (int ii = 18; ii<32; ii++) {
					branch_offset_bv.range(ii,ii) = branch_offset_bv.range(ii-1,ii-1);
				}
				branch_target = branch_offset_bv;
				
				//Set Unit Id
				cur_unit_id = branch_op_unit_id;
				break;
				
			//--------------------
			// Branch If Not Zero Word
			//--------------------
			case 66:
				
				//Branch Condition
				if (T.read().range(31,0)!=0) {
					
					//Compute Branch Target
					branch_taken = true;
					branch_offset_bv = (value_bv.range(15,0),false,false);
					for (int ii = 18; ii<32; ii++) {
						branch_offset_bv.range(ii,ii) = branch_offset_bv.range(ii-1,ii-1);
					}
					branch_target = branch_offset_bv;
					branch_target = branch_target+PC;
					
				}

				//Set Unit Id
				cur_unit_id = branch_op_unit_id;
				break;
				
				
			//--------------------
			// Branch If Zero Word
			//--------------------
			case 71:
				
				//Branch Condition
				if (T.read().range(31,0)==0) {
					
					//Compute Branch Target
					branch_taken = true;
					branch_offset_bv = (value_bv.range(15,0),false,false);
					for (int ii = 18; ii<32; ii++) {
						branch_offset_bv.range(ii,ii) = branch_offset_bv.range(ii-1,ii-1);
					}
					branch_target = branch_offset_bv;
					branch_target = branch_target+PC;
					
				}
				
				//Set Unit Id
				cur_unit_id = branch_op_unit_id;
				break;
				
				
			default:
				cout << "UNKNOWN OPCODE FOUND (ODD PIPE):" << opcode << endl;
				
				break;
		}
		
		//Chk for Negative Branch Target
		if (branch_target<0) {
			std::cout<< "WARNING: Negative PC Target Detected"<<endl;
		}
		
		//Chk for Branches & Flush
		for (int jj=0; jj<4; jj++) {
			if (pc_plus_offset[jj].read()>0) {
				cur_unit_id = 9;
				break;
			}
		}

		//Update Miss Instruction Buffer
		if (opcode!=1024) {
			ptr = 0;
			for (int jj=0; jj<8; jj++) {
				for (int ii = 0; ii<128; ii=ii+32) {
					Miss_Instructions[0][ptr].write(0);
					ptr++;
				}
			}
			Miss_Instructions[0][ptr].write(0);
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
				pc_plus_offset[0].write(-1);
				break;
			
			//Permute
			case 6:
				Result[0].write(result_bv);
				Result_Destination[0].write(rt);
				Stage_When_Ready[0].write(permute_op_latency);
				Reg_Write[0].write(false);
				Unit_Id[0].write(cur_unit_id);
				pc_plus_offset[0].write(-1);
				break;
				
			//Local Store
			case 7:
				
				if (local_store_write) {
					
					Result[0].write(0);
					Result_Destination[0].write(0);
					Stage_When_Ready[0].write(0);
					local_store[local_store_index] = T.read();
				}
				else {
					
					Result[0].write(local_store[local_store_index]);
					Result_Destination[0].write(rt);
					Stage_When_Ready[0].write(local_store_latency);
				}

				
				Reg_Write[0].write(false);
				Unit_Id[0].write(cur_unit_id);
				pc_plus_offset[0].write(-1);
				
				
				break;
			
			//Branch
			case 8:
				Result[0].write(0);
				Result_Destination[0].write(0);
				Stage_When_Ready[0].write(branch_op_latency);
				Reg_Write[0].write(false);
				Unit_Id[0].write(cur_unit_id);
				if (branch_taken) {
					pc_plus_offset[0].write(branch_target+1);
				}
				else {
					pc_plus_offset[0].write(-1);
				}
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
				std::cout << " "<<Result_Destination[ii].read() << ",";
			}
			else {
				std::cout << Result_Destination[ii].read() << ",";
			}

			
		}
		std::cout << "|";
		
		//Print PC+Offset (Note: 'if' statements are for print spacing)
		int pc_to_print = pc_plus_offset[3].read();
		if (pc_to_print>0){
			pc_to_print = pc_to_print-1;
		}
		if (pc_plus_offset[3].read()<10 & pc_plus_offset[3].read()>=0) {
			std::cout << " " << pc_to_print << " ";
		}
		else {
			if (pc_plus_offset[3].read()<100) {
				std::cout << pc_to_print << " ";
			}
			else {
				std::cout << pc_to_print;
			}

		}
		std::cout << "|";
		
		//Result
		if (print_double) {
			for (int ii=0; ii<7; ii++) {
				sc_biguint<64> temp;
				temp = Result[ii].read().range(127,64);
				std::cout <<"["<< temp << ",";
				temp = Result[ii].read().range(63,0);
				std::cout << temp << "],";
			}
		}
		else {
			sc_biguint<32> temp;
			for (int ii=0; ii<7; ii++) {
				temp = Result[ii].read().range(31,0);
				std::cout << temp << ",";
			}
		}
		std::cout << endl;
	}
	
	
	//Constructor
	SC_CTOR(OddPipe){
		
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
		if (f1==3) {
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
		permute_op_latency = 4-1;
		local_store_latency = 6-1;
		branch_op_latency = 3-1;
		nop_unit_id = 9;
		permute_op_unit_id = 6;
		local_store_unit_id = 7;
		branch_op_unit_id = 8;
		
		print_counter = 1;
		
		//Open Testbench.bin
		fp1 = fopen("/Users/asherhensley/Documents/GradSchool/ESE_545/project_part2/CellSpuPipe/TestBench.bin","r");
		if (NULL==fp1) {
			std::cout << "ERROR: Could not open TestBench.bin" << endl;
		}
		
		//Write Instructions to Local Store
		unsigned int I;
		for (int ii=0; ii<2048; ii++) {
			for (int jj=1; jj<5; jj++) {
				if (feof(fp1)) {
					local_store[ii].range(32*jj-1,32*jj-32) = 0;
				}
				else {
					fread(&I,sizeof(I),1,fp1);
					local_store[ii].range(32*jj-1,32*jj-32) = I;
				}	
			}
		}
		
		//Print-Out Header
		if (f1==3) {
			std::cout << "ODD PIPE PRINTOUT:"<<endl;
			std::cout << "---|";
			std::cout << "--------------|";
			std::cout << "--------------|";
			std::cout << "--------------|";
			std::cout << "---------------------|";
			std::cout << "---|";
			std::cout << "-------------- " << endl;
			
			std::cout << "Clk|";
			std::cout << "Unit Id       |";
			std::cout << "Stage Ready   |";
			std::cout << "Register Write|";
			std::cout << "Result Destination   |";
			std::cout << "PC |";
			std::cout << "Result           "<<endl;
			
			std::cout << "---|";
			std::cout << "--------------|";
			std::cout << "--------------|";
			std::cout << "--------------|";
			std::cout << "---------------------|";
			std::cout << "---|";
			std::cout << "-------------- " << endl;
		}
	}
};









