#include "memory_pool.h"
#include "logger.h"
#include <string.h>
#include <pthread.h>
#include <stddef.h>

// Memory block structure
typedef struct MemoryBlock {
    struct MemoryBlock* next;
    char data[1]; // Flexible array member
} MemoryBlock;

// Memory pool structure
struct MemoryPool {
    size_t block_size;
    MemoryBlock* free_list;
    size_t total_blocks;
    size_t used_blocks;
    pthread_mutex_t mutex;
};

// String pool entry
typedef struct StringEntry {
    char* str;
    size_t len;
    struct StringEntry* next;
} StringEntry;

// String pool structure
struct StringPool {
    StringEntry* entries;
    size_t count;
    size_t capacity;
    size_t total_memory;
    pthread_mutex_t mutex;
};

MemoryPool* memory_pool_create(size_t block_size, size_t initial_blocks) {
    if (block_size == 0 || initial_blocks == 0) {
        LOG_ERROR("Invalid parameters for memory pool creation");
        return NULL;
    }

    MemoryPool* pool = (MemoryPool*)malloc(sizeof(MemoryPool));
    if (!pool) {
        LOG_ERROR("Failed to allocate memory pool structure");
        return NULL;
    }

    pool->block_size = block_size;
    pool->free_list = NULL;
    pool->total_blocks = 0;
    pool->used_blocks = 0;

    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize memory pool mutex");
        free(pool);
        return NULL;
    }

    // Allocate initial blocks
    for (size_t i = 0; i < initial_blocks; i++) {
        MemoryBlock* block = (MemoryBlock*)malloc(sizeof(MemoryBlock) + block_size - 1);
        if (!block) {
            LOG_ERROR("Failed to allocate initial memory block");
            memory_pool_destroy(pool);
            return NULL;
        }

        block->next = pool->free_list;
        pool->free_list = block;
        pool->total_blocks++;
    }

    LOG_DEBUG("Memory pool created with block size %zu, %zu initial blocks", block_size, initial_blocks);
    return pool;
}

void memory_pool_destroy(MemoryPool* pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);

    MemoryBlock* current = pool->free_list;
    while (current) {
        MemoryBlock* next = current->next;
        free(current);
        current = next;
    }

    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);

    free(pool);
    LOG_DEBUG("Memory pool destroyed");
}

void* memory_pool_alloc(MemoryPool* pool) {
    if (!pool) return NULL;

    pthread_mutex_lock(&pool->mutex);

    MemoryBlock* block = pool->free_list;
    if (block) {
        pool->free_list = block->next;
        pool->used_blocks++;
        pthread_mutex_unlock(&pool->mutex);
        return block->data;
    }

    // Allocate new block if pool is empty
    block = (MemoryBlock*)malloc(sizeof(MemoryBlock) + pool->block_size - 1);
    if (!block) {
        LOG_ERROR("Failed to allocate new memory block");
        pthread_mutex_unlock(&pool->mutex);
        return NULL;
    }

    pool->total_blocks++;
    pool->used_blocks++;

    pthread_mutex_unlock(&pool->mutex);

    LOG_DEBUG("Allocated new block, total blocks: %zu, used: %zu", pool->total_blocks, pool->used_blocks);
    return block->data;
}

void memory_pool_free(MemoryPool* pool, void* ptr) {
    if (!pool || !ptr) return;

    // Calculate the block address from the data pointer
    MemoryBlock* block = (MemoryBlock*)((char*)ptr - offsetof(MemoryBlock, data));

    pthread_mutex_lock(&pool->mutex);

    block->next = pool->free_list;
    pool->free_list = block;
    pool->used_blocks--;

    pthread_mutex_unlock(&pool->mutex);
}

void memory_pool_stats(MemoryPool* pool, size_t* total_blocks, size_t* used_blocks, size_t* free_blocks) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);

    if (total_blocks) *total_blocks = pool->total_blocks;
    if (used_blocks) *used_blocks = pool->used_blocks;
    if (free_blocks) *free_blocks = pool->total_blocks - pool->used_blocks;

    pthread_mutex_unlock(&pool->mutex);
}

void memory_pool_trim(MemoryPool* pool, size_t keep_minimum) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);

    // Count free blocks
    size_t free_count = 0;
    MemoryBlock* current = pool->free_list;
    while (current) {
        free_count++;
        current = current->next;
    }

    // Free excess blocks
    size_t to_free = free_count > keep_minimum ? free_count - keep_minimum : 0;
    current = pool->free_list;
    MemoryBlock* prev = NULL;

    for (size_t i = 0; i < to_free && current; i++) {
        MemoryBlock* next = current->next;
        free(current);
        pool->total_blocks--;

        if (prev) {
            prev->next = next;
        } else {
            pool->free_list = next;
        }

        current = next;
    }

    pthread_mutex_unlock(&pool->mutex);

    if (to_free > 0) {
        LOG_DEBUG("Trimmed %zu blocks from memory pool", to_free);
    }
}

StringPool* string_pool_create(size_t initial_capacity) {
    StringPool* pool = (StringPool*)malloc(sizeof(StringPool));
    if (!pool) {
        LOG_ERROR("Failed to allocate string pool");
        return NULL;
    }

    pool->entries = NULL;
    pool->count = 0;
    pool->capacity = initial_capacity;
    pool->total_memory = 0;

    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize string pool mutex");
        free(pool);
        return NULL;
    }

    LOG_DEBUG("String pool created with initial capacity %zu", initial_capacity);
    return pool;
}

void string_pool_destroy(StringPool* pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);

    StringEntry* current = pool->entries;
    while (current) {
        StringEntry* next = current->next;
        free(current->str);
        free(current);
        current = next;
    }

    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);

    free(pool);
    LOG_DEBUG("String pool destroyed");
}

const char* string_pool_add(StringPool* pool, const char* str) {
    if (!pool || !str) return NULL;

    size_t len = strlen(str);

    pthread_mutex_lock(&pool->mutex);

    // Check if string already exists
    StringEntry* current = pool->entries;
    while (current) {
        if (current->len == len && strcmp(current->str, str) == 0) {
            pthread_mutex_unlock(&pool->mutex);
            return current->str;
        }
        current = current->next;
    }

    // Create new entry
    StringEntry* entry = (StringEntry*)malloc(sizeof(StringEntry));
    if (!entry) {
        LOG_ERROR("Failed to allocate string entry");
        pthread_mutex_unlock(&pool->mutex);
        return NULL;
    }

    entry->str = (char*)malloc(len + 1);
    if (!entry->str) {
        LOG_ERROR("Failed to allocate string data");
        free(entry);
        pthread_mutex_unlock(&pool->mutex);
        return NULL;
    }

    strcpy(entry->str, str);
    entry->len = len;
    entry->next = pool->entries;
    pool->entries = entry;
    pool->count++;
    pool->total_memory += len + 1 + sizeof(StringEntry);

    pthread_mutex_unlock(&pool->mutex);

    LOG_DEBUG("Added string to pool: '%s' (%zu chars)", str, len);
    return entry->str;
}

void string_pool_stats(StringPool* pool, size_t* total_strings, size_t* total_memory) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);

    if (total_strings) *total_strings = pool->count;
    if (total_memory) *total_memory = pool->total_memory;

    pthread_mutex_unlock(&pool->mutex);
}