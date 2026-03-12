\# DtTsa



DtTsa is a synchronization-aware dynamic thread-sharing analysis built on top of an LLVM instrumentation pass and a lightweight runtime library.  

It instruments memory accesses together with key synchronization events, records execution traces during program runs, and reports source-level \*\*thread-sharing points\*\* that can be used for program understanding, concurrency debugging, and schedule-focused testing.



This repository contains:



\- the LLVM pass implementation (`src/MemAccessInstrumentPass.cpp`)

\- the runtime support library (`src/runtime.c`)

\- the benchmark programs used in our evaluation (`evaluation/`)

\- the analysis outputs produced by DtTsa on the three benchmark suites (`evaluation/results/`)



---



\## Repository Layout



```text

DtTsa/

├── README.md

├── .gitignore                 # recommended

├── LICENSE                    # recommended

├── src/

│   ├── MemAccessInstrumentPass.cpp

│   └── runtime.c

└── evaluation/

&nbsp;   ├── CVE-Benchmark/         # benchmark sources (recommended unified naming)

&nbsp;   ├── DataRaceBench/

&nbsp;   ├── SCTBench/

&nbsp;   └── results/

&nbsp;       ├── CVE-Benchmark/

&nbsp;       ├── DataRaceBench/

&nbsp;       └── SCTBench/

```



> \*\*Naming recommendation.\*\*  

> For consistency, it is better to use the same dataset names everywhere:

> `CVE-Benchmark`, `DataRaceBench`, and `SCTBench`.  

> If your current local folders are named `dataracebench` / `sctbench`, consider renaming them before publication.



---



\## What DtTsa Does



Given a multithreaded C/C++ program, DtTsa:



1\. compiles the program to LLVM bitcode,

2\. inserts instrumentation before memory accesses and synchronization operations,

3\. links the instrumented program with a runtime support library,

4\. records dynamic trace events during execution,

5\. aggregates the observed events across runs, and

6\. reports \*\*source-level thread-sharing points\*\* together with their frequency across runs.



DtTsa is designed to capture \*\*executed cross-thread sharing behavior\*\*, rather than only static may-share approximations or only racy access pairs.



---



\## Environment Requirements



The following environment is recommended:



\- Linux or WSL2 (Ubuntu recommended)

\- `clang` / `clang++`

\- `llvm-config`

\- `opt`

\- `python3`

\- standard Unix build tools (`bash`, `make`, `sed`, `grep`, `awk`, etc.)



DtTsa is implemented as an LLVM pass plus a runtime library, so the LLVM toolchain version used to build the pass should match the version used to compile and instrument target programs.



\### Suggested checks



```bash

clang --version

clang++ --version

llvm-config --version

opt --version

python3 --version

```



---



\## Building DtTsa



\### 1. Build the LLVM instrumentation pass



From the repository root:



```bash

clang++ -fPIC -shared src/MemAccessInstrumentPass.cpp -o libMemAccessInstrumentPass.so \\

&nbsp; `llvm-config --cxxflags --ldflags --system-libs --libs core passes`

```



\### 2. Build the runtime library



```bash

clang -fPIC -shared src/runtime.c -o libruntime.so

```



After these two commands, the repository root should contain:



\- `libMemAccessInstrumentPass.so`

\- `libruntime.so`



---



\## Input and Output



\## Input



DtTsa expects the following inputs:



1\. \*\*Program source code\*\* written in C/C++

2\. \*\*LLVM bitcode\*\* generated from the program

3\. \*\*Program executions\*\* (tests, PoCs, or benchmark drivers)



At the repository level, the benchmark inputs are organized under:



\- `evaluation/CVE-Benchmark/`

\- `evaluation/DataRaceBench/`

\- `evaluation/SCTBench/`



\## Output



DtTsa produces dynamic-analysis outputs derived from runtime traces.  

Depending on the benchmark and run script, the output may include:



\- raw trace logs

\- merged or summarized sharing-point reports

\- source-level access-site summaries

\- JSON/TXT/CSV summaries used for evaluation



The benchmark-level outputs are stored under:



\- `evaluation/results/CVE-Benchmark/`

\- `evaluation/results/DataRaceBench/`

\- `evaluation/results/SCTBench/`



At a minimum, the reported outputs identify the source locations involved in cross-thread sharing and, when applicable, the frequency with which they are observed across repeated runs.



---



\## Using DtTsa on a Single Program



This section describes the generic workflow for running DtTsa on one program.



\### Step 1. Compile the target program to LLVM bitcode



For a C program:



```bash

clang -O0 -g -emit-llvm -c target.c -o target.bc

```



For a C++ program:



```bash

clang++ -O0 -g -emit-llvm -c target.cpp -o target.bc

```



If the target uses threads or OpenMP, add the appropriate flags during both compilation and linking, for example:



\- `-pthread`

\- `-fopenmp`



\### Step 2. Instrument the bitcode



Use the DtTsa pass to instrument memory and synchronization events.



Depending on whether your pass is implemented for the \*\*new pass manager\*\* or the \*\*legacy pass manager\*\*, use one of the following invocation styles.



\#### New pass manager style



```bash

opt -load-pass-plugin ./libMemAccessInstrumentPass.so \\

&nbsp;   -passes="<PASS\_NAME>" \\

&nbsp;   target.bc -o target.inst.bc

```



\#### Legacy pass manager style



```bash

opt -load ./libMemAccessInstrumentPass.so \\

&nbsp;   -<PASS\_NAME> \\

&nbsp;   target.bc -o target.inst.bc

```



> Replace `<PASS\_NAME>` with the pass name registered in `src/MemAccessInstrumentPass.cpp`.



\### Step 3. Link the instrumented program with the runtime library



For a C program:



```bash

clang target.inst.bc -L. -lruntime -Wl,-rpath,. -o target.inst

```



For a C++ program:



```bash

clang++ target.inst.bc -L. -lruntime -Wl,-rpath,. -o target.inst

```



If needed, also add `-pthread` and/or `-fopenmp`.



\### Step 4. Run the instrumented executable



```bash

./target.inst

```



Or, if the program requires arguments:



```bash

./target.inst <program-arguments>

```



\### Step 5. Collect and inspect outputs



The exact output filenames depend on your runtime implementation and experimental scripts, but the expected workflow is:



\- run the instrumented program once or repeatedly,

\- let the runtime emit dynamic trace data,

\- post-process the collected traces into source-level thread-sharing points,

\- place the final summaries under `evaluation/results/...`.



---



\## Repeated Runs and Union Aggregation



DtTsa is intended to be run multiple times when the benchmark behavior depends on scheduling or input diversity.



For a subject program \\(p\\), repeated runs produce per-run sharing sets:



\\\[

P\_1, P\_2, \\ldots, P\_N

\\]



The final reported set is the union across runs:



\\\[

P\_{\\mathrm{dyn}}(p) = \\bigcup\_{i=1}^{N} P\_i

\\]



In addition, DtTsa can track how often a source-level sharing point appears across repeated runs and report a stability/frequency measure. This is useful for separating consistently observed sharing sites from rare schedule-sensitive ones.



---



\## Benchmark Suites Included in This Repository



This repository is organized around three benchmark families.



\### 1. CVE-Benchmark



`evaluation/CVE-Benchmark/`



This benchmark suite contains real-world concurrency-related vulnerability subjects.  

The goal here is to test whether DtTsa can recover source-level sharing points that are relevant to known bug-triggering interleavings under available PoCs or test drivers.



\### 2. DataRaceBench



`evaluation/DataRaceBench/`



This suite contains curated concurrency micro-benchmarks, especially useful for evaluating how DtTsa relates to known race-relevant access sites reported by dynamic race detectors.



\### 3. SCTBench



`evaluation/SCTBench/`



This suite contains programs used for controlled concurrency testing and schedule-sensitive bug studies.  

For DtTsa, it is particularly useful for examining repeated-run stability and for evaluating whether reported sharing points can serve as compact schedule-point candidates.



---



\## Evaluation Results Included in This Repository



The repository stores the outputs already produced by DtTsa under:



```text

evaluation/results/

├── CVE-Benchmark/

├── DataRaceBench/

└── SCTBench/

```



These results correspond to the experiments reported in our evaluation.



\### Summary of observed effectiveness



The current repository includes DtTsa outputs on all three benchmark suites.  

At a high level, the observed results are:



\- \*\*DataRaceBench:\*\* DtTsa achieves strong overlap with dynamic race-relevant access sites while keeping the reported sharing set compact.

\- \*\*CVE-Benchmark:\*\* DtTsa identifies bug-relevant sharing sites in real-world subjects, including synchronization-aware order/state bugs beyond plain data races.

\- \*\*SCTBench:\*\* DtTsa reports highly stable sharing points across repeated runs and shows promise as a source of compact schedule-point candidates.



\### RQ1: Stability across runs



The fraction of high-/mid-/low-frequency sharing points (median across programs) is summarized below.



| Dataset | High-frequency | Mid-frequency | Low-frequency |

|---|---:|---:|---:|

| CVE-Benchmark | 77.8% | 20.8% | 1.4% |

| DataRaceBench | 82.3% | 14.1% | 3.6% |

| SCTBench | 84.6% | 12.5% | 2.9% |



These numbers indicate that most reported sharing points are stable across runs, while only a small fraction are strongly schedule/input-sensitive.



\### RQ2: Coverage of race-relevant sites



Micro-averaged endpoint and pair coverage against OpenRace/TSan are summarized below.



| Dataset | OpenRace #Pairs | OpenRace Cov\_end | OpenRace Cov\_pair | TSan #Pairs | TSan Cov\_end | TSan Cov\_pair |

|---|---:|---:|---:|---:|---:|---:|

| CVE-Benchmark | 63 | 0.60 | 0.40 | 12 | 0.45 | 0.42 |

| DataRaceBench | 269 | 0.67 | 0.63 | 201 | 0.64 | 0.61 |

| SCTBench | 5 | 1.00 | 1.00 | 14 | 0.67 | 0.79 |



These results show that DtTsa covers a substantial portion of race-relevant access sites and access pairs reported by dynamic race detectors, while also capturing concurrency-relevant behavior beyond plain races.



\### RQ3: Overhead summary



The repository also includes overhead measurements collected under three builds:



\- \*\*Base\*\*: uninstrumented

\- \*\*DtTsa\*\*: instrumented with our tracing

\- \*\*TSan\*\*: compiled with ThreadSanitizer



| Dataset | Slowdown (DtTsa) | Slowdown (TSan) | Peak RSS KB (B/D/T) | Binary KB (B/D/T) |

|---|---:|---:|---|---|

| CVE-Benchmark | 1.33 | 5.8 | 2800 / 2880 / 14560 | 27.5 / 31.5 / 33.0 |

| DataRaceBench | 4.36 | 12.5 | 2880 / 2881 / 19840 | 19.5 / 19.6 / 19.6 |

| SCTBench | 1.86 | 6.2 | 1920 / 1920 / 10944 | 19.9 / 20.1 / 20.1 |



These measurements indicate that DtTsa is substantially lighter than TSan in both runtime overhead and memory overhead on the evaluated subjects.



---



\## Recommended Repository Hygiene



Although not strictly required, the following files are strongly recommended before publication.



\### `.gitignore`



Do \*\*not\*\* commit:



\- object files

\- shared libraries

\- temporary trace files

\- compiler outputs

\- large raw logs

\- editor caches



A minimal example:



```gitignore

\*.o

\*.so

\*.bc

\*.ll

\*.out

\*.log

\*.tmp

\_\_pycache\_\_/

build/

```



\### `LICENSE`



Add a license file if the code is intended to be shared publicly or reused by others.



\### `docs/` (optional)



If available, you may also include:



\- workflow figures

\- architecture diagrams

\- example reports

\- a copy of the artifact description used in the paper



---



\## Reproducibility Notes



To make this repository easy to use for others, please ensure that:



1\. the LLVM version used to build the pass is documented,

2\. the exact pass invocation (`<PASS\_NAME>`) is confirmed and written explicitly,

3\. the expected runtime output filenames are documented once finalized,

4\. dataset paths in this repository match the paths described in this README,

5\. raw experimental outputs are either pruned or summarized to keep the repository size manageable.



---



\## Citation



If you use this code or the benchmark outputs, please cite the corresponding paper describing DtTsa and its evaluation.



---



\## Contact



For questions about the code, artifact setup, or evaluation results, please contact the repository maintainer(s).



