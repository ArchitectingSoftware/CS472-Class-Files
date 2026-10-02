# dp Project, Part 2: Rubric

**Total: 100 points**

| Component | Points | What it measures |
|---|---:|---|
| Code | 20 | Your `dp-proto.c` works and implements the protocol as specified |
| Test evidence | 25 | Evidence produced by your code with your parameters |
| Interpretation questions | 55 | Whether you understand what your code did and why |

The weighting is deliberate. Working code matters, and evidence points depend on it, so a working implementation is effectively worth 45 points. But most of the grade is for showing that you understand the protocol, using your own traces.

## Code - 20 points

The TA runs `make clean && make && make test` on your submission, then runs the tests again with different seeds.

| Criterion | Points |
|---|---:|
| All 9 tests pass with the provided seeds | 6 |
| Tests pass with TA-chosen seeds (no luck-dependent behavior) | 4 |
| Retransmissions use a new packet number with the same stream and offset (TODOs 1, 3) | 3 |
| Window loop correct: fills to `window`, waits until earliest deadline, selective retransmission (TODOs 2, 4) | 3 |
| Receiver: duplicates detected by offset, FIN/final size enforced, in-order delivery per stream (TODOs 5, 6) | 4 |

Deductions:

- Modifying provided files (anything other than `dp-proto.c` TODO bodies and `questions.md`): up to -10, and evidence may be invalid.
- Does not compile: code component capped at 4.

## Test evidence - 25 points

From the committed `evidence/` directory.

| Criterion | Points |
|---|---:|
| `test-output.txt` shows all 9 tests PASS (about 1 point each) | 9 |
| `params.txt` matches your Drexel ID, and all four personalized runs used those parameters (DROP and SIZES visible in the logs) | 6 |
| Personalized runs complete with `verify: PASS` | 4 |
| `hol-results.txt` present and complete | 3 |
| Evidence matches the submitted code (regenerated after the final code change) | 3 |

Evidence that was not produced by the submitted code, or that was produced with another student's parameters, earns 0 for this component and is referred under the academic integrity policy.

## Interpretation questions - 55 points

General standard for every question:

- **Full credit:** correct reasoning, specific evidence quoted from your own files (exact lines, packet numbers, times), and evidence that actually supports the claim.
- **Partial credit:** correct concept with weak, missing, or mismatched evidence.
- **No credit:** generic explanation that does not engage with your evidence, or claims that contradict your evidence.

| # | Points | Full credit requires |
|---|---:|---|
| P. Prediction | 5 | Committed before `evidence/` (3); reasoned prediction for all four parts (2). Correctness of the prediction is **not** graded. Not committed first: 0 |
| 1. Dropped packet | 8 | Correct stream/offset/file for DROP (2); TIMEOUT and retransmission lines quoted, retransmission number explained (3); a concrete ACK-ambiguity scenario (3) |
| 2. HOL in your runs | 8 | Correct times table (2); correct explanation of which files waited in each mode, and which mode matches HTTP/2 over TCP (4); honest comparison with prediction (2) |
| 3. Arrived but not delivered | 7 | Correct count with method (2); explanation naming the function and variable (3); why multi mode differs (2) |
| 4. Window size 1 | 8 | Times reported (2); sender-side explanation with quoted lines (4); connection to stop-and-wait / why streams need a window (2) |
| 5. Duplicates | 8 | Complete chunk trace with every packet number (3); why duplicates occur without loss (2); experiment result and idempotence explanation contrasted with Part 1 (3) |
| 6. Loss-rate experiment | 8 | Correct computation (2); explanation of why the advantage shrinks (4); timeout-quantization explanation (2) |
| 7. Our design vs QUIC | 3 | One feature, one concrete effect on the traces |

## Submission requirements

Not separately scored, but missing items cost points in the components above:

- `proj-dp-p2/` directory in your private GitHub repository; Canvas contains only the link.
- Commit history shows incremental work and the Prediction commit before `evidence/`.
- No binaries or object files committed.
