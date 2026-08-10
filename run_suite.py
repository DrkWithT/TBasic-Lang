"""
    run_suite.py
    Runs all TBasic suite snippets to test the interpreter.
    By: DrkWithT
"""

import os
import subprocess

TEST_SUITE_DIR = os.path.relpath('./suite')
TEST_PROCESS_COUNT = 2;

def get_test_names(test_suite_path: str = TEST_SUITE_DIR) -> list[str]:
    current_folder_entries = os.listdir(test_suite_path)

    all_test_names = [
        f'{test_suite_path}/{test_case_filename}'
        for test_case_filename
        in current_folder_entries if not os.path.isdir(f'{test_suite_path}/{test_case_filename}') and test_case_filename[0] != '.'
    ]
    
    return all_test_names

def run_tests_by_n(test_file_paths: list[str], worker_count: int = TEST_PROCESS_COUNT):
    total_tests = len(test_file_paths)
    total_passed = 0

    while test_file_paths:
        batch = [(
            test_path,
            f'./build/tbasic -r {test_path}'
        ) for test_path in test_file_paths[:worker_count]]
        test_file_paths = test_file_paths[worker_count:]

        batched_procs = [subprocess.Popen(
            named_test[1],
            shell=True
        ) for named_test in batch]

        for batch_test_id, test_cmd in enumerate(batched_procs):
            if test_cmd.wait() == 0:
                print(f'Test \x1b[1;33m{batch[batch_test_id][0]}\x1b[0m:  \x1b[1;32mPASS\x1b[0m')
                total_passed += 1
            else:
                print(f'Test \x1b[1;33m{batch[batch_test_id][0]}\x1b[0m:  \x1b[1;31mFAIL\x1b[0m')

    return (total_passed, total_tests - total_passed, total_tests)   

if __name__ == '__main__':
    if not os.path.exists("./build/tbasic"):
        print(f'The executable \x1b[1;33m./build/tbasic\x1b[0m is missing, please build it first.')
        exit(1)

    pass_count, fail_count, test_count = run_tests_by_n(
        get_test_names()
    )

    print(f'\nTEST REPORT:\n\x1b[1;34mPASSED:\x1b[0m {pass_count}/{test_count}\n\x1b[1;34mFAILED:\x1b[0m {fail_count}/{test_count}')

    exit(0 if fail_count == 0 else 1)
