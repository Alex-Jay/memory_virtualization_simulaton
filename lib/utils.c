#ifdef  __linux__
	#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "constants.h"
#include "utils.h"

int page_table_page_counter 	= 0;
int physical_frame_counter		= 0;
int disk_frame_counter			= 0;

/*=============================== INITIALIZATION ===============================*/
void init_random_seed ()
{
	printf("%s - Initializing Random Generator Seed...\n", INIT_PRINT_TAG);
	srand(time(NULL));
	time_t t;
	srand((unsigned) time(&t));
}

// Source: https://www.tutorialspoint.com/c_standard_library/c_function_rand.htm
// Author: Tutorials Point
int get_random_int (int min, int max)
{
	/* If equal payload size bounds, return PAYLOAD_LOWER_BOUNDS */
	if (PAYLOAD_LOWER_BOUNDS == PAYLOAD_UPPER_BOUNDS)
		return PAYLOAD_LOWER_BOUNDS;

	/* Generate a random integer */
	int random_int = rand() % (max + 1 - min) + min;

	return random_int;
}

int get_random_payload_size ()
{
	printf("%s - Generating Random Payload size...\n", INIT_PRINT_TAG);
	if (PAYLOAD_UPPER_BOUNDS == PAYLOAD_LOWER_BOUNDS)
		return 0;
	else
		return get_random_int(PAYLOAD_LOWER_BOUNDS, PAYLOAD_UPPER_BOUNDS);
}

int get_random_ascii_index ()
{
	return get_random_int(ASCII_MIN_RANGE, ASCII_MAX_RANGE);
}

void init_page_table_entries (char* physical_memory)
{
	printf("%s - Initializing Empty Page Table...\n", INIT_PRINT_TAG);

	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	for(int i = 0; i < PAGE_TABLE_SIZE; i += 2)
	{
		physical_memory[i] = 0x00;		// Physical Frame Number
		physical_memory[i + 1] = 0x00;	// Control Bits
	}
}

void init_disk_entries (char* disk_memory)
{
	printf("%s - Initializing Empty Disk Block...\n", INIT_PRINT_TAG);

	if (!disk_memory)
	{
		printf("%s - Disk Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	for(int i = 0; i < PAGE_TABLE_SIZE; ++i)
	{
		disk_memory[i] = 0x00;		// Data
	}
}

/*================================== FILE I/O ==================================*/
void write_physical_memory_to_file (char* physical_memory, const char *file_path)
{
	printf("%s - Writing Physical Memory: %s\n", FILEIO_PRINT_TAG, file_path);

	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	/* Open file for writing [Creates file if doesn't exist, deletes & overwrites the file] */
    FILE *fp = fopen(file_path, "wb");

	if (fp != NULL)
	{
		fprintf(fp, TABLE_PHYSICAL_HEADER, "Address", "Frame", "Content");
		fprintf(fp, "%s|%s|%s\r\n", "----------------", "---------------", "----------------");

		for(int i = 0; i < PHYSICAL_MEMORY_SIZE; ++i)
		{
			fprintf(fp, TABLE_PHYSICAL_MEMORY, i, physical_address_to_frame(i), physical_memory[i]);			
		}
	}

	/* Close stream */
	fclose(fp);
}

void write_page_table_to_file (char* physical_memory, const char *file_path)
{
	printf("%s - Writing Page Table: %s\n", FILEIO_PRINT_TAG, file_path);

	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	int page_count = 0;

	FILE *fp = fopen(file_path, "wb");

	if (fp != NULL)
	{
		fprintf(fp, TABLE_PAGE_HEADER, "Page", "Physical Frame", "Is Present?", "Can RW?", "Is Dirty?", "On Disk?");
		fprintf(fp, "%s|%s|%s|%s|%s|%s\r\n", "--------", "-----------------------", "---------------", "---------------", "---------------", "---------");
		
		/* Loop every 2 bytes */
		for(int i = 0; i < PAGE_TABLE_SIZE; i += 2)
		{
			uchar_t physical_frame_number = physical_memory[i];
			uchar_t control_bits = physical_memory[i + 1];

			uint_t present_flag 	= ((control_bits & C_PRESENT) != 0) ? 1 : 0;
			uint_t rw_flag 		= ((control_bits & C_READWRITE) != 0) ? 1 : 0;
			uint_t dirty_flag 	= ((control_bits & C_DIRTY) != 0) ? 1 : 0;
			uint_t disk_flag 		= ((control_bits & C_DISK) != 0) ? 1 : 0;

			fprintf(fp, TABLE_PAGE_TABLE, page_count, physical_frame_number, present_flag, rw_flag, dirty_flag, disk_flag);
			page_count++;
		}
	}

	/* Close stream */
	fclose(fp);
}

void write_disk_memory_to_file (char* disk_memory, const char *file_path)
{
	printf("%s - Writing Disk Memory: %s\n", FILEIO_PRINT_TAG, file_path);

	int disk_frame_counter = 0;

	if (!disk_memory)
	{
		printf("%s - Disk Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	/* Open file for writing [Creates file if doesn't exist, deletes & overwrites the file] */
    FILE *fp = fopen(file_path, "wb");

	if (fp != NULL)
	{
		fprintf(fp, "%-3s\t\t| %-3s\r\n", "Page", "Content");
		fprintf(fp, "%s|%s\r\n", "----------------", "----------------");

		for(int i = 0; i < DISK_MEMORY_SIZE; ++i)
		{
			fprintf(fp, "0x%02X\t\t| %-3c\r\n", disk_frame_counter, disk_memory[i]);

			if (i > 0 && i % PAGE_SIZE == 0)
				disk_frame_counter++;	
		}
	}

	/* Close stream */
	fclose(fp);
}

// Source: http://www.codebind.com/cprogramming/get-current-directory-using-c-program/
// Author: CodeBind Administrator (Name unknown)
char* get_current_working_directory ()
{
	printf("%s - Retrieving Current Working Directory...\n", FILEIO_PRINT_TAG);
	char buff[FILENAME_MAX + 1];
	buff[FILENAME_MAX] = '\0';
	getcwd( buff, FILENAME_MAX );

	// Source: https://stackoverflow.com/questions/46190920/how-do-i-convert-a-char-array-to-a-string-in-c
	// Author: [Hitsugaya198] - https://stackoverflow.com/users/4636686/hitsugaya198
	char* string;
	string = malloc(sizeof(buff)+1);
	memset(&string[0], 0x00, sizeof(buff)+1);
	memcpy(&string[0], &buff[0], sizeof(buff));

	return string;
}

/*================================= OPERATIONAL ================================*/
void write_random_payload (char* physical_memory, char* disk_memory, int payload_size, int start_address)
{
	print_payload_header();

	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	/* Memory not defined or start_address is out of bounds */
	if (physical_memory == NULL || start_address >= PHYSICAL_MEMORY_SIZE)
	{
		printf("%s - Failed To Write To Physical Memory...\n", ERROR_PRINT_TAG);
		return;
	}

	printf("%s - Writing Random Payload to Physical Memory...\n", CORE_PRINT_TAG);

	int start_frame 			= floor(start_address / PAGE_SIZE);
	int payload_in_frames 		= floor(payload_size / PAGE_SIZE);
	int end_frame				= start_frame + payload_in_frames;
	int out_of_bounds_delta 	= end_frame - FRAME_COUNT;

	/* Shift start_frame backwards until there are no segmentation faults */
	if (end_frame >= FRAME_COUNT)
	{
		int move_back_frames = out_of_bounds_delta + OOR_FRAME_OFFSET;

		printf("%s - Out of bounds...\n", SEGMENT_PRINT_TAG);
		printf("[Start Frame]:\t\tx0%04X\n", start_frame);
		printf("[Payload Frames]:\t%i (frames)\n", payload_in_frames);
		printf("[End Frame]:\t\tx0%04X\n", end_frame);
		printf("[Overflow Frame Delta]:\t%i (frames)\n", out_of_bounds_delta);
		printf("%s - Shifting Back Start Address by %i (frames)...\n", SEGMENT_PRINT_TAG, move_back_frames);

		/* Shift back address N amount of frames in which the payload will fully fit into the address-space */
		start_address -= move_back_frames * PAGE_SIZE;
	}

	/* Write to physical memory */
	for(int i = start_address; i < (start_address + payload_size); ++i)
	{
		/* Write data to physical memory */
		physical_memory[i] = (char) get_random_ascii_index();

		// If [i] is the start of a frame
		if (i % PAGE_SIZE == 0)
		{
			/* Reset every frame */
			uchar_t control_bits = 0x0000;
			uint_t current_frame = physical_address_to_frame (i);

			/* All frames, except last 2 frames */
			if	(physical_frame_counter < (payload_in_frames - 2))
			{
				/* Set current flags to true */
				control_bits |= C_PRESENT | C_READWRITE;
				write_page_table_entry(physical_memory, current_frame, control_bits);
			}
			else /* Last 2 frames */
			{
				/* Set DISK flag to true */
				control_bits |= C_READWRITE | C_DISK;

				write_page_table_entry(physical_memory, disk_frame_counter, control_bits);
				write_disk_entry(physical_memory, disk_memory);

				/* Skip a frame of data if DISK*/
				i += PAGE_SIZE;

				/* Set any frame that is with DISK flag to 0x00 onwards */
				disk_frame_counter++;
			}

			physical_frame_counter++;
		}
	}

	printf("Starting Frame:\t\t0x%02X\n", (start_address / PAGE_SIZE));
	printf("Wrote Bytes:\t\t%'i (bytes)\n", payload_size);
	printf("Frames Occupied:\t%i (frames)\n", payload_in_frames);

	print_header_end('=', strlen(TABLE_PAYLOAD_HEADER));
}

void write_page_table_entry (char* physical_memory, uchar_t frame_number, uchar_t control_bits)
{
	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	/* Write 2 bytes for page table entry */
	physical_memory[page_table_page_counter] = frame_number;
	physical_memory[page_table_page_counter + 1] = control_bits;

	page_table_page_counter += 2;
}

void write_disk_entry (char* physical_memory, char* disk_memory)
{
	printf("%s - Writing To DISK...\n", DISK_PRINT_TAG);

	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	/* Write pseudo-random data for DISK memory */
	for (int i = 0; i < PAGE_TABLE_SIZE; ++i)
	{
		disk_memory[i] = (char) get_random_ascii_index();
	}
}

int get_random_physical_frame ()
{
	printf("%s - Retrieving Random Physical Frame...\n", INIT_PRINT_TAG);
	// Example: 256 frames - 254 frames = 2 frames [Frame 0-1: Page Table]
	int min = FRAME_COUNT - get_available_physical_frame_count();
	int max = FRAME_COUNT;

	return get_random_int(min, max);
}

int get_available_random_pte (char* physical_memory)
{
	//printf("%s - Retrieving Random Available Page Table Entry...\n\n", CORE_PRINT_TAG);

	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return -1;
	}

	for(int pte_index = 0; pte_index < PAGE_TABLE_SIZE; pte_index += 2)
	{
		//uchar_t physical_frame_number = physical_memory[pte_index];	/* Physical Frame Number */
		uchar_t control_bits = physical_memory[pte_index + 1];			/* Control Bits */
		
		/* Frame is not occupied [No C_PRESENT flag set] */
		if ((control_bits & C_PRESENT) == 0)
		{
			/* Set C_PRESENT flag upon returning out of the function */
			physical_memory[pte_index + 1] |= C_PRESENT;

			return pte_index;
		}
	}

	return -1;
}

int get_available_physical_frame_count ()
{
	if (PAGE_TABLE_SIZE < 0)
		return 0;

	/* Get available free memory (including page table size difference) and determine frame count */
	return (PHYSICAL_MEMORY_SIZE - PAGE_TABLE_SIZE) / PAGE_SIZE;
}

int frame_to_physical_address (int frame_number)
{
	printf("%s - Converting Frame to Physical Address...\n", INIT_PRINT_TAG);
	// Example: 0 * 256 = 0
	// Example: 1 * 256 = 256
	// Example: 2 * 256 = 512
	return frame_number * PAGE_SIZE;
}

int physical_address_to_frame (int physical_address)
{
	return floor(physical_address / PAGE_SIZE);
}

int page_to_physical_frame (char* physical_memory, ushort_t input_address, int virtual_page_number, int offset)
{
	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return -1;
	}

	uchar_t virtual_address 			= virtual_page_number * 2;
	uchar_t control_bits 				= physical_memory[(virtual_page_number * 2) + 1];
	uchar_t physical_frame_number 		= physical_memory[virtual_address];
	// Source: https://www.anintegratedworld.com/masking-bit-shifting-and-0xff00/
	// Author: [CLOUDNTHINGS] - https://www.anintegratedworld.com/author/sean/
	ushort_t l_shifted_page_number 		= (physical_frame_number & OFFSET_MASK) << 8;
	ushort_t masked_page_number 		= l_shifted_page_number | offset;

	/* If on disk, retrieve into any available frame */
	if ((control_bits & C_DISK) != 0)
	{
		int pte_index = get_available_pte_slot_for_disk (physical_memory);
	}

	print_translation_data(physical_memory, input_address,
		virtual_page_number, virtual_address, control_bits,
		physical_frame_number, l_shifted_page_number, offset,
		masked_page_number
	);

	return masked_page_number;
}

int get_available_pte_slot_for_disk (char* physical_memory)
{
	printf("%s - Segmentation Fault. Retrieving Available PTE...\n\n", DISK_PRINT_TAG);

	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return -1;
	}

	return get_available_random_pte(physical_memory);
}

/*================================== DEBUGGING =================================*/
void print_mem_config (int payload_size, int frame)
{
	printf ("\n%s", TABLE_MEMORY_HEADER);
	printf (TABLE_BODY_FORMAT, PHYSICAL_MEMORY_SIZE, payload_size, get_available_physical_frame_count(), frame);
	print_header_end ('=', strlen(TABLE_MEMORY_HEADER));
}

void print_payload_header ()
{
	printf("%s", TABLE_PAYLOAD_HEADER);
}

void print_translation_data (char* physical_memory, ushort_t input_address,
	ushort_t virtual_page_number, uchar_t virtual_address, uchar_t control_bits,
	uchar_t phy_frame_number, ushort_t l_shifted_page_number, ushort_t page_offset,
	ushort_t masked_page_number)
{
	printf("%s", TABLE_TRSLT_HEADER);
	printf("%s - Beginning Address Translation...\n", TRANSLATION_PRINT_TAG);

	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	printf("[User Input]:\t\t\t0x%04X\n", input_address);
	printf("[Page Number]:\t\t\t0x%04X\n", virtual_page_number);
	printf("[Virtual Address]:\t\t0x%04X\n", virtual_address);
	printf("[Control Bits]:\t\t\t0x%04X\n", control_bits);
	printf("[Physical Frame Number]:\t0x%04X\n", l_shifted_page_number);
	printf("[Page Offset]:\t\t\t0x%04X\n", page_offset);
	printf("[Frame Number & Offset]:\t0x%04X\n", masked_page_number);
	//printf("[Content @ 0x%04X]:\t\t%c\n", masked_page_number, data);

	printf("%s", TABLE_FRAME_HEADER);
	printf(TABLE_PHYSICAL_HEADER, "Address", "Frame", "Content");
	printf("%s|%s|%s\r\n", "----------------", "---------------", "----------------");
	printf(TABLE_PHYSICAL_MEMORY, masked_page_number, physical_address_to_frame(masked_page_number), physical_memory[masked_page_number]);	
	print_header_end('=', 50);
	print_header_end('=', strlen(TABLE_TRSLT_HEADER));
}

void print_page_table_entry (char* physical_memory, int page_number)
{
	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	uint_t page_address = page_number * PAGE_SIZE;

	uchar_t physical_frame_number = physical_memory[page_address];
	uchar_t control_bits = physical_memory[page_address + 1];

	int present_flag 	= ((control_bits & C_PRESENT) 	!= 0) 	?	1	:	0;
	int rw_flag 		= ((control_bits & C_READWRITE) != 0) 	?	1	:	0;
	int dirty_flag 		= ((control_bits & C_DIRTY) 	!= 0) 	?	1	:	0;
	int disk_flag 		= ((control_bits & C_DISK) 		!= 0) 	?	1	:	0;

	printf("%s", TABLE_P_ENTRY_EXAMPLE);
	printf(TABLE_PAGE_HEADER, "Page", "Physical Frame", "Is Present?", "Can RW?", "Is Dirty?", "On Disk?");
	printf("%s|%s|%s|%s|%s|%s\r\n", "--------", "-----------------------", "---------------", "---------------", "---------------", "---------");
	printf(TABLE_PAGE_TABLE, page_address, physical_frame_number, present_flag, rw_flag, dirty_flag, disk_flag);
	print_header_end ('=', strlen(TABLE_P_ENTRY_EXAMPLE) - 1);
}

void print_physical_frame_contents (char* physical_memory, ushort_t input_address)
{
	if (!physical_memory)
	{
		printf("%s - Physical Memory Not Defined...\n", ERROR_PRINT_TAG);
		return;
	}

	ushort_t virtual_page_number 	= input_address >> BIT_SHIFT_BY;
	ushort_t page_offset 			= input_address & OFFSET_MASK;
	ushort_t physical_frame 		= page_to_physical_frame (physical_memory, input_address, virtual_page_number, page_offset);
	uchar_t data 					= physical_memory[physical_frame];

	if (data)
		printf("[Content @ 0x%04X]:\t%c\n", physical_frame, data);
	else
		printf("[Content @ 0x%04X]:\tN/A\n", physical_frame);
	
}

void print_header_end (char char_to_print, int count)
{
	int current = 0;
	while(current < count - 1)
	{
		printf("%c", char_to_print);
		++current;
	}
	printf("\n\n");
}

// Source: https://stackoverflow.com/questions/2347770/how-do-you-clear-the-console-screen-in-c
// Author: [nbro] - https://stackoverflow.com/users/3924118/nbro
void clear_console ()
{
    #if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
        system("clear");
    #endif

    #if defined(_WIN32) || defined(_WIN64)
        system("cls");
    #endif
}