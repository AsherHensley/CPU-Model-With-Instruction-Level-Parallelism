/*
 *  Issue.h
 *  CellSpuPipe Instruction Issue
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

SC_MODULE(Issue) {
	
	//Inputs
	sc_in<bool> clock;
	sc_in<sc_uint<32> > info1_in[4],info2_in[4];	//info = [PC,opcode,rt,value];
	sc_in<int> rabc1_in[3], rabc2_in[3];			//rabc = [ra,rb,rc];
	sc_in<sc_int<32> > new_pc;
	
	//Forwarding Inputs
	sc_in<sc_biguint<128> > Result1[8], Result2[8];
	sc_in<sc_uint<8> > Result_Destination1[8], Result_Destination2[8];
	sc_in<bool> Reg_Write1[8], Reg_Write2[8];
	sc_in<sc_uint<32> > info1_RF[4],info2_RF[4];
	sc_in<sc_uint<32> > info1_RFa[4],info2_RFa[4];
	sc_in<sc_uint<8> > Stage_When_Ready1[8], Stage_When_Ready2[8];
	
	//Output
	sc_out<bool> stall;
	sc_out<sc_uint<32> > info1_out[4],info2_out[4];	//info = [PC,opcode,rt,value];
	sc_out<int> rabc1_out[3], rabc2_out[3];			//rabc = [ra,rb,rc];
	
	//Variables
	FILE *fp;
	unsigned int nopCode, lnopCode;
	bool stall_flag;
	int route_map[2][2];
	bool print_hazards;
	bool haltSet;
	
	//Buffers
	int pipe1, pipe2;
	sc_uint<32> info1_buf[4], info2_buf[4];
	sc_int<32> rabc1_buf[3], rabc2_buf[3];	
	
	//Route Method
	void route(){
		
		//Store Current Opcodes
		int opcode1 = info1_in[1].read();
		int opcode2 = info2_in[1].read();
		
		//Chk Flush
		if (new_pc.read()>0) {
			
			//Invalidate Opcodes
			opcode1 = 0;
			opcode2 = 0;
			
			//Flush Buffer
			route_map[0][0] = 0;
			route_map[0][1] = 0;
			route_map[1][0] = 0;
			route_map[1][1] = 0;	
			
			//Reset Flags
			stall_flag = false;
			haltSet = false;
		}
		
		//New Instructons
		if (stall_flag==0 & opcode1>0 & opcode2>0 & !haltSet) {
			
			//Assign to Buffers (Don't know pipe)
			for (int ii=0; ii<4; ii++) {
				info1_buf[ii] = info1_in[ii].read();
				info2_buf[ii] = info2_in[ii].read();
			}
			for (int ii=0; ii<3; ii++) {
				rabc1_buf[ii] = rabc1_in[ii].read();
				rabc2_buf[ii] = rabc2_in[ii].read();
			}
			
			//Get Pipe Assignments
			fpos_t pc1 = info1_in[0].read();
			fsetpos (fp,&pc1);
			fread(&pipe1,sizeof(pipe1),1,fp);
			fpos_t pc2 = info2_in[0].read();
			fsetpos (fp,&pc2);
			fread(&pipe2,sizeof(pipe2),1,fp);
			
			//Change Pipe Assignments for Misses
			if (opcode1==1024) {
				pipe1 = 2;
				pipe2 = 1;
			}
			else {
				if (opcode2==1024) {
					pipe2 = 2;
				}
			}

			//Update Route Mapping
			bool C = info1_buf[2]!=info2_buf[2] | (info1_buf[2]==0 & info2_buf[2]==0);
			if (pipe1==1 & pipe2==2 & C) {
				route_map[0][1] = 1;
				route_map[1][1] = 2;
			}
			else {
				
				//Chk Halt Cond. (Pipe1)
				if (pipe1==101) {
					route_map[0][1] = 0;
					route_map[1][1] = 0;
					haltSet = true;
				}
				else {
					
					//Chk Halt Cond. (Pipe1)
					if (pipe2==101) {
						haltSet = true;
						if (pipe1==1) {
							route_map[0][1] = 1;
							route_map[1][1] = 0;
						}
						else {
							route_map[0][1] = 0;
							route_map[1][1] = 1;
						}
					}
					
					//Else: Stall Condition
					else {
						
						//Structural Hazards
						stall_flag = true;
						bool no_case_found = true;
						
						//2 Instructions Routed to EvenPipe
						if (pipe1==1 & pipe2==1) {
							no_case_found = false;
							route_map[0][1] = 1;
							route_map[0][0] = 2;
							if (print_hazards) {
								std::cout<<"STALL: 2 Instr. Routed to EvenPipe" << endl;
							}
						}
						else {
							
							//2 Instructions Routed to OddPipe
							if (pipe1==2 & pipe2==2) {
								no_case_found = false;
								route_map[1][1] = 1;
								route_map[1][0] = 2;
								if (print_hazards) {
									std::cout<<"STALL: 2 Instr. Routed to OddPipe" << endl;
								}	
							}
							else {
								
								//Pipe Swap
								if (pipe1==2 & pipe2==1) {
									no_case_found = false;
									route_map[1][1] = 1;
									route_map[0][0] = 2;
									if (print_hazards) {
										std::cout<<"STALL: Instr. Pipe Swap" << endl;
									}
								}
								else {
									
									//Same Tgt Addr
									bool C = info1_buf[2]==info2_buf[2] & info1_buf[2]>0 & info2_buf[2]>0;
									if(pipe1==1 & pipe2==2 & C){
										no_case_found = false;
										route_map[0][1] = 1;
										route_map[1][0] = 2;
										if (print_hazards) {
											std::cout<<"STALL: 2 Instr. w/ Same Tgt Addr" << endl;
										}
									}
									else {
										
										//No Case Found
										if (no_case_found) {
											std::cout<< info1_buf[2] << "," << info2_buf[2] << endl;
											std::cout<< pipe1 << "," << pipe2 << endl;
											std::cout << "WARNING: Route Mapping Error" << endl;
										}
									}
								}
							}	
						}
					}
				}
			}
		}
		
		//Verify Forwarding for I1
		bool fwd_verify1 = true;
		if (route_map[0][1]==1 | route_map[1][1]==1) {
			
			//Register Data Needed
			int rvec[4];
			rvec[0] = rabc1_buf[0];
			rvec[1] = rabc1_buf[1];
			rvec[2] = rabc1_buf[2];
			rvec[3] = info1_buf[2];
			
			//Register Loop
			for (int ii = 0; ii<4; ii++) {
				
				//Chk for Valid Registers
				if (rvec[ii]>0) {
					
					//Chk RF
					bool C1 = rvec[ii]==info1_RF[2].read() | rvec[ii]==info2_RF[2].read();
					bool C2 = rvec[ii]==info1_RFa[2].read() | rvec[ii]==info2_RFa[2].read();
					if (C1|C2) {
						fwd_verify1 = false;
					}
					
					//Chk Pipes
					else {
						for (int jj = 0; jj<8; jj++) {
							
							//EvenPipe
							if (Result_Destination1[jj].read()==rvec[ii] & Result_Destination1[jj].read()>0) {
								if (Reg_Write1[jj].read()==0) {
									fwd_verify1 = false;
									break;
								}
							}
							
							//OddPipe
							if (Result_Destination2[jj].read()==rvec[ii] & Result_Destination2[jj].read()>0) {
								if (Reg_Write2[jj].read()==0) {
									fwd_verify1 = false;
									break;
								}
							}
						}
					}
				}
			}
		}
		
		//Verify Forwarding for I2
		bool fwd_verify2 = true;
		if (route_map[0][1]==2 | route_map[1][1]==2) {
			
			//Register Data Needed
			int rvec[4];
			rvec[0] = rabc2_buf[0];
			rvec[1] = rabc2_buf[1];
			rvec[2] = rabc2_buf[2];
			rvec[3] = info2_buf[2];
			
			//Register Loop
			for (int ii = 0; ii<4; ii++) {
				
				//Chk for Valid Registers
				if (rvec[ii]>0) {
					
					//Chk I1
					if (rvec[ii]==info1_buf[2]) {
						fwd_verify2 = false;
					}
					else {
						//Chk RF
						bool C1 = rvec[ii]==info1_RF[2].read() | rvec[ii]==info2_RF[2].read();
						bool C2 = rvec[ii]==info1_RFa[2].read() | rvec[ii]==info2_RFa[2].read();
						if (C1|C2) {
							fwd_verify2 = false;
						}
						//Chk Pipes
						else {
							for (int jj = 0; jj<8; jj++) {
								
								//EvenPipe
								if (Result_Destination1[jj].read()==rvec[ii] & Result_Destination1[jj].read()>0) {
									if (Reg_Write1[jj].read()==0) {
										fwd_verify2 = false;
										break;
									}
								}
								
								//OddPipe
								if (Result_Destination2[jj].read()==rvec[ii] & Result_Destination2[jj].read()>0) {
									if (Reg_Write2[jj].read()==0) {
										fwd_verify2 = false;
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		
		//Set Defaults
		sc_uint<32> info1_temp[4];
		info1_temp[0] = 0;
		info1_temp[1] = nopCode;
		info1_temp[2] = 0;
		info1_temp[3] = 0;
		
		sc_uint<32> info2_temp[4];
		info2_temp[0] = 0;
		info2_temp[1] = lnopCode;
		info2_temp[2] = 0;
		info2_temp[3] = 0;
		
		int rabc1_temp[3];
		rabc1_temp[0] = -1;
		rabc1_temp[1] = -1;
		rabc1_temp[2] = -1;
		
		int rabc2_temp[3];
		rabc2_temp[0] = -1;
		rabc2_temp[1] = -1;
		rabc2_temp[2] = -1;
		
		//Chk for Data to Send
		if (route_map[0][1]>0 | route_map[1][1]>0) {
			
			//I1 -> EvenPipe
			if (route_map[0][1]==1 & fwd_verify1) {
				for (int ii=0; ii<4; ii++) {
					info1_temp[ii] = info1_buf[ii];
					info1_buf[ii] = 0;
				}
				for (int ii=0; ii<3; ii++) {
					rabc1_temp[ii] = rabc1_buf[ii];
					rabc1_buf[ii] = 0;
				}
				route_map[0][1] = route_map[0][0];
				route_map[0][0] = 0;
			}
			else {
				
				//I2 -> EvenPipe
				if (route_map[0][1]==2 & fwd_verify2) {
					for (int ii=0; ii<4; ii++) {
						info1_temp[ii] = info2_buf[ii];
						info2_buf[ii] = 0;
					}
					for (int ii=0; ii<3; ii++) {
						rabc1_temp[ii] = rabc2_buf[ii];
						rabc2_buf[ii] = 0;
					}
					route_map[0][1] = route_map[0][0];
					route_map[0][0] = 0;
				}
			}

			//I1 -> OddPipe
			if (route_map[1][1]==1 & fwd_verify1) {
				for (int ii=0; ii<4; ii++) {
					info2_temp[ii] = info1_buf[ii];
					info1_buf[ii] = 0;
				}
				for (int ii=0; ii<3; ii++) {
					rabc2_temp[ii] = rabc1_buf[ii];
					rabc1_buf[ii] = 0;
				}
				route_map[1][1] = route_map[1][0];
				route_map[1][0] = 0;
			}
			else {
				
				//I2 -> OddPipe
				if (route_map[1][1]==2 & fwd_verify2) {
					for (int ii=0; ii<4; ii++) {
						info2_temp[ii] = info2_buf[ii];
						info2_buf[ii] = 0;
					}
					for (int ii=0; ii<3; ii++) {
						rabc2_temp[ii] = rabc2_buf[ii];
						rabc2_buf[ii] = 0;
					}
					route_map[1][1] = route_map[1][0];
					route_map[1][0] = 0;
				}
			}
			
			//Chk for Remaining Inst.
			if (route_map[0][1]==0 & route_map[1][1]==0) {
				if (route_map[0][0]>0) {
					route_map[0][1] = route_map[0][0];
					route_map[0][0] = 0;
				}
				if (route_map[1][0]>0) {
					route_map[1][1] = route_map[1][0];
					route_map[1][0] = 0;
				}
			}
		
			//Update Stall Flag
			if (route_map[0][1]==0 & route_map[1][1]==0) {
				stall_flag = false;
			}
		}
		
		//Stall if we cannot forward
		if (fwd_verify1==0 | fwd_verify2==0) {
			stall_flag = true;
			if (print_hazards) {
				std::cout<<"STALL: Data Hazard Detected" << endl;
			}
		}
		
		//Update Stall Output
		stall.write(stall_flag);

		//Write to Output Ports
		for (int ii=0; ii<4; ii++) {
			info1_out[ii].write(info1_temp[ii]);
			info2_out[ii].write(info2_temp[ii]);
		}
		for (int ii=0; ii<3; ii++) {
			rabc1_out[ii].write(rabc1_temp[ii]);
			rabc2_out[ii].write(rabc2_temp[ii]);
		}
	}
	
	//Constructor
	SC_CTOR(Issue){
		
		//Methods
		SC_METHOD(route);
		sensitive << clock.pos();
		
		//Open Pipe.bin
		fp = fopen("/Users/asherhensley/Documents/GradSchool/ESE_545/project_part2/CellSpuPipe/Pipe.bin","r");
		if (NULL==fp) {
			std::cout << "ERROR: Could not open Pipe.bin" << endl;
		}
		
		//Variables
		haltSet = false;
		print_hazards = false;
		nopCode = 513;
		lnopCode = 1;
		stall_flag = false;
		route_map[0][0] = 0;
		route_map[0][1] = 0;
		route_map[1][0] = 0;
		route_map[1][1] = 0;
		
	}
};




