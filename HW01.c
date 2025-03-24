#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>
#include <string.h>
#include <unistd.h>

#define CHUNK_SIZE 1024 * 1024  // 1MB buffer for merging

typedef struct {
    char url[1024];
    long start;
    long end;
    int thread_id;
    char *buffer;
    size_t size;
} DownloadTask;

// Function to write data to memory instead of a file
size_t write_to_memory(void *ptr, size_t size, size_t nmemb, void *data) {
    DownloadTask *task = (DownloadTask *)data;
    size_t total_size = size * nmemb;
    
    task->buffer = realloc(task->buffer, task->size + total_size);
    if (!task->buffer) {
        fprintf(stderr, "Error: Memory allocation failed for chunk %d\n", task->thread_id);
        return 0;
    }
    
    memcpy(task->buffer + task->size, ptr, total_size);
    task->size += total_size;
    
    return total_size;
}

// Extract the filename from the URL
void extract_filename_from_url(const char *url, char *filename) {
    const char *last_slash = strrchr(url, '/');
    if (last_slash) {
        strcpy(filename, last_slash + 1);
    } else {
        strcpy(filename, "downloaded_file");
    }
}

// Check if file exists
int file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

// Perform a HEAD request to get file size
long get_file_size(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Failed to initialize cURL\n");
        return -1;
    }

    double filesize = 0.0;
    long response_code = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);

    if (curl_easy_perform(curl) == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) {
            fprintf(stderr, "Error: Invalid URL or file not found (HTTP %ld)\n", response_code);
            curl_easy_cleanup(curl);
            return -1;
        }
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
    } else {
        fprintf(stderr, "Error: Could not connect to the URL.\n");
    }

    curl_easy_cleanup(curl);
    return (long)filesize;
}

// Download a chunk of the file into memory
void *download_chunk(void *arg) {
    DownloadTask *task = (DownloadTask *)arg;
    CURL *curl;
    CURLcode res;
    char range[128];

    snprintf(range, sizeof(range), "%ld-%ld", task->start, task->end);

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Thread %d: Failed to initialize cURL\n", task->thread_id);
        pthread_exit(NULL);
    }

    task->buffer = NULL;
    task->size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, task->url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_memory);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, task);
    curl_easy_setopt(curl, CURLOPT_RANGE, range);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Thread %d: Download error: %s\n", task->thread_id, curl_easy_strerror(res));
    } else {
        printf("Chunk %d downloaded (%ld - %ld)\n", task->thread_id, task->start, task->end);
    }

    curl_easy_cleanup(curl);
    pthread_exit(NULL);
}

// Merge the downloaded chunks into the final file
void merge_chunks(const char *output_file, DownloadTask *tasks, int num_threads) {
    FILE *fp_out = fopen(output_file, "wb");
    if (!fp_out) {
        fprintf(stderr, "Error: Failed to create output file %s\n", output_file);
        return;
    }

    for (int i = 0; i < num_threads; i++) {
        fwrite(tasks[i].buffer, 1, tasks[i].size, fp_out);
        free(tasks[i].buffer);  // Free allocated memory
        printf("Merged chunk %d\n", i);
    }

    fclose(fp_out);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <URL> <num_threads>\n", argv[0]);
        return 1;
    }

    char url[1024];
    char output_file[256];
    int num_threads = atoi(argv[2]);

    snprintf(url, sizeof(url), "%s", argv[1]);

    if (num_threads < 1) {
        fprintf(stderr, "Error: Number of threads must be at least 1.\n");
        return 1;
    }

    // Extract the filename
    extract_filename_from_url(url, output_file);

    // Check if file already exists
    if (file_exists(output_file)) {
        printf("❌ Error: The file '%s' already exists! Download aborted.\n", output_file);
        return 1;
    }

    long file_size = get_file_size(url);
    if (file_size <= 0) {
        fprintf(stderr, "Error: Could not retrieve file size. Exiting.\n");
        return 1;
    }

    printf("Downloading from: %s\n", url);
    printf("Saving to: %s\n", output_file);
    printf("Using %d threads.\n", num_threads);
    printf("File size: %ld bytes\n", file_size);

    pthread_t threads[num_threads];
    DownloadTask tasks[num_threads];

    long chunk_size = file_size / num_threads;
    for (int i = 0; i < num_threads; i++) {
        tasks[i].start = i * chunk_size;
        tasks[i].end = (i == num_threads - 1) ? file_size - 1 : (tasks[i].start + chunk_size - 1);
        tasks[i].thread_id = i;
        snprintf(tasks[i].url, sizeof(tasks[i].url), "%s", url);
        tasks[i].buffer = NULL;
        tasks[i].size = 0;

        if (pthread_create(&threads[i], NULL, download_chunk, &tasks[i]) != 0) {
            fprintf(stderr, "Error: Failed to create thread %d\n", i);
            num_threads = i;
            break;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    merge_chunks(output_file, tasks, num_threads);
    printf("✅ Download complete: %s\n", output_file);

    return 0;
}
