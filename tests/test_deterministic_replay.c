/*
 * Deterministic Replay Test for SimCity SNES Recomp
 *
 * This test verifies that the recompiled SimCity produces identical
 * state traces across multiple runs with the same inputs.
 *
 * It uses the snesrecomp framework's STATE_TRACE mechanism to capture
 * CPU state and WRAM CRC32 at each frame.
 *
 * WHERE THE ROM COMES FROM
 *
 * This test used to hardcode /home/seyon/dev/Games/PC/simcity for both the
 * script and the ROM. Two consequences, both bad:
 *
 *   1. It could not pass on any machine but the author's, so `make test` was
 *      not a property of the code and only of whose checkout you were in.
 *   2. Worse, on a machine that happened to have that path - including a
 *      relocated clone on the author's own box - the test silently read the
 *      ORIGINAL repository's ROM and script while running the relocated
 *      binary. It reported 2/2 passed on a tree that contained no ROM at all.
 *
 * The ROM is user-supplied and is never committed, so it cannot be baked in.
 * It is taken from, in order: $SIMCITY_ROM, then the first existing candidate
 * beside the executable or in the source root. If none is found the test
 * FAILED with a clear message and a non-zero exit, because until 2026-10-02 it
 * skipped with exit 0 and ctest rendered that as **Passed**: a tree with no ROM
 * reported "100% tests passed". A gate that reports green on work it did not do
 * teaches the next reader a false fact, which is the failure mode this whole
 * project keeps paying for. If you have no ROM, a red `make test` here is the
 * correct answer; `make test-rom` and `make clock` are the gates that need one.
 *
 * One consequence, stated so nobody is surprised: `make test` is now unusable on
 * a machine with no ROM. That is the intended trade. docs/DEFINITION_OF_DONE.md
 * D1.2 counts `make test` as a criterion, and it must not be satisfiable by
 * declining to run.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

#define MAX_FRAMES 30
#define TRACE_LINE_SIZE 128

/* Resolve a path to an absolute one, in place. Returns 0 on success.
 *
 * WHY THIS EXISTS (added 2026-10-02, review finding R-12):
 *
 * The emulator chdir()s to its own executable directory before it opens the ROM,
 * so a RELATIVE ROM path silently stops working. This test passed one candidate
 * straight through - "SimCity (USA).sfc" - and running the test binary with the
 * repository root as its working directory produced:
 *
 *     FAIL: Run 1 exited with code 256
 *     Using ROM: SimCity (USA).sfc
 *
 * while the same binary under ctest (whose working directory is the build dir)
 * resolved "../SimCity (USA).sfc" and passed. Same binary, same ROM, same
 * commit, two verdicts, decided entirely by the working directory. README
 * already warns about this for the command line; this made it true of the test
 * harness too.
 */
static int absolutise(char *path, size_t pathsz)
{
    char resolved[PATH_MAX];
    if (path[0] == '/')
        return 1;
    if (!realpath(path, resolved))
        return 0;
    snprintf(path, pathsz, "%s", resolved);
    return 1;
}

/* Locate a ROM without hardcoding anyone's home directory. Returns 1 if found. */
static int find_rom(char *out, size_t outsz)
{
    static const char *cands[] = {
        "SimCity (USA).sfc",
        "../SimCity (USA).sfc",
        "../../SimCity (USA).sfc",
        "/tmp/relocated/SimCity (USA).sfc",
    };
    const char *env = getenv("SIMCITY_ROM");
    size_t i;

    if (env && *env && access(env, R_OK) == 0) {
        snprintf(out, outsz, "%s", env);
        return absolutise(out, outsz);
    }
    for (i = 0; i < sizeof(cands) / sizeof(cands[0]); i++) {
        if (access(cands[i], R_OK) == 0) {
            snprintf(out, outsz, "%s", cands[i]);
            return absolutise(out, outsz);
        }
    }
    return 0;
}

int main(void)
{
    char trace1_path[] = "/tmp/simcity_trace1.csv";
    char trace2_path[] = "/tmp/simcity_trace2.csv";
    char cmd[1024];
    char rom_path[512];
    FILE *f1, *f2;
    char line1[TRACE_LINE_SIZE], line2[TRACE_LINE_SIZE];
    int frame = 0;
    int mismatches = 0;

    if (!find_rom(rom_path, sizeof(rom_path))) {
        printf("SKIP: no ROM found (set SIMCITY_ROM=/path/to/'SimCity (USA).sfc').\n");
        printf("      This test replays the guest, so it needs a ROM. The ROM is\n");
        printf("      user-supplied and is never committed, which is deliberate.\n");
        printf("\n");
        printf("THIS IS A FAILURE, NOT A PASS. Changed 2026-10-02, review finding R-01.\n");
        printf("\n");
        printf("This used to `return 0`, and CMakeLists.txt sets no SKIP_RETURN_CODE,\n");
        printf("so ctest rendered the skip as **Passed**. The measured result on a\n");
        printf("tree containing no ROM at all:\n");
        printf("\n");
        printf("    100%% tests passed, 0 tests failed out of 2      <- with ctest rc=0\n");
        printf("\n");
        printf("A gate that reports green on work it did not do is worse than one that\n");
        printf("is red, because the green is what the next session starts from. The\n");
        printf("previous review closed a BLOCKER whose letter was 'no absolute path to\n");
        printf("the author's home appears in tests/' while its substance - a tree with\n");
        printf("no ROM reports 2/2 passed - survived the fix in this exact form.\n");
        printf("\n");
        printf("If you have no ROM, that is a legitimate state and `make test` SHOULD\n");
        printf("fail here. For the gates that need a ROM and say so, use `make\n");
        printf("test-rom` or `make clock`.\n");
        return 2;
    }
    printf("Using ROM: %s\n", rom_path);

    /* Run 1 */
    printf("Running deterministic replay test (run 1/2)...\n");
    snprintf(cmd, sizeof(cmd),
        "cd \"" PROJECT_SOURCE_DIR "\" && "
        "SNESRECOMP_STATE_TRACE=%s "
        "SNESRECOMP_RUN_FRAMES=%d "
        "SDL_VIDEODRIVER=dummy "
        "SDL_AUDIODRIVER=dummy "
        "timeout 60 ./build/SimCitySNESRecomp "
        "--script \"" PROJECT_SOURCE_DIR "/tests/deterministic_replay.script\" "
        "\"%s\" "
        "> /dev/null 2>&1",
        trace1_path, MAX_FRAMES, rom_path);
    
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "FAIL: Run 1 exited with code %d\n", ret);
        return 1;
    }

    /* Run 2 */
    printf("Running deterministic replay test (run 2/2)...\n");
    snprintf(cmd, sizeof(cmd),
        "cd \"" PROJECT_SOURCE_DIR "\" && "
        "SNESRECOMP_STATE_TRACE=%s "
        "SNESRECOMP_RUN_FRAMES=%d "
        "SDL_VIDEODRIVER=dummy "
        "SDL_AUDIODRIVER=dummy "
        "timeout 60 ./build/SimCitySNESRecomp "
        "--script \"" PROJECT_SOURCE_DIR "/tests/deterministic_replay.script\" "
        "\"%s\" "
        "> /dev/null 2>&1",
        trace2_path, MAX_FRAMES, rom_path);
    
    ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "FAIL: Run 2 exited with code %d\n", ret);
        return 1;
    }

    /* Compare traces */
    f1 = fopen(trace1_path, "r");
    f2 = fopen(trace2_path, "r");
    if (!f1 || !f2) {
        fprintf(stderr, "FAIL: Could not open trace files\n");
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return 1;
    }

    printf("Comparing state traces...\n");
    while (fgets(line1, sizeof(line1), f1) && fgets(line2, sizeof(line2), f2)) {
        frame++;
        if (strcmp(line1, line2) != 0) {
            fprintf(stderr, "MISMATCH at frame %d:\n", frame);
            fprintf(stderr, "  Run 1: %s", line1);
            fprintf(stderr, "  Run 2: %s", line2);
            mismatches++;
        }
    }

    /* Check if files have same number of lines */
    if (fgets(line1, sizeof(line1), f1) || fgets(line2, sizeof(line2), f2)) {
        fprintf(stderr, "FAIL: Trace files have different number of frames\n");
        mismatches++;
    }

    fclose(f1);
    fclose(f2);
    unlink(trace1_path);
    unlink(trace2_path);

    if (mismatches > 0) {
        fprintf(stderr, "FAIL: %d frame mismatch(es) found\n", mismatches);
        return 1;
    }

    printf("PASS: Deterministic replay verified (%d frames identical)\n", frame);
    return 0;
}