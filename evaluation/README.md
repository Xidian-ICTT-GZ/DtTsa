# Evaluation

This directory documents the benchmark suites used in the evaluation of DtTsa and provides the corresponding analysis outputs under `results/`.

## Benchmark Suites

DtTsa was evaluated on three benchmark families:

### 1. CVE-Benchmark

CVE-Benchmark consists of real-world concurrency-related vulnerability subjects.  
In our evaluation, it is used to assess whether DtTsa can recover source-level thread-sharing points that are relevant to bug-triggering interleavings in practical software.

Source repository:
- https://github.com/mryancai/ConVul

### 2. DataRaceBench

DataRaceBench is a curated benchmark suite for data-race analysis, containing a large collection of concurrent micro-benchmarks.  
It is mainly used to evaluate whether DtTsa covers race-relevant access sites and pairs reported by dynamic race-detection tools.

Source repository:
- https://github.com/llnl/dataracebench/tree/master/micro-benchmarks

### 3. SCTBench

SCTBench is a benchmark suite for controlled concurrency testing and schedule-sensitive bug studies.  
It is mainly used to evaluate the stability of DtTsa-reported sharing points across repeated runs and their usefulness as compact schedule-point candidates.

Source repository:
- https://github.com/mc-imperial/sctbench

## Included Results

The DtTsa outputs included in this repository are organized under:

- `results/CVE-Benchmark/`
- `results/DataRaceBench/`
- `results/SCTBench/`

These directories contain the analysis outputs retained for artifact release and documentation.

## Notes

Users who would like to reproduce the evaluation should obtain the benchmark suites from the original repositories listed above and prepare them locally before running DtTsa.
