# Auto v2: byte budgets and joint resource targets

## Root causes and changes

This remains a private experiment. The production parser, public presets and defaults are unchanged.
The [v1 histories](auto-v2-msvc1951/v1-root-causes.json) explain two distinct failures:
- Speed expanded 86,434 stored tokens to 1,383,202 slots; final count was only 90,180. Dense prefix stability did not describe the whitespace suffix.
- Memory clipped a forecast of 65,538 to 65,534, then applied an 81,917-slot geometric floor near EOF. Old/new buffer overlap dominated peak memory.
- V1 speed copy-pressure threshold >2n was inactive under its 1.5× floor over the representable range. V2 uses >1.5n; memory retains >3n.

V2 applies a byte ceiling to speculative slots beyond the already scanned pending token. Budgets are rounded down by sizeof(Token).
If that ceiling is below the minimum geometric growth, the floor takes precedence and the actual exception is logged.
This avoids additive tiny expansions and preserves amortized-linear token relocation. It is a soft reserve ceiling, not a total-memory or latency guarantee.

A second change uses a proven fact: each future non-EOF token consumes at least one remaining source byte.
pending tokens + remaining bytes + required EOF therefore bounds the final count. When this is below the proposed reserve,
v2 can reserve that bound instead of the geometric floor. The allocation fits all remaining tokens, so no further growth is needed.
This performs no scan or restart. Saturating/checked arithmetic prevents overflow; if the bound cannot fit max_size it is not used.

## Calibration and freeze

Three private candidate pairs were compared on 75 known inputs (all v1 development and evaluation fixtures).
Calibration uses one round × 3 samples × 2 operations and is exploratory, not final performance evidence.
The [pre-recorded plan](../auto-v2/plan.json) filters candidates by memory targets, treats times within 10% of fastest as ties,
and breaks ties by the lowest worst peak/oracle ratio. The offline oracle is the better production fixed mode per document.

| Candidate / goal | Budget MiB | Geomean time/Balanced | Worst peak/oracle | Memory eligible |
| --- | ---: | ---: | ---: | --- |
| tight/speed | 1.00 | 1.073 | 1.428 | True |
| tight/memory | 0.25 | 1.198 | 1.408 | True |
| medium/speed | 4.00 | 1.046 | 1.427 | True |
| medium/memory | 0.50 | 1.200 | 1.386 | True |
| relaxed/speed | 8.00 | 1.105 | 1.566 | True |
| relaxed/memory | 1.00 | 1.202 | 1.357 | True |

Selected speed budget: 4 MiB; memory budget: 1 MiB.
Sources and original candidate observations are committed as `77c50bf9fbf9c254e7247e3094d7c34b1f2dd79b`.
Each goal uses its selected frozen calibration executable; the unused other-goal macro in that artifact does not select behavior.
After repeated known-set measurement, policy sources, budgets and all comparison executables were frozen at `2026-11-06T18:49:16.177294+00:00`.
Only then were 16 new cases generated with seed 20261108 and counts 149/3079: duplicate keys, UTF-8, chunked escaping, ragged numbers,
internal gaps, space-filled string values and front/trailing padding around nested cells. No parameters were retuned using held-out observations.

## Measurement and unchanged acceptance rules

Both stages compare six executables: production Balanced/LowMemory, frozen v1 speed/memory and selected v2 speed/memory.
Six cyclic rounds put every policy in every position once; each round has 5 samples × 3 operations, plus five warm-ups.
Tables use medians of round medians. Loading, semantic checks and shape inspection occur outside timing; parse includes result destruction.
Timing has no journal hooks. Fixed-size stack journals and successful C++ new interception use separate builds.
Captured peak is simultaneous requested heap bytes, including overlapping token buffers. Caller input, stack/root Json, allocator rounding/metadata,
direct malloc/CRT internals and profiler bookkeeping are excluded. This is not RSS. Logical relocation bytes are old live tokens × sizeof(Token).
Journal overlap_bytes is the logical old+requested token-buffer size at an expansion; it excludes source/DOM/allocator overhead.

Joint empirical targets on non-tiny inputs (Balanced ≥1 ms) were fixed before calibration:
- Speed: worst time/oracle ≤1.5, peak/oracle ≤2, peak/Balanced ≤1.5.
- Memory: worst time/Balanced ≤2 and peak/oracle ≤1.5.
All inputs remain in the full tables. These are local evaluation thresholds, not runtime guarantees or CI speed assertions.

| Split / mode | Geomean time/Balanced (all) | Worst time/oracle | Worst time/Balanced | Worst peak/oracle | Worst peak/Balanced | Joint target |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| known/v1-speed | 1.049 | 1.307 | 1.307 | 5.769 | 1.363 | FAIL |
| known/v2-speed | 1.078 | 1.723 | 1.723 | 1.427 | 1.289 | FAIL |
| known/v1-memory | 1.142 | 1.624 | 1.624 | 1.580 | 1.580 | FAIL |
| known/v2-memory | 1.172 | 2.421 | 2.421 | 1.357 | 1.357 | FAIL |
| holdout/v1-speed | 0.999 | 1.101 | 1.101 | 1.390 | 1.390 | PASS |
| holdout/v2-speed | 1.000 | 1.089 | 1.046 | 1.390 | 1.390 | PASS |
| holdout/v1-memory | 1.117 | 1.915 | 1.915 | 1.350 | 1.350 | PASS |
| holdout/v2-memory | 1.188 | 1.758 | 1.758 | 1.350 | 1.350 | PASS |

Original failure cases remeasured in the same run:

| Input | Balanced peak MiB | LowMemory peak MiB | V1 speed | V2 speed | V1 memory | V2 memory |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| held-tail-records-4099.json | 54.116 | 12.219 | 70.494 | 15.132 | 12.231 | 11.207 |
| dense-zeros-32768.json | 4.313 | 6.438 | 4.500 | 4.313 | 6.812 | 5.557 |

## Full parse-time results

Milliseconds; every known/evaluation input is retained.

| Split / input | Balanced | LowMemory | V1 speed | V2 speed | V1 memory | V2 memory |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| known / alternating-262144.json | 22.6804 | 36.0381 | 23.3341 | 33.5759 | 27.2725 | 41.7796 |
| known / alternating-32768.json | 2.5564 | 4.0760 | 3.0659 | 2.9792 | 4.1509 | 3.8875 |
| known / alternating-4096.json | 0.3765 | 0.7096 | 0.5055 | 0.5137 | 0.6944 | 0.6833 |
| known / auto-held-bursts-2053.json | 5.0052 | 5.6678 | 4.6560 | 4.6932 | 5.5166 | 5.6553 |
| known / auto-held-bursts-73.json | 0.1809 | 0.1874 | 0.1823 | 0.1835 | 0.1798 | 0.1883 |
| known / auto-held-inner-padding-2053.json | 3.5223 | 6.2915 | 3.4809 | 3.7829 | 5.5047 | 5.5170 |
| known / auto-held-inner-padding-73.json | 0.1343 | 0.2014 | 0.1882 | 0.1739 | 0.1756 | 0.1953 |
| known / auto-held-late-padding-2053.json | 5.8834 | 6.0468 | 5.7215 | 5.7456 | 6.6134 | 6.1506 |
| known / auto-held-late-padding-73.json | 0.2108 | 0.2108 | 0.2230 | 0.2150 | 0.2389 | 0.2198 |
| known / auto-held-long-keys-2053.json | 2.2956 | 2.3976 | 2.2003 | 2.2316 | 2.2944 | 2.3826 |
| known / auto-held-long-keys-73.json | 0.0563 | 0.0557 | 0.0586 | 0.0549 | 0.0552 | 0.0553 |
| known / auto-held-phases-2053.json | 22.8637 | 25.6845 | 22.7469 | 22.9400 | 23.6741 | 24.1369 |
| known / auto-held-phases-73.json | 0.8907 | 1.0478 | 0.9528 | 0.8807 | 0.8941 | 0.8957 |
| known / auto-held-ragged-numbers-2053.json | 1.7851 | 2.1336 | 1.7041 | 1.6770 | 2.2015 | 2.1744 |
| known / auto-held-ragged-numbers-73.json | 0.0338 | 0.0511 | 0.0359 | 0.0631 | 0.0789 | 0.0827 |
| known / auto-held-spiky-strings-2053.json | 24.2367 | 24.2124 | 24.0164 | 22.8079 | 24.1864 | 23.3528 |
| known / auto-held-spiky-strings-73.json | 1.1386 | 1.0751 | 1.1843 | 1.0795 | 1.1360 | 1.0756 |
| known / auto-held-zigzag-2053.json | 1.6775 | 1.6761 | 1.6863 | 1.6627 | 1.6847 | 1.6883 |
| known / auto-held-zigzag-73.json | 0.0909 | 0.0856 | 0.0820 | 0.0774 | 0.0778 | 0.0869 |
| known / deep-128.json | 0.0109 | 0.0108 | 0.0112 | 0.0112 | 0.0110 | 0.0112 |
| known / deep-64.json | 0.0060 | 0.0060 | 0.0062 | 0.0062 | 0.0061 | 0.0062 |
| known / deep-8.json | 0.0007 | 0.0007 | 0.0007 | 0.0008 | 0.0007 | 0.0008 |
| known / dense-first-262144.json | 21.2126 | 32.9924 | 22.1656 | 32.9219 | 26.8477 | 41.6244 |
| known / dense-first-32768.json | 2.9350 | 4.3231 | 2.8426 | 2.7416 | 4.2476 | 4.3808 |
| known / dense-first-4096.json | 0.2915 | 0.5140 | 0.4297 | 0.4207 | 0.5645 | 0.5435 |
| known / dense-zeros-262144.json | 16.9354 | 29.3924 | 18.9853 | 29.1788 | 22.9021 | 40.9960 |
| known / dense-zeros-32768.json | 2.1428 | 3.5250 | 1.9844 | 2.0081 | 3.3406 | 3.2588 |
| known / dense-zeros-4096.json | 0.1782 | 0.4941 | 0.2497 | 0.2363 | 0.3494 | 0.2919 |
| known / escaped-4096.json | 12.7057 | 12.7415 | 12.9970 | 13.1543 | 13.0117 | 12.5463 |
| known / escaped-512.json | 1.3808 | 1.4577 | 1.4835 | 1.3828 | 1.4266 | 1.4108 |
| known / escaped-64.json | 0.1519 | 0.1490 | 0.1766 | 0.1671 | 0.1560 | 0.1596 |
| known / flat-object-16384.json | 5.4001 | 6.6381 | 5.4013 | 5.3057 | 6.7100 | 6.2042 |
| known / flat-object-2048.json | 0.4822 | 0.6582 | 0.5566 | 0.5589 | 0.5893 | 0.5865 |
| known / flat-object-256.json | 0.0498 | 0.0622 | 0.0517 | 0.0492 | 0.0497 | 0.0502 |
| known / held-empty-mix-257.json | 0.1247 | 0.0861 | 0.1597 | 0.1562 | 0.1264 | 0.1257 |
| known / held-empty-mix-4099.json | 1.7618 | 2.3846 | 1.8249 | 1.8111 | 2.3414 | 2.2624 |
| known / held-escape-transition-257.json | 0.0971 | 0.0956 | 0.0941 | 0.0967 | 0.0945 | 0.0905 |
| known / held-escape-transition-4099.json | 0.5706 | 0.5273 | 0.4897 | 0.5023 | 0.4257 | 0.4194 |
| known / held-front-nested-257.json | 0.7106 | 0.7682 | 0.7845 | 0.7648 | 0.6565 | 0.5947 |
| known / held-front-nested-4099.json | 11.6746 | 16.9377 | 11.6322 | 11.6024 | 12.5621 | 12.3603 |
| known / held-long-keys-257.json | 0.0306 | 0.0322 | 0.0312 | 0.0315 | 0.0321 | 0.0297 |
| known / held-long-keys-4099.json | 0.5171 | 0.5128 | 0.5227 | 0.5475 | 0.5366 | 0.5349 |
| known / held-sawtooth-257.json | 0.2556 | 0.2414 | 0.2374 | 0.2476 | 0.2319 | 0.2387 |
| known / held-sawtooth-4099.json | 4.3726 | 4.4054 | 4.2398 | 4.3663 | 4.0781 | 4.3484 |
| known / held-tail-records-257.json | 0.4550 | 0.4519 | 0.6288 | 0.5800 | 0.5185 | 0.4588 |
| known / held-tail-records-4099.json | 9.1930 | 10.0741 | 9.8468 | 9.8329 | 10.1866 | 10.4993 |
| known / large.json | 16.3165 | 21.3517 | 18.4131 | 22.0011 | 21.4573 | 26.2479 |
| known / long-ascii-4096.json | 9.7030 | 9.5902 | 9.5841 | 9.6593 | 8.8133 | 9.6450 |
| known / long-ascii-512.json | 1.0495 | 1.1401 | 0.9929 | 1.0448 | 0.9822 | 1.0626 |
| known / long-ascii-64.json | 0.1287 | 0.1325 | 0.1168 | 0.1234 | 0.1167 | 0.1315 |
| known / medium.json | 1.7761 | 2.2716 | 1.6451 | 1.6459 | 2.3791 | 2.1781 |
| known / nested.json | 0.0182 | 0.0198 | 0.0176 | 0.0175 | 0.0179 | 0.0186 |
| known / small.json | 0.0911 | 0.0948 | 0.0883 | 0.0873 | 0.0867 | 0.0905 |
| known / sparse-first-262144.json | 33.8238 | 59.5358 | 39.3330 | 37.8817 | 41.4091 | 42.9930 |
| known / sparse-first-32768.json | 4.2569 | 7.4338 | 3.6285 | 3.6874 | 4.5500 | 4.1344 |
| known / sparse-first-4096.json | 0.2218 | 0.2772 | 0.2341 | 0.2457 | 0.2393 | 0.2434 |
| known / tiny.json | 0.0012 | 0.0013 | 0.0012 | 0.0012 | 0.0012 | 0.0012 |
| known / unicode-4096.json | 11.3299 | 11.6339 | 11.4118 | 11.9238 | 12.0980 | 11.6242 |
| known / unicode-512.json | 1.2917 | 1.2237 | 1.2027 | 1.2698 | 1.2791 | 1.2687 |
| known / unicode-64.json | 0.1559 | 0.1468 | 0.1422 | 0.1499 | 0.1456 | 0.1511 |
| known / whitespace-interleaved-16384.json | 3.2597 | 3.6738 | 4.2596 | 4.1759 | 5.0450 | 4.7092 |
| known / whitespace-interleaved-2048.json | 0.3015 | 0.3152 | 0.4305 | 0.4188 | 0.4269 | 0.4261 |
| known / whitespace-interleaved-256.json | 0.0366 | 0.0363 | 0.0526 | 0.0526 | 0.0531 | 0.0524 |
| known / whitespace-leading-16384.json | 4.0338 | 5.5589 | 4.3020 | 4.1684 | 4.3117 | 4.0260 |
| known / whitespace-leading-2048.json | 0.3416 | 0.4057 | 0.3445 | 0.3672 | 0.3764 | 0.3615 |
| known / whitespace-leading-256.json | 0.0418 | 0.0449 | 0.0426 | 0.0424 | 0.0457 | 0.0459 |
| known / whitespace-trailing-16384.json | 3.6059 | 4.2496 | 3.5805 | 3.5604 | 4.1718 | 4.0072 |
| known / whitespace-trailing-2048.json | 0.3323 | 0.3575 | 0.3487 | 0.3287 | 0.3474 | 0.3405 |
| known / whitespace-trailing-256.json | 0.0412 | 0.0437 | 0.0439 | 0.0423 | 0.0414 | 0.0415 |
| known / wide-numbers-262144.json | 23.0819 | 33.1114 | 24.4871 | 33.7236 | 28.7118 | 42.6687 |
| known / wide-numbers-32768.json | 2.9274 | 3.9004 | 2.8083 | 2.8450 | 4.0508 | 4.1287 |
| known / wide-numbers-4096.json | 0.2316 | 0.2705 | 0.2258 | 0.2268 | 0.2417 | 0.2494 |
| known / wide-objects-16384.json | 31.8829 | 43.8889 | 33.2149 | 43.3221 | 42.6274 | 52.5163 |
| known / wide-objects-2048.json | 3.4834 | 4.9462 | 3.6446 | 3.3241 | 4.6360 | 4.7569 |
| known / wide-objects-256.json | 0.3196 | 0.3585 | 0.3374 | 0.3352 | 0.3277 | 0.3454 |
| holdout / v2-held-chunks-149.json | 0.3010 | 0.2902 | 0.2953 | 0.2871 | 0.2813 | 0.3044 |
| holdout / v2-held-chunks-3079.json | 4.9580 | 5.2291 | 4.3399 | 4.2076 | 5.2062 | 5.3958 |
| holdout / v2-held-duplicate-keys-149.json | 0.0709 | 0.0959 | 0.0932 | 0.0896 | 0.0831 | 0.0814 |
| holdout / v2-held-duplicate-keys-3079.json | 1.1651 | 2.1020 | 1.2824 | 1.2184 | 2.2314 | 2.0488 |
| holdout / v2-held-front-cells-149.json | 0.3761 | 0.4629 | 0.3238 | 0.3082 | 0.4524 | 0.4151 |
| holdout / v2-held-front-cells-3079.json | 7.8683 | 11.0544 | 7.5579 | 7.4440 | 8.1649 | 8.5051 |
| holdout / v2-held-internal-gap-149.json | 0.1739 | 0.2652 | 0.2141 | 0.2047 | 0.2494 | 0.2700 |
| holdout / v2-held-internal-gap-3079.json | 4.1957 | 6.4389 | 3.7330 | 3.8701 | 5.2909 | 6.0817 |
| holdout / v2-held-ragged-width-149.json | 0.0709 | 0.0761 | 0.0841 | 0.0836 | 0.0851 | 0.0560 |
| holdout / v2-held-ragged-width-3079.json | 1.3408 | 1.8706 | 1.3392 | 1.3844 | 1.7388 | 1.8251 |
| holdout / v2-held-space-strings-149.json | 0.0895 | 0.0743 | 0.0634 | 0.0682 | 0.0464 | 0.0968 |
| holdout / v2-held-space-strings-3079.json | 1.8432 | 1.9978 | 1.7760 | 1.8390 | 1.8425 | 2.0762 |
| holdout / v2-held-tail-cells-149.json | 0.2064 | 0.3120 | 0.2677 | 0.2749 | 0.2986 | 0.3203 |
| holdout / v2-held-tail-cells-3079.json | 5.9902 | 6.1092 | 5.5619 | 5.6189 | 6.2517 | 7.2877 |
| holdout / v2-held-utf8-149.json | 0.0566 | 0.0534 | 0.0520 | 0.0532 | 0.0534 | 0.0565 |
| holdout / v2-held-utf8-3079.json | 1.4705 | 1.3887 | 1.4339 | 1.5126 | 1.5081 | 1.5679 |

## Full captured-peak results

MiB; retained DOM accounting is identical for every mode.

| Split / input | Balanced | LowMemory | V1 speed | V2 speed | V1 memory | V2 memory |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| known / alternating-262144.json | 41.197 | 53.501 | 40.372 | 47.523 | 38.597 | 53.932 |
| known / alternating-32768.json | 5.150 | 6.688 | 5.166 | 5.166 | 7.062 | 5.924 |
| known / alternating-4096.json | 0.977 | 0.883 | 0.735 | 0.735 | 0.860 | 0.860 |
| known / auto-held-bursts-2053.json | 4.504 | 4.492 | 4.420 | 4.420 | 4.609 | 4.609 |
| known / auto-held-bursts-73.json | 0.183 | 0.178 | 0.186 | 0.186 | 0.175 | 0.175 |
| known / auto-held-inner-padding-2053.json | 50.154 | 6.163 | 6.110 | 6.110 | 6.310 | 6.023 |
| known / auto-held-inner-padding-73.json | 0.827 | 0.217 | 0.258 | 0.219 | 0.223 | 0.214 |
| known / auto-held-late-padding-2053.json | 51.785 | 5.285 | 7.540 | 7.540 | 5.270 | 5.270 |
| known / auto-held-late-padding-73.json | 0.878 | 0.178 | 0.186 | 0.186 | 0.226 | 0.226 |
| known / auto-held-long-keys-2053.json | 2.032 | 1.893 | 1.870 | 1.870 | 1.927 | 1.927 |
| known / auto-held-long-keys-73.json | 0.073 | 0.069 | 0.073 | 0.073 | 0.070 | 0.070 |
| known / auto-held-phases-2053.json | 26.711 | 20.884 | 27.582 | 23.179 | 21.099 | 20.813 |
| known / auto-held-phases-73.json | 0.752 | 0.742 | 0.779 | 0.740 | 0.750 | 0.740 |
| known / auto-held-ragged-numbers-2053.json | 2.455 | 2.455 | 2.337 | 2.337 | 2.446 | 2.446 |
| known / auto-held-ragged-numbers-73.json | 0.081 | 0.084 | 0.084 | 0.084 | 0.108 | 0.108 |
| known / auto-held-spiky-strings-2053.json | 18.631 | 18.577 | 18.566 | 18.566 | 18.586 | 18.586 |
| known / auto-held-spiky-strings-73.json | 0.827 | 0.827 | 0.827 | 0.827 | 0.827 | 0.827 |
| known / auto-held-zigzag-2053.json | 1.070 | 1.076 | 1.065 | 1.065 | 1.069 | 1.069 |
| known / auto-held-zigzag-73.json | 0.103 | 0.109 | 0.098 | 0.098 | 0.102 | 0.102 |
| known / deep-128.json | 0.017 | 0.017 | 0.018 | 0.017 | 0.017 | 0.017 |
| known / deep-64.json | 0.009 | 0.009 | 0.009 | 0.009 | 0.009 | 0.009 |
| known / deep-8.json | 0.001 | 0.001 | 0.001 | 0.001 | 0.001 | 0.001 |
| known / dense-first-262144.json | 62.500 | 74.500 | 77.818 | 47.338 | 38.617 | 53.931 |
| known / dense-first-32768.json | 16.813 | 9.313 | 5.769 | 5.769 | 9.324 | 6.301 |
| known / dense-first-4096.json | 0.977 | 1.164 | 2.278 | 2.161 | 1.145 | 1.145 |
| known / dense-zeros-262144.json | 34.500 | 51.500 | 36.000 | 42.435 | 36.535 | 45.073 |
| known / dense-zeros-32768.json | 4.313 | 6.438 | 4.500 | 4.313 | 6.812 | 5.557 |
| known / dense-zeros-4096.json | 0.539 | 0.805 | 0.653 | 0.630 | 0.828 | 0.748 |
| known / escaped-4096.json | 8.752 | 8.797 | 8.774 | 8.774 | 8.831 | 8.831 |
| known / escaped-512.json | 1.094 | 1.100 | 1.097 | 1.097 | 1.094 | 1.094 |
| known / escaped-64.json | 0.143 | 0.143 | 0.143 | 0.143 | 0.143 | 0.143 |
| known / flat-object-16384.json | 5.116 | 6.635 | 5.582 | 5.582 | 7.010 | 5.872 |
| known / flat-object-2048.json | 0.712 | 0.826 | 0.690 | 0.690 | 0.850 | 0.800 |
| known / flat-object-256.json | 0.082 | 0.103 | 0.085 | 0.085 | 0.086 | 0.086 |
| known / held-empty-mix-257.json | 0.227 | 0.241 | 0.235 | 0.235 | 0.229 | 0.229 |
| known / held-empty-mix-4099.json | 3.609 | 3.838 | 3.750 | 3.750 | 3.832 | 3.832 |
| known / held-escape-transition-257.json | 0.112 | 0.112 | 0.112 | 0.112 | 0.114 | 0.110 |
| known / held-escape-transition-4099.json | 0.807 | 0.841 | 0.781 | 0.781 | 0.767 | 0.767 |
| known / held-front-nested-257.json | 0.702 | 0.728 | 0.663 | 0.663 | 0.815 | 0.725 |
| known / held-front-nested-4099.json | 11.771 | 11.121 | 11.110 | 11.110 | 11.744 | 11.660 |
| known / held-long-keys-257.json | 0.043 | 0.043 | 0.043 | 0.043 | 0.043 | 0.043 |
| known / held-long-keys-4099.json | 0.548 | 0.547 | 0.550 | 0.550 | 0.558 | 0.547 |
| known / held-sawtooth-257.json | 0.277 | 0.268 | 0.266 | 0.266 | 0.265 | 0.265 |
| known / held-sawtooth-4099.json | 4.281 | 4.159 | 4.136 | 4.136 | 4.193 | 4.193 |
| known / held-tail-records-257.json | 1.133 | 0.764 | 4.405 | 4.405 | 0.747 | 0.747 |
| known / held-tail-records-4099.json | 54.116 | 12.219 | 70.494 | 15.132 | 12.231 | 11.207 |
| known / large.json | 21.915 | 26.584 | 22.563 | 28.241 | 28.163 | 28.514 |
| known / long-ascii-4096.json | 8.615 | 8.660 | 8.637 | 8.637 | 8.694 | 8.694 |
| known / long-ascii-512.json | 1.077 | 1.083 | 1.080 | 1.080 | 1.077 | 1.077 |
| known / long-ascii-64.json | 0.141 | 0.141 | 0.141 | 0.141 | 0.141 | 0.141 |
| known / medium.json | 2.709 | 3.321 | 2.820 | 2.820 | 3.495 | 3.495 |
| known / nested.json | 0.038 | 0.038 | 0.038 | 0.034 | 0.040 | 0.036 |
| known / small.json | 0.172 | 0.208 | 0.176 | 0.176 | 0.194 | 0.194 |
| known / sparse-first-262144.json | 45.792 | 48.224 | 62.422 | 50.469 | 55.580 | 47.732 |
| known / sparse-first-32768.json | 6.014 | 6.270 | 5.573 | 5.417 | 5.418 | 5.355 |
| known / sparse-first-4096.json | 0.790 | 0.825 | 0.732 | 0.692 | 0.867 | 0.782 |
| known / tiny.json | 0.003 | 0.002 | 0.002 | 0.002 | 0.002 | 0.002 |
| known / unicode-4096.json | 8.638 | 8.684 | 8.660 | 8.660 | 8.718 | 8.718 |
| known / unicode-512.json | 1.080 | 1.086 | 1.083 | 1.083 | 1.080 | 1.080 |
| known / unicode-64.json | 0.141 | 0.141 | 0.141 | 0.141 | 0.141 | 0.141 |
| known / whitespace-interleaved-16384.json | 4.328 | 5.266 | 4.298 | 4.298 | 5.440 | 5.440 |
| known / whitespace-interleaved-2048.json | 0.528 | 0.658 | 0.538 | 0.538 | 0.657 | 0.657 |
| known / whitespace-interleaved-256.json | 0.068 | 0.082 | 0.069 | 0.069 | 0.068 | 0.068 |
| known / whitespace-leading-16384.json | 4.565 | 5.111 | 5.539 | 4.934 | 4.726 | 4.609 |
| known / whitespace-leading-2048.json | 0.588 | 0.624 | 0.716 | 0.626 | 0.607 | 0.599 |
| known / whitespace-leading-256.json | 0.076 | 0.077 | 0.076 | 0.073 | 0.083 | 0.078 |
| known / whitespace-trailing-16384.json | 50.781 | 6.531 | 6.613 | 6.613 | 6.525 | 6.020 |
| known / whitespace-trailing-2048.json | 1.082 | 0.817 | 0.579 | 0.579 | 0.795 | 0.795 |
| known / whitespace-trailing-256.json | 0.794 | 0.102 | 0.102 | 0.102 | 0.079 | 0.079 |
| known / wide-numbers-262144.json | 39.161 | 52.697 | 38.865 | 46.535 | 37.814 | 53.128 |
| known / wide-numbers-32768.json | 4.650 | 6.573 | 4.932 | 4.932 | 6.948 | 5.810 |
| known / wide-numbers-4096.json | 0.615 | 0.820 | 0.673 | 0.673 | 0.843 | 0.784 |
| known / wide-objects-16384.json | 43.923 | 53.182 | 45.227 | 47.020 | 56.366 | 53.613 |
| known / wide-objects-2048.json | 5.449 | 6.644 | 5.674 | 5.674 | 7.019 | 5.952 |
| known / wide-objects-256.json | 0.692 | 0.830 | 0.705 | 0.705 | 0.854 | 0.854 |
| holdout / v2-held-chunks-149.json | 0.245 | 0.221 | 0.252 | 0.221 | 0.226 | 0.223 |
| holdout / v2-held-chunks-3079.json | 5.223 | 4.404 | 4.518 | 4.518 | 4.570 | 4.570 |
| holdout / v2-held-duplicate-keys-149.json | 0.107 | 0.135 | 0.151 | 0.151 | 0.122 | 0.122 |
| holdout / v2-held-duplicate-keys-3079.json | 2.558 | 3.285 | 2.266 | 2.266 | 3.453 | 3.453 |
| holdout / v2-held-front-cells-149.json | 0.300 | 0.309 | 0.286 | 0.286 | 0.340 | 0.311 |
| holdout / v2-held-front-cells-3079.json | 7.311 | 6.543 | 6.870 | 6.445 | 6.662 | 6.457 |
| holdout / v2-held-internal-gap-149.json | 0.858 | 0.222 | 0.226 | 0.226 | 0.234 | 0.218 |
| holdout / v2-held-internal-gap-3079.json | 4.439 | 4.537 | 6.171 | 6.171 | 5.104 | 4.491 |
| holdout / v2-held-ragged-width-149.json | 0.104 | 0.122 | 0.153 | 0.153 | 0.110 | 0.110 |
| holdout / v2-held-ragged-width-3079.json | 2.283 | 2.283 | 2.208 | 2.208 | 2.275 | 2.275 |
| holdout / v2-held-space-strings-149.json | 0.112 | 0.095 | 0.097 | 0.097 | 0.097 | 0.096 |
| holdout / v2-held-space-strings-3079.json | 2.224 | 1.853 | 2.218 | 1.855 | 1.918 | 1.918 |
| holdout / v2-held-tail-cells-149.json | 0.913 | 0.359 | 0.410 | 0.410 | 0.339 | 0.339 |
| holdout / v2-held-tail-cells-3079.json | 40.469 | 6.356 | 7.313 | 7.313 | 6.356 | 5.850 |
| holdout / v2-held-utf8-149.json | 0.065 | 0.061 | 0.065 | 0.065 | 0.062 | 0.062 |
| holdout / v2-held-utf8-3079.json | 1.316 | 1.231 | 1.286 | 1.286 | 1.199 | 1.199 |

## Journal evidence

| Split / goal | Growths | Byte-limited growths | Actual floor overrides | Proven final-capacity reservations | Copy-pressure states |
| --- | ---: | ---: | ---: | ---: | ---: |
| known/speed | 245 | 21 | 13 | 18 | 32 |
| known/memory | 485 | 64 | 48 | 26 | 46 |
| holdout/speed | 50 | 0 | 0 | 3 | 6 |
| holdout/memory | 100 | 1 | 0 | 6 | 9 |

Actual floor overrides mean the configured byte ceiling was exceeded to preserve geometric progress. A final-capacity reservation can instead
bypass the floor safely, because its remaining-byte proof excludes any subsequent growth. Both exceptions are explicit in the journal.
The configured number is not a hard total-memory limit; a smaller value can increase copying and old/new overlap.

## Checks, limits and reproduction

Exact comparators verify every token field, canonical numeric/zero representation, acceptance and full diagnostic class/location/message against both production parsers.
Journal validation requires geometric progress or a proved final bound, justified byte-budget exceptions, zero dropped records, no growth after the final bound,
and linear relocation-work bounds. Final journal movement/capacity/count equals the independent heap profile for every input.
All tracked memory windows return live bytes and blocks to zero; profile categories partition retained bytes. Saturating math is checked near size_t/max_size limits.
Windows/Linux smoke, sanitizers, corpus mutations and separate v2 coverage fuzzers exercise the same controller. CI does not enforce noisy timing targets.

Passed targets are evidence for this machine and these inputs; failed rows remain failures. Both priorities remain private experimental tools pending wider workloads.
CPU frequency, background load and compiler/code-layout variation limit small time differences. Source/binary/input hashes and per-round sample spread are preserved.

Raw [calibration, selection, known/evaluation results and journals](auto-v2-msvc1951/) are reproducible through [the v2 instructions](../README.md#auto-v2-byte-budgets-and-joint-targets).
Render with `python benchmarks/auto-v2/summarize.py benchmarks/results/auto-v2-msvc1951 REPORT.md`.
