# ResourceCopy

**ResourceCopy** is a command-line tool and C++ API for performing batch file operations like copying, transferring, and deleting files and directories. It is designed for developers working with Source engine projects, modding, or any workflow that requires fast and flexible file management.

---

## 📦 Installation

To install **ResourceCopy**, follow these steps:

1. Start with a clean copy of the **Source SDK 2013** source code (works both for SP & MP).  
2. Drag and drop the contents of this repository into your SDK source directory.  
3. Overwrite any existing files when prompted.  
4. Regenerate the Visual Studio solution files (e.g., by running `createallprojects.bat`).  
5. Open the solution in Visual Studio and compile it. The `resourcecopy.exe` executable will be built and ready to use.

---

## ✨ Features

- 🔍 Recursive and shallow directory scanning  
- 📂 Support for wildcards (`*`) and extension filters (e.g., `*.vtf`)  
- 📑 Batch **copy**, **transfer**, and **delete** operations  
- ⚡ Multithreading support for faster processing  
- 🛠️ Reports and logging (operation reports, error reports, hardware report)  
- 🗑️ Safe deletion (`-safe` mode prevents overwriting or deleting important files)  
- 📊 Utilities for quick checks: folder size, drive free space, file size  
- 🖨️ Print directory contents with filters  

### 💻 C++ API (`CResourceCopy`)

You can use **CResourceCopy** as a C++ API for integration into tools or SDK projects:

- **Single File Operations**:  
  `CopyFileTo`, `TransferFileTo`, `DeleteFileIn`, `CreateDir`, `DirExist`, `FileExist`, `DeleteEmptyFolder`, `IsWritable`

- **Batch File Operations (multithreaded, supports wildcards)**:  
  `CopyDirTo`, `TransferDirTo`, `DeleteDirRecursive`

- **Sanity Checks**:  
  `GetFileSizeFast`, `GetDriveFreeSpace`, `GetFolderSize`

- **Reports**:  
  `GenerateGlobalOperationReport`, `GenerateErrorReport`, `GenerateHardwareReport`

- **Configuration**:  
  `SetThreads`, `SetVerboseSpewMode`, `SetNormalSpewMode`, `SetQuietSpewMode`

**Example usage in C++:**
```cpp
#include "cresourcecopy.h"

CResourceCopy* pResourceCopy = new CResourceCopy();
pResourceCopy->SetQuietSpewMode();
pResourceCopy->CopyDirTo("C:\\source_dir", "C:\\destination_dir", true, true);
pResourceCopy->GenerateGlobalOperationReport();
delete g_pResourceCopy;
```
## Contact

Steam: https://steamcommunity.com/profiles/76561199073832016/  
Twitter: https://x.com/47Z14  
Discord: carlossuarez7285  

---
