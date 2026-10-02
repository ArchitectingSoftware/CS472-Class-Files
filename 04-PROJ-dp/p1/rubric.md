# dp Project, Part 1: Rubric

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
| All 6 tests pass with the provided seeds | 6 |
| Tests pass with TA-chosen seeds (no luck-dependent behavior) | 4 |
| `dp_connect()` and `dp_close()`: retransmit the same packet on timeout, bounded retries (TODOs 1, 4) | 3 |
| `send_data_packet()`: matching ACK, deadline-based wait, retransmit same packet, only timeouts count as retries (TODO 2) | 3 |
| `dp_recv()`: expected-packet check, re-ACK and log duplicates without re-delivering, never ACK unaccepted data, answer repeated CONNECT, FIN required (TODO 3) | 4 |

Deductions:

- Modifying provided files (anything other than the four TODO bodies in `dp-proto.c` and `questions.md`): up to -10, and evidence may be invalid.
- Does not compile: code component capped at 4.

## Test evidence - 25 points

From the committed `evidence/` directory.

| Criterion | Points |
|---|---:|
| `test-output.txt` shows all 6 tests PASS | 9 |
| `params.txt` matches your Drexel ID, and the three personalized runs used those parameters (forced drops and size visible in the logs) | 6 |
| Personalized runs complete with `verify: PASS`; `p-ack` shows a `DUP` line | 5 |
| Merged traces present for all tests and personalized runs | 2 |
| Evidence matches the submitted code (regenerated after the final code change) | 3 |

Evidence that was not produced by the submitted code, or that was produced with another student's parameters, earns 0 for this component and is referred under the academic integrity policy.

## Interpretation questions - 55 points

General standard for every question:

- **Full credit:** correct reasoning, specific evidence quoted from your own files (exact lines, packet numbers, times), and evidence that actually supports the claim.
- **Partial credit:** correct concept with weak, missing, or mismatched evidence.
- **No credit:** generic explanation that does not engage with your evidence, or claims that contradict your evidence.

| # | Points | Full credit requires |
|---|---:|---|
| P. Prediction | 5 | Committed before `evidence/` (3); reasoned prediction for all four parts (2). Correctness is **not** graded. Not committed first: 0 |
| 1. Timeout is not proof of loss | 8 | Both traces quoted (2); transport's view identical in both, and why the statistics differ but cannot be used (3); a no-loss timeout from `slowack` and why the response is the same (3) |
| 2. Duplicate detection | 8 | DUP line quoted (2); how the server knew, naming the variable (3); consequences of not re-ACKing and of copying, with your actual byte positions (3) |
| 3. Packet number vs offset | 7 | Correct numbers and computed offset (2); what each field identifies and who uses it, and why TCP can use one number (3); offset-based duplicate detection if numbers change (2) |
| 4. FIN vs message length | 5 | Correct final packet and payload size (2); what FIN adds, tied to multiple streams (3) |
| 5. Stop-and-wait performance | 10 | Measured values and comparison with prediction (2); model and percent difference (3); explanation of the gap with evidence from the trace (3); 1 MiB at 100 ms RTT and the fix (2) |
| 6. Socket timeouts and deadlines | 6 | Timeout interval and its source (2); why blocking forever is poor design, with an example and a response (2); a concrete fresh-timeout failure case (2) |
| 7. Closing reliably | 6 | Dropped CONN_ACK traced, answering function named (2); dropped CLOSE_ACK traced (2); why the server lingers, the limit of certainty, and TIME_WAIT (2) |

## Submission requirements

Not separately scored, but missing items cost points in the components above:

- `proj-dp-p1/` directory in your private GitHub repository; Canvas contains only the link.
- Commit history shows incremental work and the Prediction commit before `evidence/`.
- No binaries or object files committed.
