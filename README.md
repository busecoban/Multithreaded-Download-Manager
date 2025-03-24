# Multithreaded Download Manager

## Overview
This project is a **Multithreaded Download Manager** written in C. It leverages the **pthread library** for multithreading and **libcurl** for handling HTTP requests. The program splits a download task into multiple threads, each handling a chunk of the file, and then merges these chunks into a single output file.

## Features
- **Multithreaded Downloads:** Parallel downloading for faster performance.
- **Dynamic Memory Allocation:** Efficient handling of file chunks in memory.
- **Automatic Merging:** Combines all downloaded parts into one file.
- **Error Handling:** Provides detailed messages for unsuccessful downloads and memory allocation failures.
- **File Existence Check:** Prevents overwriting existing files.

## Requirements
To compile and run the program, you need:
- **GCC** (GNU Compiler Collection)
- **libcurl** (For HTTP requests)

Install libcurl:
```bash
sudo apt-get install libcurl4-openssl-dev
```

## Compilation
```bash
gcc -o downloader downloader.c -lcurl -lpthread
```

## Usage
The program requires two arguments: URL of the file to download and the number of threads to use.
```bash
./downloader <URL> <num_threads>
```
Example usage:
```bash
./downloader https://example.com/largefile.zip 4
```

## Example Output
```
Downloading from: https://example.com/largefile.zip
Saving to: largefile.zip
Using 4 threads.
File size: 104857600 bytes
Chunk 0 downloaded (0 - 26214399)
Chunk 1 downloaded (26214400 - 52428799)
Chunk 2 downloaded (52428800 - 78643199)
Chunk 3 downloaded (78643200 - 104857599)
✅ Download complete: largefile.zip
```

## Code Structure
- **DownloadTask:** Struct holding information about each download task (URL, start, end, thread ID, buffer, and size).
- **download_chunk():** Downloads a file segment using cURL.
- **merge_chunks():** Combines all downloaded chunks into one file.
- **get_file_size():** Retrieves the file size via a HEAD request.
- **write_to_memory():** Callback function for writing data to memory.

## Error Handling
- If the file already exists, download is aborted.
- If memory allocation fails, an error message is displayed.
- Only successfully downloaded chunks are merged.


