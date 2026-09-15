/*
 * Deterministic Replay Test for SimCity SNES Recomp
 *
 * This test verifies that the recompiled SimCity produces identical
 * state traces across multiple runs with the same inputs.
 *
 * It uses the snesrecomp framework's STATE_TRACE mechanism to capture
 * CPU state and WRAM CRC32 at each frame.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FRAMES 30
#define TRACE_LINE_SIZE 128

int main(void)
{
    char trace1_path[] = "/tmp/simcity_trace1.csv";
    char trace2_path[] = "/tmp/simcity_trace2.csv";
    char cmd[512];
    FILE *f1, *f2;
    char line1[TRACE_LINE_SIZE], line2[TRACE_LINE_SIZE];
    int frame = 0;
    int mismatches = 0;

    /* Run 1 */
    printf("Running deterministic replay test (run 1/2)...\n");
    snprintf(cmd, sizeof(cmd),
        "cd /home/seyon/dev/Games/PC/simcity && "
        "SNESRECOMP_STATE_TRACE=%s "
        "SNESRECOMP_RUN_FRAMES=%d "
        "SDL_VIDEODRIVER=dummy "
        "SDL_AUDIODRIVER=dummy "
        "timeout 60 ./build/SimCitySNESRecomp "
        "--script /home/seyon/dev/Games/PC/simcity/tests/deterministic_replay.script "
        "\"/home/seyon/dev/Games/PC/simcity/SimCity (USA).sfc\" "
        "> /dev/null 2>&1",
        trace1_path, MAX_FRAMES);
    
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "FAIL: Run 1 exited with code %d\n", ret);
        return 1;
    }

    /* Run 2 */
    printf("Running deterministic replay test (run 2/2)...\n");
    snprintf(cmd, sizeof(cmd),
        "cd /home/seyon/dev/Games/PC/simcity && "
        "SNESRECOMP_STATE_TRACE=%s "
        "SNESRECOMP_RUN_FRAMES=%d "
        "SDL_VIDEODRIVER=dummy "
        "SDL_AUDIODRIVER=dummy "
        "timeout 60 ./build/SimCitySNESRecomp "
        "--script /home/seyon/dev/Games/PC/simcity/tests/deterministic_replay.script "
        "\"/home/seyon/dev/Games/PC/simcity/SimCity (USA).sfc\" "
        "> /dev/null 2>&1",
        trace2_path, MAX_FRAMES);
    
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