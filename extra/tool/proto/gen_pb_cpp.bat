@echo off
set curpath=%cd%
echo ======================================================================
echo gen cpp pb start...
start python gen_pb.py cpp ./ ./
echo gen cpp pb end....
echo ======================================================================
pause

