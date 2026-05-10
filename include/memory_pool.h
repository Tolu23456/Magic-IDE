#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stdlib.h>
#include <stdbool.h>

// Memory pool for efficient allocation/deallocation
typedef struct MemoryPool MemoryPool;

/**
 * @brief Create a new memory pool
 *
 * @param block_size Size of each block in the pool
 * @param initial_blocks Initial number of blocks to allocate
 * @return Memory pool instance, NULL on failure
 */
MemoryPool* memory_pool_create(size_t block_size, size_t initial_blocks);

/**
 * @brief Destroy a memory pool and free all memory
 *
 * @param pool Memory pool to destroy
 */
void memory_pool_destroy(MemoryPool* pool);

/**
 * @brief Allocate memory from the pool
 *
 * @param pool Memory pool
 * @return Pointer to allocated memory, NULL on failure
 */
void* memory_pool_alloc(MemoryPool* pool);

/**
 * @brief Free memory back to the pool
 *
 * @param pool Memory pool
 * @param ptr Pointer to free
 */
void memory_pool_free(MemoryPool* pool, void* ptr);

/**
 * @brief Get pool statistics
 *
 * @param pool Memory pool
 * @param total_blocks Total blocks allocated
 * @param used_blocks Blocks currently in use
 * @param free_blocks Blocks available for allocation
 */
void memory_pool_stats(MemoryPool* pool, size_t* total_blocks, size_t* used_blocks, size_t* free_blocks);

/**
 * @brief Trim unused blocks from the pool
 *
 * @param pool Memory pool
 * @param keep_minimum Minimum blocks to keep
 */
void memory_pool_trim(MemoryPool* pool, size_t keep_minimum);

// String pool for efficient string management
typedef struct StringPool StringPool;

/**
 * @brief Create a string pool
 *
 * @param initial_capacity Initial capacity
 * @return String pool instance, NULL on failure
 */
StringPool* string_pool_create(size_t initial_capacity);

/**
 * @brief Destroy a string pool
 *
 * @param pool String pool to destroy
 */
void string_pool_destroy(StringPool* pool);

/**
 * @brief Add a string to the pool (makes a copy)
 *
 * @param pool String pool
 * @param str String to add
 * @return Pooled string pointer, NULL on failure
 */
const char* string_pool_add(StringPool* pool, const char* str);

/**
 * @brief Get string pool statistics
 *
 * @param pool String pool
 * @param total_strings Total strings in pool
 * @param total_memory Total memory used
 */
void string_pool_stats(StringPool* pool, size_t* total_strings, size_t* total_memory);

#endif // MEMORY_POOL_H