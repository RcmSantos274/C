#include "cache.h"
#include "queue.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Queue **queues;

Config *parse_arguments(int argc, const char *argv[])
{
  if (argc != 7)
  {
    fprintf(stderr, "Incorrect number of arguments.\n");
    exit(EXIT_FAILURE);
  }

  Config *config = (Config *)malloc(sizeof(Config));

  config->nsets = atoi(argv[1]);
  config->bsize = atoi(argv[2]);
  config->assoc = atoi(argv[3]);
  config->r_policy = argv[4][0];
  config->o_flag = atoi(argv[5]);
  // file_path = argv[6]

  if (config->nsets <= 0)
  {
    fprintf(stderr, "Invalid set number.\n");
    exit(EXIT_FAILURE);
  }

  if (config->bsize <= 0)
  {
    fprintf(stderr, "Invalid block number.\n");
    exit(EXIT_FAILURE);
  }

  if (config->assoc < 0)
  {
    fprintf(stderr, "Invalid associativity.\n");
    exit(EXIT_FAILURE);
  }

  if (strcmp(argv[4], "L") && strcmp(argv[4], "F") && strcmp(argv[4], "R"))
  {
    fprintf(stderr, "You must choose 'L', 'F' or 'R' to replacement policy.\n");
    exit(EXIT_FAILURE);
  }

  if (config->o_flag != 0 && config->o_flag != 1)
  {
    fprintf(stderr, "Invalid output flag; must be either 1 or 0. \n");
    exit(EXIT_FAILURE);
  }

  return config;
}

Cache *new_cache(Config *config)
{
  Cache *cache = (Cache *)malloc(sizeof(Cache));

  uint32_t index_bits = calculate_bits_needed(config->nsets);
  uint32_t offset_bits = calculate_bits_needed(config->bsize);
  uint32_t tag_bits = ADDRESS_BITS - index_bits - offset_bits;

  printf("index bits %d\n", index_bits);
  printf("offset bits %d\n", offset_bits);
  printf("tag bits %d\n", tag_bits);

  AddressFormat addr_format = {
      .index_bits = index_bits,
      .tag_bits = tag_bits,
      .offset_bits = offset_bits,
  };

  cache->address_format = addr_format;
  cache->config = *config;
  cache->memory = allocate_cache_mem(cache);
  cache->empty_blocks = (config->nsets) * (config->assoc);

  // Fill the queues for each set
  queues = (Queue **)malloc(cache->config.nsets * sizeof(Queue));
  for (int i = 0; i < cache->config.nsets; i++)
  {
    queues[i] = new_q();
  }

  return cache;
}

uint32_t bits_to_bytes(uint32_t bits)
{
  if (bits % 8 == 0)
  {
    return bits / 8;
  }

  return (bits / 8) + 1;
}

uint8_t *allocate_cache_mem(Cache *cache)
{
  uint32_t lines = cache->config.nsets * cache->config.assoc;
  uint32_t line_bytes = 1 + bits_to_bytes(cache->address_format.tag_bits) + cache->config.bsize;
  uint8_t *memory = (uint8_t *)calloc(lines, line_bytes);

  return memory;
}

// Skip x bits from right to left and then take y bits from right to left
uint32_t extract_bits(uint32_t value, uint32_t skip, uint32_t take)
{
  uint32_t mask = ((1 << take) - 1) << skip;
  return (mask & value) >> skip;
}

// Return the bits of index, tag and offset from the address
AddressData decode_address(uint32_t address, AddressFormat format)
{
  AddressData addr_data = {
      .tag_data = extract_bits(address, format.offset_bits + format.index_bits, format.tag_bits),
      .index_data = extract_bits(address, format.offset_bits, format.index_bits),
      .offset_data = extract_bits(address, 0, format.offset_bits),
  };

  return addr_data;
}

// Evaluate if an address is a hit or miss in the cache memory
bool request_address(Cache *cache, uint32_t address, uint32_t *compulsory_misses, uint32_t *conflict_misses)
{
  AddressData addr_data = decode_address(address, cache->address_format);

  uint32_t cache_index = address % cache->config.nsets;

  uint8_t *memory_ptr = cache->memory + (cache_index * (1 + bits_to_bytes(cache->address_format.tag_bits) + cache->config.bsize) * cache->config.assoc);
  uint8_t *current_set = memory_ptr;

  // Iterate all lines in the set (in case of not direct mapping)
  for (int i = 0; i < cache->config.assoc; i++)
  {
    uint8_t validation_bit;
    uint8_t *validation_bit_ptr = memory_ptr;
    memcpy(&validation_bit, memory_ptr, 1);
    memory_ptr += 1;

    uint32_t tag;
    uint8_t *tag_ptr = memory_ptr;
    memcpy(&tag, memory_ptr, bits_to_bytes(cache->address_format.tag_bits));
    memory_ptr += bits_to_bytes(cache->address_format.tag_bits);

    // Skip block
    memory_ptr += cache->config.bsize;

    // printf("iteration %d\n", i);
    // Hit
    if (validation_bit == 1 && tag == addr_data.tag_data)
    {
      if (cache->config.r_policy == 'L') // If LRU, refresh the use of this tag
      {
        refresh(queues[cache_index], i);
      }

      return true;
    }

    // Compulsory miss
    if (validation_bit == 0)
    {
      memset(validation_bit_ptr, 1, 1);
      memcpy(tag_ptr, &addr_data.tag_data, bits_to_bytes(cache->address_format.tag_bits)); // change tag to new address tag

      cache->empty_blocks--;

      if (cache->config.r_policy == 'F' || cache->config.r_policy == 'L')
      {
        enqueue(queues[cache_index], i);
      }

      (*compulsory_misses)++;
      return false;
    }

    switch (cache->config.r_policy)
    {
    case 'R':
      // Conflict miss
      if (tag != addr_data.tag_data && i + 1 == cache->config.assoc) // different tag and last line
      {
        uint32_t line_bytes = 1 + bits_to_bytes(cache->address_format.tag_bits) + cache->config.bsize;
        uint32_t choosen_line = rand() % cache->config.assoc;
        // ptr to tag of choosen line
        uint8_t *choosen_line_tag_ptr = current_set + (choosen_line * line_bytes) + 1;

        // change choosen line tag
        memcpy(choosen_line_tag_ptr, &addr_data.tag_data, bits_to_bytes(cache->address_format.tag_bits));

        if (cache->empty_blocks != 0)
        {
          (*conflict_misses)++;
        }
        return false;
      }

      break;
    case 'F':
      // Conflict miss
      if (tag != addr_data.tag_data && i + 1 == cache->config.assoc) // different tag and last line
      {
        uint32_t line_bytes = 1 + bits_to_bytes(cache->address_format.tag_bits) + cache->config.bsize;
        uint32_t choosen_line = dequeue(queues[cache_index]);
        enqueue(queues[cache_index], choosen_line);
        // ptr to tag of choosen line
        uint8_t *choosen_line_tag_ptr = current_set + (choosen_line * line_bytes) + 1;

        // change choosen line tag
        memcpy(choosen_line_tag_ptr, &addr_data.tag_data, bits_to_bytes(cache->address_format.tag_bits));

        if (cache->empty_blocks != 0)
        {
          (*conflict_misses)++;
        }

        return false;
      }
      break;
    case 'L':
      // Conflict miss
      if (tag != addr_data.tag_data && i + 1 == cache->config.assoc) // different tag and last line
      {
        uint32_t line_bytes = 1 + bits_to_bytes(cache->address_format.tag_bits) + cache->config.bsize;
        uint32_t choosen_line = dequeue(queues[cache_index]);
        enqueue(queues[cache_index], choosen_line);

        // ptr to tag of choosen line
        uint8_t *choosen_line_tag_ptr = current_set + (choosen_line * line_bytes) + 1;

        // change choosen line tag
        memcpy(choosen_line_tag_ptr, &addr_data.tag_data, bits_to_bytes(cache->address_format.tag_bits));

        if (cache->empty_blocks != 0)
        {
          (*conflict_misses)++;
        }

        return false;
      }
      break;
    }
  }

  return false;
}

// Calculate bits needed to representate n values
int calculate_bits_needed(int stored)
{
  double bits_needed = 0;

  while (pow((double)2, bits_needed) < stored)
  {
    bits_needed++;
  }

  return (uint32_t)bits_needed;
}
