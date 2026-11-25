# Kaminari benchmark results

Raw JSON files from local GPU runs live here. Commit the raw data, never model weights
or datasets. Each file must contain:

```json
{
  "commit": "<git hash of the fork>",
  "date": "ISO-8601",
  "driver": "<NVIDIA/AMD/Intel driver version>",
  "gpu": "<device name and deviceID>",
  "methodology": {
    "warmup_iterations": 0,
    "measured_iterations": 0,
    "sync": "explicit",
    "timestamps": "GPU timestamp query or wall-clock, labeled",
    "inputs": "seeded and identical across backends"
  },
  "results": []
}
```

Summaries go into `docs/BENCHMARKS.md`; no performance claim may be made without a file
here. Benchmarks run locally; CI only runs cheap shader compile and op tests.
