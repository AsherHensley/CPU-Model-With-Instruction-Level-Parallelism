function TestBenchEval(testbenchFile,printout,precision)
%TESTBENCHEVAL() Run TestBench.txt File 
%   TESTBENCHEVAL() runs TestBench.txt file through the Cell SPU Pipeline
%   SystemC model.
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

%File Setup
instructionFile = 'InstructionSet/HensleyInstructionSet_RevC.xlsx';

%Import Instruction Set
[~,~,RAW] = xlsread(instructionFile);
nfields = size(RAW,2);
for kk = 1:nfields
   iset.(RAW{1,kk}) = RAW(2:end,kk);
   if ~ischar(iset.(RAW{1,kk}){1})
       iset.(RAW{1,kk}) = cell2mat(iset.(RAW{1,kk}));
   end
end

%Parse Operation Strings
nops = length(iset.DecOp);
iset.opstr = cell(nops,1);
for kk = 1:nops
    cur = iset.Syntax{kk};
    spc = find(isspace(cur));
    if ~isempty(spc)
        iset.opstr{kk} = cur(1:spc-1);
    else
        iset.opstr{kk} = cur;
    end
end

%Read TestBench.txt File
fid = fopen(testbenchFile,'r');
C = textscan(fid,'%s');
C = C{1};

%Find Instruction Locations
imap = find(ismember(C,iset.opstr));
if any(diff(imap)>2)
    error('Unknown Instruction Found')
end

%Loop Through Instructions
N = length(imap);
xwrite = zeros(N,1,'uint32');
opDec = zeros(N,1);
fmt = zeros(N,1);
pipe = zeros(N,1);
for ii = 1:N
    
    %Get Current Code
    curop = C{imap(ii)};
    
    %Find Instruction
    mask = strcmp(iset.opstr,curop);
    opcode = iset.BinOp{mask};
    opDec(ii) = iset.DecOp(mask);
    fmt(ii) = iset.Fmt(mask);
    pipe(ii) = iset.Pipe(mask);
    syntax = iset.Syntax{mask};
    
    %Parse Syntax
    spc = find(isspace(syntax));
    argcell = {};
    if ~isempty(spc)
       argstr = [',' syntax(spc+1:end) ','];
       com = strfind(argstr,',');
       argcell = cell(1,length(com)-1);
       for jj = 1:length(com)-1
          argcell{jj} = argstr(com(jj)+1:com(jj+1)-1);
       end
       argval = str2num(C{imap(ii)+1});
    end
    
    %Make Binary Instruction
    I = repmat('0',1,32);
    switch fmt(ii)
        case 1
            
            %Assign Opcode
            I(1:11) = opcode;
            
            %Assign Rb
            rbmask = strcmpi(argcell,'rb');
            if any(rbmask)
                I(12:18) = dec2bin(argval(rbmask),7);
            end
            
            %Assign Value
            vmask = strcmpi(argcell,'value');
            if any(vmask)
                I(12:18) = dec2bin(argval(vmask),7);
            end
            
            %Assign Ra
            ramask = strcmpi(argcell,'ra');
            if any(ramask)
                I(19:25) = dec2bin(argval(ramask),7);
            end
            
            %Assign Rt
            rtmask = strcmpi(argcell,'rt');
            if any(rtmask)
                I(26:32) = dec2bin(argval(rtmask),7);
            end
            
        case 2
            
            %Assign Opcode
            I(1:9) = opcode;
            
            %Assign Symbol
            smask = strcmpi(argcell,'symbol');
            if any(smask)
                if argval(smask)<0
                    argval(smask) = twosComplement(argval(smask),16);
                end
                I(10:25) = dec2bin(argval(smask),16);
            end
            
            %Assign Rt
            rtmask = strcmpi(argcell,'rt');
            if any(rtmask)
                I(26:32) = dec2bin(argval(rtmask),7);
            end
            
        case 3
            
            %Assign Opcode
            I(1:8) = opcode;
            
            %Assign Value
            vmask = strcmpi(argcell,'value');
            if any(vmask)
                if argval(vmask)<0
                    argval(vmask) = twosComplement(argval(vmask),10);
                end
                I(9:18) = dec2bin(argval(vmask),10);
            end
            
            %Assign Ra
            ramask = strcmpi(argcell,'ra');
            if any(ramask)
                I(19:25) = dec2bin(argval(ramask),7);
            end
            
            %Assign Rt
            rtmask = strcmpi(argcell,'rt');
            if any(rtmask)
                I(26:32) = dec2bin(argval(rtmask),7);
            end
            
        case 4
            
            %Assign Opcode
            I(1:4) = opcode;
            
            %Assign Rt
            rtmask = strcmpi(argcell,'rt');
            I(5:11) = dec2bin(argval(rtmask),7);
            
            %Assign Rb
            rbmask = strcmpi(argcell,'rb');
            I(12:18) = dec2bin(argval(rbmask),7);
            
            %Assign Ra
            ramask = strcmpi(argcell,'ra');
            I(19:25) = dec2bin(argval(ramask),7);
            
            %Assign Ra
            rcmask = strcmpi(argcell,'rc');
            I(26:32) = dec2bin(argval(rcmask),7);
            
        otherwise
            error('I dont''t know that one')
    end
    
    %Update
    xwrite(ii) = bin2dec(I);
    
end

%Write TestBench.bin
fid = fopen('CellSpuPipe/TestBench.bin','w');
fwrite(fid,xwrite,'uint32');
fclose all;

%Write Format.bin Database
fid = fopen('CellSpuPipe/Format.bin','w');
fwrite(fid,fmt,'uint32');
fclose all;

%Write Pipe.bin Database
fid = fopen('CellSpuPipe/Pipe.bin','w');
fwrite(fid,pipe+1,'uint32');
fclose all;

%Write Registers.bin
regfile = [...
    0,0,0,0;...
    0,0,0,1;...
    0,0,0,2;...
    0,0,0,7;...
    0,0,0,10;...
    0,0,0,4294967295;...
    0,0,0,299792458;...
    0,0,1078530010,1078530010;...
    0,0,3204448256,3204448256;...
    0,0,1074118410,2333366122;...
    0,0,1073127582,1719614413;...
    0,0,3218079744,0;...
    0,0,2963868871,2963868871;...
    0,0,0,1;...
    1,0,0,0;...
    zeros(113,4),...
    ];
regfile = uint32(regfile');
fid = fopen('CellSpuPipe/Registers.bin','w');
fwrite(fid,regfile(:),'uint32');
fclose all;

%Write Printout.bin
fid = fopen('CellSpuPipe/Printout.bin','w');
switch lower(printout)
    case '-printeven'
        fwrite(fid,2,'uint32');
    case '-printodd'
        fwrite(fid,3,'uint32');
    otherwise
        error('Unknown Print Out Type')
end
switch lower(precision)
    case '-32bit'
        fwrite(fid,32,'uint32');
    case '-64bit'
        fwrite(fid,64,'uint32');
    otherwise
        error('Unknown Precision Type')
end
fclose all;
        
%Run CellSpuPipe.exe
system('CellSpuPipe/build/debug/CellSpuPipe');

% regfile[0] = 0;
% regfile[1] = 1;
% regfile[2] = 2;
% regfile[3] = 7;
% regfile[4] = 10;
% regfile[5] = 4294967295;   //-1 (uint32, 2s complement)
% regfile[6] = 299792458;	   //Speed of Light (uint32)
% 
% //Pi (float)
% regfile[7].range(31,0) = 1078530010;
% regfile[7].range(63,32) = 1078530010;
% 
% //-1/2 (float)
% regfile[8].range(31,0) = 3204448256;
% regfile[8].range(63,32) = 3204448256;
% 
% //e constant (double)
% regfile[9].range(31,0) = 2333366122;
% regfile[9].range(63,32) = 1074118410;
% 
% //sqrt(2) (double)
% regfile[10].range(31,0) = 1719614413;
% regfile[10].range(63,32) = 1073127582;
% 
% //-1/4 (double)
% regfile[11].range(31,0) = 0;
% regfile[11].range(63,32) = 3218079744;
% 
% //-1.22999999074835e-09 (float)
% regfile[12].range(31,0) = 2963868871;
% regfile[12].range(63,32) = 2963868871;
% 
% //Shift and Rotate Test Values
% regfile[13] = 1;
% regfile[14].range(127,96) = 1;
% regfile[14].range(95,0) = 0;
% 
% //Blank Registers
% for (int ii=15; ii<128; ii++) {
%     regfile[ii] = 0;
% }

function y = twosComplement(x,b)

if any((x<-2^(b-1) | (x>=2^(b-1))))
    error('x must satisfy -2^(b-1) <= x < 2^(b-1))')
end
s = sign(x);
sb = s<0;
y = (1-sb).*x + sb.*(2^b+x);








