#include "os.h"
#define LEVELS 5

int calc_off_in_level(uint64_t vpn, int level);
int is_valid(uint64_t address);
void check_delete(uint64_t **base_addresses_for_check, int *off_in_all_levels);


void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn)
{
    uint64_t *root = (uint64_t *)phys_to_virt(pt << 12); /*pt is ppn and phys_to_virt need physical address, then shift to add offset*/
    uint64_t *curr_entry = root;
    int off_in_level = 0;
    uint64_t allocated_ppn = 0;
    uint64_t *address_from_vpn[5] = {}; /*save the relevant adresses in case we will need to check for delete and free*/
    int off_in_levels[5] = {}; /*save the relevant offset in each level in case we will need to check for delete and free*/

    for (int i = 0; i < LEVELS; i++)
    {
        off_in_level = calc_off_in_level(vpn, i);
        if (ppn != NO_MAPPING) /*we want to create map*/
        {
            if (i == 4)
            {
                /*in last level we just want to put the physical address, ppn with offset and valid bit = 1*/
                curr_entry[off_in_level] = (ppn << 12) + 1;
            }
            else /*we are not in last level, maybe need to allocate frame, and go to next level*/
            {
                if (!is_valid(curr_entry[off_in_level])) /*no mapping in this entry*/
                {
                    allocated_ppn = alloc_page_frame();
                    curr_entry[off_in_level] = (allocated_ppn << 12) + 1; /*new ppn with offset and valid bit = 1*/
                }
                curr_entry = (uint64_t *)phys_to_virt(curr_entry[off_in_level]-1); /*get address of next level*/
            }
        }
        else /*we want to destroy mapping*/
        {
           if (is_valid(curr_entry[off_in_level]))
           {
               address_from_vpn[i] = curr_entry;
               off_in_levels[i] = off_in_level;
               if (i == 4)
               {
                   curr_entry[off_in_level] = curr_entry[off_in_level] - 1; /*turn off valid bit, so valid bit = 0*/
                   check_delete(address_from_vpn, off_in_levels);
               }
               else /*go to next level*/
               {
                    curr_entry = (uint64_t *)phys_to_virt(curr_entry[off_in_level]-1); /*get address of next level*/
               }
           }
           else /*mapping does not exist in this entry, we can stop*/
           {
               break;
           }
        }
    }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn)
{
    uint64_t *root = (uint64_t *)phys_to_virt(pt << 12); /*pt is ppn and phys_to_virt need physical addrees, then shift to add offset*/
    uint64_t *curr_entry = root;
    int off_in_level = 0;

    for (int i = 0; i < LEVELS; i++)
    {
        off_in_level = calc_off_in_level(vpn, i);
        if (!is_valid(curr_entry[off_in_level])) /*no mapping exists*/
        {
            return NO_MAPPING;
        }
        else
        {
            if (i != 4)
            {
                curr_entry = (uint64_t *)phys_to_virt(curr_entry[off_in_level]-1); /*get address of next level*/
            }
            /*else, we are in last level so loop will end now*/
        }
    }
    return curr_entry[off_in_level] >> 12; /*in the last level, we have the pte. ppn is without offset*/
}

/*helper functions*/

/*Calculates the 9 relevant bits of vpn, according to level.*/
int calc_off_in_level(uint64_t vpn, int level)
{
    return (vpn >> (LEVELS - 1 - level)*9) & 0x1ff; /*0x1ff is 9 bits of 1, so we get the bits for the level*/
}

/*Returns 1 if address is valid, otherwise returns 0.*/
int is_valid(uint64_t pa)
{
    return pa & 1;
}

/*Checks if the relevant adresses is empty, after we delete a mapping.
Starts from the 4'th level, and go up if needed.*/
void check_delete(uint64_t **base_addresses_for_check, int *off_in_all_levels)
{
    int level = 4;
    int finish = 0;
    int cnt_level_delete = 0;
    uint64_t *curr_entry = base_addresses_for_check[4];

    while (level > 0 && !finish) /*level > 0 becuase we never free the root*/
    {
        for(int i = 0; i < 512; i++)
        {
            if (!is_valid(curr_entry[i]))
            {
                cnt_level_delete++;
            }
        }

        /*check id need to free and deal with parent*/
        level--;
        curr_entry = base_addresses_for_check[level];
        if (cnt_level_delete == 512) /*page is empty, need to free and mark in it's parent*/
        {
            free_page_frame(curr_entry[off_in_all_levels[level]] >> 12);
            curr_entry[off_in_all_levels[level]] = curr_entry[off_in_all_levels[level]] - 1; /*bit valid = 0 in the parent if we delete an enitre page*/
            cnt_level_delete = 0; /*cnt will be 0 for next iteration*/
        }
        else
        {
            /*if we didn't delete an entire page and mark it in it's parent, we can stop cheking*/
            finish = 1;
        }
    }
}