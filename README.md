# iCSC-Profiling
A small profiling example presented in the iCSC


## Before starting

### Need to allow monitoring events

```
sudo sysctl kernel.perf_event_paranoid=-1
sudo sysctl kernel.kptr_restrict=0
sudo sysctl kernel.nmi_watchdog=0
```

Small threads on the need of these permissions:
https://unix.stackexchange.com/questions/14227/
https://stackoverflow.com/questions/51911368/

Security implications:
https://www.kernel.org/doc/html/latest/admin-guide/perf-security.html
https://unix.stackexchange.com/questions/519070/


### Building ROOT for profiling

```
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" -Dccache=ON ..
```

Build options: https://root.cern/install/build_from_source/#all-build-options

Remark: you can disable options, which are enabled by default, by setting them to OFF.


## Compiling the example and getting the input file

### Compiling the example

```
g++ -O2 -g -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -o rdf rdf.cpp $(root-config --cflags --libs)
```

### Getting the input file

This input file is only ~ 100 MB.

```
mkdir input
wget Run2012B_SingleMu_1M https://root.cern/files/rootbench/Run2012B_SingleMu_1M.root -P input
```

Note that since the file is so small, if you work with only 1 instance of the file, you will notice that most of the work is spent in initialization.

However, you can initialize the dataframe with multiple instances of the file, and then you will see that the work is spent in the event loop. So you do not need to make neither hard or soft links to the file. Fyi: there is a some small difference in I/O performance between these options, studied at https://indico.cern.ch/event/1191473/.

## Profiling the example

### Perf stat

```
perf stat ./rdf original input/ 30
```

The result should be similar to:
```
Info in <TCanvas::Print>: png file original_rdf.png has been created
mean h1 is 26.435
mean h2 is 0.553772

 Performance counter stats for './rdf original input/ 30':

         53 065,07 msec task-clock                #    1,000 CPUs utilized          
               233      context-switches          #    4,391 /sec                   
                 0      cpu-migrations            #    0,000 /sec                   
           181 519      page-faults               #    3,421 K/sec                  
   190 973 918 953      cycles                    #    3,599 GHz                    
   376 844 363 732      instructions              #    1,97  insn per cycle         
    58 532 205 284      branches                  #    1,103 G/sec                  
     1 204 603 018      branch-misses             #    2,06% of all branches        

      53,068551395 seconds time elapsed

      52,082176000 seconds user
       0,914011000 seconds sys
```

In addition, on Intel CPUs, you can get more insight already here with either:
```
perf stat --topdown -a ./rdf original input/ 30
```
or equivalently:
```
perf stat -M TopdownL1 ./rdf original input/ 30
```
or also:
```
perf stat -M Frontend_Bound,Backend_Bound,Bad_Speculation,Retiring time
```

All of the above should give:
```
mean h1 is 26.435
mean h2 is 0.553772

 Performance counter stats for './rdf original input/ 30':

   437 075 361 555      uops_issued.any           #     0,12 Bad_Speculation
                                                  #     0,22 Backend_Bound            (33,33%)                              
   191 218 334 772      cycles
                                                  # 764873339088,00 SLOTS
                                                  #     0,49 Retiring
                                                  #     0,17 Frontend_Bound           (33,33%)                              
   128 748 925 064      idq_uops_not_delivered.core                                     (33,33%)                            
     7 158 428 964      int_misc.recovery_cycles                                      (33,33%)                              
   373 401 307 787      uops_retired.retire_slots                                     (33,33%)                              
     1 311 634 367      cpu_clk_unhalted.one_thread_active #     1,97 CoreIPC_SMT              (33,34%)                     
   376 724 917 454      inst_retired.any          # 376724917454,00 Instructions      (33,34%)                              
     1 312 820 545      cpu_clk_unhalted.ref_xclk                                     (33,34%)                              
   191 017 221 261      cpu_clk_unhalted.thread                                       (33,34%)                              
   376 836 235 015      inst_retired.any          #     1,97 CoreIPC                  (66,67%)                              
   191 049 315 819      cycles                                                        (66,67%)                              

      53,202590594 seconds time elapsed

      52,025563000 seconds user
       0,952009000 seconds sys

```

An AMD CPU would not accept the first two options, but similarly to the last option, you can do:
```
perf stat -M Backend_Bound,Bad_Speculation,Retiring time
```
(As of the moment of this upload (Feb '23) - there is no Frontend_Bound metric for AMD CPUs!)

### Perf record and producing a flamegraph

#### Perf record

```
perf record -o original.data -F 99 --call-graph=fp -g ./rdf original input/ 30
```

One very useful approach is to see which event was reported as a bottleneck by perf stat, and then to record the events that are related to that bottleneck. You can specify this event with the `-e` option of perf record.

#### Perf report

Read the produced data file with perf report:

```
perf report -i original.data
```

So far, you should be able to see where is the bottleneck, but not why, e.g. the place with the most samples. To understand what is the cause, you would need to annotate functions with "a" inside of the report. Refer to the help ("h") of perf report for more information.

#### Flamegraph

It is often highly useful to visually argue about performance. For this, you can use flamegraphs. And you cannot properly open a data file from different machine! So to convince others about your results, you need to produce a flamegraph.

You need to get some necessary tools first:

```
wget https://raw.githubusercontent.com/brendangregg/FlameGraph/master/stackcollapse-perf.pl
wget https://raw.githubusercontent.com/brendangregg/FlameGraph/master/flamegraph.pl
chmod +x stackcollapse-perf.pl flamegraph.pl
```


You can produce a flamegraph with the following command:

```
mkdir graphs
perf script -i original.data | ./stackcollapse-perf.pl | ./flamegraph.pl -w 1500 --colors java --bgcolors=#FFFFFF > graphs/original.svg
```

This should already give you a good idea of what is going on.

For the purpose of this example, convince yourself why find_trijet is expensive. Then pick the optimized version, which you can invoke by simply changing the first extra argument of the executable to "direct".

First of all, assure that the results are identical.

Then you can go through the same steps as above, but with the optimized version.


#### Differential flamegraph

As a final stroke, you would like to assert that your optimization solved the problem that you were trying to solve, and it did not introduce any new overheads.

You can compare multiple perf reports with perf diff, e.g.:

```
perf diff original.data transposed.data direct.data
```

And you can visualize the difference between two reports with a differential flamegraph.

You would first need an extra tool:
```
wget https://raw.githubusercontent.com/brendangregg/FlameGraph/master/difffolded.pl
chmod +x difffolded.pl
```

Then you can produce the differential flamegraph with the following commands:

```
perf script -i original.data | ./stackcollapse-perf.pl > out.folded1
perf script -i direct.data | ./stackcollapse-perf.pl > out.folded2
./difffolded.pl out.folded1 out.folded2 | ./flamegraph.pl -w 1500 > graphs/diff.svg
```

## Sources:

* [Perf man page](https://man7.org/linux/man-pages/man1/perf.1.html)
* [Example Perf commands](https://www.brendangregg.com/perf.html)