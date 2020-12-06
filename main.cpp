#include <windows.h>
#include <iostream>
#include <cmath>
#include <vector>

int N = 100000000;
int blockSize = 8301260;
double pi = 0;
int curBlock = 0;

int getCurBlock() {
    int result = curBlock;
    curBlock += blockSize;
    if (result >= N) {
        return -1;
    }
    return result;
}

DWORD WINAPI partialSolver(LPVOID mutexArg) {
    double result = 0;
    int localCurBlock;
    HANDLE mutex = (HANDLE)mutexArg;
    while (true) {
        auto waitResult = WaitForSingleObject(mutex, INFINITE);
        if (waitResult == WAIT_FAILED) {
            std::cout << "WaitForSingleObject failed" << std::endl;
            break;
        }

        pi += result;
        localCurBlock = getCurBlock();
        ReleaseMutex(mutex);
        if (localCurBlock == -1) {
            break;
        }
        result = 0;
        for (int i = localCurBlock; i < std::min(localCurBlock + blockSize, N); ++i) {
            result += 4 / (1.0 + pow((i + 0.5) / N, 2.0));
        }
    }
    CloseHandle(mutex);
    return 0;
}


int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "Usage: " << argv[0] << " <thread amount>" << std::endl;
        return 0;
    }
    int threadAmount = atoi(argv[1]);
    HANDLE process = GetCurrentProcess();
    HANDLE mutex = CreateMutexW(nullptr, FALSE, nullptr);
    std::vector<HANDLE> threads;
    for (int i = 0; i < threadAmount; ++i) {
        HANDLE threadMutex;
        DuplicateHandle(process, mutex, process, &threadMutex, 0, FALSE, DUPLICATE_SAME_ACCESS);
        threads.push_back(CreateThread(nullptr, 0, partialSolver, threadMutex, CREATE_SUSPENDED, nullptr));
    }
    auto start = GetCurrentTime();
    for (HANDLE thread : threads) {
        ResumeThread(thread);
    }

    while (true) {
        auto waitResult = WaitForMultipleObjects(threads.size(), threads.data(), TRUE, INFINITE);
        if (WAIT_OBJECT_0 <= waitResult && waitResult < WAIT_OBJECT_0 + threads.size()) {
            break;
        }
        std::cout << "WaitForMultipleObjects returned " << waitResult << std::endl;
    }

    pi /= N;
    auto end = GetCurrentTime();
    std::cout << "Time: " << end - start << "(ms) Result: " << pi << std::endl;
    for (HANDLE thread : threads) {
        CloseHandle(thread);
    }
    CloseHandle(mutex);
}
