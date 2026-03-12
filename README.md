# DtTsa

DtTsa is a synchronization-aware dynamic thread-sharing analysis for multithreaded C/C++ programs. It is implemented as an LLVM instrumentation pass together with a lightweight runtime library.

DtTsa instruments memory accesses together with key synchronization events, records dynamic execution traces during program runs, and reports source-level thread-sharing points that can be used for program understanding, concurrency debugging, and schedule-focused testing.

This repository contains:

- the LLVM pass implementation (`src/MemAccessInstrumentPass.cpp`)
- the runtime support library (`src/runtime.c`)
- the analysis outputs produced by DtTsa on three benchmark suites (`evaluation/results/`)

---

## Repository Layout

```text
DtTsa/
├── README.md
├── src/
│   ├── MemAccessInstrumentPass.cpp
│   └── runtime.c
└── evaluation/
    ├── README.md
    └── results/
        ├── CVE-Benchmark/
        ├── DataRaceBench/
        └── SCTBench/
```

- `src/` contains the core implementation of DtTsa.
- `evaluation/README.md` documents how to obtain and organize the three benchmark suites.
- `evaluation/results/` contains the outputs produced by DtTsa on the three benchmark suites.

---

## What DtTsa Does

Given a multithreaded C/C++ program, DtTsa:

1. compiles the program to LLVM bitcode;
2. inserts instrumentation before memory accesses and synchronization operations;
3. links the instrumented program with a runtime support library;
4. records dynamic trace events during execution;
5. aggregates the observed events across runs; and
6. reports source-level thread-sharing points together with their frequency across runs.

Unlike conservative static may-share analysis, DtTsa reports sharing behavior that is actually observed during execution. Unlike plain dynamic race detectors, DtTsa is designed to capture a broader class of concurrency-relevant sharing sites, including order-/state-sensitive accesses that may not appear as plain data races.

---

## Environment Requirements

DtTsa is intended to be built and used on Linux or WSL2.

### Required Tools

- clang
- clang++
- llvm-config
- opt
- python3
- standard Unix utilities such as bash, make, sed, grep, and awk

### Suggested Environment

- Ubuntu 20.04 / 22.04, or WSL2 with Ubuntu
- a consistent LLVM/Clang toolchain version for:
  - clang / clang++
  - opt
  - llvm-config

### Version Check

```bash
clang --version
clang++ --version
opt --version
llvm-config --version
python3 --version
```

Important: the LLVM version used to build the pass should match the LLVM version used to instrument the target program.

---

## Building DtTsa

The following commands were used in our setup and are recommended for building the pass and the runtime library.

### 1. Build the LLVM Pass

From the repository root:

```bash
clang++ `llvm-config --cxxflags` -fPIC -shared src/MemAccessInstrumentPass.cpp \
  -o libMemInst.so `llvm-config --ldflags --system-libs --libs core ipo passes`
```

### 2. Build the Runtime Library

```bash
clang -fPIC -shared src/runtime.c -o libruntime.so -lpthread
```

After successful compilation, the repository root should contain:

- `libMemInst.so`
- `libruntime.so`

---

## Input and Output

### Input

DtTsa takes the following inputs:

1. a multithreaded C/C++ program;
2. LLVM bitcode generated from that program;
3. one or more program runs, such as tests, benchmark drivers, or PoCs.

This repository does not directly store the full benchmark source trees due to repository-size considerations. Instead, the benchmark suites should be obtained separately and placed under the expected local directories described in `evaluation/README.md`.

### Output

DtTsa produces dynamic-analysis outputs derived from runtime traces. Depending on the target program and benchmark setup, the output may include:

- raw trace logs;
- intermediate per-run dynamic records;
- merged source-level sharing summaries;
- text, JSON, or other summarized results.

The benchmark-level outputs included in this repository are stored under:

- `evaluation/results/CVE-Benchmark/`
- `evaluation/results/DataRaceBench/`
- `evaluation/results/SCTBench/`

At a minimum, DtTsa reports source-level thread-sharing points observed during execution. When repeated runs are used, DtTsa can also summarize the frequency or stability of each reported site across runs.

---

## Using DtTsa on a Single Program

This section describes the generic workflow for applying DtTsa to a single program.

### Step 1. Compile the Target Program to LLVM Bitcode

For a C program:

```bash
clang -g -emit-llvm -c target.c -o target.bc
```

For a C++ program:

```bash
clang++ -g -emit-llvm -c target.cpp -o target.bc
```

If the target uses threads or OpenMP, add the corresponding flags during compilation and linking, for example:

- `-pthread`
- `-fopenmp`

### Step 2. Instrument the Bitcode with DtTsa

Use the DtTsa pass to transform the bitcode into an instrumented version.

The pass has been tested with the following invocation style:

```bash
opt -load-pass-plugin /path/to/libMemInst.so -passes=meminst target.bc -o target_inst.bc
```

Replace:

- `/path/to/libMemInst.so` with the actual path to the compiled pass library
- `target.bc` with the input LLVM bitcode file
- `target_inst.bc` with the instrumented output bitcode file

### Step 3. Link the Instrumented Bitcode with the Runtime Library

For a C target:

```bash
clang target_inst.bc /path/to/libruntime.so -lpthread -o prog
```

For a C++ target:

```bash
clang++ target_inst.bc /path/to/libruntime.so -lpthread -o prog
```

Replace:

- `/path/to/libruntime.so` with the actual path to the runtime library
- `prog` with the desired executable name

If needed, also add `-fopenmp`.

### Step 4. Run the Instrumented Executable

```bash
./prog
```

If the program requires command-line arguments:

```bash
./prog <program-arguments>
```

### Step 5. Inspect the Output

After execution, DtTsa emits runtime-generated dynamic-analysis outputs. Depending on the current implementation of `runtime.c`, these outputs may be written as trace files, text logs, or merged summaries.

If your current implementation uses fixed output filenames, document them here explicitly. For example:

- `memtrace.log`
- `thread_accesses.txt`
- `summary_pairs.txt`

If the filenames vary by benchmark, describe the naming convention used in `evaluation/results/`.

---

## Repeated Runs and Union Aggregation

Many concurrency behaviors depend on scheduling and input diversity. DtTsa is therefore intended to support repeated runs of the same program.

For a subject program `p`, repeated runs produce per-run sharing sets:

`P1, P2, ..., PN`

The final reported dynamic sharing set is the union across runs:

`P_dyn(p) = union of Pi over all runs`

DtTsa may also track how frequently a reported sharing point appears across runs. This is useful for distinguishing highly stable thread-sharing points from rare schedule-sensitive ones.

---

## Benchmark Suites

This repository is organized around three benchmark families.

### 1. CVE-Benchmark

This benchmark suite contains real-world concurrency-related vulnerability subjects. It is used to evaluate whether DtTsa can recover source-level sharing sites that are relevant to known bug-triggering interleavings under available PoCs or test drivers.

### 2. DataRaceBench

This suite contains curated concurrency micro-benchmarks, especially suitable for evaluating whether DtTsa covers access sites that are relevant to dynamic race reports.

### 3. SCTBench

This suite contains programs used in controlled concurrency testing and schedule-sensitive bug studies. It is useful for evaluating both the stability of reported sharing points across repeated runs and their potential value as compact schedule-point candidates.

Please see `evaluation/README.md` for dataset sources, expected directory layout, and preparation instructions.

---

## Included Evaluation Results

The repository includes DtTsa outputs under:

```text
evaluation/results/
├── CVE-Benchmark/
├── DataRaceBench/
└── SCTBench/
```

These directories contain the results produced by DtTsa on the three benchmark suites.

---

## Experimental Summary

### RQ1: Stability Across Runs

The fractions of high-, mid-, and low-frequency sharing points (median across programs) are summarized below.

| Dataset | High-frequency | Mid-frequency | Low-frequency |
|---|---:|---:|---:|
| CVE-Benchmark | 77.8% | 20.8% | 1.4% |
| DataRaceBench | 82.3% | 14.1% | 3.6% |
| SCTBench | 84.6% | 12.5% | 2.9% |

These results indicate that most reported sharing points are stable across runs, while only a small fraction are strongly schedule- or input-sensitive.

### RQ2: Coverage of Race-Relevant Sites

Micro-averaged coverage against OpenRace and TSan is summarized below.

| Dataset | OpenRace #Pairs | OpenRace Cov_end | OpenRace Cov_pair | TSan #Pairs | TSan Cov_end | TSan Cov_pair |
|---|---:|---:|---:|---:|---:|---:|
| CVE-Benchmark | 63 | 0.60 | 0.40 | 12 | 0.45 | 0.42 |
| DataRaceBench | 269 | 0.67 | 0.63 | 201 | 0.64 | 0.61 |
| SCTBench | 5 | 1.00 | 1.00 | 14 | 0.67 | 0.79 |

These results show that DtTsa covers a substantial portion of race-relevant access endpoints and pairs reported by dynamic race detectors, while also capturing concurrency-relevant sharing sites beyond plain data races.

### RQ3: Overhead Summary

The following overhead measurements were collected under three builds:

- Base: uninstrumented
- DtTsa: instrumented with our tracing
- TSan: compiled with ThreadSanitizer

| Dataset | Slowdown (DtTsa) | Slowdown (TSan) | Peak RSS KB (B/D/T) | Binary KB (B/D/T) |
|---|---:|---:|---|---|
| CVE-Benchmark | 1.33 | 5.8 | 2800 / 2880 / 14560 | 27.5 / 31.5 / 33.0 |
| DataRaceBench | 4.36 | 12.5 | 2880 / 2881 / 19840 | 19.5 / 19.6 / 19.6 |
| SCTBench | 1.86 | 6.2 | 1920 / 1920 / 10944 | 19.9 / 20.1 / 20.1 |

These measurements suggest that DtTsa is substantially lighter than TSan in both runtime and memory overhead on the evaluated subjects.

---

## Notes on Reproducibility

To help others reproduce the results, please make sure the following information is consistent with the actual repository contents before publication:

1. the LLVM version used to build the pass is documented;
2. the actual pass invocation name (`meminst`) is confirmed and written explicitly;
3. the output filenames emitted by `runtime.c` are documented once finalized;
4. dataset directory names in the repository match the names used in this README;
5. large temporary logs, compiler outputs, and intermediate files are excluded from the public repository.

---

## Contact

For questions about the code, benchmark setup, or included results, please contact the repository maintainer(s).

