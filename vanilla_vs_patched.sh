# record the .data for the original and direct versions
mkdir NEW_DATAs
perf record -o NEW_DATAs/original.data -g -F 99 --call-graph=fp ./rdf original input/ 30
# perf record -o NEW_DATAs/equivalent.data -g -F 99 --call-graph=fp ./rdf equivalent input/ 30
# perf record -o NEW_DATAs/transposed.data -g -F 99 --call-graph=fp ./rdf transposed input/ 30
perf record -o NEW_DATAs/direct.data -g -F 99 --call-graph=fp ./rdf direct input/ 30
# perf record -o NEW_DATAs/nsquare.data -g -F 99 --call-graph=fp ./rdf nsquare input/ 30

mkdir Vanilla_vs_G4_GRAPHS
# produce vanilla flamegraphs of original and direct flamegraphs
perf script -i NEW_DATAs/original.data | ./stackcollapse-perf.pl | ./flamegraph.pl -w 1500 --colors java --bgcolors=#FFFFFF > Vanilla_vs_G4_GRAPHS/vanilla_original.svg
perf script -i NEW_DATAs/direct.data | ./stackcollapse-perf.pl | ./flamegraph.pl -w 1500 --colors java --bgcolors=#FFFFFF > Vanilla_vs_G4_GRAPHS/vanilla_direct.svg

# produce the vanilla differential flamegraphs of original and direct
perf script -i NEW_DATAs/original.data | ./stackcollapse-perf.pl > NEW_DATAs/vanilla.f1
perf script -i NEW_DATAs/direct.data | ./stackcollapse-perf.pl > NEW_DATAs/vanilla.f2
./difffolded.pl NEW_DATAs/vanilla.f1 NEW_DATAs/vanilla.f2 | ./flamegraph.pl -w 1500 > Vanilla_vs_G4_GRAPHS/vanilla_diff.svg

# produce geant4-patched flamegraphs of original and direct flamegraphs

perf script -i NEW_DATAs/original.data | ./g4_stackcollapse.pl | ./g4_flamegraph.pl -w 1500 --colors java --bgcolors=#FFFFFF > Vanilla_vs_G4_GRAPHS/g4_original.svg
perf script -i NEW_DATAs/direct.data | ./g4_stackcollapse.pl | ./g4_flamegraph.pl -w 1500 --colors java --bgcolors=#FFFFFF > Vanilla_vs_G4_GRAPHS/g4_direct.svg

# produce the geant4-patched differential flamegraphs of original and direct
perf script -i NEW_DATAs/original.data | ./g4_stackcollapse.pl > NEW_DATAs/g4.f1
perf script -i NEW_DATAs/direct.data | ./g4_stackcollapse.pl > NEW_DATAs/g4.f2
./difffolded.pl NEW_DATAs/g4.f1 NEW_DATAs/g4.f2 | ./g4_flamegraph.pl -w 1500 > Vanilla_vs_G4_GRAPHS/g4_diff.svg 