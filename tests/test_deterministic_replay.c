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
 * SKIPS with a clear message and exit 0, because a gate that fails for want of
 * a file the project is not allowed to ship trains people to disable tests.
 * Callers that need a hard requirement should run the ROM gates
 * (`make test-rom`, `make clock`), which do take a ROM and do fail without one.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FRAMES 30
#define TRACE_LINE_SIZE 128

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
        return 1;
    }
    for (i = 0; i < sizeof(cands) / sizeof(cands[0]); i++) {
        if (access(cands[i], R_OK) == 0) {
            snprintf(out, outsz, "%s", cands[i]);
            return 1;
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
        printf("      For a gate that REQUIRES one, use `make test-rom` or\n");
        printf("      `make clock` - both take a ROM and both fail without it.\n");
        return 0;
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