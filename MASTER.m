%% MASTER TEST SCRIPT
%   Runs all test cases in _REPORT.pdf
%
%   Created by Asher Hensley on 02/27/17.
%   Copyright 2017. All rights reserved.
%
%   MIT License
%   
%   Permission is hereby granted, free of charge, to any person obtaining a 
%   copy of this software and associated documentation files (the 
%   "Software"), to deal in the Software without restriction, including 
%   without limitation the rights to use, copy, modify, merge, publish, 
%   distribute, sublicense, and/or sell copies of the Software, and to 
%   permit persons to whom the Software is furnished to do so, subject to 
%   the following conditions:
% 
%   The above copyright notice and this permission notice shall be included 
%   in all copies or substantial portions of the Software.
% 
%   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
%   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
%   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
%   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
%   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
%   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
%   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

%Setup
close all
clear
clc
testid = 'FORMAL';

%% CASE 1: No Hazards

diary([testid '_Results_Case1_NoHazards_EvenPipe.txt'])
disp(' ')
disp('***********************************')
disp('TEST CASE 1: NO HAZARDS (EVEN PIPE)')
disp('***********************************')
TestBenchEval('Script1_NoHazards.txt','-printEven','-32bit')
diary off

diary([testid '_Results_Case1_NoHazards_OddPipe.txt'])
disp(' ')
disp('**********************************')
disp('TEST CASE 1: NO HAZARDS (ODD PIPE) ')
disp('**********************************')
TestBenchEval('Script1_NoHazards.txt','-printOdd','-32bit')
diary off

%% CASE 2: Structural Hazards

diary([testid '_Results_Case2_StructuralHazards_EvenPipe.txt'])
disp(' ')
disp('*******************************************')
disp('TEST CASE 2: STRUCTURAL HAZARDS (EVEN PIPE)')
disp('*******************************************')
TestBenchEval('Script2_StructuralHazards.txt','-printEven','-32bit')
diary off

diary([testid '_Results_Case2_StructuralHazards_OddPipe.txt'])
disp(' ')
disp('******************************************')
disp('TEST CASE 2: STRUCTURAL HAZARDS (ODD PIPE)')
disp('******************************************')
TestBenchEval('Script2_StructuralHazards.txt','-printOdd','-32bit')
diary off

%% CASE 3-1: Data Hazards (No Stalls)

diary([testid '_Results_Case3_DataHazards_NoStalls_EvenPipe.txt'])
disp(' ')
disp('*************************************************')
disp('TEST CASE 3: DATA HAZARDS - NO STALLS (EVEN PIPE)')
disp('*************************************************')
TestBenchEval('Script3_DataHazards1.txt','-printEven','-32bit')
diary off

diary([testid '_Results_Case3_DataHazards_NoStalls_OddPipe.txt'])
disp(' ')
disp('************************************************')
disp('TEST CASE 3: DATA HAZARDS - NO STALLS (ODD PIPE)')
disp('************************************************')
TestBenchEval('Script3_DataHazards1.txt','-printOdd','-32bit')
diary off

%% CASE 3-2: Data Hazards (With Stalls)

diary([testid '_Results_Case3_DataHazards_WithStalls_EvenPipe.txt'])
disp(' ')
disp('***************************************************')
disp('TEST CASE 3: DATA HAZARDS - WITH STALLS (EVEN PIPE)')
disp('***************************************************')
TestBenchEval('Script3_DataHazards2.txt','-printEven','-32bit')
diary off

diary([testid '_Results_Case3_DataHazards_WithStalls_OddPipe.txt'])
disp(' ')
disp('***************************************************')
disp('TEST CASE 3: DATA HAZARDS - WITH STALLS (ODD PIPE)')
disp('***************************************************')
TestBenchEval('Script3_DataHazards2.txt','-printOdd','-32bit')
diary off

%% CASE 4: Control Hazards

diary([testid '_Results_Case4_ControlHazards_EvenPipe.txt'])
disp(' ')
disp('****************************************')
disp('TEST CASE 4: CONTROL HAZARDS (EVEN PIPE)')
disp('****************************************')
TestBenchEval('Script4_ControlHazards.txt','-printEven','-32bit')
diary off

diary([testid '_Results_Case4_ControlHazards_OddPipe.txt'])
disp(' ')
disp('***************************************')
disp('TEST CASE 4: CONTROL HAZARDS (ODD PIPE)')
disp('***************************************')
TestBenchEval('Script4_ControlHazards.txt','-printOdd','-32bit')
diary off

%% CASE 5: Matrix Multiply

diary([testid '_Results_Case5_MatrixMultiply_EvenPipe.txt'])
disp(' ')
disp('********************************************')
disp('TEST CASE 5: 2x2 MATRIX MULTIPLY (EVEN PIPE)')
disp('********************************************')
TestBenchEval('Script5_MatrixMultiply.txt','-printEven','-64bit')
diary off

diary([testid '_Results_Case5_MatrixMultiply_OddPipe.txt'])
disp(' ')
disp('*******************************************')
disp('TEST CASE 5: 2x2 MATRIX MULTIPLY (ODD PIPE)')
disp('*******************************************')
TestBenchEval('Script5_MatrixMultiply.txt','-printOdd','-64bit')
diary off


