# Parse_Klik_en_Klaar_logs

A parser for Klik & Klaar (NR5307) router logs to track errors and logs in a more structured and human-friendly format.  
These logs come from Odido's "Klik en Klaar" interface at `192.168.1.1`.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
  - [1. Obtaining the Logs](#1-obtaining-the-logs)
  - [2. Decompressing the Logs](#2-decompressing-the-logs)
  - [3. Running the Parser](#3-running-the-parser)
- [Building from Source](#building-from-source)
- [Usage](#usage)
- [Output Format](#output-format)
- [Use Cases](#use-cases)
- [License](#license)

---

## Overview

`Parse_Klik_en_Klaar_logs` processes batches of router log files exported from the NR5307 (Klik & Klaar by Odido). It reads various `.log` files (and optionally `.txt` files) from a `syslog` folder and converts them into one or more CSV files organized by log type.  

Common log types include:
- `syslog`
- `syslog.log.1` (which is still classified under the same `syslog` type)
- `CM`, `FOTA`, `MMM` logs
- and any other logs conforming to the defined patterns

---

## Features

- **Timestamp Normalization**  
  Takes timestamps in multiple formats (e.g., `Feb 14 2:20:11` or `[02/19 01:55:01]`) and normalizes them to `MM/DD HH:MM:SS`.
- **Log Type Extraction**  
  Identifies log types from their filenames (e.g., `syslog.log` → `syslog`).
- **CSV Output**  
  Generates a separate CSV file for each log type in an `output` folder.
- **Lightweight**  
  Single executable (or two source files) that quickly parse and reorder your log lines.

---

## Installation

### 1. Obtaining the Logs

1. **Log into your NR5307 (Klik & Klaar) interface**:  
   Open your browser and go to [192.168.1.1](http://192.168.1.1).
   
2. **Navigate to the Logs section**:  
   From the interface (`Logboek` → `Logboek exporteren`), export the log files. These are typically compressed into a `.tar.gz` archive.

### 2. Decompressing the Logs

1. **Use any decompressor** (7-Zip, WinRAR, tar commands, etc.) to extract the `.tar.gz` file.  
2. **After extraction**, you'll have a `syslog` folder containing multiple `.log` files.

### 3. Running the Parser

1. **Place the `syslog` folder** in the same directory as the `System Logs Handling.exe`.  
2. **Run the `.exe`** by double-clicking it or via a terminal/command prompt:
   ```bash
   ./System Logs Handling.exe
   ```
   If you wish to run it from command line with a custom folder path, you can do:
   ```bash
   ./System Logs Handling.exe path/to/your/syslog
   ```
   The parser will create an output folder in the same directory, containing the resulting CSV files.

---

### Building from Source

If you prefer to compile the code yourself rather than using the provided `.exe`:

1. **Clone or download** this repository.
2. **Open** `System Logs Handling.cpp` in any C++ compiler or IDE (GCC, Clang, MSVC, etc.).
3. **Compile**:
   ```bash
   g++ -std=c++17 -o SystemLogsHandling System\ Logs\ Handling.cpp
   ```
4. **Run** the compiled binary:
   ```bash
   ./SystemLogsHandling
   ```

---

### Usage

1. **Export logs** from your router:
   - Go to `Logboek` → `Logboek exporteren`.
2. **Decompress** the `.tar.gz` archive.
3. **Place** the `syslog` folder in the same location as `System Logs Handling.exe` (or your compiled binary).
4. **Run** the executable:
   ```bash
   System Logs Handling.exe
   ```
   A message will appear if it successfully finds `syslog` in the current directory.
5. **Check** the `output` folder for the resulting CSV files.

---

### Output Format

Each log type (e.g., `syslog`, `fota`, `mmm`, etc.) will produce a separate CSV file inside an `output` directory:

- **File name**: `output/<logtype>.csv`
- **Headers**: `Timestamp, Source, Log Level, Message, File`

  - **Timestamp** – Normalized to `MM/DD HH:MM:SS` if parsed from the log.
  - **Source** – Where the log originated (`CM`, `FOTA`, `MMM`, `cellwan`, etc.).
  - **Log Level** – Typically `INFO`, `user.notice`, `kern.info`, etc.
  - **Message** – The core message text.
  - **File** – The original file from which the log line was read.

For example:
```csv
Timestamp,Source,Log Level,Message,File
02/19 01:55:01,CM,INFO,[MSTC_MI]checkRecovery(338)enableRadio=1,...,syslog.log
02/19 01:15:01,FOTA,INFO,Get CPE IMEI info Success.,syslog.log
...
```

---

### Use Cases

- **Troubleshooting Router Issues**: Gather all system logs in chronological order to trace unexpected behavior or network disconnections.
- **Monitoring Specific Sources**: Quickly filter logs by source (FOTA, MMM, CM) to focus on firmware, modem, or connectivity issues.
- **Archiving Logs**: Convert logs into well-structured CSV format for long-term archival or further analysis in spreadsheets or external tools.

---

### License

This project is provided under the **GNU** license. See the LICENSE file (if included) or visit [GNU Licenses](https://www.gnu.org/licenses/) for more details.
```
