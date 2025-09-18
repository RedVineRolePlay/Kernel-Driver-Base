#include <iostream>
#include <string>
#include "driver.h"

int main() {
    using namespace std;

    SetConsoleTitleA("KapaUm Lol");

    
    if (!DriverHandle::init()) {
        system("color 4");
        cout << "\n[!] Driver communications not initialized.\n";
        cin.get();
        return 1;
    }

    
    wstring processNameW;
    cout << "Enter a process name: ";
    string processNameA;
    getline(cin, processNameA);
    int len = MultiByteToWideChar(CP_UTF8, 0, processNameA.c_str(), -1, nullptr, 0);
    processNameW.resize(len);
    MultiByteToWideChar(CP_UTF8, 0, processNameA.c_str(), -1, &processNameW[0], len);

    
    DriverHandle::ProcessIdentifier = DriverHandle::GetProcessId(processNameW.c_str());
    if (DriverHandle::ProcessIdentifier == 0) {
        cout << "[!] Didnt find process nigga.\n";
        cin.get();
        return 1;
    }
    cout << "PID of " << processNameA << ": " << DriverHandle::ProcessIdentifier << endl;

    
    uint64_t baseAddress = 0;
    if (DriverHandle::GetBaseAddress(&baseAddress)) {
        cout << "Base Address: 0x" << hex << baseAddress << dec << endl;
    } else {
        cout << "[!] Failed to get base address" << endl;
    }

    
    uint64_t cr3 = 0;
    if (DriverHandle::GetCr3(&cr3)) {
        cout << "CR3: 0x" << hex << cr3 << dec << endl;
    } else {
        cout << "[!] Failed to get CR3" << endl;
    }

    
    // Test read operation
    uint8_t testBuffer[16] = { 0 };
    if (DriverHandle::ReadPhysicalMemory(baseAddress, testBuffer, sizeof(testBuffer))) {
        cout << "Read test successful. First 16 bytes: ";
        for (int i = 0; i < 16; i++) {
            cout << hex << (int)testBuffer[i] << " ";
        }
        cout << dec << endl;
    } else {
        cout << "[!] Read test failed" << endl;
    }

    // Test write operation (write some test data)
    uint8_t writeData[8] = { 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x21, 0x21, 0x21 }; // "Hello!!!"
    
    // First, let's try writing to our own process memory to test if write functionality works at all
    uint8_t selfTestBuffer[8] = { 0 };
    uint64_t ourProcessAddress = (uint64_t)selfTestBuffer;
    cout << "Testing write to our own process memory at 0x" << hex << ourProcessAddress << dec << endl;
    
    // Temporarily switch to our own process for testing
    INT32 originalProcessId = DriverHandle::ProcessIdentifier;
    DriverHandle::ProcessIdentifier = GetCurrentProcessId();
    cout << "Switched to our own process ID: " << DriverHandle::ProcessIdentifier << endl;
    
    if (DriverHandle::WritePhysicalMemory(ourProcessAddress, writeData, sizeof(writeData))) {
        cout << "Self-write test successful! Write functionality works." << endl;
        cout << "Our buffer now contains: ";
        for (int i = 0; i < 8; i++) {
            cout << hex << (int)selfTestBuffer[i] << " ";
        }
        cout << dec << endl;
    } else {
        cout << "Self-write test failed. There's a fundamental issue with write functionality." << endl;
    }
    
    // Switch back to target process
    DriverHandle::ProcessIdentifier = originalProcessId;
    cout << "Switched back to target process ID: " << DriverHandle::ProcessIdentifier << endl;
    
    // Try writing to base address + some offset to avoid read-only code section
    uint64_t writeAddress = baseAddress + 0x1000; // Try 4KB offset
    cout << "Attempting to write " << sizeof(writeData) << " bytes to address 0x" << hex << writeAddress << dec << endl;
    
    // Also try writing to a heap address (more likely to be writable)
    uint64_t heapAddress = baseAddress + 0x100000; // Try 1MB offset (likely in data section)
    cout << "Also trying heap address 0x" << hex << heapAddress << dec << endl;
    
    if (DriverHandle::WritePhysicalMemory(writeAddress, writeData, sizeof(writeData))) {
        cout << "Write test successful. Wrote " << sizeof(writeData) << " bytes." << endl;
        
        // Read back to verify
        uint8_t verifyBuffer[8] = { 0 };
        if (DriverHandle::ReadPhysicalMemory(writeAddress, verifyBuffer, sizeof(verifyBuffer))) {
            cout << "Verification read successful. Data: ";
            for (int i = 0; i < 8; i++) {
                cout << hex << (int)verifyBuffer[i] << " ";
            }
            cout << dec << endl;
            
            // Check if the data matches
            bool dataMatches = true;
            for (int i = 0; i < 8; i++) {
                if (writeData[i] != verifyBuffer[i]) {
                    dataMatches = false;
                    break;
                }
            }
            if (dataMatches) {
                cout << "Write verification: SUCCESS - Data matches!" << endl;
            } else {
                cout << "Write verification: FAILED - Data does not match!" << endl;
            }
        } else {
            cout << "[!] Verification read failed" << endl;
        }
    } else {
        cout << "[!] Write test failed at offset 0x1000" << endl;
        
        // Try the heap address
        cout << "Trying heap address instead..." << endl;
        if (DriverHandle::WritePhysicalMemory(heapAddress, writeData, sizeof(writeData))) {
            cout << "Heap write test successful. Wrote " << sizeof(writeData) << " bytes." << endl;
            
            // Read back to verify
            uint8_t verifyBuffer2[8] = { 0 };
            if (DriverHandle::ReadPhysicalMemory(heapAddress, verifyBuffer2, sizeof(verifyBuffer2))) {
                cout << "Heap verification read successful. Data: ";
                for (int i = 0; i < 8; i++) {
                    cout << hex << (int)verifyBuffer2[i] << " ";
                }
                cout << dec << endl;
                
                // Check if the data matches
                bool dataMatches = true;
                for (int i = 0; i < 8; i++) {
                    if (writeData[i] != verifyBuffer2[i]) {
                        dataMatches = false;
                        break;
                    }
                }
                if (dataMatches) {
                    cout << "Heap write verification: SUCCESS - Data matches!" << endl;
                } else {
                    cout << "Heap write verification: FAILED - Data does not match!" << endl;
                }
            } else {
                cout << "[!] Heap verification read failed" << endl;
            }
        } else {
            cout << "[!] Heap write test also failed" << endl;
        }
    }

    cin.get();
    return 0;
}