# TraceMemoryAccess
### 1. Introduction.
    This pintool trace all the memory access operation and log regins(heap,stack,data section) of operands.
    
    
### 2. Compile and run pintool. 
  #### Compiling    
  ```bash   
  mv TraceMemoryAccess.cpp to pin-packet/source/tool/MyPinTool/TraceMemoryAccess.cpp
  mkdir obj-ia32
  make obj-ia32/TraceMemoryAccess.so TARGET=ia32   
  ```

  #### log memory access in a program        
  ```bash 
  ../../../pin -t obj-ia32/TraceMemoryAccess.so -- ./demoprogram      
  ```    
  #### output
  inst0:memory access ins code   
  memtrace0: memory access tag(heap, stack, global) of ins   
  maps0: address regin of heap, stack and global data   
  
### 3. Run data set.
   #### data set
   https://github.com/njuwangzhilong/TraceMemoryAccess/blob/master/DataSet-Programs%20to%20run/testcases.xlsx
   
   #### result
   https://github.com/njuwangzhilong/TraceMemoryAccess/blob/master/results
