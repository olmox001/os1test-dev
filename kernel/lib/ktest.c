/*
 * kernel/lib/ktest.c
 * Kernel Unit Test Runner
 *
 * Purpose:
 *   Provides ktest_run_all(), which iterates over all KTEST_CASE() entries
 *   registered in the `.ktests` ELF section and runs each test function.
 *   Called unconditionally from kernel/main.c:87 during boot.
 *
 * Role:
 *   The KTEST_CASE() macro (kernel/include/kernel/test.h) places a ktest_case_t
 *   descriptor into the `.ktests` ELF section:
 *     __attribute__((used, section(".ktests"))) static const ktest_case_t ...;
 *   The linker script collects all such descriptors between the symbols
 *   __ktests_start and __ktests_end.  ktest_run_all() walks this range.
 *
 * KASSERT behaviour:
 *   KASSERT(cond) in test.h expands to:
 *     if (!(cond)) { printk(...FAIL...); return; }
 *   When a KASSERT fires, the test function returns early — control returns
 *   to ktest_run_all() at the line AFTER test->func().  The runner then
 *   unconditionally prints "PASS" and increments passed.
 *
 * Known issues:
 *   LIB-KTEST-01  (W3 BUG)  ktest_run_all() always prints "PASS" and
 *                increments `passed` after test->func() returns, regardless of
 *                whether the test exited via KASSERT (failure) or ran to
 *                completion (success).  The final summary always reports
 *                N PASSED / 0 FAILED even when tests have failed.  The KASSERT
 *                failure message IS printed (via printk) but is not counted.
 *                Fix: add a ktest_failed counter; have KASSERT increment it
 *                and set a per-test flag that ktest_run_all() checks.
 */
#include <kernel/test.h>
#include <kernel/printk.h>

/* __ktests_start / __ktests_end: linker-defined symbols bracketing the
 * .ktests ELF section that holds all KTEST_CASE() descriptors.
 * The pointer arithmetic (__ktests_end - __ktests_start) gives the test count. */
extern ktest_case_t __ktests_start[];
extern ktest_case_t __ktests_end[];

/*
 * ktest_run_all - run every KTEST_CASE registered in the .ktests section.
 *
 * Iterates from __ktests_start to __ktests_end.  For each entry:
 *   1. Prints "[KTEST] Running: <name>... ".
 *   2. Calls test->func().
 *   3. Prints "PASS" and increments passed.
 * After all tests, prints the pass/fail summary.
 *
 * NOTE(LIB-KTEST-01): Step 3 runs unconditionally, even if test->func()
 *   returned early via KASSERT.  The `count - passed` in the summary is
 *   therefore always 0 — the FAILED count is never non-zero.  Failures are
 *   visible only from the KASSERT printk message, not the summary line.
 *
 * No crash recovery: if a test triggers a kernel panic or fault, the machine
 * halts.  The comment "setjmp/longjmp" notes the direction for future hardening.
 *
 * Locking: none; called single-threaded from kernel/main.c before SMP starts.
 * Side effects: writes to UART via printk; calls all registered test functions.
 */
void ktest_run_all(void) {
    size_t count = __ktests_end - __ktests_start;
    size_t passed = 0;

    printk("\n[KTEST] Starting Kernel Unit Tests (%d cases found)...\n", (int)count);

    for (ktest_case_t *test = __ktests_start; test < __ktests_end; test++) {
        printk("[KTEST] Running: %s... ", test->name);

        /* Note: This is a simplified runner. In a real system, we might want
         * to use setjmp/longjmp for crash recovery in tests.
         */
        test->func();

        /* NOTE(LIB-KTEST-01): "PASS" is always printed here.  If test->func()
         * returned early via KASSERT, this line still executes, producing a
         * misleading "PASS" line after the KASSERT failure message. */
        printk("PASS\n");
        passed++;
    }

    /* NOTE(LIB-KTEST-01): (count - passed) is always 0; the FAILED count is
     * never incremented.  Real failures are lost in the summary. */
    printk("[KTEST] Completed. Summary: %d PASSED, %d FAILED\n\n",
           (int)passed, (int)(count - passed));
}
